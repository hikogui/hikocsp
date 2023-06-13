
/** @file csp_parser.hpp
 *
 *
 * ```
 * #include <string>
 * #include <format>
 * #include <vector>
 *
 * [[nodiscard]] constexpr std::string generate_page(std::vector<int> list, int b) noexcept
 * {
 *   auto _out = std::string{};
 *   auto _default_escape = hi::sgml_escape;
 *   auto url = hi::url_escape;
 * $<
 * <html>
 *   <head><title>Page</title></head>
 *   <body>
 *     A list of values.
 *     <li>
 *       $for (auto value: list) {
 *       <ul><a href="value_page?${value `url}">${value} + ${b} = ${value + b}</a></ul>
 *       $}
 *     <li>
 *   </body>
 * </html>
 * $>
 *
 *   return _out;
 * }
 *
 * ```
 */

#pragma once

#include "csp_token.hpp"
#include <vector>
#include <string_view>

namespace hi { inline namespace v1 {
namespace detail {

class csp_parser {
public:
private:
    enum class state_type { verbatim, text };

    state_type _state = state_type::verbatim;
    char const *_first;
    char const *_last;
    char const *_it;
    std::string _path;
    std::vector<csp_token> _tokens;

    mutable std::pair<char const *, size_t> _line_cache;

    void emplace_token(token_kind kind, char const *first, char const *last) noexcept
    {
        if (first != last) {
            _tokens.emplace_back(kind, first, last);
        }
    }

    [[nodiscard]] constexpr size_t line_location(char const *position) const noexcept
    {
        auto it = _first;
        auto line = size_t{};

        if (position >= _line_cache.first) {
            std::tie(it, line) = _line_cache;
        }

        while (it < position) {
            if (*it++ == '\n') {
                ++line;
            }
        }

        _line_cache = {it, line};
        return line;
    }

    [[nodiscard]] constexpr std::string location(char const *it) const noexcept
    {
        return std::format("{}:{}", _path, line_location(it) + 1);
    }

    /** Parse a C++ expression.
     *
     * The expression is parsed until one of the following characters is found
     * outside of a string-literal or sub-expression: '}', ')', ']', ',', '`', '$' or '@'.
     *
     * @param[in,out] context The context to keep track of line_nr.
     * @param[in,out] it A char-pointer to the first character of a C++ expression.
     * @param last A char-pointer to the last character of the csp-file.
     * @return a placeholder or simple-placeholder token.
     * @throws csp_error syntax error.
     */
    [[nodiscard]] constexpr std::string_view parse_csp_expression()
    {
        auto const start = _it;
        auto in_sub_expression = std::vector<char>{};
        auto in_string = '\0';
        auto found_special = '\0';

        while (_it != last) {
            auto const c = *_it++;

            if (in_sub_expression.empty() and not in_string and
                (c == '}' or c == ')' or c == ']' or c == ',' or c == '`' or c == '$' or c == '@')) {
                // Do not include this last character.
                return std::string_view{start, --_it};
            }

            switch (c) {
            case '\\':
                if (found_special != c) {
                    found_special = c;
                    // prevent found_special to be reset.
                    continue;
                }
                break;

            case '"':
            case '\'':
                if (found_special == '\\') {
                } else if (not in_string) {
                    in_string = c;
                } else if (in_string == c) {
                    in_string = '\0';
                }
                break;

            case '{':
                in_sub_expression.push_back('}');
                break;

            case '(':
                in_sub_expression.push_back(')');
                break;

            case '[':
                in_sub_expression.push_back(']');
                break;

            case '}':
            case ')':
            case ']':
                if (in_sub_expression.back() != c) {
                    throw csp_error(std::format(
                        "{}: C++ placeholder expression has unbalanced bracket pair, expected '{}', got '{}'",
                        location(_it),
                        in_sub_expression.back(),
                        c));
                }
                in_sub_expression.pop_back();
                break;

            default:;
            }

            found_special = '\0';
        }

        throw csp_error(
            std::format("{}: C++ placeholder expression is not terminated with '}'", location(start)));
    }

