#include "shared.hpp"
#include <algorithm>
#include <sstream>
#include <vector>

namespace computo {

auto is_truthy(const nlohmann::json& value) -> bool {
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number()) {
        return value.get<double>() != 0.0;
    }
    if (value.is_string()) {
        return !value.get<std::string>().empty();
    }
    if (value.is_array() || value.is_object()) {
        return !value.empty();
    }
    if (value.is_null()) {
        return false;
    }
    // All other types are truthy
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto validate_numeric_args(const nlohmann::json& args, const std::string& op_name,
                           const std::string& path) -> void {
    for (size_t i = 0; i < args.size(); ++i) {
        if (!args[i].is_number()) {
            std::ostringstream oss;
            oss << op_name << " requires numeric arguments, got " << get_type_name(args[i])
                << " at argument " << i;
            throw InvalidArgumentException(oss.str(), path);
        }
    }
}

// NOLINTBEGIN(readability-function-size)
auto evaluate_lambda(const nlohmann::json& lambda_expr,
                     const std::vector<nlohmann::json>& lambda_args, ExecutionContext& ctx)
    -> EvaluationResult {
    // Lambda must be an array with exactly 2 elements: [params, body]
    if (!lambda_expr.is_array() || lambda_expr.size() != 2) {
        throw InvalidArgumentException("Lambda must be an array with 2 elements: [params, body]",
                                       ctx.get_path_string());
    }

    // First element must be an array of parameter names
    if (!lambda_expr[0].is_array()) {
        throw InvalidArgumentException("Lambda parameters must be an array", ctx.get_path_string());
    }

    // Check parameter count matches argument count
    if (lambda_expr[0].size() != lambda_args.size()) {
        std::ostringstream oss;
        oss << "Lambda expects " << lambda_expr[0].size() << " arguments, got "
            << lambda_args.size();
        throw InvalidArgumentException(oss.str(), ctx.get_path_string());
    }

    // Build variable bindings
    std::map<std::string, nlohmann::json> bindings;
    for (size_t i = 0; i < lambda_expr[0].size(); ++i) {
        if (!lambda_expr[0][i].is_string()) {
            throw InvalidArgumentException("Lambda parameter names must be strings",
                                           ctx.get_path_string());
        }
        std::string param_name = lambda_expr[0][i].get<std::string>();
        bindings[param_name] = lambda_args[i];
    }

    // Execute lambda body with parameter bindings
    auto lambda_ctx = ctx.with_variables(bindings);
    return evaluate_internal(lambda_expr[1], lambda_ctx.with_path("lambda_body"));
}
// NOLINTEND(readability-function-size)

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto to_numeric(const nlohmann::json& value, const std::string& op_name, const std::string& path)
    -> double {
    if (!value.is_number()) {
        std::ostringstream oss;
        oss << op_name << " requires numeric argument, got " << get_type_name(value);
        throw InvalidArgumentException(oss.str(), path);
    }
    return value.get<double>();
}

auto get_type_name(const nlohmann::json& value) -> std::string {
    if (value.is_null()) {
        return "null";
    }
    if (value.is_boolean()) {
        return "boolean";
    }
    if (value.is_number_integer()) {
        return "integer";
    }
    if (value.is_number_float()) {
        return "number";
    }
    if (value.is_string()) {
        return "string";
    }
    if (value.is_array()) {
        return "array";
    }
    if (value.is_object()) {
        return "object";
    }
    return "unknown";
}

auto extract_array_data(const nlohmann::json& array_input, const std::string& op_name,
                        const std::string& path) -> nlohmann::json {
    // Handle both {"array": [...]} format and direct array format
    if (array_input.is_object() && array_input.contains("array")
        && array_input["array"].is_array()) {
        return array_input["array"];
    }
    if (array_input.is_array()) {
        return array_input;
    }

    throw InvalidArgumentException("'" + op_name + "' requires an array argument", path);
}

// NOLINTBEGIN(readability-function-size)
auto calculate_levenshtein_distance(const std::string& first_string,
                                    const std::string& second_string) -> int {
    const auto len1 = first_string.length();
    const auto len2 = second_string.length();

    // Create a matrix of size (len1+1) x (len2+1)
    std::vector<std::vector<int>> distance_matrix(len1 + 1, std::vector<int>(len2 + 1));

    // Initialize the matrix
    for (size_t i = 0; i <= len1; ++i) {
        distance_matrix[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= len2; ++j) {
        distance_matrix[0][j] = static_cast<int>(j);
    }

    // Fill the matrix
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (first_string[i - 1] == second_string[j - 1]) {
                distance_matrix[i][j] = distance_matrix[i - 1][j - 1];
            } else {
                distance_matrix[i][j] = 1
                                        + std::min({
                                            distance_matrix[i - 1][j],    // deletion
                                            distance_matrix[i][j - 1],    // insertion
                                            distance_matrix[i - 1][j - 1] // substitution
                                        });
            }
        }
    }

