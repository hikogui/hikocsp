// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "csp_parser.hpp"
#include <gtest/gtest.h>

TEST(csp_parser, verbatim)
{
    auto s = std::string{"foo"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "foo");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, verbatim_text)
{
    auto s = std::string{"foo{{bar"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "foo");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "bar");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, verbatim_brace_text)
{
    auto s = std::string{"foo{{{bar"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "foo{");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "bar");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, verbatim_text_verbatim)
{
    auto s = std::string{"foo{{bar}}baz"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "foo");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "bar");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "baz");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, verbatim_brace_text_brace_verbatim)
{
    auto s = std::string{"foo{{{bar}}}baz"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "foo{");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "bar");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "}baz");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, empty_placeholder)
{
    auto s = std::string{"{{${}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, empty_filter_placeholder)
{
    auto s = std::string{"{{${`}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_filter);
    ASSERT_EQ(it->text, "");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, filter_placeholder)
{
    auto s = std::string{"{{${`foo}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_filter);
    ASSERT_EQ(it->text, "foo");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, escape_placeholder)
{
    auto s = std::string{"{{${\"$\"}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "\"$\"");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, simple_placeholder)
{
    auto s = std::string{"{{${foo}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "foo");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, format_placeholder)
{
    auto s = std::string{"{{${\"{}\", foo}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "\"{}\"");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, " foo");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, placeholder_lambda)
{
    auto s = std::string{"{{${\"{}\", [foo]{ return foo + 1}(), bar}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "\"{}\"");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, " [foo]{ return foo + 1}()");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, " bar");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, placeholder_filter)
{
    auto s = std::string{"{{${\"{}\", foo + 1 `bar}"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "\"{}\"");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, " foo + 1 ");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_filter);
    ASSERT_EQ(it->text, "bar");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, format_cppline)
{
    auto s = std::string{"{{$for (auto i: list){\nfoo $}\n"};
    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "for (auto i: list){\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "foo ");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "}\n");
    ASSERT_EQ(++it, tokens.end());
}

TEST(csp_parser, example)
{
    using namespace std::literals;

    auto s =
        "[[nodiscard]] csp::generator<std::string> test1(std::vector<int> list, int a) noexcept\n"
        "{{{\n"
        "foo\n"
        "$for(auto x : list) {\n"
        "x=${x + a}, $\n"
        "$}\n"
        "bar\n"
        "}}}\n"s;

    auto tokens = csp::parse_csp(s, "<none>");
    auto it = tokens.begin();

    ASSERT_NE(it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "[[nodiscard]] csp::generator<std::string> test1(std::vector<int> list, int a) noexcept\n{");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "\nfoo\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "for(auto x : list) {\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "x=");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_argument);
    ASSERT_EQ(it->text, "x + a");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::placeholder_end);
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, ", ");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "}\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::text);
    ASSERT_EQ(it->text, "bar\n");
    ASSERT_NE(++it, tokens.end());
    ASSERT_EQ(it->kind, csp::csp_token_type::verbatim);
    ASSERT_EQ(it->text, "}\n");
    ASSERT_EQ(++it, tokens.end());
}
