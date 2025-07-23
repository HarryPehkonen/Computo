#include "operators/shared.hpp"
#include "operators/sort_utils.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <uni_algo/case.h>
#include <uni_algo/conv.h>
#include <uni_algo/ranges.h>
#include <uni_algo/ranges_conv.h>
#include <uni_algo/prop.h>

namespace computo::operators {

// NOLINTBEGIN(readability-function-size)
auto split_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'split' requires exactly 2 arguments (string, delimiter)",
                                       ctx.get_path_string());
    }

    auto str_val = evaluate(args[0], ctx);
    auto delim_val = evaluate(args[1], ctx);

    if (!str_val.is_string()) {
        throw InvalidArgumentException("'split' requires a string as first argument",
                                       ctx.get_path_string());
    }
    if (!delim_val.is_string()) {
        throw InvalidArgumentException("'split' requires a string delimiter as second argument",
                                       ctx.get_path_string());
    }

    std::string str = str_val.get<std::string>();
    std::string delimiter = delim_val.get<std::string>();

    nlohmann::json result = nlohmann::json::array();

    if (delimiter.empty()) {
        // Split into individual Unicode characters
        for (char32_t codepoint : str | una::ranges::views::utf8) {
            result.push_back(una::utf32to8(std::u32string(1, codepoint)));
        }
    } else {
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        result.push_back(str.substr(start));
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto join_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'join' requires exactly 2 arguments (array, delimiter)",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto delim_val = evaluate(args[1], ctx);

    if (!delim_val.is_string()) {
        throw InvalidArgumentException("'join' requires a string delimiter as second argument",
                                       ctx.get_path_string());
    }

    auto array_data = extract_array_data(array_input, "join", ctx.get_path_string());

    std::string delimiter = delim_val.get<std::string>();
    std::string result;

    for (size_t i = 0; i < array_data.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }

        // Convert element to string
        if (array_data[i].is_string()) {
            result += array_data[i].get<std::string>();
        } else if (array_data[i].is_boolean()) {
            result += array_data[i].get<bool>() ? "true" : "false";
        } else if (array_data[i].is_null()) {
            result += "null";
        } else {

            // numbers and unknown types
            result += array_data[i].dump();
        }
    }

    return EvaluationResult(result);
}
// NOLINTEND(readability-function-size)

auto trim_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'trim' requires exactly 1 argument", ctx.get_path_string());
    }

    auto str_val = evaluate(args[0], ctx);
    if (!str_val.is_string()) {
        throw InvalidArgumentException("'trim' requires a string argument", ctx.get_path_string());
    }

    std::string str = str_val.get<std::string>();

    // Collect all Unicode characters with their whitespace status
    std::vector<char32_t> codepoints;
    for (char32_t codepoint : str | una::ranges::views::utf8) {
        codepoints.push_back(codepoint);
    }
    
    if (codepoints.empty()) {
        return EvaluationResult(std::string{});
    }
    
    // Find first non-whitespace character
    size_t start = 0;
    while (start < codepoints.size() && una::codepoint::is_whitespace(codepoints[start])) {
        ++start;
    }
    
    // Find last non-whitespace character
    size_t end = codepoints.size();
    while (end > start && una::codepoint::is_whitespace(codepoints[end - 1])) {
        --end;
    }
    
    // Reconstruct string from non-whitespace portion
    if (start >= end) {
        // All whitespace
        return EvaluationResult(std::string{});
    }
    
    std::string result;
    for (size_t i = start; i < end; ++i) {
        result += una::utf32to8(std::u32string(1, codepoints[i]));
    }

    return EvaluationResult(result);
}

auto upper_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'upper' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto str_val = evaluate(args[0], ctx);
    if (!str_val.is_string()) {
        throw InvalidArgumentException("'upper' requires a string argument", ctx.get_path_string());
    }

    std::string str = str_val.get<std::string>();
    str = una::cases::to_uppercase_utf8(str);

    return EvaluationResult(str);
}

