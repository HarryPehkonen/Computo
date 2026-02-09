#include "operators/shared.hpp"

namespace computo::operators {

auto greater_than(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'>' requires at least 2 arguments", ctx.get_path_string());
    }

    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto lhs = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        auto rhs = evaluate(args[i + 1], ctx.with_path("arg" + std::to_string(i + 1)));
        if (!lhs.is_number() || !rhs.is_number()) {
            throw InvalidArgumentException("'>' requires numeric arguments", ctx.get_path_string());
        }
        if (!(lhs.as<double>() > rhs.as<double>())) {
            return EvaluationResult(false);
        }
    }
    return EvaluationResult(true);
}

auto less_than(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'<' requires at least 2 arguments", ctx.get_path_string());
    }

    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto lhs = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        auto rhs = evaluate(args[i + 1], ctx.with_path("arg" + std::to_string(i + 1)));
        if (!lhs.is_number() || !rhs.is_number()) {
            throw InvalidArgumentException("'<' requires numeric arguments", ctx.get_path_string());
        }
        if (!(lhs.as<double>() < rhs.as<double>())) {
            return EvaluationResult(false);
        }
    }
    return EvaluationResult(true);
}

auto greater_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'>=' requires at least 2 arguments", ctx.get_path_string());
    }

    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto lhs = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        auto rhs = evaluate(args[i + 1], ctx.with_path("arg" + std::to_string(i + 1)));
        if (!lhs.is_number() || !rhs.is_number()) {
            throw InvalidArgumentException("'>=' requires numeric arguments",
                                           ctx.get_path_string());
        }
        if (!(lhs.as<double>() >= rhs.as<double>())) {
            return EvaluationResult(false);
        }
    }
    return EvaluationResult(true);
}

auto less_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'<=' requires at least 2 arguments", ctx.get_path_string());
    }

    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto lhs = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        auto rhs = evaluate(args[i + 1], ctx.with_path("arg" + std::to_string(i + 1)));
        if (!lhs.is_number() || !rhs.is_number()) {
            throw InvalidArgumentException("'<=' requires numeric arguments",
                                           ctx.get_path_string());
        }
        if (!(lhs.as<double>() <= rhs.as<double>())) {
            return EvaluationResult(false);
        }
    }
    return EvaluationResult(true);
}

auto equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() < 2) {
        throw InvalidArgumentException("'==' requires at least 2 arguments", ctx.get_path_string());
    }

    auto first = evaluate(args[0], ctx.with_path("arg0"));
    for (size_t i = 1; i < args.size(); ++i) {
        auto current = evaluate(args[i], ctx.with_path("arg" + std::to_string(i)));
        if (first != current) {
            return EvaluationResult(false);
        }
    }
    return EvaluationResult(true);
}

auto not_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'!=' requires exactly 2 arguments", ctx.get_path_string());
    }

    auto lhs = evaluate(args[0], ctx.with_path("arg0"));
    auto rhs = evaluate(args[1], ctx.with_path("arg1"));
    return EvaluationResult(lhs != rhs);
}

} // namespace computo::operators
