#include "sugar_writer.hpp"
#include <cmath>
#include <set>
#include <sstream>

namespace computo {

// Precedence levels (higher = tighter binding)
enum class Prec : int {
    None = 0,
    LetIfLambda = 1,  // let...in, if...then...else, (x) => body
    Or = 2,
    And = 3,
    Not = 4,          // prefix not
    Comparison = 5,   // >, <, >=, <=, ==, !=
    Additive = 6,     // +, -
    Multiplicative = 7, // *, /, %
    UnaryNeg = 8,     // unary -
    Call = 9,         // f(...), x/path
};

static auto op_precedence(const std::string& op) -> Prec {
    if (op == "or") return Prec::Or;
    if (op == "and") return Prec::And;
    if (op == "not") return Prec::Not;
    if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=")
        return Prec::Comparison;
    if (op == "+" || op == "-") return Prec::Additive;
    if (op == "*" || op == "/" || op == "%") return Prec::Multiplicative;
    return Prec::None;
}

static auto is_infix_op(const std::string& op) -> bool {
    static const std::set<std::string> infix_ops = {
        "+", "-", "*", "/", "%",
        ">", "<", ">=", "<=", "==", "!=",
        "and", "or"
    };
    return infix_ops.count(op) > 0;
}

static auto is_valid_identifier(const std::string& s) -> bool {
    if (s.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_') return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    // Reserved keywords can still be identifiers in function-call position
    return true;
}

// Check if a string is a JSON pointer path (starts with /)
static auto is_json_pointer(const std::string& s) -> bool {
    return !s.empty() && s[0] == '/';
}

// Convert a JSON pointer path to dot-separated sugar path segments
// "/users/0/name" -> "users/0/name"
static auto pointer_to_path(const std::string& pointer) -> std::string {
    if (pointer.empty() || pointer[0] != '/') return pointer;
    return pointer.substr(1); // strip leading /
}

// Check if a path segment needs no escaping for sugar syntax
static auto is_simple_path(const std::string& path) -> bool {
    for (char c : path) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '/') return false;
    }
    return !path.empty();
}

static void append_escaped_string(std::string& out, const std::string& s) {
    out += '"';
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
    out += '"';
}

// Forward declaration
static void write_node(const jsom::JsonDocument& node, Prec parent_prec,
                       const SugarWriterOptions& opts, int indent, std::string& out);

static void append_indent(std::string& out, int indent, int width) {
    for (int i = 0; i < indent * width; ++i) {
        out += ' ';
    }
}

// Write a variable access: ["$", "/x"] -> x, ["$", "/x/name"] -> x/name
static void write_var_access(const jsom::JsonDocument& node, const SugarWriterOptions& /*opts*/,
                             std::string& out) {
    if (node.size() < 2 || !node[1].is_string()) {
        // Bare $ with no path - shouldn't normally happen, fallback
        out += "$()";
        return;
    }
    const auto& path = node[1].as<std::string>();
    if (is_json_pointer(path) && is_simple_path(pointer_to_path(path))) {
        out += pointer_to_path(path);
    } else {
        // Complex path, use function-call fallback
        out += "$(";
        append_escaped_string(out, path);
        out += ')';
    }
}

// Write $input or $inputs access
static void write_input_access(const jsom::JsonDocument& node, const std::string& op_name,
                               const SugarWriterOptions& /*opts*/, std::string& out) {
    out += op_name;
    if (node.size() >= 2 && node[1].is_string()) {
        const auto& path = node[1].as<std::string>();
        if (is_json_pointer(path) && is_simple_path(pointer_to_path(path))) {
            out += '/';
            out += pointer_to_path(path);
        } else {
            out += '(';
            append_escaped_string(out, path);
            out += ')';
        }
    }
}

// Write lambda: ["lambda", ["x", "y"], body] -> (x, y) => body
static void write_lambda(const jsom::JsonDocument& node, Prec parent_prec,
                         const SugarWriterOptions& opts, int indent, std::string& out) {
    bool needs_parens = static_cast<int>(parent_prec) > static_cast<int>(Prec::LetIfLambda);
    if (needs_parens) out += '(';

    out += '(';
    if (node.size() > 1 && node[1].is_array()) {
        for (size_t i = 0; i < node[1].size(); ++i) {
            if (i > 0) out += ", ";
            if (node[1][i].is_string()) {
                out += node[1][i].as<std::string>();
            }
        }
    }
    out += ") => ";

    if (node.size() > 2) {
        write_node(node[2], Prec::LetIfLambda, opts, indent, out);
    }

    if (needs_parens) out += ')';
}

