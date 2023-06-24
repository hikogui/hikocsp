#line 1 "C:/Users/Tjienta/Projects/hikocsp/examples/hikocsp_yield_tests.cpp.csp"
#line 1
// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikocsp/generator.hpp"
#include <gtest/gtest.h>
#include <format>

[[nodiscard]] constexpr std::string reverse_filter(std::string str) noexcept
{
    std::reverse(str.begin(), str.end());
    return str;
}

[[nodiscard]] constexpr std::string duplicate_filter(std::string str) noexcept
{
    return str + str;
}

[[nodiscard]] csp::generator<std::string> yield_page(std::vector<int> list, int a) noexcept
{
#line 21
co_yield "\n"
  "foo\n";
#line 23
for (auto x: list) {
#line 24
co_yield "x + a = ";
#line 24
co_yield (reverse_filter)(std::format(("{}"), (x)));
#line 24
co_yield " + ";
#line 24
co_yield ([](auto const &x){return x;})(std::format(("{}"), (a)));
#line 24
co_yield " = ";
#line 24
co_yield (duplicate_filter)(std::format(("{}"), (x + a)));
#line 24
co_yield ",\n";
#line 25
}
#line 26
co_yield "bar\n";
#line 27
}

TEST(yield_example, yield_page)
{
    auto result = std::string{};
    for (auto const &s: yield_page(std::vector{12, 34, 56}, 42)) {
        result += s;
    }

    auto const expected = std::string{
        "\nfoo\n"
        "x + a = 21 + 42 = 5454,\n"
        "x + a = 43 + 42 = 7676,\n"
        "x + a = 65 + 42 = 9898,\n"
        "bar\n"
    };

    ASSERT_EQ(result, expected);
}
