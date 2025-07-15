#pragma once
#include <cmath>
#include <computo.hpp>

namespace computo {
// Forward declaration so operator helpers can recursively evaluate expressions
auto evaluate(nlohmann::json expr, ExecutionContext ctx) -> nlohmann::json;
}

namespace computo::operators {

// Return JSON truthiness consistent with spec
inline auto is_truthy(const nlohmann::json& value) -> bool {
    if (value.is_boolean()) {
        return value.get<bool>();
}
    if (value.is_number()) {
        return value.get<double>() != 0.0;
}
    if (value.is_string()) {
        return !value.get<std::string>().empty();
}
    if (value.is_null()) {
        return false;
}
    return !value.empty(); // arrays/objects
}

// Evaluate a simple single-parameter lambda expression against arg
inline auto evaluate_lambda(const nlohmann::json& lambda,
    const nlohmann::json& arg,
    ExecutionContext& ctx) -> nlohmann::json {
    if (!lambda.is_array() || lambda.size() != 3 || lambda[0] != "lambda") {
        throw InvalidArgumentException("Invalid lambda expression");
    }
    if (!lambda[1].is_array() || lambda[1].size() != 1 || !lambda[1][0].is_string()) {
        throw InvalidArgumentException("Lambda must have exactly one string parameter");
    }
    std::string param = lambda[1][0].get<std::string>();
    std::map<std::string, nlohmann::json> vars = { { param, arg } };
    ExecutionContext new_ctx = ctx.with_variables(vars);
    return computo::evaluate(lambda[2], new_ctx);
}

inline void validate_numeric_args(const nlohmann::json& evaluated, const std::string& op_name) {
    if (!evaluated.is_number()) {
        throw InvalidArgumentException(op_name + " requires numeric arguments");
    }
}

} // namespace computo::operators