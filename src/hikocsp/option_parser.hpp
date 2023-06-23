// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cassert>
#include <format>

namespace csp { inline namespace v1 {

/** Results of low level parse options.
 */
struct parse_options_result {
    struct option_type {
        /** Name of the option.
         */
        std::string name;

        /** Optional argument.
         */
        std::optional<std::string> argument;

        [[nodiscard]] constexpr friend bool operator==(option_type const& lhs, std::string_view rhs) noexcept
        {
            return lhs.name == rhs;
        }

        [[nodiscard]] constexpr friend std::string to_string(option_type const& rhs) noexcept
        {
            if (not rhs.argument) {
                return rhs.name;
            } else if (rhs.name.starts_with("--")) {
                return std::format("{}={}", rhs.name, *rhs.argument);
            } else {
                return std::format("{} {}", rhs.name, *rhs.argument);
            }
        }
    };

    /** Name of the program argv[0].
     */
    std::string program_name;

    /** A list of parse options.
     */
    std::vector<option_type> options;

    /** A list of non-option arguments.
     */
    std::vector<std::string> arguments;
};

/** Low level parse options.
 *
 * @param args A list of arguments.
 * @param argument_required A list of short option that require an argument.
 * @return results.
 */
[[nodiscard]] constexpr parse_options_result
parse_options(std::vector<std::string_view> const& args, std::string_view argument_required) noexcept
{
    assert(not args.empty());

    auto r = parse_options_result{};

    auto it = args.begin();
    r.program_name = *it++;

    bool need_option_argument = false;
    for (; it != args.end(); ++it) {
        auto const arg = *it;

        if (need_option_argument) {
            assert(not r.options.empty());
            r.options.back().argument = arg;
            need_option_argument = false;

        } else if (arg.starts_with("--")) {
            auto const end_name = arg.find("=");
            if (end_name == std::string::npos) {
                r.options.emplace_back(std::string{arg}, std::nullopt);
            } else {
                r.options.emplace_back(std::string{arg.substr(0, end_name)}, std::string{arg.substr(end_name + 1)});
            }

        } else if (arg.starts_with("-")) {
            auto option_argument = std::string{};

            for (auto const c : arg.substr(1)) {
                if (need_option_argument) {
                    option_argument.push_back(c);

                } else {
                    r.options.emplace_back(std::format("-{}", c), std::nullopt);

                    if (argument_required.find(c) != std::string_view::npos) {
                        need_option_argument = true;
                    }
                }
            }

            if (not option_argument.empty()) {
                assert(not r.options.empty());
                r.options.back().argument = std::move(option_argument);
                need_option_argument = false;
            }

        } else {
            // Non-option argument
            r.arguments.emplace_back(arg);
        }
    }

    return r;
}

/** Low level parse options.
 *
 * @param args A list of arguments.
 * @param argument_required A list of short option that require an argument.
 * @return results.
 */
[[nodiscard]] parse_options_result parse_options(int argc, char *argv[], std::string_view argument_required) noexcept
{
    auto args = std::vector<std::string_view>{};
    for (auto i = 0; i != argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return parse_options(args, argument_required);
}

}} // namespace csp::v1
