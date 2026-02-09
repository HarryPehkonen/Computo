#pragma once

#include <cstdint>
#include <string>

namespace computo {

enum class ColorMode : std::uint8_t { Auto, Always, Never };

struct JsonColorTheme {
    const char* key = "\033[36m";      // Cyan
    const char* string = "\033[32m";   // Green
    const char* number = "\033[97m";   // Bright white
    const char* boolean = "\033[33m";  // Yellow
    const char* null = "\033[2m";      // Dim
    const char* structural = "\033[2m"; // Dim
    const char* reset = "\033[0m";

    static auto default_theme() -> JsonColorTheme { return {}; }
};

class JsonColorizer {
public:
    static auto colorize(const std::string& json, const JsonColorTheme& theme = JsonColorTheme::default_theme()) -> std::string;
};

auto resolve_color_mode(ColorMode mode) -> bool;

} // namespace computo
