#line 1 "C:/Users/Tjienta/Projects/hikocsp/examples/hikocsp_callback_tests.cpp.csp"
#line 1
// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikocsp/generator.hpp"
#include <gtest/gtest.h>
#include <format>

template<typename Callback>
constexpr void callback_page(std::vector<int> list, int a, Callback const &sink) noexcept
{
#line 11
sink("\n"
  "foo\n");
#line 13
for (auto x: list) {
#line 14
sink("x=");
#line 14
sink(std::format(("{}"), (x + a)));
#line 14
sink("\x24");
#line 14
sink(", ");
#line 14

#line 15
}
#line 16
sink("bar\n");
#line 17
}

TEST(callback_example, callback_page)
{
    auto result = std::string{};
    callback_page(std::vector{1, 2, 3}, 5, [&result](std::string_view x) { result += x; });

    auto const expected = std::string{
        "\nfoo\n"
        "x=6$, x=7$, x=8$, "
        "bar\n"
    };

    ASSERT_EQ(result, expected);
}
