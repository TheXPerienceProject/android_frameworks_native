/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ftl/optional.h>
#include <ftl/static_vector.h>
#include <ftl/string.h>
#include <ftl/unit.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <functional>
#include <numeric>
#include <utility>

using namespace std::placeholders;
using namespace std::string_literals;

namespace android::test {

using ftl::Optional;
using ftl::StaticVector;

TEST(Optional, Transform) {
  // Empty.
  EXPECT_EQ(std::nullopt, Optional<int>().transform([](int) { return 0; }));

  // By value.
  EXPECT_EQ(0, Optional(0).transform([](int x) { return x; }));
  EXPECT_EQ(100, Optional(99).transform([](int x) { return x + 1; }));
  EXPECT_EQ("0b100"s, Optional(4).transform(std::bind(ftl::to_string<int>, _1, ftl::Radix::kBin)));

  // By reference.
  {
    Optional opt = 'x';
    EXPECT_EQ('z', opt.transform([](char& c) {
      c = 'y';
      return 'z';
    }));

    EXPECT_EQ('y', opt);
  }

  // By rvalue reference.
  {
    std::string out;
    EXPECT_EQ("xyz"s, Optional("abc"s).transform([&out](std::string&& str) {
      out = std::move(str);
      return "xyz"s;
    }));

    EXPECT_EQ(out, "abc"s);
  }

  // No return value.
  {
    Optional opt = "food"s;
    EXPECT_EQ(ftl::unit, opt.transform(ftl::unit_fn([](std::string& str) { str.pop_back(); })));
    EXPECT_EQ(opt, "foo"s);
  }

  // Chaining.
  EXPECT_EQ(14u, Optional(StaticVector{"upside"s, "down"s})
                     .transform([](StaticVector<std::string, 3>&& v) {
                       v.push_back("cake"s);
                       return v;
                     })
                     .transform([](const StaticVector<std::string, 3>& v) {
                       return std::accumulate(v.begin(), v.end(), std::string());
                     })
                     .transform([](const std::string& s) { return s.length(); }));
}

namespace {

Optional<int> parse_int(const std::string& str) {
  if (const int i = std::atoi(str.c_str())) return i;
  return std::nullopt;
}

}  // namespace

TEST(Optional, AndThen) {
  // Empty.
  EXPECT_EQ(std::nullopt, Optional<int>().and_then([](int) -> Optional<int> { return 0; }));
  EXPECT_EQ(std::nullopt, Optional<int>().and_then([](int) { return Optional<int>(); }));

  // By value.
  EXPECT_EQ(0, Optional(0).and_then([](int x) { return Optional(x); }));
  EXPECT_EQ(123, Optional("123").and_then(parse_int));
  EXPECT_EQ(std::nullopt, Optional("abc").and_then(parse_int));

  // By reference.
  {
    Optional opt = 'x';
    EXPECT_EQ('z', opt.and_then([](char& c) {
      c = 'y';
      return Optional('z');
    }));

    EXPECT_EQ('y', opt);
  }

  // By rvalue reference.
  {
    std::string out;
    EXPECT_EQ("xyz"s, Optional("abc"s).and_then([&out](std::string&& str) {
      out = std::move(str);
      return Optional("xyz"s);
    }));

    EXPECT_EQ(out, "abc"s);
  }

  // Chaining.
  using StringVector = StaticVector<std::string, 3>;
  EXPECT_EQ(14u, Optional(StaticVector{"-"s, "1"s})
                     .and_then([](StringVector&& v) -> Optional<StringVector> {
                       if (v.push_back("4"s)) return v;
                       return {};
                     })
                     .and_then([](const StringVector& v) -> Optional<std::string> {
                       if (v.full()) return std::accumulate(v.begin(), v.end(), std::string());
                       return {};
                     })
                     .and_then(parse_int)
                     .and_then([](int i) {
                       return i > 0 ? std::nullopt : std::make_optional(static_cast<unsigned>(-i));
                     }));
}

}  // namespace android::test
