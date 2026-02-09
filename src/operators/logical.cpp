#include "operators/shared.hpp"

namespace computo::operators {

auto logical_and(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'and' requires at least 1 argument", ctx.get_path_string());
    }

    // N-ary AND with short-circuit evaluation
    for (size_t i = 0; i < args.size(); ++i) {
        auto value = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        if (!is_truthy(value)) {
            return EvaluationResult(jsom::JsonDocument(false));
        }
    }

    return EvaluationResult(jsom::JsonDocument(true));
}

auto logical_or(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'or' requires at least 1 argument", ctx.get_path_string());
    }

    // N-ary OR with short-circuit evaluation
    for (size_t i = 0; i < args.size(); ++i) {
        auto value = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        if (is_truthy(value)) {
            return EvaluationResult(jsom::JsonDocument(true));
        }
    }

    return EvaluationResult(jsom::JsonDocument(false));
}

auto logical_not(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'not' requires exactly 1 argument", ctx.get_path_string());
    }

    auto value = evaluate(args[0], ctx.with_path("arg0"));
    bool result = !is_truthy(value);

    return EvaluationResult(jsom::JsonDocument(result));
}

} // namespace computo::operators
