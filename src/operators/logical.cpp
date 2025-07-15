#include "declarations.hpp"
#include "shared.hpp"

namespace computo::operators {
using computo::evaluate;

auto logical_and(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    if (args.empty()) {
        throw InvalidArgumentException("&& requires at least 1 argument");
}
    for (const auto& expr : args) {
        if (!is_truthy(evaluate(expr, ctx))) {
            return false;
        }
    }
    return true;
}

auto logical_or(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    if (args.empty()) {
        throw InvalidArgumentException("|| requires at least 1 argument");
}
    for (const auto& expr : args) {
        if (is_truthy(evaluate(expr, ctx))) {
            return true;
        }
    }
    return false;
}

auto logical_not(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    if (args.size() != 1) {
        throw InvalidArgumentException("not requires exactly 1 argument");
}
    return !is_truthy(evaluate(args[0], ctx));
}

}