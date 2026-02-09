#include "operators/shared.hpp"

namespace computo::operators {

auto if_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 3) {
        throw InvalidArgumentException("'if' requires exactly 3 arguments (condition, then, else)",
                                       ctx.get_path_string());
    }

    // Evaluate condition
    auto condition = evaluate(args[0], ctx.with_path("condition"));

    // Return tail call for TCO optimization
    if (is_truthy(condition)) {
        // Return tail call to then branch
        return {args[1], ctx.with_path("then")};
    }
    // Return tail call to else branch
    return {args[2], ctx.with_path("else")};
}

auto lambda_operator(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'lambda' requires exactly 2 arguments: [params, body]",
                                       ctx.get_path_string());
    }

    // First argument must be an array of parameter names
    if (!args[0].is_array()) {
        throw InvalidArgumentException("Lambda parameters must be an array", ctx.get_path_string());
    }

    // Validate parameter names are strings
    for (const auto& param : args[0]) {
        if (!param.is_string()) {
            throw InvalidArgumentException("Lambda parameter names must be strings",
                                           ctx.get_path_string());
        }
    }

    // Return the lambda as [params, body] for evaluate_lambda to process
    jsom::JsonDocument lambda_expr = jsom::JsonDocument::make_array();
    lambda_expr.push_back(args[0]); // params
    lambda_expr.push_back(args[1]); // body (not evaluated here)

    return EvaluationResult(lambda_expr);
}

} // namespace computo::operators
