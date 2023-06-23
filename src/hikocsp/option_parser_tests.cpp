// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "option_parser.hpp"
#include <gtest/gtest.h>

using namespace std::literals;

TEST(option_parser, empty)
{
    auto args = std::vector{
        "program"sv,
    };

    auto r = csp::parse_options(args, "");
    ASSERT_EQ(r.program_name, "program");
    ASSERT_TRUE(r.options.empty());
    ASSERT_TRUE(r.arguments.empty());
}

TEST(option_parser, short_option_argument)
{
    auto args = std::vector{"program"sv, "-h"sv, "-o"sv, "foo.txt"sv, "bar"sv};

    auto r = csp::parse_options(args, "o");
    ASSERT_EQ(r.program_name, "program");
    ASSERT_EQ(r.options.size(), 2);
    ASSERT_EQ(r.options[0].name, "-h");
    ASSERT_EQ(r.options[0].argument, std::nullopt);
    ASSERT_EQ(r.options[1].name, "-o");
    ASSERT_EQ(r.options[1].argument, "foo.txt");
    ASSERT_EQ(r.arguments.size(), 1);
    ASSERT_EQ(r.arguments[0], "bar");
}

TEST(option_parser, short_option_concat_argument)
{
    auto args = std::vector{"program"sv, "-hofoo.txt"sv, "bar"sv};

    auto r = csp::parse_options(args, "o");
    ASSERT_EQ(r.program_name, "program");
    ASSERT_EQ(r.options.size(), 2);
    ASSERT_EQ(r.options[0].name, "-h");
    ASSERT_EQ(r.options[0].argument, std::nullopt);
    ASSERT_EQ(r.options[1].name, "-o");
    ASSERT_EQ(r.options[1].argument, "foo.txt");
    ASSERT_EQ(r.arguments.size(), 1);
    ASSERT_EQ(r.arguments[0], "bar");
}


TEST(option_parser, long_option_argument)
{
    auto args = std::vector{"program"sv, "--help"sv, "--output=foo.txt"sv, "bar"sv};

    auto r = csp::parse_options(args, "o");
    ASSERT_EQ(r.program_name, "program");
    ASSERT_EQ(r.options.size(), 2);
    ASSERT_EQ(r.options[0].name, "--help");
    ASSERT_EQ(r.options[0].argument, std::nullopt);
    ASSERT_EQ(r.options[1].name, "--output");
    ASSERT_EQ(r.options[1].argument, "foo.txt");
    ASSERT_EQ(r.arguments.size(), 1);
    ASSERT_EQ(r.arguments[0], "bar");
}

