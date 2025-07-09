#include "shared.hpp"
#include "declarations.hpp"

namespace computo::operators {
using computo::evaluate;

static void ensure_numeric(const nlohmann::json& v, const std::string& name) {
    if (!v.is_number()) throw InvalidArgumentException(name + " requires numeric arguments");
}

nlohmann::json greater_than(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("> requires at least 2 arguments");
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        ensure_numeric(a, ">");
        ensure_numeric(b, ">");
        if (!(a.get<double>() > b.get<double>())) return false;
    }
    return true;
}

nlohmann::json less_than(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("< requires at least 2 arguments");
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        ensure_numeric(a, "<");
        ensure_numeric(b, "<");
        if (!(a.get<double>() < b.get<double>())) return false;
    }
    return true;
}

nlohmann::json greater_equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException(">= requires at least 2 arguments");
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        ensure_numeric(a, ">=");
        ensure_numeric(b, ">=");
        if (!(a.get<double>() >= b.get<double>())) return false;
    }
    return true;
}

nlohmann::json less_equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("<= requires at least 2 arguments");
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        ensure_numeric(a, "<=");
        ensure_numeric(b, "<=");
        if (!(a.get<double>() <= b.get<double>())) return false;
    }
    return true;
}

nlohmann::json equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("== requires at least 2 arguments");
    auto first = evaluate(args[0], ctx);
    for (size_t i = 1; i < args.size(); ++i) {
        if (first != evaluate(args[i], ctx)) return false;
    }
    return true;
}

nlohmann::json not_equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) throw InvalidArgumentException("!= requires exactly 2 arguments");
    return evaluate(args[0], ctx) != evaluate(args[1], ctx);
}

} 