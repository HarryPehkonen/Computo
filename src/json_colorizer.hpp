#pragma once

#include <cstdint>
#include <jsom/json_document.hpp>
#include <string>

namespace computo {

enum class ColorMode : std::uint8_t { Auto, Always, Never };

struct ScriptColorTheme {
    const char* op = "\033[1;36m";           // Bold cyan - operators
    const char* lambda_kw = "\033[1;35m";    // Bold magenta - lambda keyword
    const char* param = "\033[33m";          // Yellow - lambda parameter names
    const char* var_access = "\033[32m";     // Green - $, $input, $inputs
    const char* pointer = "\033[4;32m";      // Green underline - JSON pointers
    const char* string_lit = "\033[0m";      // Default - string literals
    const char* number = "\033[97m";         // Bright white - numbers
    const char* bool_null = "\033[31m";      // Red - booleans/null
    const char* array_wrapper = "\033[2m";   // Dim - {"array": ...} key
    const char* structural = "\033[2m";      // Dim - brackets, colons, commas
    const char* reset = "\033[0m";

    static auto default_theme() -> ScriptColorTheme { return {}; }
    static auto no_color() -> ScriptColorTheme {
        return {"", "", "", "", "", "", "", "", "", "", ""};
    }
};

class ScriptColorizer {
public:
    static auto colorize(const jsom::JsonDocument& doc,
                         const ScriptColorTheme& theme = ScriptColorTheme::default_theme(),
                         const std::string& array_key = "array") -> std::string;
};

auto resolve_color_mode(ColorMode mode) -> bool;

} // namespace computo
