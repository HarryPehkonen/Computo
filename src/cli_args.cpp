#include "cli_args.hpp"
#include <cstring>
#include <iostream>

namespace computo {

auto ArgumentParser::parse(int argc, char* const argv[]) -> ComputoArgs {
    ComputoArgs args{};
    bool script_mode = false;
    bool repl_mode = false;

    if (argc == 1) {
        args.show_help = true;
        return args;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--script") == 0) {
            if (repl_mode) {
                throw ArgumentError("--script and --repl are mutually exclusive");
            }
            script_mode = true;
            args.mode = ComputoArgs::Mode::SCRIPT;
            if (++i >= argc) {
                throw ArgumentError("--script requires a script file argument");
            }
            args.script_file = argv[i];
        } else if (strcmp(argv[i], "--repl") == 0) {
            if (script_mode) {
                throw ArgumentError("--script and --repl are mutually exclusive");
            }
            repl_mode = true;
            args.mode = ComputoArgs::Mode::REPL;
        } else if (strcmp(argv[i], "--comments") == 0) {
            args.enable_comments = true;
        } else if (strcmp(argv[i], "--debug") == 0) {
            args.debug_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            args.show_help = true;
            return args;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            args.show_version = true;
            return args;
        } else if (strcmp(argv[i], "--list-operators") == 0) {
            args.list_operators = true;
            return args;
        } else if (argv[i][0] == '-') {
            throw ArgumentError("Unknown option: " + std::string(argv[i]));
        } else {
            // Non-flag argument - treat as input file
            args.input_files.push_back(argv[i]);
        }
    }

    if (!script_mode && !repl_mode) {
        throw ArgumentError("Must specify either --script or --repl mode");
    }

    return args;
}

void ArgumentParser::print_help() {
    std::cout << R"(Computo - JSON Data Transformation Engine

USAGE:
    computo --script <SCRIPT> [OPTIONS] [INPUT_FILES...]
    computo --repl [OPTIONS] [INPUT_FILES...]

MODES:
    --script <file>    Execute JSON script from file
    --repl             Start interactive REPL

OPTIONS:
    --comments         Enable JSON comment parsing
    --debug            Enable debugging features (REPL only)
    --list-operators   Output JSON array of all available operators
    --help, -h         Show this help message
    --version, -v      Show version information

EXAMPLES:
    computo --script transform.json data.json
    computo --script script.json input1.json input2.json
    computo --repl --comments users.json orders.json
    computo --repl --debug
)";
}

void ArgumentParser::print_version() {
    std::cout << "Computo v1.0.0\n";
}

} // namespace computo
