#include "declarations.hpp"
#include "shared.hpp"
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace computo::operators {
using computo::evaluate;

// String operations
nlohmann::json split_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) {
        throw InvalidArgumentException("split requires exactly 2 arguments: [string, delimiter]");
    }

    auto str_expr = evaluate(args[0], ctx);
    auto delimiter_expr = evaluate(args[1], ctx);

    if (!str_expr.is_string()) {
        throw InvalidArgumentException("split requires string as first argument");
    }

    if (!delimiter_expr.is_string()) {
        throw InvalidArgumentException("split requires string delimiter as second argument");
    }

    std::string str = str_expr.get<std::string>();
    std::string delimiter = delimiter_expr.get<std::string>();

    nlohmann::json result = nlohmann::json::array();

    if (delimiter.empty()) {
        // Split into individual characters
        for (char c : str) {
            result.push_back(std::string(1, c));
        }
    } else {
        // Split by delimiter
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        result.push_back(str.substr(start));
    }

    return nlohmann::json::object({ { "array", result } });
}

nlohmann::json join_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) {
        throw InvalidArgumentException("join requires exactly 2 arguments: [array, separator]");
    }

    auto array_container = evaluate(args[0], ctx);
    auto separator_expr = evaluate(args[1], ctx);

    if (!array_container.is_object() || array_container.find("array") == array_container.end()) {
        throw InvalidArgumentException("join requires array container as first argument");
    }

    if (!separator_expr.is_string()) {
        throw InvalidArgumentException("join requires string separator as second argument");
    }

    auto array = array_container["array"];
    if (!array.is_array()) {
        throw InvalidArgumentException("join requires array of values");
    }

    std::string separator = separator_expr.get<std::string>();
    std::string result;

    for (size_t i = 0; i < array.size(); ++i) {
        if (i > 0) {
            result += separator;
        }

        // Convert array element to string
        const auto& element = array[i];
        if (element.is_string()) {
            result += element.get<std::string>();
        } else if (element.is_number()) {
            result += element.dump();
        } else if (element.is_boolean()) {
            result += element.get<bool>() ? "true" : "false";
        } else if (element.is_null()) {
            result += "null";
        } else {
            // For objects and arrays, use JSON representation
            result += element.dump();
        }
    }

    return nlohmann::json(result);
}

nlohmann::json trim_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("trim requires exactly 1 argument");
    }

    auto str_expr = evaluate(args[0], ctx);
    if (!str_expr.is_string()) {
        throw InvalidArgumentException("trim requires string argument");
    }

    std::string str = str_expr.get<std::string>();

    // Trim whitespace from both ends
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return nlohmann::json("");
    }

    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return nlohmann::json(str.substr(start, end - start + 1));
}

nlohmann::json upper_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("upper requires exactly 1 argument");
    }

    auto str_expr = evaluate(args[0], ctx);
    if (!str_expr.is_string()) {
        throw InvalidArgumentException("upper requires string argument");
    }

    std::string str = str_expr.get<std::string>();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return nlohmann::json(str);
}

nlohmann::json lower_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("lower requires exactly 1 argument");
    }

    auto str_expr = evaluate(args[0], ctx);
    if (!str_expr.is_string()) {
        throw InvalidArgumentException("lower requires string argument");
    }

    std::string str = str_expr.get<std::string>();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return nlohmann::json(str);
}

// Array operations

/*
 * Enhanced Sort Operator
 *
 * Supports multiple API patterns:
 * - Simple arrays: ["sort", array] (ascending), ["sort", array, "desc"] (descending)
 * - Object arrays: ["sort", array, "/field"] (ascending), ["sort", array, ["/field", "desc"]]
 * - Multi-field: ["sort", array, "/name", ["/age", "desc"]]
 *
 * Uses JSON Pointer syntax for field access and type-aware comparison for mixed arrays.
 */

// Field descriptor for object sorting
struct FieldDescriptor {
    std::string pointer;
    bool ascending = true;
};

// Sort configuration
struct SortConfig {
    bool is_simple_array;
    std::string direction; // for simple arrays
    std::vector<FieldDescriptor> fields; // for object arrays
};

// Parse a single field descriptor from string "/field" or array ["/field", "desc"]
FieldDescriptor parse_field_descriptor(const nlohmann::json& field_spec) {
    FieldDescriptor desc;

    if (field_spec.is_string()) {
        // Simple string field: "/field" -> ascending
        desc.pointer = field_spec.get<std::string>();
        desc.ascending = true;
    } else if (field_spec.is_array()) {
        if (field_spec.size() == 0) {
            throw InvalidArgumentException("Field descriptor array cannot be empty");
        }

        // Array format: ["/field"] or ["/field", "desc"]
        desc.pointer = field_spec[0].get<std::string>();
        desc.ascending = true; // default

        if (field_spec.size() > 1) {
            std::string direction = field_spec[1].get<std::string>();
            if (direction == "desc") {
                desc.ascending = false;
            } else if (direction == "asc") {
                desc.ascending = true;
            } else {
                throw InvalidArgumentException("Invalid sort direction: " + direction + " (use 'asc' or 'desc')");
            }
        }
    } else {
        throw InvalidArgumentException("Field descriptor must be string or array");
    }

    return desc;
}

