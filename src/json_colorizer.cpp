#include "json_colorizer.hpp"
#include <cstdlib>

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

// Context passed during recursive colorization
enum class NodeContext : std::uint8_t {
    Expression,    // Normal expression context
    LambdaParams,  // Inside lambda parameter list
};

// Escape a string value for JSON output (add quotes and escape special chars)
static void append_json_string(std::string& out, const std::string& str) {
    out += '"';
    for (char chr : str) {
        switch (chr) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(chr) < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(chr));
                out += buf;
            } else {
                out += chr;
            }
            break;
        }
    }
    out += '"';
}

static auto is_var_access_op(const std::string& name) -> bool {
    return name == "$" || name == "$input" || name == "$inputs";
}

static void colorize_node(const jsom::JsonDocument& node, NodeContext ctx,
                          int indent, const ScriptColorTheme& theme,
                          const std::string& array_key, std::string& out);

static void append_indent(std::string& out, int indent) {
    for (int idx = 0; idx < indent; ++idx) {
        out += "  ";
    }
}

// Compact serialized length of a node (for inline vs multiline decisions)
static auto compact_length(const jsom::JsonDocument& node) -> size_t {
    return node.to_json().size();
}

// Emit a colored operator name
static void emit_op_name(const std::string& op_name, const ScriptColorTheme& theme,
                         std::string& out) {
    const char* color = theme.op;
    if (op_name == "lambda") {
        color = theme.lambda_kw;
    } else if (is_var_access_op(op_name)) {
        color = theme.var_access;
    }
    out += color;
    append_json_string(out, op_name);
    out += theme.reset;
}

// Emit a colorized argument (handles JSON pointer special case for $ operators)
static void emit_arg(const jsom::JsonDocument& child, const std::string& op_name,
                     size_t arg_idx, int indent, const ScriptColorTheme& theme,
                     const std::string& array_key, std::string& out) {
    if (op_name == "lambda" && arg_idx == 1 && child.is_array()) {
        colorize_node(child, NodeContext::LambdaParams, indent, theme, array_key, out);
    } else if (is_var_access_op(op_name) && child.is_string()) {
        const auto& str_val = child.as<std::string>();
        if (!str_val.empty() && str_val[0] == '/') {
            out += theme.pointer;
            append_json_string(out, str_val);
            out += theme.reset;
        } else {
            colorize_node(child, NodeContext::Expression, indent, theme, array_key, out);
        }
    } else {
        colorize_node(child, NodeContext::Expression, indent, theme, array_key, out);
    }
}

static void emit_comma(const ScriptColorTheme& theme, std::string& out) {
    out += theme.structural;
    out += ',';
    out += theme.reset;
}

static void emit_open_bracket(const ScriptColorTheme& theme, std::string& out) {
    out += theme.structural;
    out += '[';
    out += theme.reset;
}

static void emit_close_bracket(const ScriptColorTheme& theme, std::string& out) {
    out += theme.structural;
    out += ']';
    out += theme.reset;
}

// Check if a node is a lambda expression
static auto is_lambda(const jsom::JsonDocument& node) -> bool {
    return node.is_array() && !node.empty() && node[0].is_string()
        && node[0].as<std::string>() == "lambda";
}

// --- Semantic formatters for specific operators ---

// let: ["let", [[name, val], ...], body]
// Bindings array opens on next line, each binding on its own line, body at end
static void format_let(const jsom::JsonDocument& node, int indent,
                       const ScriptColorTheme& theme, const std::string& array_key,
                       std::string& out) {
    emit_open_bracket(theme, out);
    emit_op_name("let", theme, out);
    emit_comma(theme, out);

    // Bindings array (element 1)
    if (node.size() > 1) {
        out += '\n';
        append_indent(out, indent + 1);
        const auto& bindings = node[1];
        if (bindings.is_array()) {
            emit_open_bracket(theme, out);
            for (size_t idx = 0; idx < bindings.size(); ++idx) {
                if (idx > 0) {
                    emit_comma(theme, out);
                }
                out += '\n';
                append_indent(out, indent + 2);
                const auto& binding = bindings[idx];
                if (binding.is_array() && binding.size() == 2) {
                    // Binding: [name, value]
                    emit_open_bracket(theme, out);
                    emit_arg(binding[0], "let", 0, indent + 3, theme, array_key, out);
                    emit_comma(theme, out);
                    out += '\n';
                    append_indent(out, indent + 3);
                    emit_arg(binding[1], "let", 1, indent + 3, theme, array_key, out);
                    out += '\n';
                    append_indent(out, indent + 2);
                    emit_close_bracket(theme, out);
                } else {
                    colorize_node(binding, NodeContext::Expression, indent + 2, theme, array_key, out);
                }
            }
            out += '\n';
            append_indent(out, indent + 1);
            emit_close_bracket(theme, out);
        } else {
            colorize_node(bindings, NodeContext::Expression, indent + 1, theme, array_key, out);
        }
    }

    // Body (element 2)
    if (node.size() > 2) {
        emit_comma(theme, out);
        out += '\n';
        append_indent(out, indent + 1);
        emit_arg(node[2], "let", 2, indent + 1, theme, array_key, out);
    }

    out += '\n';
    append_indent(out, indent);
    emit_close_bracket(theme, out);
}