// Write let: ["let", [["x", 10], ["y", 20]], body] -> let x = 10, y = 20 in body
static void write_let(const jsom::JsonDocument& node, Prec parent_prec,
                      const SugarWriterOptions& opts, int indent, std::string& out) {
    bool needs_parens = static_cast<int>(parent_prec) > static_cast<int>(Prec::LetIfLambda);
    if (needs_parens) out += '(';

    out += "let\n";
    if (node.size() > 1) {
        const auto& bindings = node[1];
        if (bindings.is_array()) {
            for (size_t i = 0; i < bindings.size(); ++i) {
                append_indent(out, indent + 1, opts.indent_width);
                const auto& binding = bindings[i];
                if (binding.is_array() && binding.size() == 2 && binding[0].is_string()) {
                    out += binding[0].as<std::string>();
                    out += " = ";
                    write_node(binding[1], Prec::None, opts, indent + 1, out);
                }
                out += '\n';
            }
        } else if (bindings.is_object()) {
            // Object-style bindings {"x": 10, "y": 20}
            for (const auto& [key, value] : bindings.items()) {
                append_indent(out, indent + 1, opts.indent_width);
                out += key;
                out += " = ";
                write_node(value, Prec::None, opts, indent + 1, out);
                out += '\n';
            }
        }
    }

    append_indent(out, indent, opts.indent_width);
    out += "in\n";
    append_indent(out, indent + 1, opts.indent_width);
    if (node.size() > 2) {
        write_node(node[2], Prec::LetIfLambda, opts, indent + 1, out);
    }

    if (needs_parens) out += ')';
}

// Write if: ["if", cond, then, else] -> if cond then then_expr else else_expr
static void write_if(const jsom::JsonDocument& node, Prec parent_prec,
                     const SugarWriterOptions& opts, int indent, std::string& out) {
    bool needs_parens = static_cast<int>(parent_prec) > static_cast<int>(Prec::LetIfLambda);
    if (needs_parens) out += '(';

    out += "if ";
    if (node.size() > 1) {
        write_node(node[1], Prec::None, opts, indent, out);
    }
    out += " then ";
    if (node.size() > 2) {
        write_node(node[2], Prec::None, opts, indent, out);
    }
    out += " else ";
    if (node.size() > 3) {
        write_node(node[3], Prec::LetIfLambda, opts, indent, out);
    }

    if (needs_parens) out += ')';
}

// Write not: ["not", x] -> not x
static void write_not(const jsom::JsonDocument& node, Prec parent_prec,
                      const SugarWriterOptions& opts, int indent, std::string& out) {
    bool needs_parens = static_cast<int>(parent_prec) > static_cast<int>(Prec::Not);
    if (needs_parens) out += '(';

    out += "not ";
    if (node.size() > 1) {
        write_node(node[1], Prec::Not, opts, indent, out);
    }

    if (needs_parens) out += ')';
}

// Write infix: ["+", a, b, c] -> a + b + c
static void write_infix(const jsom::JsonDocument& node, const std::string& op,
                        Prec parent_prec, const SugarWriterOptions& opts,
                        int indent, std::string& out) {
    Prec my_prec = op_precedence(op);
    bool needs_parens = static_cast<int>(parent_prec) > static_cast<int>(my_prec);
    if (needs_parens) out += '(';

    // Unary minus: ["-", x] -> -x
    if (op == "-" && node.size() == 2) {
        out += '-';
        write_node(node[1], Prec::UnaryNeg, opts, indent, out);
        if (needs_parens) out += ')';
        return;
    }

    for (size_t i = 1; i < node.size(); ++i) {
        if (i > 1) {
            out += ' ';
            out += op;
            out += ' ';
        }
        // For left-associative: left operand gets same prec, right gets prec+1
        // But since we flatten variadic, all children get my_prec (they'll paren if lower)
        Prec child_prec = my_prec;
        // Right-most child of non-commutative ops needs higher prec to force parens
        // e.g., a - (b - c) needs parens, but we flatten so a - b - c is fine
        // For division: a / (b / c) would need special handling but Computo flattens these
        write_node(node[i], child_prec, opts, indent, out);
    }

    if (needs_parens) out += ')';
}