// Parse sort arguments and determine whether simple array or object array sorting
SortConfig parse_sort_arguments(const nlohmann::json& args) {
    SortConfig config;

    if (args.size() == 1) {
        // Simple array sort, ascending
        config.is_simple_array = true;
        config.direction = "asc";
    } else if (args.size() == 2) {
        // Could be simple array with direction or object array with single field
        if (args[1].is_string()) {
            std::string second_arg = args[1].get<std::string>();
            if (second_arg == "desc" || second_arg == "asc") {
                // Simple array with direction
                config.is_simple_array = true;
                config.direction = second_arg;
            } else {
                // Could be JSON pointer for object array field
                if (second_arg.empty() || second_arg[0] == '/') {
                    // Object array with single field (string)
                    config.is_simple_array = false;
                    config.fields.push_back(parse_field_descriptor(args[1]));
                } else {
                    throw InvalidArgumentException("Invalid sort direction: " + second_arg + " (use 'asc' or 'desc')");
                }
            }
        } else {
            // Object array with single field (array format)
            config.is_simple_array = false;
            config.fields.push_back(parse_field_descriptor(args[1]));
        }
    } else {
        // Multiple fields for object sorting
        config.is_simple_array = false;
        for (size_t i = 1; i < args.size(); ++i) {
            config.fields.push_back(parse_field_descriptor(args[i]));
        }
    }

    return config;
}

// Type-aware comparison (-1: a < b, 0: a == b, 1: a > b)
// Ensures consistent ordering for mixed-type arrays
int type_aware_compare(const nlohmann::json& a, const nlohmann::json& b) {
    // Type ordering: null < numbers < strings < booleans < arrays < objects
    auto get_type_order = [](const nlohmann::json& val) {
        if (val.is_null())
            return 0;
        if (val.is_number())
            return 1;
        if (val.is_string())
            return 2;
        if (val.is_boolean())
            return 3;
        if (val.is_array())
            return 4;
        if (val.is_object())
            return 5;
        return 6;
    };

    int type_a = get_type_order(a);
    int type_b = get_type_order(b);

    if (type_a != type_b) {
        return type_a - type_b;
    }

    // Same type, compare values
    if (a.is_null())
        return 0;
    if (a.is_number())
        return (a.get<double>() < b.get<double>()) ? -1 : (a.get<double>() > b.get<double>()) ? 1
                                                                                              : 0;
    if (a.is_string())
        return a.get<std::string>().compare(b.get<std::string>());
    if (a.is_boolean())
        return (a.get<bool>() < b.get<bool>()) ? -1 : (a.get<bool>() > b.get<bool>()) ? 1
                                                                                      : 0;

    // For arrays and objects, use JSON string representation
    std::string a_str = a.dump();
    std::string b_str = b.dump();
    return a_str.compare(b_str);
}

// Extract field value using JSON Pointer, returns null if field not found
nlohmann::json extract_field_value(const nlohmann::json& obj, const std::string& pointer) {
    if (pointer.empty()) {
        // Empty pointer means the value itself
        return obj;
    }

    try {
        return obj.at(nlohmann::json::json_pointer(pointer));
    } catch (const std::exception&) {
        // Field not found, return null
        return nlohmann::json(nullptr);
    }
}

// Create multi-field comparator that sorts by each field in sequence
std::function<bool(const nlohmann::json&, const nlohmann::json&)>
create_multi_field_comparator(const std::vector<FieldDescriptor>& fields) {
    return [fields](const nlohmann::json& a, const nlohmann::json& b) -> bool {
        for (const auto& field : fields) {
            auto val_a = extract_field_value(a, field.pointer);
            auto val_b = extract_field_value(b, field.pointer);

            int cmp = type_aware_compare(val_a, val_b);
            if (cmp != 0) {
                return field.ascending ? (cmp < 0) : (cmp > 0);
            }
            // Fields are equal, continue to next field
        }
        return false; // All fields equal
    };
}

// Main sort implementation - orchestrates argument parsing and sorting
nlohmann::json sort_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 1) {
        throw InvalidArgumentException("sort requires at least 1 argument");
    }

    auto array_container = evaluate(args[0], ctx);
    if (!array_container.is_object() || array_container.find("array") == array_container.end()) {
        throw InvalidArgumentException("sort requires array container as first argument");
    }

    auto array = array_container["array"];
    if (!array.is_array()) {
        throw InvalidArgumentException("sort requires array of values");
    }

    // Parse arguments
    SortConfig config = parse_sort_arguments(args);

    // Make a copy to sort
    auto result = array;

    if (config.is_simple_array) {
        // Simple array sorting
        bool ascending = (config.direction == "asc");
        std::sort(result.begin(), result.end(), [ascending](const nlohmann::json& a, const nlohmann::json& b) {
            int cmp = type_aware_compare(a, b);
            return ascending ? (cmp < 0) : (cmp > 0);
        });
    } else {
        // Object array sorting with fields
        auto comparator = create_multi_field_comparator(config.fields);
        std::sort(result.begin(), result.end(), comparator);
    }

    return nlohmann::json::object({ { "array", result } });
}

