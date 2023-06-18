
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
#include <array>

namespace csp { inline namespace v1 {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct csp_token {
    enum class type { verbatim, placeholder_argument, placeholder_filter, placeholder_end, text };

    std::string_view text;
    int line_nr;
    type kind;

    ~csp_token() = default;
    constexpr csp_token(csp_token const&) noexcept = default;
    constexpr csp_token(csp_token&&) noexcept = default;
    constexpr csp_token& operator=(csp_token const&) noexcept = default;
    constexpr csp_token& operator=(csp_token&&) noexcept = default;

    constexpr csp_token() noexcept : text(), line_nr(), kind() {}

    constexpr csp_token(type kind, int line_nr, std::string_view text) noexcept : text(text), line_nr(line_nr), kind(kind) {}

    constexpr csp_token(type kind, int line_nr) noexcept : csp_token(kind, line_nr, std::string_view{}) {}

    constexpr csp_token(type kind, int line_nr, char const *first, char const *last) noexcept :
        csp_token(kind, line_nr, std::string_view{first, last})
    {
    }
};

template<typename T>
constexpr auto to_underlying(T x) noexcept
    requires(std::is_enum_v<T>)
{
    return static_cast<std::underlying_type_t<T>>(x);
}

generator<csp_token> parse_csp(char const *first, char const *last, std::filesystem::path const& path)
{
    // 16 states.
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
    constexpr auto state_type_size = size_t{16};

    using enum state_type;

    enum class jump_type : uint8_t {
        verbatim_j_obrace,
        verbatim_brace_j_obrace,
        verbatim_dbrace_j_obrace,
        verbatim_j_squote,
        verbatim_squote_j_squote,
        verbatim_squote_j_bslash,
        verbatim_j_dquote,
        verbatim_dquote_j_dquote,
        verbatim_dquote_j_bslash,
        text_j_cbrace,
        text_brace_j_cbrace,
        text_j_dollar,
        text_dollar_j_obrace,
        placeholder_j_comma,
        placeholder_j_bquote,
        placeholder_j_obrace,
        placeholder_j_oparen,
        placeholder_j_obracket,
        placeholder_j_cbrace,
        placeholder_j_cparen,
        placeholder_j_cbracket,
        placeholder_j_squote,
        placeholder_squote_j_squote,
        placeholder_squote_j_bslash,
        placeholder_j_dquote,
        placeholder_dquote_j_dquote,
        placeholder_dquote_j_bslash,
        line_verbatim_j_linefeed,
        verbatim_squote_escape_j_linefeed,
        verbatim_squote_escape_j,
        verbatim_dquote_escape_j_linefeed,
        verbatim_dquote_escape_j,
        verbatim_brace_j_linefeed,
        verbatim_brace_j,
        verbatim_dbrace_j_linefeed,
        verbatim_dbrace_j,
        text_brace_j_linefeed,
        text_brace_j,
        text_dollar_j_linefeed,
        text_dollar_j,
        placeholder_squote_escape_j_linefeed,
        placeholder_squote_escape_j,
        placeholder_dquote_escape_j_linefeed,
        placeholder_dquote_escape_j,
        default_j_linefeed,
        default_j
    };
    using enum jump_type;

    constexpr auto jump_index = [](state_type s, char c) {
        return (static_cast<size_t>(s) << 8) | static_cast<uint8_t>(c);
    };

    // One entry for each state (16) and character (256).
    constexpr auto jump_table = [] {
        constexpr auto jump_table_size = state_type_size * 0x100;
        auto r = std::array<jump_type, jump_table_size>{};

        for (auto state = 0; state != state_type_size; ++state) {
            r[jump_index(static_cast<state_type>(state), '\n')] = default_j_linefeed;
        }

        for (auto c = 0; c != 256; ++c) {
            for (auto state = 0; state != state_type_size; ++state) {
                r[jump_index(static_cast<state_type>(state), c)] = default_j;
            }
            r[jump_index(verbatim_squote_escape, static_cast<char>(c))] = verbatim_squote_escape_j;
            r[jump_index(verbatim_dquote_escape, static_cast<char>(c))] = verbatim_dquote_escape_j;
            r[jump_index(verbatim_brace, static_cast<char>(c))] = verbatim_brace_j;
            r[jump_index(verbatim_dbrace, static_cast<char>(c))] = verbatim_dbrace_j;
            r[jump_index(text_brace, static_cast<char>(c))] = text_brace_j;
            r[jump_index(text_dollar, static_cast<char>(c))] = text_dollar_j;
            r[jump_index(placeholder_squote, static_cast<char>(c))] = placeholder_squote_escape_j;
            r[jump_index(placeholder_dquote, static_cast<char>(c))] = placeholder_dquote_escape_j;
        }

        r[jump_index(verbatim_squote_escape, '\n')] = verbatim_squote_escape_j_linefeed;
        r[jump_index(verbatim_dquote_escape, '\n')] = verbatim_dquote_escape_j_linefeed;
        r[jump_index(verbatim_brace, '\n')] = verbatim_brace_j_linefeed;
        r[jump_index(verbatim_dbrace, '\n')] = verbatim_dbrace_j_linefeed;
        r[jump_index(text_brace, '\n')] = text_brace_j_linefeed;
        r[jump_index(text_dollar, '\n')] = text_dollar_j_linefeed;
        r[jump_index(placeholder_squote, '\n')] = placeholder_squote_escape_j_linefeed;
        r[jump_index(placeholder_dquote, '\n')] = placeholder_dquote_escape_j_linefeed;

        r[jump_index(verbatim, '{')] = verbatim_j_obrace;
        r[jump_index(verbatim_brace, '{')] = verbatim_brace_j_obrace;
        r[jump_index(verbatim_dbrace, '{')] = verbatim_dbrace_j_obrace;
        r[jump_index(verbatim, '\'')] = verbatim_j_squote;
        r[jump_index(verbatim_squote, '\'')] = verbatim_squote_j_squote;
        r[jump_index(verbatim_squote, '\\')] = verbatim_squote_j_bslash;
        r[jump_index(verbatim, '"')] = verbatim_j_dquote;
        r[jump_index(verbatim_dquote, '"')] = verbatim_dquote_j_dquote;
        r[jump_index(verbatim_dquote, '\\')] = verbatim_dquote_j_bslash;
        r[jump_index(text, '}')] = text_j_cbrace;
        r[jump_index(text_brace, '}')] = text_brace_j_cbrace;
        r[jump_index(text, '$')] = text_j_dollar;
        r[jump_index(text_dollar, '{')] = text_dollar_j_obrace;
        r[jump_index(placeholder, ',')] = placeholder_j_comma;
        r[jump_index(placeholder, '`')] = placeholder_j_bquote;
        r[jump_index(placeholder, '{')] = placeholder_j_obrace;
        r[jump_index(placeholder, '(')] = placeholder_j_oparen;
        r[jump_index(placeholder, '[')] = placeholder_j_obracket;
        r[jump_index(placeholder, '}')] = placeholder_j_cbrace;
        r[jump_index(placeholder, ')')] = placeholder_j_cparen;
        r[jump_index(placeholder, ']')] = placeholder_j_cbracket;
        r[jump_index(placeholder, '\'')] = placeholder_j_squote;
        r[jump_index(placeholder_squote, '\'')] = placeholder_squote_j_squote;
        r[jump_index(placeholder_squote, '\\')] = placeholder_squote_j_bslash;
        r[jump_index(placeholder, '"')] = placeholder_j_dquote;
        r[jump_index(placeholder_dquote, '"')] = placeholder_dquote_j_dquote;
        r[jump_index(placeholder_dquote, '\\')] = placeholder_dquote_j_bslash;
        r[jump_index(line_verbatim, '\n')] = line_verbatim_j_linefeed;

        return r;
    }();

    constexpr auto jump = [](state_type s, char c) {
        return jump_table[jump_index(s, c)];
    };

    auto token_start = first;
    auto co_it = first;
    auto co_last = last;
    auto co_line_nr = 1;
    auto co_state = state_type::verbatim;
    std::array<char, 64> stack;
    auto co_stack_size = uint8_t{0};
    auto is_filter = false;
    auto has_arguments = false;

    while (true) {
        // Because we are parsing character-per-character this needs to be
        // very fast. We create local (CPU registers) variables before we
        // enter the inner-loop. The inner loop uses goto-to-outer-loop
        // to yield a value so that it can restore the co_* variables.
        auto it = co_it;
        auto last = co_last;
        auto line_nr = co_line_nr;
        auto state = co_state;
        auto stack_size = co_stack_size;
        csp_token token;

        while (it != last) {
            switch (jump(state, *it)) {
            // Handle switch to text-mode '{{'.
            case verbatim_j_obrace:
                state = verbatim_brace;
                break;

            case verbatim_brace_j_obrace:
                state = verbatim_dbrace;
                break;

            case verbatim_dbrace_j_obrace:
                break;

            // Handle char-literal
            case verbatim_j_squote:
                state = verbatim_squote;
                break;

            case verbatim_squote_j_squote:
                state = verbatim;
                break;

            case verbatim_squote_j_bslash:
                state = verbatim_squote_escape;
                break;

            // Handle string-literal
            case verbatim_j_dquote:
                state = verbatim_dquote;
                break;

            case verbatim_dquote_j_dquote:
                state = verbatim;
                break;

            case verbatim_dquote_j_bslash:
                state = verbatim_dquote_escape;
                break;

            // Handle switch to verbatim-mode
            case text_j_cbrace:
                state = text_brace;
                break;

            case text_brace_j_cbrace:
                {
                    auto const text = std::string_view{token_start, it - 1};
                    token_start = ++it;
                    state = verbatim;

                    if (not text.empty()) {
                        token = csp_token(csp_token::type::text, line_nr, text);
                        goto do_yield;
                    }
                }
                continue;

            case text_j_dollar:
                state = text_dollar;
                break;

            case text_dollar_j_obrace:
                {
                    auto const text = std::string_view{token_start, it - 1};
                    token_start = ++it;
                    state = placeholder;
                    if (not text.empty()) {
                        token = csp_token(csp_token::type::text, line_nr, text);
                        goto do_yield;
                    }
                }
                continue;

            case placeholder_j_comma:
                {
                    if (stack_size == 0) {
                        auto const text = std::string_view{token_start, it};
                        token_start = ++it;
                        has_arguments |= not is_filter;

                        auto const t = is_filter ? csp_token::type::placeholder_filter : csp_token::type::placeholder_argument;
                        token = csp_token(t, line_nr, text);
                        goto do_yield;

                    } else {
                        ++it;
                    }
                }
                continue;

            case placeholder_j_bquote:
                {
                    if (stack_size == 0) {
                        auto const text = std::string_view{token_start, it};
                        auto const t = is_filter ? csp_token::type::placeholder_filter : csp_token::type::placeholder_argument;

                        is_filter = true;
                        token_start = ++it;

                        if (t == csp_token::type::placeholder_filter or not text.empty() or has_arguments) {
                            has_arguments |= t == csp_token::type::placeholder_argument;
                            token = csp_token(t, line_nr, text);
                            goto do_yield;
                        }

                    } else {
                        ++it;
                    }
                }
                continue;

            case placeholder_j_obrace:
                if (stack_size == stack.size()) {
                    throw csp_error(
                        std::format("{}:{}: Subexpression stack is too deep, while processing {}.", path.string(), line_nr, *it));
                }
                stack[stack_size++] = '}';
                break;

            case placeholder_j_oparen:
                if (stack_size == stack.size()) {
                    throw csp_error(
                        std::format("{}:{}: Subexpression stack is too deep, while processing {}.", path.string(), line_nr, *it));
                }
                stack[stack_size++] = ')';
                break;

            case placeholder_j_obracket:
                if (stack_size == stack.size()) {
                    throw csp_error(
                        std::format("{}:{}: Subexpression stack is too deep, while processing {}.", path.string(), line_nr, *it));
                }
                stack[stack_size++] = ']';
                break;

            case placeholder_j_cbrace:
                if (stack_size == 0) {
                    if (is_filter or token_start != it or has_arguments) {
                        auto const text = std::string_view{token_start, it};
                        auto const t = is_filter ? csp_token::type::placeholder_filter : csp_token::type::placeholder_argument;

                        // return to back to this (placeholder_j_cbrace) state for further processing.
                        // clearing the following variables will skip this if-statement.
                        token_start = it;
                        is_filter = false;
                        has_arguments = false;

                        token = csp_token(t, line_nr, text);
                        goto do_yield;
                    }

                    token_start = ++it;
                    state = text;
                    token = csp_token(csp_token::type::placeholder_end, line_nr);
                    goto do_yield;
                }
                [[fallthrough]];
            case placeholder_j_cparen:
            case placeholder_j_cbracket:
                if (stack_size == 0) {
                    throw csp_error(std::format("{}:{}: Unexpected '{}' in placeholder expression", path.string(), line_nr, *it));

                } else if (stack[--stack_size] != *it) {
                    throw csp_error(std::format(
                        "{}:{}: Expecting '{}' got '{}' in placeholder expression", path.string(), line_nr, stack.back(), *it));
                }
                ++it;
                continue;

            // Handle char-literal
            case placeholder_j_squote:
                state = placeholder_squote;
                break;

            case placeholder_squote_j_squote:
                state = placeholder;
                break;

            case placeholder_squote_j_bslash:
                state = placeholder_squote_escape;
                break;

            // Handle string-literal
            case placeholder_j_dquote:
                state = placeholder_dquote;
                break;

            case placeholder_dquote_j_dquote:
                state = placeholder;
                break;

            case placeholder_dquote_j_bslash:
                state = placeholder_dquote_escape;
                break;

            case line_verbatim_j_linefeed:
                {
                    ++line_nr;
                    auto const text = std::string_view{token_start, it + 1};

                    token_start = it + 1;
                    state = state_type::text;

                    if (not text.empty()) {
                        token = csp_token(csp_token::type::verbatim, line_nr, text);
                        goto do_yield;
                    }
                }
                continue;

            case default_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case default_j:
                break;

            case verbatim_squote_escape_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case verbatim_squote_escape_j:
                state = verbatim_squote;
                break;

            case verbatim_dquote_escape_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case verbatim_dquote_escape_j:
                state = verbatim_dquote;
                break;

            case verbatim_brace_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case verbatim_brace_j:
                state = verbatim;
                continue;

            case verbatim_dbrace_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case verbatim_dbrace_j:
                {
                    auto const text = std::string_view{token_start, it - 2};

                    token_start = it;
                    state = state_type::text;

                    if (not text.empty()) {
                        token = csp_token(csp_token::type::verbatim, line_nr, text);
                        goto do_yield;
                    }
                }
                continue;

            case text_brace_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case text_brace_j:
                state = text;
                continue;

            case text_dollar_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case text_dollar_j:
                {
                    auto const text = std::string_view{token_start, it - 1};
                    token_start = it;
                    state = line_verbatim;

                    if (not text.empty()) {
                        auto const line_end = text.rfind('\n');

                        if (line_end == text.npos) {
                            // If no line-feed was found then that means there was
                            // a start-of-text or placeholder on this same line.
                            // Therefor include all the text on this line.
                            token = csp_token(csp_token::type::text, line_nr, text);
                            goto do_yield;

                        } else {
                            auto const has_visible = std::count_if(text.begin() + line_end + 1, text.end(), [](auto c) {
                                return c != ' ' and c != '\t';
                            });

                            if (has_visible) {
                                // The last line has visible characters, include the
                                // whole line.
                                token = csp_token(csp_token::type::text, line_nr, text);
                                goto do_yield;

                            } else {
                                // The last line consists only of white-space
                                // characters, don't display it.
                                token = csp_token(csp_token::type::text, line_nr, text.substr(0, line_end));
                                goto do_yield;
                            }
                        }
                    }
                }
                continue;

            case placeholder_squote_escape_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case placeholder_squote_escape_j:
                state = placeholder_squote;
                break;

            case placeholder_dquote_escape_j_linefeed:
                ++line_nr;
                [[fallthrough]];
            case placeholder_dquote_escape_j:
                state = placeholder_dquote;
                break;

            default:
                // Remove bound check on switch statement.
                __assume(false);
            }

            ++it;
        }

        co_it = it;
        co_last = last;
        co_line_nr = line_nr;
        co_state = state;
        co_stack_size = stack_size;
        break;

do_yield:
        co_it = it;
        co_last = last;
        co_line_nr = line_nr;
        co_state = state;
        co_stack_size = stack_size;
        co_yield token;
    }

    if (token_start != co_it) {
        switch (co_state) {
        case verbatim:
            co_yield csp_token(csp_token::type::verbatim, co_line_nr, token_start, co_it);
            break;

        case text:
            co_yield csp_token(csp_token::type::text, co_line_nr, token_start, co_it);
            break;

        case line_verbatim:
            co_yield csp_token(csp_token::type::verbatim, co_line_nr, token_start, co_it);
            break;

        default:
            throw csp_error{std::format(
                "{}:{}: Unexpected characters at end of template \"{}\"",
                path.string(),
                co_line_nr,
                std::string_view{token_start, co_it})};
        }
    }
}

generator<csp_token> parse_csp(std::string_view str, std::filesystem::path const& path)
{
    return parse_csp(str.data(), str.data() + str.size(), path);
}
}} // namespace csp::v1
