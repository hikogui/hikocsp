
/** @file csp_parser.hpp
 *
 *

 */

#pragma once

#include "generator.hpp"
#include <string_view>
#include <format>
#include <filesystem>
#include <cstdint>
#include <array>
#include <iterator>
#include <concepts>

namespace csp { inline namespace v1 {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class csp_token_type { verbatim, placeholder_argument, placeholder_filter, placeholder_end, text };

template<std::random_access_iterator It>
struct csp_token {
    It first;
    It last;
    int line_nr;
    csp_token_type kind;

    ~csp_token() = default;
    constexpr csp_token(csp_token const&) noexcept = default;
    constexpr csp_token(csp_token&&) noexcept = default;
    constexpr csp_token& operator=(csp_token const&) noexcept = default;
    constexpr csp_token& operator=(csp_token&&) noexcept = default;

    constexpr csp_token() noexcept : first(), last(), line_nr(), kind() {}

    constexpr csp_token(csp_token_type kind, int line_nr, It first, It last) noexcept :
        first(first), last(last), line_nr(line_nr), kind(kind)
    {
    }

    constexpr csp_token(csp_token_type kind, int line_nr) noexcept : csp_token(kind, line_nr, {}, {}) {}

    [[nodiscard]] constexpr std::string_view text() const noexcept
    {
        return std::string_view{first, last};
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return first == last;
    }

    explicit operator bool() const noexcept
    {
        return not empty();
    }
};

enum class csp_parse_state : uint8_t { placeholder, line_verbatim, verbatim };

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
[[nodiscard]] constexpr csp_token<It> csp_find_end_text(It& first, It last, int& line_nr, csp_parse_state& out) noexcept
{
    enum class state_type : uint8_t { idle, found_dollar, found_cbrace };

    auto num_lines = 0;
    auto state = state_type::idle;
    auto r = csp_token<It>{csp_token_type::text, line_nr};

    for (auto it = first; it != last; ++it) {
        switch (*it) {
        case '$':
            state = state_type::found_dollar;
            continue;

        case '}':
            if (state == state_type::found_dollar) {
                // The close-brace belongs to a C++ line-verbatim.
                r.first = first;
                r.last = it - 1;
                first = it;
                line_nr += num_lines;
                out = csp_parse_state::line_verbatim;
                return r;

            } else if (state == state_type::found_cbrace) {
                r.first = first;
                r.last = it - 1;
                first = it + 1;
                line_nr += num_lines;
                out = csp_parse_state::verbatim;
                return r;

            } else {
                state = state_type::found_cbrace;
                continue;
            }
            break;

        case '{':
            if (state == state_type::found_dollar) {
                r.first = first;
                r.last = it - 1;
                first = it + 1;
                line_nr += num_lines;
                out = csp_parse_state::placeholder;
                return r;
            }
            break;

        case '\n':
            ++num_lines;
            break;

        default:
            if (state == state_type::found_dollar) {
                r.first = first;
                r.last = it - 1;
                first = it;
                line_nr += num_lines;
                out = csp_parse_state::line_verbatim;
                return r;
            }
        }

        state = state_type::idle;
    }

    r.first = first;
    r.last = last;
    first = last;
    line_nr += num_lines;
    out = csp_parse_state::verbatim;
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
 */
template<std::random_access_iterator It>
[[nodiscard]] inline csp_token<It> csp_find_end_expression(It& first, It last, int& line_nr, bool is_filter)
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
                    throw csp_error(std::format("{}: Subexpression nesting is too deep.", line_nr));
                }
                stack[stack_size++] = *it == '{' ? '}' : *it == '(' ? ')' : ']';
            }
            break;

        case '}':
        case ')':
        case ']':
            if (not quote) {
                if (stack_size == 0) {
                    r.first = first;
                    r.last = it;
                    line_nr += num_lines;
                    first = it;
                    return r;

                } else if (stack[--stack_size] != *it) {
                    throw csp_error(std::format(
                        "{}: Unexpected {} when terminating subexpression, expecting {}", line_nr, *it, stack[stack_size]));
                }
            }
            break;

        case ',':
        case '`':
        case '@':
        case '$':
            if (not quote and stack_size == 0) {
                r.first = first;
                r.last = it;
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

    throw csp_error(std::format("{}: Unexpected EOF parsing C++ expression", line_nr));
}

template<std::random_access_iterator It>
[[nodiscard]] constexpr csp_token<It> csp_find_end_line_verbatim(It& first, It last, int& line_nr) noexcept
{
    auto r = csp_token<It>{csp_token_type::verbatim, line_nr};

    for (auto it = first; it != last; ++it) {
        if (*it == '\n') {
            r.first = first;
            r.last = it + 1;
            ++line_nr;
            first = it + 1;
            return r;
        }
    }

    r.first = first;
    r.last = last;
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
[[nodiscard]] constexpr csp_token<It> csp_find_end_verbatim(It& first, It last, int& line_nr) noexcept
{
    auto quote = '\0';
    bool escape = false;
    int num_obrace = 0;
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
                num_obrace = 0;
                continue;
            }
            break;

        case '{':
            ++num_obrace;
            escape = false;
            continue;

        case '\n':
            ++num_lines;
            break;

        default:
            if (not quote and num_obrace >= 2) {
                // At this point we just beyond the last two open-braces '{{'
                // in a sequence of open-braces.
                r.first = first;
                r.last = it - 2;
                line_nr += num_lines;
                first = it;
                return r;
            }
        }

        escape = false;
        num_obrace = 0;
    }

    // No double open-braces '{{' found in str.
    r.first = first;
    r.last = last;
    line_nr += num_lines;
    first = last;
    return r;
}

template<std::random_access_iterator It>
generator<csp_token<It>> parse_csp(It first, It last)
{
    int line_nr = 0;

    while (first != last) {
        if (auto const token = csp_find_end_verbatim(first, last, line_nr)) {
            co_yield token;
        }

        while (first != last) {
            auto next_token = csp_parse_state{};
            if (auto const token = csp_find_end_text(first, last, line_nr, next_token)) {
                co_yield token;
            }

            if (next_token == csp_parse_state::verbatim) {
                // This is either verbatim C++ or EOF.
                // Break from the text-loop; loop back to C++ verbatim.
                break;

            } else if (next_token == csp_parse_state::line_verbatim) {
                if (auto const token = csp_find_end_line_verbatim(first, last, line_nr)) {
                    co_yield token;
                }

            } else {
                // Found placeholder
                auto num_arguments = 0;
                auto is_filter = false;
                while (true) {
                    if (first == last) {
                        throw csp_error(std::format("{}: Incomplete placeholder found.", line_nr));

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
                        if (auto const token = csp_find_end_expression(first, last, line_nr, is_filter)) {
                            co_yield token;
                        }
                        is_filter = false;
                    }
                }
            }
        }
    }
}

generator<csp_token<std::string_view::const_iterator>> parse_csp(std::string_view str, std::filesystem::path const& path)
{
    try {
        for (auto& x : parse_csp(str.begin(), str.end())) {
            co_yield x;
        }
    } catch (csp_error const& e) {
        throw csp_error(std::format("{}:{}", path.string(), e.what()));
    }
}
}} // namespace csp::v1