// lambda: ["lambda", [params], body]
// Short bodies stay inline, long ones wrap
static void format_lambda(const jsom::JsonDocument& node, int indent,
                          const ScriptColorTheme& theme, const std::string& array_key,
                          std::string& out) {
    bool fits_inline = compact_length(node) <= 60;

    emit_open_bracket(theme, out);
    emit_op_name("lambda", theme, out);
    emit_comma(theme, out);
    out += ' ';

    // Params (element 1)
    if (node.size() > 1) {
        emit_arg(node[1], "lambda", 1, indent + 1, theme, array_key, out);
    }

    // Body (element 2)
    if (node.size() > 2) {
        emit_comma(theme, out);
        if (fits_inline) {
            out += ' ';
            emit_arg(node[2], "lambda", 2, indent + 1, theme, array_key, out);
        } else {
            out += '\n';
            append_indent(out, indent + 1);
            emit_arg(node[2], "lambda", 2, indent + 1, theme, array_key, out);
            out += '\n';
            append_indent(out, indent);
        }
    }
    emit_close_bracket(theme, out);
}

// if: ["if", cond, then, else]
// All on one line if short, otherwise each branch on its own line
static void format_if(const jsom::JsonDocument& node, int indent,
                      const ScriptColorTheme& theme, const std::string& array_key,
                      std::string& out) {
    bool fits_inline = compact_length(node) <= 60;

    emit_open_bracket(theme, out);
    emit_op_name("if", theme, out);
    emit_comma(theme, out);

    for (size_t idx = 1; idx < node.size(); ++idx) {
        if (fits_inline) {
            out += ' ';
        } else {
            out += '\n';
            append_indent(out, indent + 1);
        }
        emit_arg(node[idx], "if", idx, indent + 1, theme, array_key, out);
        if (idx + 1 < node.size()) {
            emit_comma(theme, out);
        }
    }

    if (!fits_inline) {
        out += '\n';
        append_indent(out, indent);
    }
    emit_close_bracket(theme, out);
}

// Higher-order ops (map, filter, reduce): operator + data args on first line,
// lambda on its own indented line
static void format_higher_order(const jsom::JsonDocument& node, const std::string& op_name,
                                int indent, const ScriptColorTheme& theme,
                                const std::string& array_key, std::string& out) {
    bool fits_inline = compact_length(node) <= 60;

    emit_open_bracket(theme, out);
    emit_op_name(op_name, theme, out);
    emit_comma(theme, out);

    if (fits_inline) {
        // Everything on one line
        for (size_t idx = 1; idx < node.size(); ++idx) {
            out += ' ';
            emit_arg(node[idx], op_name, idx, indent + 1, theme, array_key, out);
            if (idx + 1 < node.size()) {
                emit_comma(theme, out);
            }
        }
        emit_close_bracket(theme, out);
        return;
    }

    // Multiline: data args on first line, lambda and subsequent args on their own lines
    bool past_first_lambda = false;
    for (size_t idx = 1; idx < node.size(); ++idx) {
        const auto& child = node[idx];
        if (is_lambda(child) || past_first_lambda) {
            emit_comma(theme, out);
            out += '\n';
            append_indent(out, indent + 1);
            emit_arg(child, op_name, idx, indent + 1, theme, array_key, out);
            if (is_lambda(child)) {
                past_first_lambda = true;
            }
        } else {
            out += ' ';
            emit_arg(child, op_name, idx, indent + 1, theme, array_key, out);
        }
    }

    out += '\n';
    append_indent(out, indent);
    emit_close_bracket(theme, out);
}

// --- Generic array formatter (for operators without special rules) ---

static void format_generic_op(const jsom::JsonDocument& node, const std::string& op_name,
                              int indent, const ScriptColorTheme& theme,
                              const std::string& array_key, std::string& out) {
    bool fits_inline = compact_length(node) <= 60;

    emit_open_bracket(theme, out);
    emit_op_name(op_name, theme, out);

    for (size_t idx = 1; idx < node.size(); ++idx) {
        emit_comma(theme, out);
        if (fits_inline) {
            out += ' ';
            emit_arg(node[idx], op_name, idx, indent + 1, theme, array_key, out);
        } else {
            out += '\n';
            append_indent(out, indent + 1);
            emit_arg(node[idx], op_name, idx, indent + 1, theme, array_key, out);
        }
    }

    if (!fits_inline) {
        out += '\n';
        append_indent(out, indent);
    }
    emit_close_bracket(theme, out);
}

// --- Main array dispatcher ---

