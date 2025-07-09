#include "declarations.hpp"
#include "shared.hpp"

namespace computo::operators {
using computo::evaluate;

// Helper to split JSON pointer-like path into segments (no escaping support yet)
static std::vector<std::string> split_segments(const std::string& ptr) {
    std::vector<std::string> segs;
    size_t start = 0;
    while (start < ptr.size()) {
        size_t slash = ptr.find('/', start);
        std::string part = ptr.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        segs.push_back(part);
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
    return segs;
}

nlohmann::json var_access(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() == 0) {
        // Return entire variable scope as object
        return nlohmann::json(ctx.variables);
    }
    if (args.size() != 1) throw InvalidArgumentException("$ requires 0 or 1 argument");
    
    auto path_expr = evaluate(args[0], ctx);
    if (!path_expr.is_string()) throw InvalidArgumentException("$ requires string JSON pointer argument");
    std::string ptr = path_expr.get<std::string>();
    if (ptr.empty() || ptr[0] != '/') throw InvalidArgumentException("$ pointer must start with '/'");

    auto segs = split_segments(ptr.substr(1));
    // First segment is variable name in ctx.variables
    if (segs.empty()) throw InvalidArgumentException("$ pointer missing variable name");
    auto it = ctx.variables.find(segs[0]);
    if (it == ctx.variables.end()) {
        throw InvalidArgumentException("Undefined variable: " + segs[0]);
    }
    nlohmann::json current = it->second;
    // Traverse remaining segments for nested objects/arrays
    for (size_t i = 1; i < segs.size(); ++i) {
        const std::string& seg = segs[i];
        if (current.is_object()) {
            current = current[seg];
        } else if (current.is_array()) {
            size_t idx = std::stoul(seg);
            if (idx >= current.size()) throw InvalidArgumentException("Array index out of bounds");
            current = current[idx];
        } else {
            throw InvalidArgumentException("Cannot traverse non-container");
        }
    }
    return current;
}


nlohmann::json let_binding(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) throw InvalidArgumentException("let requires exactly 2 arguments");
    if (!args[0].is_array()) throw InvalidArgumentException("let bindings must be array");
    
    std::map<std::string, nlohmann::json> new_vars;
    for (const auto& binding : args[0]) {
        if (!binding.is_array() || binding.size() != 2) {
            throw InvalidArgumentException("let binding must be [var, value]");
        }
        if (!binding[0].is_string()) {
            throw InvalidArgumentException("let variable name must be string");
        }
        std::string var_name = binding[0].get<std::string>();
        new_vars[var_name] = evaluate(binding[1], ctx);
    }
    
    return evaluate(args[1], ctx.with_variables(new_vars));
}

nlohmann::json obj_construct(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 1) throw InvalidArgumentException("obj requires at least 1 argument");
    
    nlohmann::json result = nlohmann::json::object();
    
    // Simple syntax: ["obj", ["key1", "value1"], ["key2", "value2"]]
    for (const auto& arg : args) {
        if (!arg.is_array() || arg.size() != 2) {
            throw InvalidArgumentException("obj requires [key_expr, value_expr] pairs");
        }
        
        // Evaluate both key and value expressions
        auto key_result = evaluate(arg[0], ctx);
        auto value_result = evaluate(arg[1], ctx);
        
        if (!key_result.is_string()) {
            throw InvalidArgumentException("obj key must evaluate to string");
        }
        
        std::string key = key_result.get<std::string>();
        result[key] = value_result;
    }
    
    return result;
}

nlohmann::json if_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 3) {
        throw InvalidArgumentException("if requires exactly 3 arguments: [condition, then_expr, else_expr]");
    }
    
    auto condition = evaluate(args[0], ctx);
    if (is_truthy(condition)) {
        return evaluate(args[1], ctx);
    } else {
        return evaluate(args[2], ctx);
    }
}

} 