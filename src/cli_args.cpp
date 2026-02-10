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
        } else if (strcmp(argv[i], "--highlight") == 0) {
            args.highlight_script = true;
            if (++i >= argc) {
                throw ArgumentError("--highlight requires a script file argument");
            }
            args.highlight_file = argv[i];
        } else if (strcmp(argv[i], "--format") == 0) {
            args.format_script = true;
            if (++i >= argc) {
                throw ArgumentError("--format requires a script file argument");
            }
            args.format_file = argv[i];
        } else if (strcmp(argv[i], "--tocomputo") == 0) {
            args.to_computo = true;
            if (++i >= argc) {
                throw ArgumentError("--tocomputo requires a file argument");
            }
            args.to_computo_file = argv[i];
        } else if (strcmp(argv[i], "--tojson") == 0) {
            args.to_json = true;
            if (++i >= argc) {
                throw ArgumentError("--tojson requires a file argument");
            }
            args.to_json_file = argv[i];
        } else if (strcmp(argv[i], "--color") == 0) {
            args.color_mode = ColorMode::Always;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            args.color_mode = ColorMode::Never;
        } else if (strncmp(argv[i], "--array=", 8) == 0) {
            args.array_key = std::string(argv[i] + 8);
            if (args.array_key.empty()) {
                throw ArgumentError("--array requires a non-empty key");
            }
        } else if (argv[i][0] == '-') {
            throw ArgumentError("Unknown option: " + std::string(argv[i]));
        } else {
            // Non-flag argument - treat as input file
            args.input_files.push_back(argv[i]);
        }
    }

    if (!script_mode && !repl_mode && !args.highlight_script && !args.format_script &&
        !args.to_computo && !args.to_json) {
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
    --script <file>    Execute script from file (auto-detects JSON or sugar)
    --repl             Start interactive REPL

CONVERSION:
    --tocomputo <file> Convert JSON script to sugar syntax (.computo)
    --tojson <file>    Convert sugar syntax (.computo) to JSON

OPTIONS:
    --comments         Enable JSON comment parsing
    --debug            Enable debugging features (REPL only)
    --array=<key>      Use custom array wrapper key (default: "array")
    --format <file>    Reformat script with semantic indentation
    --highlight <file> Display script with syntax highlighting
    --color            Force colored output (with --highlight)
    --no-color         Disable colored output (with --highlight)
    --list-operators   Output JSON array of all available operators
    --help, -h         Show this help message
    --version, -v      Show version information

EXAMPLES:
    computo --script transform.json data.json
    computo --script script.computo data.json
    computo --tocomputo transform.json
    computo --tojson script.computo
    computo --script transform.json data.json --array="@data"
    computo --repl --comments users.json orders.json
    computo --repl --debug
    computo --format script.json
    computo --highlight script.computo
)";
}

void ArgumentParser::print_version() {
    std::cout << "Computo v1.0.0\n";
}

} // namespace computo
