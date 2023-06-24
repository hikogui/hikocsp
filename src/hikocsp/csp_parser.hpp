// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "generator.hpp"
#include "csp_error.hpp"
#include "csp_token.hpp"
#include <string_view>
#include <format>
#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <array>
#include <iterator>
#include <concepts>

namespace csp { inline namespace v1 {
namespace detail {

enum class parse_csp_after_text : uint8_t { placeholder, line_verbatim, verbatim, text };

/** Find the next token
 *
 * This finds the iterator at:
 *  - placeholder: starts with '${'
 *  - line-verbatim: starts with '$'
 *  - verbatim: starts with '{{'
 *
 * It also counts the number of new-lines found before the next part.
 *
 * @param str The string to parse.
 * @return iterator or last, num_lines, identifier for what follows.
 */
template<std::random_access_iterator It>
[[nodiscard]] constexpr csp_token<It> parse_csp_text(It& first, It last, int& line_nr, parse_csp_after_text& after) noexcept
{
    enum class state_type : uint8_t { idle, found_dollar, found_cbrace };

    auto num_lines = 0;
    auto state = state_type::idle;
    auto r = csp_token<It>{csp_token_type::text, line_nr};

    for (auto it = first; it != last; ++it) {
        switch (*it) {
        case '$':
            if (state == state_type::found_dollar) {
                // Escape dollar. This includes the first dollar, but not second dollar.
                r.text = std::string{first, it};
                first = it + 1;
                line_nr += num_lines;
                after = parse_csp_after_text::text;
                return r;

            } else {
                state = state_type::found_dollar;
                continue;
            }

        case '}':
            if (state == state_type::found_dollar) {
                // The close-brace belongs to a C++ line-verbatim.
                r.text = std::string{first, it - 1};
                first = it;
                line_nr += num_lines;
                after = parse_csp_after_text::line_verbatim;
                return r;

            } else if (state == state_type::found_cbrace) {
                r.text = std::string{first, it - 1};
                first = it + 1;
                line_nr += num_lines;
                after = parse_csp_after_text::verbatim;
                return r;

            } else {
                state = state_type::found_cbrace;
                continue;
            }
            break;

        case '{':
            if (state == state_type::found_dollar) {
                r.text = std::string{first, it - 1};
                first = it + 1;
                line_nr += num_lines;
                after = parse_csp_after_text::placeholder;
                return r;
            }
            break;

        case '\n':
            if (state == state_type::found_dollar) {
                // Escape line-feed. Actually is verbatim-line.
                // Make sure the \n is not counted here in num_lines.
                r.text = std::string{first, it - 1};
                first = it;
                line_nr += num_lines;
                after = parse_csp_after_text::line_verbatim;
                return r;
            }
            ++num_lines;
            break;

        default:
            if (state == state_type::found_dollar) {
                r.text = std::string{first, it - 1};
                first = it;
                line_nr += num_lines;
                after = parse_csp_after_text::line_verbatim;
                return r;
            }
        }

        state = state_type::idle;
    }

    r.text = std::string{first, last};
    first = last;
    line_nr += num_lines;
    after = parse_csp_after_text::verbatim;
    return r;
}

/** Find the end of a C++ expression.
 *
 * The C++ expressions ends when one of the following characters are found
 * outside of a subexpression or string-literal: })],`$@
 *
 * It also counts the number of new-lines found inside the expression.
 *
 * @param str The string to parse.
 * @return iterator or last on error, number of lines in the expression.
 *
 * Due to bug in MSVC: https://developercommunity.visualstudio.com/t/C3615-false-positive-when-early-return-f/10395567
 * this function can not be made constexpr.
 */
template<std::random_access_iterator It>
[[nodiscard]] csp_token<It>
parse_csp_expression(It& first, It last, std::filesystem::path const& path, int& line_nr, bool is_filter)
{
    auto quote = '\0';
    bool escape = false;
    int num_lines = 0;
    std::array<char, 64> stack;
    uint8_t stack_size = 0;

    auto r = csp_token<It>{is_filter ? csp_token_type::placeholder_filter : csp_token_type::placeholder_argument, line_nr};

    for (auto it = first; it != last; ++it) {
        switch (*it) {
        case '"':
        case '\'':
            if (escape) {
                // Ignore quotes when escaping.
            } else if (not quote) {
                quote = *it;
            } else if (quote == *it) {
                quote = '\0';
            }
            break;

        case '\\':
            if (not escape) {
                escape = true;
                continue;
            }
            break;

        case '{':
        case '(':
        case '[':
            if (not quote) {
                if (stack_size == stack.size()) {
                    throw csp_error(std::format("{}:{}: Subexpression nesting is too deep.", path.string(), line_nr));
                }
                stack[stack_size++] = *it == '{' ? '}' : *it == '(' ? ')' : ']';
            }
            break;

        case '}':
        case ')':
        case ']':
            if (not quote) {
                if (stack_size == 0) {
                    r.text = std::string{first, it};
                    line_nr += num_lines;
                    first = it;
                    return r;

                } else if (stack[--stack_size] != *it) {
                    throw csp_error(std::format(
                        "{}:{}: Unexpected {} when terminating subexpression, expecting {}",
                        path.string(),
                        line_nr,
                        *it,
                        stack[stack_size]));
                }
            }
            break;

        case ',':
        case '`':
        case '@':
        case '$':
            if (not quote and stack_size == 0) {
                r.text = std::string{first, it};
                line_nr += num_lines;
                first = it;
                return r;
            }
            break;

        case '\n':
            ++num_lines;
            break;

        default:;
        }

        escape = false;
    }

    throw csp_error(std::format("{}:{}: Unexpected EOF parsing C++ expression", path.string(), line_nr));
}

template<std::random_access_iterator It>
[[nodiscard]] constexpr csp_token<It> parse_csp_line_verbatim(It& first, It last, int& line_nr) noexcept
{
    auto r = csp_token<It>{csp_token_type::verbatim, line_nr};

    for (auto it = first; it != last; ++it) {
        if (*it == '\n') {
            r.text = std::string{first, it + 1};
            ++line_nr;
            first = it + 1;
            return r;
        }
    }

    r.text = std::string{first, last};
    first = last;
    return r;
}

/** Find the end of a C++ verbatim.
 *
 * This finds the position at the last two braces '{{' in a sequence of
 * braces outside of C++ string-literals.
 *
 * It also counts the number of new-lines found before the double open-brace.
 *
 * @param str The string to parse.
 * @return iterator at '{{' or last, number of lines.
 */
template<std::random_access_iterator It>
[[nodiscard]] constexpr csp_token<It> parse_csp_verbatim(It& first, It last, int& line_nr) noexcept
{
    auto quote = '\0';
    bool escape = false;
    bool obrace = false;
    int num_lines = 0;
    auto r = csp_token<It>{csp_token_type::verbatim, line_nr};

    for (auto it = first; it != last; ++it) {
        switch (*it) {
        case '"':
        case '\'':
            if (escape) {
                // Ignore quotes when escaping.
            } else if (not quote) {
                quote = *it;
            } else if (quote == *it) {
                quote = '\0';
            }
            break;

        case '\\':
            if (not escape) {
                escape = true;
                obrace = false;
                continue;
            }
            break;

        case '{':
            if (not quote) {
                if (not obrace) {
                    obrace = true;
                    escape = false;
                    continue;
                } else {
                    // Two consecutive open-braces; find last open brace.
                    for (++it; it != last and *it == '{'; ++it) {}

                    r.text = std::string{first, it - 2};
                    line_nr += num_lines;
                    first = it;
                    return r;
                }
            }
            break;

        case '\n':
            ++num_lines;
            break;

        default:;
        }

        escape = false;
        obrace = false;
    }

    // No double open-braces '{{' found in str.
    r.text = std::string{first, last};
    line_nr += num_lines;
    first = last;
    return r;
}

} // namespace detail

template<std::random_access_iterator It>
generator<csp_token<It>> parse_csp(It first, It last, std::filesystem::path const& path)
{
    int line_nr = 1;

    while (first != last) {
        if (auto const token = detail::parse_csp_verbatim(first, last, line_nr)) {
            co_yield token;
        }

        while (first != last) {
            auto after = detail::parse_csp_after_text{};
            if (auto const token = detail::parse_csp_text(first, last, line_nr, after)) {
                co_yield token;
            }

            if (after == detail::parse_csp_after_text::verbatim) {
                // This is either verbatim C++ or EOF.
                // Break from the text-loop; loop back to C++ verbatim.
                break;

            } else if (after == detail::parse_csp_after_text::line_verbatim) {
                if (auto const token = detail::parse_csp_line_verbatim(first, last, line_nr)) {
                    co_yield token;
                }

            } else if (after == detail::parse_csp_after_text::text) {
                // Continue parsing text, found an escape.
                continue;

            } else if (after == detail::parse_csp_after_text::placeholder) {
                // Found placeholder
                auto num_arguments = 0;
                auto is_filter = false;
                while (true) {
                    if (first == last) {
                        throw csp_error(std::format("{}:{}: Incomplete placeholder found.", path.string(), line_nr));

                    } else if (*first == '}') {
                        if (is_filter) {
                            // An empty filter was found.
                            co_yield {csp_token_type::placeholder_filter, line_nr};
                        }

                        co_yield {csp_token_type::placeholder_end, line_nr};
                        ++first;
                        break;

                    } else if (*first == ',') {
                        ++first;

                    } else if (*first == '`') {
                        is_filter = true;
                        ++first;

                    } else {
                        if (auto const token = detail::parse_csp_expression(first, last, path, line_nr, is_filter)) {
                            co_yield token;
                        }
                        is_filter = false;
                    }
                }

            } else {
                std::terminate();
            }
        }
    }
}

auto parse_csp(std::string_view str, std::filesystem::path const& path)
{
    return parse_csp(str.begin(), str.end(), path);
}

}} // namespace csp::v1
