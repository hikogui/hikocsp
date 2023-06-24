// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikocsp/generator.hpp"
#include <gtest/gtest.h>
#include <format>

[[nodiscard]] csp::generator<std::string> no_line_page(std::vector<int> list, int a) noexcept
{
co_yield "\n"
  "foo\n";
for (auto x: list) {
co_yield "x=";
co_yield std::format(("{}"), (x + a));
co_yield "\x24";
co_yield ", ";

}
co_yield "bar\n";
}

TEST(no_line_example, no_line_page)
{
    auto result = std::string{};
    for (auto const &s: no_line_page(std::vector{1, 2, 3}, 5)) {
        result += s;
    }

    auto const expected = std::string{
        "\nfoo\n"
        "x=6$, x=7$, x=8$, "
        "bar\n"
    };

    ASSERT_EQ(result, expected);
}
