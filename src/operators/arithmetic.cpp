#include "shared.hpp"
#include "declarations.hpp"
#include <cmath>

namespace computo::operators {
using computo::evaluate;

nlohmann::json addition(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("+ requires at least 1 argument");
    double result = 0.0;
    for (const auto& expr : args) {
        auto val = evaluate(expr, ctx);
        validate_numeric_args(val, "+");
        result += val.get<double>();
    }
    return result;
}

nlohmann::json subtraction(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("- requires at least 1 argument");
    auto first = evaluate(args[0], ctx);
    validate_numeric_args(first, "-");
    if (args.size() == 1) {
        return -first.get<double>();
    }
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto val = evaluate(args[i], ctx);
        validate_numeric_args(val, "-");
        result -= val.get<double>();
    }
    return result;
}

nlohmann::json multiplication(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("* requires at least 1 argument");
    double result = 1.0;
    for (const auto& expr : args) {
        auto val = evaluate(expr, ctx);
        validate_numeric_args(val, "*");
        result *= val.get<double>();
    }
    return result;
}

nlohmann::json division(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("/ requires at least 1 argument");
    auto first = evaluate(args[0], ctx);
    validate_numeric_args(first, "/");
    if (args.size() == 1) {
        if (first.get<double>() == 0.0) throw InvalidArgumentException("Division by zero");
        return 1.0 / first.get<double>();
    }
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto val = evaluate(args[i], ctx);
        validate_numeric_args(val, "/");
        double d = val.get<double>();
        if (d == 0.0) throw InvalidArgumentException("Division by zero");
        result /= d;
    }
    return result;
}

nlohmann::json modulo(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("% requires at least 2 arguments");
    auto first = evaluate(args[0], ctx);
    validate_numeric_args(first, "%");
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto val = evaluate(args[i], ctx);
        validate_numeric_args(val, "%");
        double d = val.get<double>();
        if (d == 0.0) throw InvalidArgumentException("Modulo by zero");
        result = std::fmod(result, d);
    }
    return result;
}

} 