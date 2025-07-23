#include "operators/shared.hpp"
#include <cmath> // For std::fmod

namespace computo::operators {

auto addition(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'+' requires at least 1 argument", ctx.get_path_string());
    }

    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) {
            throw InvalidArgumentException("'+' requires numeric arguments", ctx.get_path_string());
        }
        result += arg.get<double>();
    }
    return EvaluationResult(result);
}

auto subtraction(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'-' requires at least 1 argument", ctx.get_path_string());
    }

    auto first_arg = evaluate(args[0], ctx);
    if (!first_arg.is_number()) {
        throw InvalidArgumentException("'-' requires numeric arguments", ctx.get_path_string());
    }

    if (args.size() == 1) {
        return EvaluationResult(-first_arg.get<double>()); // Unary negation
    }

    double result = first_arg.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) {
            throw InvalidArgumentException("'-' requires numeric arguments", ctx.get_path_string());
        }
        result -= arg.get<double>();
    }
    return EvaluationResult(result);
}

auto multiplication(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'*' requires at least 1 argument", ctx.get_path_string());
    }

    double result = 1.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) {
            throw InvalidArgumentException("'*' requires numeric arguments", ctx.get_path_string());
        }
        result *= arg.get<double>();
    }
    return EvaluationResult(result);
}

// NOLINTBEGIN(readability-function-size)
auto division(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'/' requires at least 1 argument", ctx.get_path_string());
    }

    auto first_arg = evaluate(args[0], ctx);
    if (!first_arg.is_number()) {
        throw InvalidArgumentException("'/' requires numeric arguments", ctx.get_path_string());
    }

    if (args.size() == 1) {
        double first_val = first_arg.get<double>();
        if (first_val == 0.0) {
            throw InvalidArgumentException("Division by zero", ctx.get_path_string());
        }
        return EvaluationResult(1.0 / first_val); // Reciprocal
    }

    double result = first_arg.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) {
            throw InvalidArgumentException("'/' requires numeric arguments", ctx.get_path_string());
        }
        double divisor = arg.get<double>();
        if (divisor == 0.0) {
            throw InvalidArgumentException("Division by zero", ctx.get_path_string());
        }
        result /= divisor;
    }
    return EvaluationResult(result);
}
// NOLINTEND(readability-function-size)

auto modulo(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'%' requires at least 2 arguments", ctx.get_path_string());
    }

    auto first_arg = evaluate(args[0], ctx);
    if (!first_arg.is_number()) {
        throw InvalidArgumentException("'%' requires numeric arguments", ctx.get_path_string());
    }

    double result = first_arg.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) {
            throw InvalidArgumentException("'%' requires numeric arguments", ctx.get_path_string());
        }
        double divisor = arg.get<double>();
        if (divisor == 0.0) {
            throw InvalidArgumentException("Modulo by zero", ctx.get_path_string());
        }
        result = std::fmod(result, divisor);
    }
    return EvaluationResult(result);
}

} // namespace computo::operators
