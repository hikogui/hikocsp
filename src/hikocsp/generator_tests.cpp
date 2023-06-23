// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "generator.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace generator_tests {

csp::generator<int> my_generator()
{
    co_yield 42;
    co_yield 3;
    co_yield 12;
}

}

TEST(generator, generator)
{
    auto test = generator_tests::my_generator();
    auto it = test.begin();

    ASSERT_NE(it, test.end());
    ASSERT_EQ(*it, 42);
    ++it;
    ASSERT_NE(it, test.end());
    ASSERT_EQ(*it, 3);
    ++it;
    ASSERT_NE(it, test.end());
    ASSERT_EQ(*it, 12);
    ++it;
    ASSERT_EQ(it, test.end());
}

TEST(generator, generator_loop)
{
    auto index = 0;
    for (auto number : generator_tests::my_generator()) {
        if (index == 0) {
            ASSERT_EQ(number, 42);
        } else if (index == 1) {
            ASSERT_EQ(number, 3);
        } else if (index == 2) {
            ASSERT_EQ(number, 12);
        } else {
            FAIL();
        }
        ++index;
    }
}