    constexpr void parse_placeholder()
    {
        auto const start = _it;
        auto arguments = std::vector<std::string_view>{};
        auto filters = std::vector<std::string_view>{};

        auto is_argument = true;
        while (_it != _last) {
            switch (auto const c = *_it) {
            case '}':
                emplace_token(state_type::place_holder, start, _it++, std::move(arguments), std::move(filters));
                state = state_type::text;
                return;

            case ',':
                is_argument = true;
                break;

            case '`':
                is_argument = false;
                break;

            case ']':
            case ')':
            case '@':
            case '$':
                throw csp_error(std::format("{}: Unexpected character '{}' at end of C++ expression.", location(start), c);

            default:
                if (is_argument) {
                    arguments.push_back(parse_expression());
                } else {
                    filters.push_back(parse_expression());
                }
            }
        }
    }

    constexpr void parse_cpp_line() noexcept
    {
        auto const start = _it;

        while (_it != _last) {
            if (*_it++ == '\n') {
                emplace_token(token_kind::verbatim, start, _it);
                state = state_type::text;
                return;
            }
        }

        emplace_token(token_kind::verbatim, start, _it);
        state = state_type::end;
    }

    constexpr void parse_text() noexcept
    {
        auto const start = _it;
        auto found_special = '\0';

        while (_it != _last) {
            auto const c = *_it++;
            switch (c) {
            case '$':
                if (found_special == '$') {
                    // Add text including the first '$' as this was a dollar escape.
                    emplace_token(token_kind::text, start, _it - 1);
                    state = state_type::text;
                    return;

                } else {
                    found_special == c;
                    // Prevent found_special to be reset.
                    continue;
                }
                break;

            case '{':
                if (found_special == '$') {
                    emplace_token(token_kind::text, start, _it - 2);
                    state = state_type::placeholder;
                    return;
                }
                break;

            case '>':
                if (found_special == '$') {
                    emplace_token(token_kind::text, start, _it - 2);
                    state = state_type::verbatim;
                    return;
                }

            default:
                if (found_special == '$') {
                    for (auto jt = _it - 1; jt != start; --jt) {
                        if (*(jt - 1) != ' ' and *(jt - 1) != '\t') {
                            // Emit text excluding trailing white-space.
                            emplace_token(token_kind::text, start, jt);
                            break;
                        }
                    }
                    state = state_type::cpp_line;
                    return;
                }
            }

            found_special = '\0';
        }

        emplace_token(token_kind::text, start, _it);
        state = state_type::end;
    }

    /** Parse verbatim C++ code upto and including "$<".
     */
    constexpr void parse_verbatim() noexcept
    {
        auto state = _it;
        auto in_string = '\0';
        auto found_special = '\0';

        while (_it != _last) {
            auto const c = *_it++;

            switch (c) {
            case '\\':
                if (found_special != '\\') {
                    found_special = c;
                    // Prevent found_special to be reset.
                    continue;
                }
                break;
            case '"':
            case '\'':
                if (found_special == '\\') {
                } else if (not in_string) {
                    in_string = c;
                } else if (in_string == c) {
                    in_string = '\0';
                }
                break;
            case '$':
                if (not in_string) {
                    found_special = c;
                    // Prevent found_special to be reset.
                    continue;
                }
                break;
            case '<':
                if (found_special == '$') {
                    emplace_token(csp_token::verbatim, start, _it - 2);
                    state = state_type::text;
                    return;
                }
                break;
            }

            found_special = '\0';
        }

        emplace_token(csp_token::verbatim, start, _it);
        state = state_type::end;
    }

    void parse()
    {
        while (true) {
            switch (state) {
            case state_type::verbatim:
                parse_verbatim();
                break;
            case state_type::text:
                parse_text();
                break;
            case state_type::placeholder:
                parse_placeholder();
                break;
            case state_type::cpp_line:
                parse_cpp_line();
                break;
            case state_type::end:
                return;
            }
        }
    }
};
} // namespace detail

[[nodiscard]] constexpr std::vector<csp_token> parse_csp(char const *it, char const *last, size_t& line_nr) noexcept {}

}} // namespace hi::v1
