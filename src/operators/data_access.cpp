#include "operators/shared.hpp"

namespace computo::operators {

auto input_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        return EvaluationResult(ctx.input());
    }

    if (args.size() != 1 || !args[0].is_string()) {
        throw InvalidArgumentException("'$input' requires 0 or 1 string argument (JSON Pointer)",
                                       ctx.get_path_string());
    }

    auto result = evaluate_json_pointer(ctx.input(), args[0].as<std::string>(),
                                        ctx.get_path_string() + " (in $input)");
    return EvaluationResult(result);
}

auto inputs_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    // No arguments - return entire inputs array
    if (args.empty()) {
        jsom::JsonDocument result = jsom::JsonDocument::make_array();
        for (const auto& input : ctx.inputs()) {
            result.push_back(input);
        }
        return EvaluationResult(result);
    }

    if (args.size() != 1 || !args[0].is_string()) {
        throw InvalidArgumentException("'$inputs' requires 0 or 1 string argument (JSON Pointer)",
                                       ctx.get_path_string());
    }

    // Create inputs array for JSON Pointer navigation
    jsom::JsonDocument inputs_array = jsom::JsonDocument::make_array();
    for (const auto& input : ctx.inputs()) {
        inputs_array.push_back(input);
    }

    auto result = evaluate_json_pointer(inputs_array, args[0].as<std::string>(),
                                        ctx.get_path_string() + " (in $inputs)");
    return EvaluationResult(result);
}

// NOLINTBEGIN(readability-function-size)
auto variable_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1 || !args[0].is_string()) {
        throw InvalidArgumentException("'$' requires exactly 1 string argument (JSON Pointer)",
                                       ctx.get_path_string());
    }

    std::string json_pointer = args[0].as<std::string>();

    // Require JSON Pointer format starting with "/"
    if (json_pointer.empty() || json_pointer[0] != '/') {
        throw InvalidArgumentException(
            "'$' requires JSON Pointer format starting with '/' (e.g., '/variable_name')",
            ctx.get_path_string());
    }

    // Parse variable name and sub-path
    auto parts = parse_variable_path(json_pointer);

    // Look up the variable
    auto iter = ctx.variables.find(parts.variable_name);
    if (iter == ctx.variables.end()) {
        // Extract available variable names for suggestions
        std::vector<std::string> available_vars;
        available_vars.reserve(ctx.variables.size());
        for (const auto& [name, _] : ctx.variables) {
            available_vars.push_back(name);
        }

        auto suggestions = suggest_similar_names(parts.variable_name, available_vars, 2);

        std::string message = "Variable not found: '" + parts.variable_name + "'";
        if (!suggestions.empty()) {
            message += ". Did you mean '" + suggestions[0] + "'?";
        }

        throw InvalidArgumentException(message, ctx.get_path_string());
    }

    // If no sub-path, return the variable directly
    if (parts.sub_path.empty()) {
        return EvaluationResult(iter->second);
    }

    // Use shared JSON Pointer evaluation for sub-path
    auto result = evaluate_json_pointer(iter->second, parts.sub_path,
                                        ctx.get_path_string() + " (in variable '"
                                            + parts.variable_name + "')");
    return EvaluationResult(result);
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto let_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'let' requires exactly 2 arguments (bindings and body)",
                                       ctx.get_path_string());
    }

    std::map<std::string, jsom::JsonDocument> new_variables;

    // Support both object format {"x": 42} and array format [["x", 42]]
    if (args[0].is_object()) {
        // Object format: {"x": 42, "y": 100}
        for (const auto& [key, value] : args[0].items()) {
            new_variables[key] = evaluate(value, ctx.with_path("binding_value_for_" + key));
        }
    } else if (args[0].is_array()) {
        // Array format: [["x", 42], ["y", 100]]
        for (size_t i = 0; i < args[0].size(); ++i) {
            const auto& binding = args[0][i];
            if (!binding.is_array() || binding.size() != 2 || !binding[0].is_string()) {
                throw InvalidArgumentException(
                    "'let' binding must be a [name, value] array where name is "
                    "a string",
                    ctx.get_path_string());
            }
            std::string var_name = binding[0].as<std::string>();
            new_variables[var_name]
                = evaluate(binding[1], ctx.with_path("binding_value_for_" + var_name));
        }
    } else {
        throw InvalidArgumentException(
            "'let' bindings must be an object {\"name\": value} or array of "
            "[name, value] pairs",
            ctx.get_path_string());
    }

    ExecutionContext new_ctx = ctx.with_variables(new_variables);
    return {args[1], new_ctx.with_path("let_body")}; // Tail call
}
// NOLINTEND(readability-function-size)

} // namespace computo::operators
