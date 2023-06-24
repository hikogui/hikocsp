#line 1 "C:/Users/Tjienta/Projects/hikocsp/examples/hikocsp_tests.cpp.csp"
#line 1
// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikocsp/generator.hpp"
#include <gtest/gtest.h>
#include <format>

[[nodiscard]] csp::generator<std::string> page1(std::vector<int> list, int a) noexcept
{
#line 10
co_yield "\n"
         "foo\n";
#line 12
for (auto x: list) {
#line 13
co_yield "x=";
#line 13
co_yield std::format(("{}"), (x + a));
#line 13
co_yield ", ";
#line 13

#line 14
}
#line 15
co_yield "bar\n";
#line 16
}

TEST(examples, page1_yield)
{
    auto result = std::string{};
    for (auto const &s: page1(std::vector{1, 2, 3}, 5)) {
        result += s;
    }

    auto const expected = std::string{
        "\nfoo\n"
        "x=6, x=7, x=8, "
        "bar\n"
    };

    ASSERT_EQ(result, expected);
}
