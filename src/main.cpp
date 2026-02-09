#include "cli_args.hpp"
#include "json_colorizer.hpp"
#include "repl.hpp"
#include <algorithm>
#include <computo.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace computo {

// Forward declarations for file utilities (implemented in repl.cpp)
auto load_json_file(const std::string& filename, bool enable_comments) -> jsom::JsonDocument;
auto load_input_files(const std::vector<std::string>& filenames, bool enable_comments)
    -> std::vector<jsom::JsonDocument>;

// --- Script Execution Mode ---

auto run_script_mode(const ComputoArgs& args) -> int {
    try {
        // Load script
        auto script = load_json_file(args.script_file, args.enable_comments);

        // Load inputs and execute
        auto inputs = load_input_files(args.input_files, args.enable_comments);
        auto result = computo::execute(script, inputs, nullptr, args.array_key);

        // Output result
        std::cout << result.to_json(true) << "\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace computo

// --- Main Entry Point ---

// NOLINTBEGIN(readability-function-size)
auto main(int argc, char* argv[]) -> int {
    try {
        auto args = computo::ArgumentParser::parse(argc, argv);

        if (args.show_help) {
            computo::ArgumentParser::print_help();
            return 0;
        }

        if (args.show_version) {
            computo::ArgumentParser::print_version();
            return 0;
        }

        if (args.list_operators) {
            auto& registry = computo::OperatorRegistry::get_instance();
            auto operators = registry.get_operator_names();

            // Sort operators for consistent output
            std::sort(operators.begin(), operators.end());

            // Output as JSON array
            jsom::JsonDocument output = jsom::JsonDocument::make_array();
            for (const auto& operator_name : operators) {
                output.push_back(operator_name);
            }
            std::cout << output.to_json() << "\n";
            return 0;
        }

        if (args.highlight_script) {
            auto script = computo::load_json_file(args.highlight_file, args.enable_comments);
            bool use_color = computo::resolve_color_mode(args.color_mode);
            auto theme = use_color ? computo::ScriptColorTheme::default_theme()
                                   : computo::ScriptColorTheme::no_color();
            std::cout << computo::ScriptColorizer::colorize(script, theme, args.array_key) << "\n";
            return 0;
        }

        if (args.format_script) {
            auto script = computo::load_json_file(args.format_file, args.enable_comments);
            std::cout << computo::ScriptColorizer::colorize(script,
                computo::ScriptColorTheme::no_color(), args.array_key) << "\n";
            return 0;
        }

        switch (args.mode) {
        case computo::ComputoArgs::Mode::SCRIPT:
            return computo::run_script_mode(args);
        case computo::ComputoArgs::Mode::REPL:
            return computo::run_repl_mode(args);
        }

        return 0;

    } catch (const computo::ArgumentError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Use --help for usage information.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
// NOLINTEND(readability-function-size)