static void colorize_array(const jsom::JsonDocument& node, NodeContext ctx,
                           int indent, const ScriptColorTheme& theme,
                           const std::string& array_key, std::string& out) {
    if (node.empty()) {
        emit_open_bracket(theme, out);
        emit_close_bracket(theme, out);
        return;
    }

    // Check if first element is a string (operator call)
    if (node[0].is_string()) {
        const auto& op_name = node[0].as<std::string>();

        if (op_name == "let" && node.size() >= 3) {
            format_let(node, indent, theme, array_key, out);
        } else if (op_name == "lambda") {
            format_lambda(node, indent, theme, array_key, out);
        } else if (op_name == "if") {
            format_if(node, indent, theme, array_key, out);
        } else if (op_name == "map" || op_name == "filter" || op_name == "reduce") {
            format_higher_order(node, op_name, indent, theme, array_key, out);
        } else {
            format_generic_op(node, op_name, indent, theme, array_key, out);
        }
        return;
    }

    // Non-operator array: generic formatting
    bool fits_inline = compact_length(node) <= 60;

    emit_open_bracket(theme, out);
    for (size_t idx = 0; idx < node.size(); ++idx) {
        if (idx > 0) {
            emit_comma(theme, out);
        }
        if (fits_inline) {
            if (idx > 0) {
                out += ' ';
            }
        } else {
            out += '\n';
            append_indent(out, indent + 1);
        }
        colorize_node(node[idx], ctx, indent + 1, theme, array_key, out);
    }
    if (!fits_inline) {
        out += '\n';
        append_indent(out, indent);
    }
    emit_close_bracket(theme, out);
}

// --- Object formatter ---

static void colorize_object(const jsom::JsonDocument& node, int indent,
                             const ScriptColorTheme& theme,
                             const std::string& array_key, std::string& out) {
    if (node.empty()) {
        out += theme.structural;
        out += "{}";
        out += theme.reset;
        return;
    }

    bool multiline = compact_length(node) > 60;

    out += theme.structural;
    out += '{';
    out += theme.reset;

    size_t count = 0;
    for (const auto& [key, value] : node.items()) {
        if (count > 0) {
            emit_comma(theme, out);
        }

        if (multiline) {
            out += '\n';
            append_indent(out, indent + 1);
        } else if (count > 0) {
            out += ' ';
        }

        // Key color: dim for array wrapper key, default otherwise
        if (key == array_key) {
            out += theme.array_wrapper;
        } else {
            out += theme.structural;
        }
        append_json_string(out, key);
        out += theme.reset;

        out += theme.structural;
        out += ": ";
        out += theme.reset;

        colorize_node(value, NodeContext::Expression, indent + 1, theme, array_key, out);
        ++count;
    }

    if (multiline) {
        out += '\n';
        append_indent(out, indent);
    }
    out += theme.structural;
    out += '}';
    out += theme.reset;
}

// --- Top-level node dispatcher ---

static void colorize_node(const jsom::JsonDocument& node, NodeContext ctx,
                          int indent, const ScriptColorTheme& theme,
                          const std::string& array_key, std::string& out) {
    if (node.is_null()) {
        out += theme.bool_null;
        out += "null";
        out += theme.reset;
    } else if (node.is_bool()) {
        out += theme.bool_null;
        out += (node.as<bool>() ? "true" : "false");
        out += theme.reset;
    } else if (node.is_number()) {
        out += theme.number;
        out += node.to_json();
        out += theme.reset;
    } else if (node.is_string()) {
        const auto& str_val = node.as<std::string>();
        if (ctx == NodeContext::LambdaParams) {
            out += theme.param;
        } else if (is_var_access_op(str_val)) {
            out += theme.var_access;
        } else if (!str_val.empty() && str_val[0] == '/') {
            out += theme.pointer;
        } else {
            out += theme.string_lit;
        }
        append_json_string(out, str_val);
        out += theme.reset;
    } else if (node.is_array()) {
        if (ctx == NodeContext::LambdaParams) {
            emit_open_bracket(theme, out);
            for (size_t idx = 0; idx < node.size(); ++idx) {
                if (idx > 0) {
                    out += theme.structural;
                    out += ", ";
                    out += theme.reset;
                }
                colorize_node(node[idx], NodeContext::LambdaParams, indent + 1, theme, array_key, out);
            }
            emit_close_bracket(theme, out);
        } else {
            colorize_array(node, ctx, indent, theme, array_key, out);
        }
    } else if (node.is_object()) {
        colorize_object(node, indent, theme, array_key, out);
    }
}

auto ScriptColorizer::colorize(const jsom::JsonDocument& doc,
                                const ScriptColorTheme& theme,
                                const std::string& array_key) -> std::string {
    std::string out;
    out.reserve(256);
    colorize_node(doc, NodeContext::Expression, 0, theme, array_key, out);
    return out;
}

} // namespace computo
