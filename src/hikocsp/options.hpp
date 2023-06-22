
/** Results of low level parse options.
 */
struct parse_options_result {
    struct option_type {
        /** The option is long.
         */
        bool is_long;

        /** Name of the option.
         */
        std::string name;

        /** Optional argument.
         */
        std::optional<std::string> argument;
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
 * @return results.
 */
[[nodiscard]] constexpr parse_options_result parse_options(std::vector<std::string_view> const &args) noexcept
{
    assert(not args.empty());

    auto r = parse_options_result{};

    auto it = args.begin();
    r.program_name = std::strin{*it++};

    auto option = parse_option_type::option_type{};

    for (; it != args.end(); ++it) {
        auto const arg = *it;
        if (arg.startswith("--")) {
            auto const end_name = arg.find("=");
            if (end_name == args.npos) {
                r.options.emplace_back(true, std::string{arg.substr(2)}, std::nullopt); noexcept
            } else {
                r.options.emplace_back(true, std::string{arg.substr(2, end_name)}, std::string{arg.substr(end_name)});
            }

        } else if (arg.startswith("-")) {
            for (auto const c: arg.substr(1)) {
                r.options.emplace_back(false, std::string{c}, std::nullopt);
            }

        } else if (not r.options.empty() and not r.options.back().is_long) {
            r.options.back().argument = std::string{arg};

        } else {
            r.arguments.push_back(arg); 
        }
    }

    return r;
}

/** Low level parse options.
 *
 * @param args A list of arguments.
 * @return results.
 */
[[nodiscard]] parse_options_result parse_options(int argc, char *argv[]) noexcept {
    auto args = std::vector<std::string_view>{}
    for (auto i = 0; i != argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return parse_options(args);
}