auto lower_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'lower' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto str_val = evaluate(args[0], ctx);
    if (!str_val.is_string()) {
        throw InvalidArgumentException("'lower' requires a string argument", ctx.get_path_string());
    }

    std::string str = str_val.get<std::string>();
    str = una::cases::to_lowercase_utf8(str);

    return EvaluationResult(str);
}

auto strConcat_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'strConcat' requires at least 1 argument",
                                       ctx.get_path_string());
    }

    std::string result;

    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);

        if (arg.is_string()) {
            result += arg.get<std::string>();
        } else if (arg.is_boolean()) {
            result += arg.get<bool>() ? "true" : "false";
        } else if (arg.is_null()) {
            result += "null";
        } else {

	    // numbers and unknown types
            result += arg.dump();
        }
    }

    return EvaluationResult(result);
}

// --- Sort Operator Implementation ---

// The new, clean main operator
auto sort_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'sort' requires at least 1 argument",
                                       ctx.get_path_string());
    }

    // 1. Argument parsing and data extraction remain here
    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "sort", ctx.get_path_string());

    // Parse arguments to determine sorting strategy
    SortConfig config;
    try {
        config = parse_sort_arguments(args);
    } catch (const InvalidArgumentException& e) {
        throw InvalidArgumentException(e.what(), ctx.get_path_string());
    }

    // Make a copy to sort
    nlohmann::json result = array_data;

    // 2. Dispatch to the correct helper
    if (config.is_simple_array) {
        sort_simple_array(result, config);
    } else {
        sort_object_array(result, config);
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}

auto reverse_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'reverse' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "reverse", ctx.get_path_string());

    nlohmann::json result = nlohmann::json::array();

    // Add elements in reverse order
    for (auto it = array_data.rbegin(); it != array_data.rend(); ++it) {
        result.push_back(*it);
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}

auto unique_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'unique' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "unique", ctx.get_path_string());

    nlohmann::json result = nlohmann::json::array();
    std::set<nlohmann::json> seen;

    for (const auto& item : array_data) {
        if (seen.find(item) == seen.end()) {
            seen.insert(item);
            result.push_back(item);
        }
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}

// --- uniqueSorted operator implementation ---

struct UniqueSortedConfig {
    std::string field_pointer; // JSON Pointer for field-based uniqueness
    std::string mode;          // "firsts", "lasts", "singles", "multiples"
};

auto extract_field_value(const nlohmann::json& obj, const std::string& pointer) -> nlohmann::json {
    if (pointer.empty()) {
        return obj;
    }

    try {
        return obj.at(nlohmann::json::json_pointer(pointer));
    } catch (const std::exception&) {
        return nlohmann::json{}; // Field not found, return null
    }
}

auto extract_unique_key(const nlohmann::json& element, const std::string& field_pointer)
    -> nlohmann::json {
    if (field_pointer.empty()) {
        return element; // Simple value uniqueness
    }
    return extract_field_value(element,
                               field_pointer); // Field-based uniqueness
}

