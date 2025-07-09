#include "shared.hpp"
#include "declarations.hpp"

namespace computo::operators {
using computo::evaluate;

nlohmann::json logical_and(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("&& requires at least 1 argument");
    for (const auto& expr : args) {
        if (!is_truthy(evaluate(expr, ctx))) {
            return false;
        }
    }
    return true;
}

nlohmann::json logical_or(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("|| requires at least 1 argument");
    for (const auto& expr : args) {
        if (is_truthy(evaluate(expr, ctx))) {
            return true;
        }
    }
    return false;
}

nlohmann::json logical_not(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) throw InvalidArgumentException("not requires exactly 1 argument");
    return !is_truthy(evaluate(args[0], ctx));
}

} 