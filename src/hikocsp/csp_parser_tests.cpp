
#include "csp_parser.hpp"
#include <gtest/gtest.h>

TEST(csp_parser, verbatim)
{
    auto s = std::string{"foo"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "foo");
}

TEST(csp_parser, verbatim_text)
{
    auto s = std::string{"foo{{bar"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 2);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "foo");
    ASSERT_EQ(tokens[1].kind, hi::csp_token::type::text);
    ASSERT_EQ(tokens[1].text, "bar");
}

TEST(csp_parser, verbatim_brace_text)
{
    auto s = std::string{"foo{{{bar"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 2);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "foo{");
    ASSERT_EQ(tokens[1].kind, hi::csp_token::type::text);
    ASSERT_EQ(tokens[1].text, "bar");
}


TEST(csp_parser, verbatim_text_verbatim)
{
    auto s = std::string{"foo{{bar}}baz"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "foo");
    ASSERT_EQ(tokens[1].kind, hi::csp_token::type::text);
    ASSERT_EQ(tokens[1].text, "bar");
    ASSERT_EQ(tokens[2].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[2].text, "baz");
}

TEST(csp_parser, verbatim_brace_text_brace_verbatim)
{
    auto s = std::string{"foo{{{bar}}}baz"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "foo{");
    ASSERT_EQ(tokens[1].kind, hi::csp_token::type::text);
    ASSERT_EQ(tokens[1].text, "bar");
    ASSERT_EQ(tokens[2].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[2].text, "}baz");
}


TEST(csp_parser, simple_placeholder)
{
    auto s = std::string{"{{${foo}"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::placeholder);
    ASSERT_EQ(tokens[0].text, "foo");
    ASSERT_EQ(tokens[0].arguments.size(), 1);
    ASSERT_EQ(tokens[0].arguments[0], "foo");
}

TEST(csp_parser, format_placeholder)
{
    auto s = std::string{"{{${\"{}\", foo}"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::placeholder);
    ASSERT_EQ(tokens[0].text, "\"{}\", foo");
    ASSERT_EQ(tokens[0].arguments.size(), 2);
    ASSERT_EQ(tokens[0].arguments[0], "\"{}\"");
    ASSERT_EQ(tokens[0].arguments[1], " foo");
}

TEST(csp_parser, placeholder_lambda)
{
    auto s = std::string{"{{${\"{}\", [foo]{ return foo + 1}(), bar}"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::placeholder);
    ASSERT_EQ(tokens[0].text, "\"{}\", [foo]{ return foo + 1}(), bar");
    ASSERT_EQ(tokens[0].arguments.size(), 3);
    ASSERT_EQ(tokens[0].arguments[0], "\"{}\"");
    ASSERT_EQ(tokens[0].arguments[1], " [foo]{ return foo + 1}()");
    ASSERT_EQ(tokens[0].arguments[2], " bar");
}

TEST(csp_parser, placeholder_filter)
{
    auto s = std::string{"{{${\"{}\", foo + 1 `bar}"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::placeholder);
    ASSERT_EQ(tokens[0].text, "\"{}\", foo + 1 `bar");
    ASSERT_EQ(tokens[0].arguments.size(), 2);
    ASSERT_EQ(tokens[0].arguments[0], "\"{}\"");
    ASSERT_EQ(tokens[0].arguments[1], " foo + 1 ");
    ASSERT_EQ(tokens[0].filters.size(), 1);
    ASSERT_EQ(tokens[0].filters[0], "bar");
}

TEST(csp_parser, format_cppline)
{
    auto s = std::string{"{{$for (auto i: list){\nfoo $}\n"};
    auto tokens = hi::parse_csp(s.data(), s.data() + s.size(), "<none>");

    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[0].text, "for (auto i: list){\n");
    ASSERT_EQ(tokens[1].kind, hi::csp_token::type::text);
    ASSERT_EQ(tokens[1].text, "foo ");
    ASSERT_EQ(tokens[2].kind, hi::csp_token::type::verbatim);
    ASSERT_EQ(tokens[2].text, "}\n");
}