    return distance_matrix[len1][len2];
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto suggest_similar_names(const std::string& target, const std::vector<std::string>& candidates,
                           int max_distance) -> std::vector<std::string> {
    struct Candidate {
        std::string name;
        int distance;
    };

    std::vector<Candidate> matches;

    for (const auto& candidate : candidates) {
        int distance = calculate_levenshtein_distance(target, candidate);
        if (distance <= max_distance) {
            matches.push_back({candidate, distance});
        }
    }

    // Sort by distance (closest first), then alphabetically
    std::sort(matches.begin(), matches.end(), [](const Candidate& first, const Candidate& second) {
        if (first.distance != second.distance) {
            return first.distance < second.distance;
        }
        return first.name < second.name;
    });

    // Extract just the names
    std::vector<std::string> suggestions;
    suggestions.reserve(matches.size());
    for (const auto& match : matches) {
        suggestions.push_back(match.name);
    }

    return suggestions;
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto process_array_with_lambda(
    const nlohmann::json& args, ExecutionContext& ctx, const std::string& op_name,
    const std::function<bool(const nlohmann::json& item, const nlohmann::json& lambda_result,
                             nlohmann::json& final_result)>& processor) -> nlohmann::json {
    if (args.size() != 2) {
        throw InvalidArgumentException("'" + op_name
                                           + "' requires exactly 2 arguments (array, lambda)",
                                       ctx.get_path_string());
    }

    auto array_input = evaluate(args[0], ctx);
    auto array_data = extract_array_data(array_input, op_name, ctx.get_path_string());

    nlohmann::json final_result; // The processor will populate this

    for (const auto& item : array_data) {
        std::vector<nlohmann::json> lambda_args = {item};
        auto lambda_result = evaluate_lambda(args[1], lambda_args, ctx);

        // Resolve any tail calls from lambda evaluation
        while (lambda_result.is_tail_call) {
            lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                              lambda_result.tail_call->context);
        }

        // Let the processor handle the item and lambda result
        // The processor returns true to continue, false to break early (for find, some, every)
        bool should_continue = processor(item, lambda_result.value, final_result);
        if (!should_continue) {
            break;
        }
    }

    return final_result;
}
// NOLINTEND(readability-function-size)

auto evaluate_json_pointer(const nlohmann::json& root, const std::string& pointer_str,
                           const std::string& path_context) -> nlohmann::json {
    if (pointer_str.empty() || pointer_str[0] != '/') {
        throw InvalidArgumentException("Requires JSON Pointer format starting with '/'",
                                       path_context);
    }

    try {
        return root.at(nlohmann::json::json_pointer(pointer_str));
    } catch (const nlohmann::json::exception& e) {
        throw InvalidArgumentException(
            "Invalid JSON Pointer path '" + pointer_str + "': " + e.what(), path_context);
    }
}

auto parse_variable_path(const std::string& full_path) -> VariablePathParts {
    VariablePathParts parts;

    // Find the second slash to split variable name from sub-path
    auto second_slash = full_path.find('/', 1);

    if (second_slash == std::string::npos) {
        // Simple variable: "/variable_name"
        parts.variable_name = full_path.substr(1);
        parts.sub_path = "";
    } else {
        // Nested access: "/variable_name/nested/path"
        parts.variable_name = full_path.substr(1, second_slash - 1);
        parts.sub_path = full_path.substr(second_slash);
    }

    return parts;
}

} // namespace computo
