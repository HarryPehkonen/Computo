#include "cli_args.hpp"
#include "json_colorizer.hpp"
#include "repl.hpp"
#include "sugar_parser.hpp"
#include "sugar_writer.hpp"
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

// Load a file as raw text
static auto read_file_text(const std::string& filename) -> std::string {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Auto-detect format and load script based on extension or content
static auto load_script_file(const std::string& filename, bool enable_comments,
                             const std::string& array_key) -> jsom::JsonDocument {
    auto content = read_file_text(filename);

    // Check file extension: .computo files are always sugar syntax
    bool force_sugar = filename.size() >= 8
                       && filename.substr(filename.size() - 8) == ".computo";

    if (!force_sugar) {
        // Try JSON parse first
        try {
            if (enable_comments) {
                return jsom::parse_document(content, jsom::ParsePresets::Comments);
            }
            return jsom::parse_document(content);
        } catch (const std::exception&) {
            // JSON parse failed, try sugar syntax
        }
    }

    // Parse as sugar
    SugarParseOptions opts;
    opts.array_key = array_key;
    return SugarParser::parse(content, opts);
}

// Unwrap array wrapper for output: {"array": [...]} -> [...]
static auto unwrap_for_output(const jsom::JsonDocument& result,
                              const std::string& array_key) -> jsom::JsonDocument {
    if (result.is_object() && result.size() == 1 && result.contains(array_key)) {
        return result[array_key];
    }
    return result;
}

// --- Script Execution Mode ---

auto run_script_mode(const ComputoArgs& args) -> int {
    try {
        // Load script with auto-detection
        auto script = load_script_file(args.script_file, args.enable_comments, args.array_key);

        // Load inputs and execute
        auto inputs = load_input_files(args.input_files, args.enable_comments);
        auto result = computo::execute(script, inputs, nullptr, args.array_key);

        // Output result (unwrap array wrapper for clean output)
        auto output = unwrap_for_output(result, args.array_key);
        std::cout << output.to_json(true) << "\n";
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

        if (args.to_computo) {
            auto script = computo::load_json_file(args.to_computo_file, args.enable_comments);
            computo::SugarWriterOptions opts;
            opts.array_key = args.array_key;
            std::cout << computo::SugarWriter::write(script, opts) << "\n";
            return 0;
        }

        if (args.to_json) {
            auto content = computo::read_file_text(args.to_json_file);
            computo::SugarParseOptions opts;
            opts.array_key = args.array_key;
            auto doc = computo::SugarParser::parse(content, opts);
            std::cout << doc.to_json(true) << "\n";
            return 0;
        }

        if (args.highlight_script) {
            auto script = computo::load_script_file(args.highlight_file, args.enable_comments,
                                                     args.array_key);
            bool use_color = computo::resolve_color_mode(args.color_mode);
            auto theme = use_color ? computo::ScriptColorTheme::default_theme()
                                   : computo::ScriptColorTheme::no_color();
            std::cout << computo::ScriptColorizer::colorize(script, theme, args.array_key) << "\n";
            return 0;
        }

        if (args.format_script) {
            auto script = computo::load_script_file(args.format_file, args.enable_comments,
                                                     args.array_key);
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