// Write function call: ["count", arr] -> count(arr)
static void write_function_call(const jsom::JsonDocument& node, const std::string& name,
                                const SugarWriterOptions& opts, int indent, std::string& out) {
    out += name;
    out += '(';
    for (size_t i = 1; i < node.size(); ++i) {
        if (i > 1) out += ", ";
        write_node(node[i], Prec::None, opts, indent, out);
    }
    out += ')';
}

// Write array literal: {"array": [1,2,3]} -> [1, 2, 3]
static void write_array_literal(const jsom::JsonDocument& arr,
                                const SugarWriterOptions& opts, int indent, std::string& out) {
    out += '[';
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) out += ", ";
        write_node(arr[i], Prec::None, opts, indent, out);
    }
    out += ']';
}

// Write object literal: {"name": "x", "age": 5} -> {name: "x", age: 5}
static void write_object_literal(const jsom::JsonDocument& node,
                                 const SugarWriterOptions& opts, int indent, std::string& out) {
    out += '{';
    size_t count = 0;
    for (const auto& [key, value] : node.items()) {
        if (count > 0) out += ", ";
        if (is_valid_identifier(key)) {
            out += key;
        } else {
            append_escaped_string(out, key);
        }
        out += ": ";
        write_node(value, Prec::None, opts, indent, out);
        ++count;
    }
    out += '}';
}

static void write_node(const jsom::JsonDocument& node, Prec parent_prec,
                       const SugarWriterOptions& opts, int indent, std::string& out) {
    if (node.is_null()) {
        out += "null";
    } else if (node.is_bool()) {
        out += node.as<bool>() ? "true" : "false";
    } else if (node.is_number()) {
        out += node.to_json();
    } else if (node.is_string()) {
        append_escaped_string(out, node.as<std::string>());
    } else if (node.is_object()) {
        // Check for array wrapper: {"array": [...]}
        if (node.size() == 1 && node.contains(opts.array_key)) {
            const auto& inner = node[opts.array_key];
            if (inner.is_array()) {
                write_array_literal(inner, opts, indent, out);
                return;
            }
        }
        write_object_literal(node, opts, indent, out);
    } else if (node.is_array()) {
        if (node.empty()) {
            // Empty array - could be literal or empty call
            out += "[]";
            return;
        }

        // Check if it's an operator call (first element is string)
        if (!node[0].is_string()) {
            // Non-operator array - write as array literal wrapped form would not appear here,
            // but this is a raw array that's not an op call. Write as bracketed list.
            out += '[';
            for (size_t i = 0; i < node.size(); ++i) {
                if (i > 0) out += ", ";
                write_node(node[i], Prec::None, opts, indent, out);
            }
            out += ']';
            return;
        }

        const auto& op = node[0].as<std::string>();

        // Variable access: ["$", "/x"]
        if (op == "$") {
            write_var_access(node, opts, out);
        }
        // Input access: ["$input", "/path"] or ["$inputs", "/path"]
        else if (op == "$input" || op == "$inputs") {
            write_input_access(node, op, opts, out);
        }
        // Lambda
        else if (op == "lambda") {
            write_lambda(node, parent_prec, opts, indent, out);
        }
        // Let
        else if (op == "let") {
            write_let(node, parent_prec, opts, indent, out);
        }
        // If
        else if (op == "if") {
            write_if(node, parent_prec, opts, indent, out);
        }
        // Not (prefix)
        else if (op == "not" && node.size() == 2) {
            write_not(node, parent_prec, opts, indent, out);
        }
        // Infix operators
        else if (is_infix_op(op) && node.size() >= 2) {
            write_infix(node, op, parent_prec, opts, indent, out);
        }
        // Generic function call
        else {
            write_function_call(node, op, opts, indent, out);
        }
    }
}

auto SugarWriter::write(const jsom::JsonDocument& doc,
                        const SugarWriterOptions& options) -> std::string {
    std::string out;
    out.reserve(256);
    write_node(doc, Prec::None, options, 0, out);
    return out;
}

} // namespace computo
