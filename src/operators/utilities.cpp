#include "declarations.hpp"
#include "shared.hpp"
#include <cmath>

namespace computo::operators {
using computo::evaluate;

nlohmann::json str_concat(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2)
        throw InvalidArgumentException("strConcat requires at least 2 arguments");

    std::string result;
    for (const auto& arg : args) {
        auto evaluated = evaluate(arg, ctx);
        if (!evaluated.is_string()) {
            throw InvalidArgumentException("strConcat requires string arguments");
        }
        result += evaluated.get<std::string>();
    }
    return result;
}

nlohmann::json merge_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2)
        throw InvalidArgumentException("merge requires at least 2 arguments");

    nlohmann::json result = nlohmann::json::object();

    for (const auto& arg : args) {
        auto evaluated = evaluate(arg, ctx);
        if (!evaluated.is_object()) {
            throw InvalidArgumentException("merge requires object arguments");
        }

        for (auto it = evaluated.begin(); it != evaluated.end(); ++it) {
            result[it.key()] = it.value();
        }
    }

    return result;
}

nlohmann::json approx_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("approx requires exactly 2 arguments");

    auto val1 = evaluate(args[0], ctx);
    auto val2 = evaluate(args[1], ctx);

    if (!val1.is_number() || !val2.is_number()) {
        throw InvalidArgumentException("approx requires numeric arguments");
    }

    double num1 = val1.get<double>();
    double num2 = val2.get<double>();

    // Use a larger epsilon for floating point comparison
    const double epsilon = 1e-9;
    return std::abs(num1 - num2) < epsilon;
}

}