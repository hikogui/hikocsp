// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "csp_token.hpp"
#include "generator.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <format>
#include <vector>
#include <ranges>
#include <iterator>
#include <optional>

namespace csp { inline namespace v1 {

std::string encode_string_literal(std::string_view str)
{
    auto r = std::string{};
    // Expect ASCII + \n at end.
    r.reserve(str.size() + 1);

    auto x_escape = false;
    for (auto c : str) {
        switch (c) {
        // Basic source character set.
        case '"':
            r += "\\\"";
            break;
        case '\\':
            r += "\\\\";
            break;
        case '\a':
            r += "\\a";
            break;
        case '\b':
            r += "\\b";
            break;
        case '\f':
            r += "\\f";
            break;
        case '\n':
            r += "\\n";
            break;
        case '\r':
            r += "\\r";
            break;
        case '\t':
            r += "\\t";
            break;
        case '\v':
            r += "\\v";
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            if (x_escape) {
                // x-escape sequence doesn't stop until a non-hex character is found.
                // Use double-quote doubling to terminate.
                r += "\"\"";
            }
            [[fallthrough]];

        default:
            if (c < 0x20 or c == 0x24 or c == 0x40 or c == 0x60 or c > 0x7e) {
                // Not part of the C++20 basic character set.
                r += std::format("\\x{:02x}", static_cast<uint8_t>(c));
                x_escape = true;
                continue;

            } else {
                r += c;
            }
        }

        x_escape = false;
    }

    return r;
}

struct translate_csp_config {
    bool enable_line;
    std::optional<std::string> callback_name;
    std::optional<std::string> append_name;
};

[[nodiscard]] inline std::string translate_csp_yield(std::string_view str, translate_csp_config const& config) noexcept
{
    if (config.callback_name) {
        return std::format("{}({});\n", *config.callback_name, str);
    } else if (config.append_name) {
        return std::format("{} += {};\n", *config.append_name, str);
    } else {
        return std::format("co_yield {};\n", str);
    }
}

[[nodiscard]] inline std::optional<std::string>
translate_csp_path(std::filesystem::path const& path, translate_csp_config const& config) noexcept
{
    if (config.enable_line) {
        return std::format("#line 1 \"{}\"\n", path.generic_string());
    } else {
        return std::nullopt;
    }
}

template<typename Token>
[[nodiscard]] std::optional<std::string> translate_csp_line(Token const& token, translate_csp_config const& config) noexcept
{
    if (config.enable_line) {
        return std::format("#line {}\n", token.line_nr);
    } else {
        return std::nullopt;
    }
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] generator<std::string>
translate_csp(It first, ItEnd last, std::filesystem::path const& path, translate_csp_config const& config) noexcept
{
    using namespace std::literals;

    auto arguments = std::vector<std::string>{};
    auto filters = std::vector<std::string>{};
    auto default_filters = std::vector<std::string>{};

    if (auto x = translate_csp_path(path, config)) {
        co_yield *x;
    }

    for (auto it = first; it != last; ++it) {
        auto const& token = *it;
        if (token.kind == csp_token_type::verbatim) {
            if (not token.text.empty()) {
                if (auto x = translate_csp_line(token, config)) {
                    co_yield *x;
                }

                co_yield token.text;
                if (token.text.back() != '\n') {
                    co_yield "\n";
                }
            }

        } else if (token.kind == csp_token_type::text) {
            if (not token.text.empty()) {
                if (auto x = translate_csp_line(token, config)) {
                    co_yield *x;
                }

                auto const num_lines = std::count(token.text.begin(), token.text.end(), '\n');
                if (num_lines == 0 or (num_lines == 1 and token.text.back() == '\n')) {
                    // Only one line.
                    co_yield translate_csp_yield(std::format("\"{}\"", encode_string_literal(token.text)), config);

                } else {
                    auto str = std::string{};

                    auto prev_i = size_t{};
                    auto i = token.text.find('\n');
                    while (i != token.text.npos) {
                        ++i;

                        if (prev_i != 0) {
                            str += "\n  ";
                        }
                        str += std::format("\"{}\"", encode_string_literal(token.text.substr(prev_i, i - prev_i)));

                        i = token.text.find('\n', prev_i = i);
                    }

                    if (prev_i != token.text.size()) {
                        str += std::format("\n  \"{}\"", encode_string_literal(token.text.substr(prev_i)));
                    }
                    co_yield translate_csp_yield(str, config);
                }
            }

        } else if (token.kind == csp_token_type::placeholder_argument) {
            arguments.emplace_back(token.text);

        } else if (token.kind == csp_token_type::placeholder_filter) {
            if (token.empty()) {
                filters.emplace_back("[](auto &x){return x;}");
            } else {
                filters.emplace_back(token.text);
            }

        } else if (token.kind == csp_token_type::placeholder_end) {
            if (arguments.empty()) {
                if (filters.empty()) {
                    // empty placeholder.
                } else {
                    // update default filters.
                    default_filters.clear();
                    for (auto& filter : filters) {
                        default_filters.push_back(filter);
                    }
                }

            } else if (
                filters.empty() and arguments.size() == 1 and arguments.front().front() == '"' and
                arguments.front().back() == '"') {
                // Escape.
                if (auto x = translate_csp_line(token, config)) {
                    co_yield *x;
                }

                co_yield translate_csp_yield(arguments.front(), config);

            } else {
                if (filters.empty()) {
                    filters = default_filters;
                }

                if (arguments.size() == 1) {
                    arguments.emplace(arguments.begin(), "\"{}\"");
                }

                if (auto x = translate_csp_line(token, config)) {
                    co_yield *x;
                }

                auto str = std::string{};
                for (auto& filter : std::views::reverse(filters)) {
                    str += std::format("({})(", filter);
                }

                str += "std::format("s;
                auto is_first_argument = true;
                for (auto& argument : arguments) {
                    if (not is_first_argument) {
                        str += ", ";
                    }
                    str += std::format("({})", argument);
                    is_first_argument = false;
                }
                str += ")"s;

                for (auto& filter : filters) {
                    str += ")"s;
                }

                co_yield translate_csp_yield(str, config);
            }

            arguments.clear();
            filters.clear();

        } else {
            std::terminate();
        }
    }
}
}} // namespace csp::v1
