
#include "hikocsp/csp_parser.hpp"
#include "hikocsp/csp_translator.hpp"
#include "hikocsp/option_parser.hpp"
#include <format>
#include <iostream>
#include <fstream>

inline int verbose = 0;
inline std::filesystem::path output_path = {};
inline std::filesystem::path input_path = {};

void print_help() {
    // clang-format off
    std::cerr << std::format(
        "hikocsp is an application to translate a CSP template into C++ code.\n"
        "\n"
        "Synopsys:\n"
        "  hikocsp --help\n"
        "  hikocsp [ <options> ] <path>\n"
        "  hikocsp [ <options> ] --input=<path>\n"
        "\n"
        "Options:\n"
        "  -h, --help          Show help and exit.\n"
        "  -v, --verbose       Increase verbosity level.\n"
        "  -i, --input=<path>  The path to the template file.\n"
        "  -o, --output=<path> The path to the generated code.\n"
        "\n"
        "If the output-path is not specified it is constructed from the\n"
        "input-path after removing the extension.\n"
    );
    //clang-format on
}

int parse_options(int argc, char *argv[])
{
    auto options = csp::parse_options(argc, argv, "io");

    for (auto& option : options.options) {
        if (option == "-h" or option == "--help") {
            return 1;

        } else if (option == "-v" or option == "--verbose") {
            ++verbose;

        } else if (option == "-o" or option == "--output") {
            if (option.argument) {
                output_path = *option.argument;
            } else {
                std::cerr << std::format("Missing argument for : {}\n", to_string(option));
                return -1;
            }

        } else if (option == "-i" or option == "--input") {
            if (option.argument) {
                input_path = *option.argument;
            } else {
                std::cerr << std::format("Missing argument for : {}\n", to_string(option));
                return -1;
            }

        } else {
            std::cerr << std::format("Unknown option: {}\n", to_string(option));
            return -1;
        }
    }

    if (input_path.empty()) {
        if (options.arguments.size() == 1) {
            input_path = options.arguments.front();
        } else {
            std::cerr << std::format("Expecting a non-option argument intput-path.\n");
            return -1;
        }

    } else if (not options.arguments.empty()) {
        std::cerr << std::format("Unexpected non-option arguments {}.\n", options.arguments.front());
        return -1;
    }

    if (output_path.empty()) {
        if (not output_path.has_extension())
            std::cerr << std::format("Can not produce output-path from intput-path {}.\n", input_path.string());
        return -1;

    } else {
        output_path = input_path.parent_path() / input_path.stem();
        if (verbose > 0) {
            std::cerr << std::format(
                "Using output-path {} constructed from input-path {}.\n", output_path.string(), input_path.string());
        }
    }

    return 0;
}

[[nodiscard]] std::string read_file(std::filesystem::path const &path)
{
    auto f = std::ifstream(path);
    if (not f.is_open()) {
        throw std::runtime_error(std::format("Could not open file {}.", path.string()));
    }
    auto r = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    return r;
}

int main(int argc, char *argv[])
{
    if (auto parse_state = parse_options(argc, argv)) {
        print_help();
        return parse_state == 1 ? 0 : -2;
    }

    try {
        auto text = read_file(input_path);
        auto tokens = csp::parse_csp(text, input_path);

        auto f = std::ofstream(output_path);
        for (auto const &str: csp::translate_csp(tokens.begin(), tokens.end(), input_path)) {
            f << str;
        }
        f.close();

    } catch (std::exception const &e) {
        std::cerr << std::format("Could not translate template: {}.", e.what());
        return -1;
    }

    return 0;
}
