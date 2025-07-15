#include "read_json.hpp"
#include <computo.hpp>
#include <computo_version.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Forward declaration for benchmark function
void run_performance_benchmarks();

void print_usage(const char* program_name) {
    // accept multiple input files
    std::cout << "Usage: " << program_name << " [OPTIONS] SCRIPT_FILENAME [INPUT_FILE [INPUT_FILENAME]...]\n"
              << "Options:\n"
              << "  -h, --help     Show this help message\n"
              << "  -v, --version  Show version information\n"
              << "  --perf         Run performance benchmarks\n"
              << "  --comments     Allow comments in script files\n"
              << "  --array KEY    Use custom array key syntax in scripts (default: 'array')\n"
              << "\n"
              << "If no input file is specified, uses null input.\n";
}

void print_version() {
    std::cout << "Computo CLI v" << COMPUTO_VERSION << "\n"
              << "JSON-native data transformation engine" << '\n';
}

// hold CLI options
struct CLIOptions {
    bool help = false;
    bool version = false;
    bool perf = false;
    bool comments = false;
    std::string array_key = "array"; // Default array key
    std::string bad_option;
    std::string script_filename;
    std::vector<std::string> input_filenames;
};

auto main(int argc, char* argv[]) -> int {
    CLIOptions options;

    // CLI option constants
    const std::string ARRAY_OPT = "--array";
    const std::string ARRAY_OPT_WITH_EQUALS = "--array=";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "-v" || arg == "--version") {
            options.version = true;
        } else if (arg == "--perf") {
            options.perf = true;
        } else if (arg == "--comments") {
            options.comments = true;
        } else if (arg.substr(0, ARRAY_OPT.length()) == ARRAY_OPT) {
            if (arg == ARRAY_OPT && i + 1 < argc) {
                // --array custom_key
                options.array_key = argv[++i];
            } else if (arg.substr(0, ARRAY_OPT_WITH_EQUALS.length()) == ARRAY_OPT_WITH_EQUALS) {
                // --array=custom_key
                options.array_key = arg.substr(ARRAY_OPT_WITH_EQUALS.length());
            } else {
                options.bad_option = arg;
                break;
            }
        } else if (arg[0] == '-') {
            // unknown option
            options.bad_option = arg;
            break;
        } else {
            // first filename is the script
            if (options.script_filename.empty()) {
                options.script_filename = arg;
            } else {
                // subsequent filenames are input files
                options.input_filenames.push_back(arg);
            }
        }
    }

    if (!options.bad_option.empty()) {
        std::cerr << "Error: Unknown option: " << options.bad_option << "\n";
        print_usage(argv[0]);
        return 1;
    }

    if (options.help) {
        print_usage(argv[0]);
        return 0;
    }

    if (options.version) {
        print_version();
        return 0;
    }

    if (options.perf) {
        run_performance_benchmarks();
        return 0;
    }

    try {
        // Read script
        nlohmann::json script;
        if (!options.script_filename.empty()) {
            script = read_json(options.script_filename, options.comments);
        }

        // Read input
        std::vector<nlohmann::json> inputs;
        if (!options.input_filenames.empty()) {
            // append all input files
            for (const auto& input_file : options.input_filenames) {
                inputs.push_back(read_json(input_file, false)); // Input files never allow comments
            }
        }

        // Execute script with custom array key
        nlohmann::json result;
        if (inputs.empty()) {
            result = computo::execute(script, nlohmann::json(nullptr), options.array_key);
        } else {
            result = computo::execute(script, inputs, options.array_key);
        }

        // Output result
        std::cout << result.dump(2) << '\n';

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
