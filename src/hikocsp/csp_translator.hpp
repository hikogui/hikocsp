

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

namespace csp { inline namespace v1 {

std::string encode_string_literal(std::string_view str)
{
    return std::string{str};
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] generator<std::string> translate_csp(It first, ItEnd last, std::filesystem::path const& path) noexcept
{
    using namespace std::literals;

    auto arguments = std::vector<std::string>{};
    auto filters = std::vector<std::string>{};
    auto default_filters = std::vector<std::string>{};

    co_yield std::format("#line 1 \"{}\"\n", path.generic_string());

    for (auto it = first; it != last; ++it) {
        auto const& token = *it;
        if (token.kind == csp_token_type::verbatim) {
            co_yield std::format("#line {}\n", token.line_nr + 1);
            co_yield token.text();
            co_yield "\n"s;

        } else if (token.kind == csp_token_type::text) {
            co_yield std::format("#line {}\n", token.line_nr + 1);
            co_yield "co_yield "s;
            for (auto line_range : std::views::split(token.text_view(), "\n"sv)) {
                auto line = std::string_view{line_range.begin(), line_range.end()};
                co_yield std::format("\"{}\"\n", encode_string_literal(line));
            }
            co_yield ";\n"s;

        } else if (token.kind == csp_token_type::placeholder_argument) {
            arguments.emplace_back(token.text());

        } else if (token.kind == csp_token_type::placeholder_filter) {
            if (token.text().empty()) {
                filters.emplace_back("[](auto &x){return x;}");
            } else {
                filters.emplace_back(token.text());
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
                co_yield std::format("#line {}\n", token.line_nr + 1);
                co_yield std::format("co_yield {};\n", arguments.front());

            } else {
                if (filters.empty()) {
                    filters = default_filters;
                }

                if (arguments.size() == 1) {
                    arguments.emplace(arguments.begin(), "\"{}\"");
                }

                co_yield std::format("#line {}\n", token.line_nr + 1);
                co_yield "co_yield "s;
                for (auto &filter: std::views::reverse(filters)) {
                    co_yield std::format("({})(", filter);
                }

                co_yield "std::format("s;
                auto is_first_argument = true;
                for (auto &argument: arguments) {
                    if (not is_first_argument) {
                        co_yield ", ";
                    }
                    co_yield std::format("({})", argument);
                    is_first_argument = false;
                }
                co_yield ")"s;

                for (auto &filter: filters) {
                    co_yield ")"s;
                }
                co_yield ";"s;
            }

            arguments.clear();
            filters.clear();
        } else {
            std::terminate();
        }
    }
}

}} // namespace csp::v1