// NOLINTBEGIN(readability-function-size)
auto parse_unique_sorted_config(const nlohmann::json& args) -> UniqueSortedConfig {
    UniqueSortedConfig config;
    config.mode = "firsts"; // Default mode

    if (args.size() == 1) {
        // ["uniqueSorted", array] -> mode="firsts", field=""
        config.mode = "firsts";
    } else if (args.size() == 2) {
        // ["uniqueSorted", array, mode_or_field]
        std::string second_arg = args[1].get<std::string>();
        if (second_arg == "firsts" || second_arg == "lasts" || second_arg == "multiples"
            || second_arg == "singles") {
            config.mode = second_arg;
        } else if (second_arg.empty() || second_arg[0] == '/') {
            config.field_pointer = second_arg;
            config.mode = "firsts";
        } else {
            throw InvalidArgumentException(
                "Invalid mode or field pointer: '" + second_arg
                    + "'. Valid modes: firsts, lasts, singles, multiples. "
                      "Field pointers must start with '/'",
                "");
        }
    } else if (args.size() == 3) {
        // ["uniqueSorted", array, field, mode]
        config.field_pointer = args[1].get<std::string>();
        config.mode = args[2].get<std::string>();

        // Validate mode
        if (config.mode != "firsts" && config.mode != "lasts" && config.mode != "multiples"
            && config.mode != "singles") {
            throw InvalidArgumentException(
                "Invalid mode: " + config.mode
                    + ". Valid modes are: firsts, lasts, singles, multiples",
                "");
        }
    } else {
        throw InvalidArgumentException("'uniqueSorted' requires 1-3 arguments", "");
    }

    return config;
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto unique_sorted_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty() || args.size() > 3) {
        throw InvalidArgumentException("'uniqueSorted' requires 1-3 arguments",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, "uniqueSorted", ctx.get_path_string());

    // Parse configuration
    UniqueSortedConfig config;
    try {
        config = parse_unique_sorted_config(args);
    } catch (const InvalidArgumentException& e) {
        throw InvalidArgumentException(e.what(), ctx.get_path_string());
    }

    nlohmann::json result = nlohmann::json::array();

    // Handle empty array
    if (array_data.empty()) {
        return EvaluationResult(nlohmann::json{{"array", result}});
    }

    // Sliding window algorithm with two booleans
    bool left = false; // Current element equals previous element?
    bool right;        // Current element equals next element?

    for (size_t i = 0; i < array_data.size(); ++i) {
        // Determine right boolean: does current element equal next element?
        if (i == array_data.size() - 1) {
            right = false; // Last element, no next element to compare
        } else {
            auto current_key = extract_unique_key(array_data[i], config.field_pointer);
            auto next_key = extract_unique_key(array_data[i + 1], config.field_pointer);
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
            should_output = left || right; // Has duplicates (ALL occurrences)
        }

        if (should_output) {
            result.push_back(array_data[i]);
        }

        // Slide the window: right becomes left for next iteration
        left = right;
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}
// NOLINTEND(readability-function-size)

auto zip_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'zip' requires exactly 2 arguments", ctx.get_path_string());
    }

    auto array1_input = evaluate(args[0], ctx);
    auto array2_input = evaluate(args[1], ctx);

    auto array1_data = extract_array_data(array1_input, "zip", ctx.get_path_string());
    auto array2_data = extract_array_data(array2_input, "zip", ctx.get_path_string());

    nlohmann::json result = nlohmann::json::array();

    size_t min_size = std::min(array1_data.size(), array2_data.size());
    for (size_t i = 0; i < min_size; ++i) {
        nlohmann::json pair = nlohmann::json::array();
        pair.push_back(array1_data[i]);
        pair.push_back(array2_data[i]);
        result.push_back(pair);
    }

    return EvaluationResult(nlohmann::json{{"array", result}});
}

auto approx_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 3) {
        throw InvalidArgumentException("'approx' requires exactly 3 arguments (a, b, tolerance)",
                                       ctx.get_path_string());
    }

    auto a_val = evaluate(args[0], ctx);
    auto b_val = evaluate(args[1], ctx);
    auto tolerance_val = evaluate(args[2], ctx);

    if (!a_val.is_number() || !b_val.is_number() || !tolerance_val.is_number()) {
        throw InvalidArgumentException("'approx' requires numeric arguments",
                                       ctx.get_path_string());
    }

    double first_value = a_val.get<double>();
    double second_value = b_val.get<double>();
    double tolerance = tolerance_val.get<double>();

    if (tolerance < 0) {
        throw InvalidArgumentException("'approx' requires non-negative tolerance",
                                       ctx.get_path_string());
    }

    bool result = std::abs(first_value - second_value) <= tolerance;
    return EvaluationResult(result);
}

} // namespace computo::operators