nlohmann::json reverse_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("reverse requires exactly 1 argument");
    }

    auto array_container = evaluate(args[0], ctx);
    if (!array_container.is_object() || array_container.find("array") == array_container.end()) {
        throw InvalidArgumentException("reverse requires array container argument");
    }

    auto array = array_container["array"];
    if (!array.is_array()) {
        throw InvalidArgumentException("reverse requires array of values");
    }

    // Make a copy and reverse it
    auto result = array;
    std::reverse(result.begin(), result.end());

    return nlohmann::json::object({ { "array", result } });
}

// Unique configuration for argument parsing
struct UniqueConfig {
    std::string mode = "firsts"; // "firsts", "lasts", "multiples", "singles"
    std::string field_pointer = ""; // JSON Pointer for object field uniqueness
};

// Parse unique arguments to determine mode and optional field
UniqueConfig parse_unique_arguments(const nlohmann::json& args) {
    UniqueConfig config;

    if (args.size() == 1) {
        // Default: simple array, "firsts" mode
        config.mode = "firsts";
    } else if (args.size() == 2) {
        std::string second_arg = args[1].get<std::string>();
        if (second_arg == "firsts" || second_arg == "lasts" || second_arg == "multiples" || second_arg == "singles") {
            // Simple array with mode
            config.mode = second_arg;
        } else if (second_arg.empty() || second_arg[0] == '/') {
            // Object array with field, default mode
            config.field_pointer = second_arg;
            config.mode = "firsts";
        } else {
            throw InvalidArgumentException("Invalid unique mode: " + second_arg + " (use 'firsts', 'lasts', 'multiples', or 'singles')");
        }
    } else if (args.size() == 3) {
        // Object array with field and mode
        config.field_pointer = args[1].get<std::string>();
        config.mode = args[2].get<std::string>();
        if (config.mode != "firsts" && config.mode != "lasts" && config.mode != "multiples" && config.mode != "singles") {
            throw InvalidArgumentException("Invalid unique mode: " + config.mode + " (use 'firsts', 'lasts', 'multiples', or 'singles')");
        }
    } else {
        throw InvalidArgumentException("unique requires 1-3 arguments");
    }

    return config;
}

// Extract uniqueness key from array element using optional field pointer
nlohmann::json extract_unique_key(const nlohmann::json& element, const std::string& field_pointer) {
    if (field_pointer.empty()) {
        // Simple value uniqueness
        return element;
    } else {
        // Field-based uniqueness (reuse extraction logic from sort)
        return extract_field_value(element, field_pointer);
    }
}

// Sliding window unique algorithm using two booleans
nlohmann::json unique_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 1 || args.size() > 3) {
        throw InvalidArgumentException("unique requires 1-3 arguments");
    }

    auto array_container = evaluate(args[0], ctx);
    if (!array_container.is_object() || array_container.find("array") == array_container.end()) {
        throw InvalidArgumentException("unique requires array container argument");
    }

    auto array = array_container["array"];
    if (!array.is_array()) {
        throw InvalidArgumentException("unique requires array of values");
    }

    // Parse arguments
    UniqueConfig config = parse_unique_arguments(args);

    nlohmann::json result = nlohmann::json::array();

    if (array.size() == 0) {
        return nlohmann::json::object({ { "array", result } });
    }

    if (array.size() == 1) {
        // Single element: always include for all modes
        result.push_back(array[0]);
        return nlohmann::json::object({ { "array", result } });
    }

    // Sliding window algorithm with two booleans
    bool left = false; // Current item equals previous item?
    bool right; // Current item equals next item?

    for (size_t i = 0; i < array.size(); ++i) {
        // Determine right boolean: does current item equal next item?
        if (i == array.size() - 1) {
            right = false; // Last item, no next item to compare
        } else {
            auto current_key = extract_unique_key(array[i], config.field_pointer);
            auto next_key = extract_unique_key(array[i + 1], config.field_pointer);
            right = (current_key == next_key);
        }

        // Apply mode logic using two booleans
        bool should_output = false;
        if (config.mode == "firsts") {
            should_output = !left; // First occurrence
        } else if (config.mode == "lasts") {
            should_output = !right; // Last occurrence
        } else if (config.mode == "singles") {
            should_output = !left && !right; // Appears exactly once
        } else if (config.mode == "multiples") {
            should_output = (left || right) && !left; // Has duplicates, first occurrence only
        }

        if (should_output) {
            result.push_back(array[i]);
        }

        // Slide the window: right becomes left for next iteration
        left = right;
    }

    return nlohmann::json::object({ { "array", result } });
}

}