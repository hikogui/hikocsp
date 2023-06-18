
/** @file csp_parser.hpp
 *
 *

 */

#pragma once

#include "generator.hpp"
#include <vector>
#include <string_view>
#include <format>
#include <filesystem>
#include <cstdint>

namespace csp { inline namespace v1 {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct csp_token {
    enum class type { verbatim, placeholder, text };

    type kind;
    std::string_view text;
    std::vector<std::string_view> arguments;
    std::vector<std::string_view> filters;
};

template<typename T>
constexpr auto to_underlying(T x) noexcept
    requires(std::is_enum_v<T>)
{
    return static_cast<std::underlying_type_t<T>>(x);
}

generator<csp_token> parse_csp(char const *it, char const *last, std::filesystem::path const& path)
{
    enum class state_type : uint8_t {
        verbatim,
        verbatim_brace,
        verbatim_dbrace,
        verbatim_squote,
        verbatim_dquote,
        verbatim_squote_escape,
        verbatim_dquote_escape,

        text,
        text_brace,
        text_dollar,

        placeholder,
        placeholder_squote,
        placeholder_dquote,
        placeholder_squote_escape,
        placeholder_dquote_escape,

        line_verbatim,
    };
    using enum state_type;

    auto token_start = it;

    constexpr auto S = [](state_type s, char c) -> uint16_t {
        return to_underlying(s) | (static_cast<uint16_t>(c) << 8);
    };

    auto line_nr = 0;
    auto state = state_type::verbatim;
    auto subexpression_stack = std::vector<char>{};
    auto is_filter = false;
    auto filters = std::vector<std::string_view>{};
    auto arguments = std::vector<std::string_view>{};

    while (it != last) {
        auto c = *it;

        if (c == '\n') {
            ++line_nr;
        }

        switch (S(state, c)) {
        // Handle switch to text-mode '{{'.
        case S(verbatim, '{'):
            state = verbatim_brace;
            break;

        case S(verbatim_brace, '{'):
            state = verbatim_dbrace;
            break;

        case S(verbatim_dbrace, '{'):
            break;

        // Handle char-literal
        case S(verbatim, '\''):
            state = verbatim_squote;
            break;

        case S(verbatim_squote, '\''):
            state = verbatim;
            break;

        case S(verbatim_squote, '\\'):
            state = verbatim_squote_escape;
            break;

        // Handle string-literal
        case S(verbatim, '"'):
            state = verbatim_dquote;
            break;

        case S(verbatim_dquote, '"'):
            state = verbatim;
            break;

        case S(verbatim_dquote, '\\'):
            state = verbatim_dquote_escape;
            break;

        // Handle switch to verbatim-mode
        case S(text, '}'):
            state = text_brace;
            break;

        case S(text_brace, '}'):
            if (token_start != it - 1) {
                co_yield csp_token{csp_token::type::text, std::string_view{token_start, it - 1}};
            }
            token_start = it + 1;
            state = verbatim;
            break;

        case S(text, '$'):
            state = text_dollar;
            break;

        case S(text_dollar, '{'):
            if (token_start != it - 1) {
                co_yield csp_token{csp_token::type::text, std::string_view{token_start, it - 1}};
            }
            token_start = it + 1;
            state = placeholder;
            break;

        case S(line_verbatim, '\n'):
            if (token_start != it + 1) {
                co_yield csp_token{csp_token::type::verbatim, std::string_view{token_start, it + 1}};
            }
            token_start = it + 1;
            state = text;
            break;

        case S(placeholder, ','):
            if (subexpression_stack.empty()) {
                if (std::exchange(is_filter, false)) {
                    filters.emplace_back(token_start, it);
                } else {
                    arguments.emplace_back(token_start, it);
                }
                token_start = it + 1;
            }
            break;

        case S(placeholder, '`'):
            if (subexpression_stack.empty()) {
                if (std::exchange(is_filter, true)) {
                    filters.emplace_back(token_start, it);

                } else if (token_start != it or not arguments.empty()) {
                    // Only add a empty argument if there where arguments before.
                    arguments.emplace_back(token_start, it);
                }
                token_start = it + 1;
            }
            break;

        case S(placeholder, '{'):
            subexpression_stack.push_back('}');
            break;

        case S(placeholder, '('):
            subexpression_stack.push_back(')');
            break;

        case S(placeholder, '['):
            subexpression_stack.push_back(']');
            break;

        case S(placeholder, '}'):
            if (subexpression_stack.empty()) {
                if (std::exchange(is_filter, false)) {
                    filters.emplace_back(token_start, it);

                } else if (token_start != it or not arguments.empty()) {
                    // Only add a empty argument if there where arguments before.
                    arguments.emplace_back(token_start, it);
                }

                co_yield csp_token{csp_token::type::placeholder, std::string_view{}, std::move(arguments), std::move(filters)};

                arguments.clear();
                filters.clear();

                token_start = it + 1;
                state = text;
                // Don't fallthrough, but iterator must be incremented.
                ++it;
                continue;
            }
            [[fallthrough]];
        case S(placeholder, ')'):
        case S(placeholder, ']'):
            if (subexpression_stack.empty()) {
                throw csp_error(std::format("{}:{}: Unexpected '{}' in placeholder expression", path.string(), line_nr, c));
            } else if (subexpression_stack.back() != c) {
                throw csp_error(std::format(
                    "{}:{}: Expecting '{}' got '{}' in placeholder expression",
                    path.string(),
                    line_nr,
                    subexpression_stack.back(),
                    c));
            } else {
                subexpression_stack.pop_back();
            }
            break;

        // Handle char-literal
        case S(placeholder, '\''):
            state = placeholder_squote;
            break;

        case S(placeholder_squote, '\''):
            state = placeholder;
            break;

        case S(placeholder_squote, '\\'):
            state = placeholder_squote_escape;
            break;

        // Handle string-literal
        case S(placeholder, '"'):
            state = placeholder_dquote;
            break;

        case S(placeholder_dquote, '"'):
            state = placeholder;
            break;

        case S(placeholder_dquote, '\\'):
            state = placeholder_dquote_escape;
            break;

        default:
            switch (state) {
            case verbatim_squote_escape:
                state = verbatim_squote;
                break;

            case verbatim_dquote_escape:
                state = verbatim_dquote;
                break;

            case verbatim_brace:
                state = verbatim;
                continue;

            case verbatim_dbrace:
                if (token_start != it - 2) {
                    co_yield csp_token{csp_token::type::verbatim, std::string_view{token_start, it - 2}};
                }
                token_start = it;
                state = text;
                continue;

            case text_brace:
                state = text;
                continue;

            case text_dollar:
                if (token_start != it - 1) {
                    auto const text = std::string_view{token_start, it - 1};
                    auto const line_end = text.rfind('\n');

                    if (line_end == text.npos) {
                        // If no line-feed was found then that means there was
                        // a start-of-text or placeholder on this same line.
                        // Therefor include all the text on this line.
                        co_yield csp_token{csp_token::type::text, text};

                    } else {
                        auto const has_visible = std::count_if(text.begin() + line_end + 1, text.end(), [](auto c) {
                            return c != ' ' and c != '\t';
                        });

                        if (has_visible) {
                            // The last line has visible characters, include the
                            // whole line.
                            co_yield csp_token{csp_token::type::text, text};

                        } else {
                            // The last line consists only of white-space
                            // characters, don't display it.
                            co_yield csp_token{csp_token::type::text, text.substr(0, line_end)};
                        }
                    }
                }
                token_start = it;
                state = line_verbatim;
                continue;

            case placeholder_squote_escape:
                state = placeholder_squote;
                break;

            case placeholder_dquote_escape:
                state = placeholder_dquote;
                break;

            default:;
            }
        }

        ++it;
    }

    if (token_start != it) {
        switch (state) {
        case verbatim:
            co_yield csp_token{csp_token::type::verbatim, std::string_view{token_start, it}};
            break;

        case text:
            co_yield csp_token{csp_token::type::text, std::string_view{token_start, it}};
            break;

        case line_verbatim:
            co_yield csp_token{csp_token::type::verbatim, std::string_view{token_start, it}};
            break;

        default:
            throw csp_error{std::format(
                "{}:{}: Unexpected characters at end of template \"{}\"",
                path.string(),
                line_nr,
                std::string_view{token_start, it})};
        }
    }
}

generator<csp_token> parse_csp(std::string_view str, std::filesystem::path const& path)
{
    return parse_csp(str.data(), str.data() + str.size(), path);
}

}} // namespace csp::v1
