#include "json_colorizer.hpp"
#include <cstdlib>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace computo {

auto resolve_color_mode(ColorMode mode) -> bool {
    if (mode == ColorMode::Always) {
        return true;
    }
    if (mode == ColorMode::Never) {
        return false;
    }

    // Auto: check NO_COLOR env variable
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* no_color = std::getenv("NO_COLOR");
    if (no_color != nullptr && no_color[0] != '\0') {
        return false;
    }

    // Check if stdout is a TTY
    if (!isatty(fileno(stdout))) {
        return false;
    }

#ifdef _WIN32
    // Try to enable virtual terminal processing on Windows
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD mode_val = 0;
    if (!GetConsoleMode(h_out, &mode_val)) {
        return false;
    }
    if (!SetConsoleMode(h_out, mode_val | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return false;
    }
#endif

    return true;
}

// Context for tracking whether we're inside an object or array
enum class Context : std::uint8_t { Object, Array };

auto JsonColorizer::colorize(const std::string& json, const JsonColorTheme& theme) -> std::string {
    std::string out;
    out.reserve(json.size() + json.size() / 3); // ~1.3x pre-allocation

    std::vector<Context> stack;
    // Track whether next string in an object context is a key
    // After '{' or ',' inside object → next string is a key
    bool expect_key = false;

    size_t i = 0;
    const size_t len = json.size();

    while (i < len) {
        char ch = json[i];

        switch (ch) {
        case '{':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            stack.push_back(Context::Object);
            expect_key = true;
            ++i;
            break;

        case '}':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            if (!stack.empty()) {
                stack.pop_back();
            }
            // After closing brace, if we're back in an object, next token after comma is a key
            expect_key = false;
            ++i;
            break;

        case '[':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            stack.push_back(Context::Array);
            expect_key = false;
            ++i;
            break;

        case ']':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            if (!stack.empty()) {
                stack.pop_back();
            }
            expect_key = false;
            ++i;
            break;

        case ':':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            expect_key = false;
            ++i;
            break;

        case ',':
            out += theme.structural;
            out += ch;
            out += theme.reset;
            // After comma in object context, next string is a key
            if (!stack.empty() && stack.back() == Context::Object) {
                expect_key = true;
            }
            ++i;
            break;

        case '"': {
            // Determine color: key vs string value
            bool is_key = expect_key && !stack.empty() && stack.back() == Context::Object;
            const char* color = is_key ? theme.key : theme.string;

            out += color;
            out += ch; // opening quote
            ++i;

            // Scan through string contents, handling escape sequences
            while (i < len) {
                char sc = json[i];
                if (sc == '\\' && i + 1 < len) {
                    out += sc;
                    out += json[i + 1];
                    i += 2;
                } else if (sc == '"') {
                    out += sc; // closing quote
                    ++i;
                    break;
                } else {
                    out += sc;
                    ++i;
                }
            }

            out += theme.reset;

            if (is_key) {
                expect_key = false;
            }
            break;
        }

        case 't': // true
            if (i + 3 < len && json[i + 1] == 'r' && json[i + 2] == 'u' && json[i + 3] == 'e') {
                out += theme.boolean;
                out += "true";
                out += theme.reset;
                i += 4;
            } else {
                out += ch;
                ++i;
            }
            break;

        case 'f': // false
            if (i + 4 < len && json[i + 1] == 'a' && json[i + 2] == 'l' && json[i + 3] == 's' && json[i + 4] == 'e') {
                out += theme.boolean;
                out += "false";
                out += theme.reset;
                i += 5;
            } else {
                out += ch;
                ++i;
            }
            break;

        case 'n': // null
            if (i + 3 < len && json[i + 1] == 'u' && json[i + 2] == 'l' && json[i + 3] == 'l') {
                out += theme.null;
                out += "null";
                out += theme.reset;
                i += 4;
            } else {
                out += ch;
                ++i;
            }
            break;

        default:
            // Numbers: digits, minus, dot, e/E, +
            if (ch == '-' || (ch >= '0' && ch <= '9')) {
                out += theme.number;
                while (i < len) {
                    char nc = json[i];
                    if ((nc >= '0' && nc <= '9') || nc == '.' || nc == '-' || nc == '+' || nc == 'e' || nc == 'E') {
                        out += nc;
                        ++i;
                    } else {
                        break;
                    }
                }
                out += theme.reset;
            } else {
                // Whitespace and anything else: pass through
                out += ch;
                ++i;
            }
            break;
        }
    }

    return out;
}

} // namespace computo
