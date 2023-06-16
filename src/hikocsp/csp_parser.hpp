
/** @file csp_parser.hpp
 *
 *
 
 */

#pragma once

#include "csp_token.hpp"
#include "line_count.hpp"
#include <vector>
#include <string_view>
#include <format>
#include <filesystem>

namespace hi { inline namespace v1 {
namespace detail {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class csp_parser {
public:
    csp_parser(char const *first, char const *last, std::filesystem::path path) noexcept :
        _state(state_type::verbatim),
        _begin(first),
        _end(last),
        _it(first),
        _path(std::move(path)),
        _tokens(),
        _line_cache{_begin, 0}
    {
    }

    [[nodiscard]] std::vector<csp_token> parse()
    {
        _parse();
        return _tokens;
    }

    [[nodiscard]] std::string location(char const *it) const noexcept
    {
        auto const line = line_count(_begin, _end, it);
        return std::format("{}:{}", _path.string(), line);
    }

private:
    enum class state_type { verbatim, text, placeholder, cpp_line, end };

    state_type _state;
    char const *_begin;
    char const *_end;
    char const *_it;
    std::filesystem::path _path;
    std::vector<csp_token> _tokens;

    mutable std::pair<char const *, size_t> _line_cache;

    void emplace_token(csp_token::type kind, std::string_view text) noexcept
    {
        if (not text.empty()) {
            _tokens.emplace_back(kind, text);
        }
    }

    void emplace_token(csp_token::type kind, char const *first, char const *last) noexcept
    {
        return emplace_token(kind, std::string_view{first, last});
    }

    void emplace_token(
        csp_token::type kind,
        std::string_view text,
        std::vector<std::string_view>&& arguments,
        std::vector<std::string_view>&& filters) noexcept
    {
        if (not text.empty()) {
            _tokens.emplace_back(kind, text, std::move(arguments), std::move(filters));
        }
    }

    void emplace_token(
        csp_token::type kind,
        char const *first,
        char const *last,
        std::vector<std::string_view>&& arguments,
        std::vector<std::string_view>&& filters) noexcept
    {
        return emplace_token(kind, std::string_view{first, last}, std::move(arguments), std::move(filters));
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
    [[nodiscard]] std::string_view parse_expression()
    {
        auto const start = _it;
        auto in_sub_expression = std::vector<char>{};
        auto in_string = '\0';
        auto found_special = '\0';

        while (_it != _end) {
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

        throw csp_error(std::format("{}: C++ placeholder expression is not terminated with '}}'", location(start)));
    }

    void parse_placeholder()
    {
        auto const start = _it;
        auto arguments = std::vector<std::string_view>{};
        auto filters = std::vector<std::string_view>{};

        auto is_argument = true;
        while (_it != _end) {
            switch (auto const c = *_it) {
            case '}':
                emplace_token(csp_token::type::placeholder, start, _it++, std::move(arguments), std::move(filters));
                _state = state_type::text;
                return;

            case ',':
                ++_it;
                is_argument = true;
                break;

            case '`':
                ++_it;
                is_argument = false;
                break;

            case ']':
            case ')':
            case '@':
            case '$':
                throw csp_error(std::format("{}: Unexpected character '{}' at end of C++ expression.", location(start), c));

            default:
                if (is_argument) {
                    arguments.push_back(parse_expression());
                } else {
                    filters.push_back(parse_expression());
                }
            }
        }
    }

    void parse_cpp_line() noexcept
    {
        auto const start = _it;

        while (_it != _end) {
            if (*_it++ == '\n') {
                emplace_token(csp_token::type::verbatim, start, _it);
                _state = state_type::text;
                return;
            }
        }

        emplace_token(csp_token::type::verbatim, start, _it);
        _state = state_type::end;
    }

    void parse_text() noexcept
    {
        auto const start = _it;
        auto found_special = '\0';

        while (_it != _end) {
            auto const c = *_it++;
            switch (c) {
            case '$':
                if (found_special == '$') {
                    // Add text including the first '$' as this was a dollar escape.
                    emplace_token(csp_token::type::text, start, _it - 1);
                    _state = state_type::text;
                    return;

                } else {
                    found_special = c;
                    // Prevent found_special to be reset.
                    continue;
                }
                break;

            case '{':
                if (found_special == '$') {
                    emplace_token(csp_token::type::text, start, _it - 2);
                    _state = state_type::placeholder;
                    return;
                }
                break;

            case '>':
                if (found_special == '$') {
                    emplace_token(csp_token::type::text, start, _it - 2);
                    _state = state_type::verbatim;
                    return;
                }

            default:
                if (found_special == '$') {
                    // Do not consume this character.
                    --_it;

                    // Strip of white-space preceding '$' when there is only
                    // white-space preceding '$' on a line.
                    auto const text = std::string_view{start, _it - 1};
                    if (auto const i = text.rfind('\n'); i == text.npos) {
                        // No line-feed.
                        emplace_token(csp_token::type::text, text);

                    } else {
                        auto const after = text.substr(i + 1);
                        auto const has_visible_chars = std::count_if(after.begin(), after.end(), [](auto c) {
                            return c != ' ' and c != '\t';
                        });

                        if (has_visible_chars) {
                            emplace_token(csp_token::type::text, text);
                        } else {
                            auto const before = text.substr(0, i + 1);
                            emplace_token(csp_token::type::text, before);
                        }
                    }

                    _state = state_type::cpp_line;
                    return;
                }
            }

            found_special = '\0';
        }

        emplace_token(csp_token::type::text, start, _it);
        _state = state_type::end;
    }

    /** Parse verbatim C++ code upto and including "$<".
     */
    void parse_verbatim() noexcept
    {
        auto start = _it;
        auto in_string = '\0';
        auto found_special = '\0';

        while (_it != _end) {
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
                    emplace_token(csp_token::type::verbatim, start, _it - 2);
                    _state = state_type::text;
                    return;
                }
                break;
            }

            found_special = '\0';
        }

        emplace_token(csp_token::type::verbatim, start, _it);
        _state = state_type::end;
    }

    void _parse()
    {
        while (true) {
            switch (_state) {
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

std::vector<csp_token> parse_csp(char const *begin, char const *end, std::filesystem::path path)
{
    auto p = detail::csp_parser(begin, end, std::move(path));
    return p.parse();
}

}} // namespace hi::v1
