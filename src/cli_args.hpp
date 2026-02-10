#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "json_colorizer.hpp"

namespace computo {

struct ComputoArgs {
    enum class Mode : std::uint8_t { SCRIPT, REPL };
    Mode mode;
    std::string script_file; // Only valid in SCRIPT mode
    std::vector<std::string> input_files;
    std::string highlight_file; // Only valid when highlight_script is true
    std::string format_file;    // Only valid when format_script is true
    std::string to_computo_file; // --tocomputo: convert JSON to sugar syntax
    std::string to_json_file;    // --tojson: convert sugar to JSON
    bool enable_comments = false;
    bool debug_mode = false;
    bool show_help = false;
    bool show_version = false;
    bool list_operators = false;
    bool highlight_script = false;
    bool format_script = false;
    bool to_computo = false;
    bool to_json = false;
    std::string array_key = "array"; // Custom array wrapper key (default: "array")
    ColorMode color_mode = ColorMode::Auto;
};

class ArgumentError : public std::runtime_error {
public:
    explicit ArgumentError(const std::string& msg) : std::runtime_error(msg) {}
};

class ArgumentParser {
public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static auto parse(int argc, char* const argv[]) -> ComputoArgs;
    static void print_help();
    static void print_version();
};

} // namespace computo
