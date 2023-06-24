#line 1 "C:/Users/Tjienta/Projects/hikocsp/examples/hikocsp_append_tests.cpp.csp"
#line 1
// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikocsp/generator.hpp"
#include <gtest/gtest.h>
#include <format>

[[nodiscard]] constexpr std::string append_page(std::vector<int> list, int a) noexcept
{
    auto out = std::string{};
#line 12
out += "\n"
  "foo\n";
#line 14
for (auto x: list) {
#line 15
out += "x=";
#line 15
out += std::format(("{}"), (x + a));
#line 15
out += "\x24";
#line 15
out += ", ";
#line 15

#line 16
}
#line 17
out += "bar\n";
#line 18

    return out;
}

TEST(append_example, append_page)
{
    auto result = append_page(std::vector{1, 2, 3}, 5);

    auto const expected = std::string{
        "\nfoo\n"
        "x=6$, x=7$, x=8$, "
        "bar\n"
    };

    ASSERT_EQ(result, expected);
}
