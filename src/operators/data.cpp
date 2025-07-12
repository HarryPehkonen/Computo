#include "declarations.hpp"
#include "shared.hpp"
#include <set>

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
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return segs;
}

nlohmann::json var_access(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() == 0) {
        // Return entire variable scope as object
        return nlohmann::json(ctx.variables);
    }
    if (args.size() > 2)
        throw InvalidArgumentException("$ requires 0, 1, or 2 arguments");

    auto path_expr = evaluate(args[0], ctx);
    if (!path_expr.is_string())
        throw InvalidArgumentException("$ requires string JSON pointer argument");
    std::string ptr = path_expr.get<std::string>();
    if (ptr.empty() || ptr[0] != '/')
        throw InvalidArgumentException("$ pointer must start with '/'");

    auto segs = split_segments(ptr.substr(1));
    // First segment is variable name in ctx.variables
    if (segs.empty())
        throw InvalidArgumentException("$ pointer missing variable name");
    auto it = ctx.variables.find(segs[0]);
    if (it == ctx.variables.end()) {
        // If we have a default value (2nd argument), return it
        if (args.size() == 2) {
            return evaluate(args[1], ctx);
        }
        throw InvalidArgumentException("Undefined variable: " + segs[0]);
    }

    nlohmann::json current = it->second;
    // Traverse remaining segments for nested objects/arrays
    try {
        for (size_t i = 1; i < segs.size(); ++i) {
            const std::string& seg = segs[i];
            if (current.is_object()) {
                // Check if key exists before accessing
                if (current.find(seg) == current.end()) {
                    throw InvalidArgumentException("Object key not found: " + seg);
                }
                current = current[seg];
            } else if (current.is_array()) {
                size_t idx = std::stoul(seg);
                if (idx >= current.size())
                    throw InvalidArgumentException("Array index out of bounds");
                current = current[idx];
            } else {
                throw InvalidArgumentException("Cannot traverse non-container");
            }
        }
    } catch (const std::exception&) {
        // If we have a default value (2nd argument), return it
        if (args.size() == 2) {
            return evaluate(args[1], ctx);
        }
        throw; // Re-throw the original exception
    }
    return current;
}

nlohmann::json let_binding(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("let requires exactly 2 arguments");
    if (!args[0].is_array())
        throw InvalidArgumentException("let bindings must be array");

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
    if (args.size() < 1)
        throw InvalidArgumentException("obj requires at least 1 argument");

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

// Object operations
nlohmann::json keys_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("keys requires exactly 1 argument");
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("keys requires object argument");
    }

    nlohmann::json result = nlohmann::json::array();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        result.push_back(it.key());
    }

    return nlohmann::json::object({ { "array", result } });
}

nlohmann::json values_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("values requires exactly 1 argument");
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("values requires object argument");
    }

    nlohmann::json result = nlohmann::json::array();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        result.push_back(it.value());
    }

    return nlohmann::json::object({ { "array", result } });
}

nlohmann::json objFromPairs_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) {
        throw InvalidArgumentException("objFromPairs requires exactly 1 argument");
    }

    auto pairs_container = evaluate(args[0], ctx);
    if (!pairs_container.is_object() || pairs_container.find("array") == pairs_container.end()) {
        throw InvalidArgumentException("objFromPairs requires array container argument");
    }

    auto pairs = pairs_container["array"];
    if (!pairs.is_array()) {
        throw InvalidArgumentException("objFromPairs requires array of pairs");
    }

    nlohmann::json result = nlohmann::json::object();
    for (const auto& pair : pairs) {
        if (!pair.is_array() || pair.size() != 2) {
            throw InvalidArgumentException("objFromPairs requires array of [key, value] pairs");
        }

        auto key = pair[0];
        auto value = pair[1];

        if (!key.is_string()) {
            throw InvalidArgumentException("objFromPairs requires string keys");
        }

        result[key.get<std::string>()] = value;
    }

    return result;
}

nlohmann::json pick_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) {
        throw InvalidArgumentException("pick requires exactly 2 arguments: [object, keys_array]");
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("pick requires object as first argument");
    }

    auto keys_container = evaluate(args[1], ctx);
    if (!keys_container.is_object() || keys_container.find("array") == keys_container.end()) {
        throw InvalidArgumentException("pick requires array container as second argument");
    }

    auto keys = keys_container["array"];
    if (!keys.is_array()) {
        throw InvalidArgumentException("pick requires array of keys");
    }

    nlohmann::json result = nlohmann::json::object();
    for (const auto& key : keys) {
        if (!key.is_string()) {
            throw InvalidArgumentException("pick requires string keys");
        }

        std::string key_str = key.get<std::string>();
        if (obj.find(key_str) != obj.end()) {
            result[key_str] = obj[key_str];
        }
    }

    return result;
}

nlohmann::json omit_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) {
        throw InvalidArgumentException("omit requires exactly 2 arguments: [object, keys_array]");
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("omit requires object as first argument");
    }

    auto keys_container = evaluate(args[1], ctx);
    if (!keys_container.is_object() || keys_container.find("array") == keys_container.end()) {
        throw InvalidArgumentException("omit requires array container as second argument");
    }

    auto keys = keys_container["array"];
    if (!keys.is_array()) {
        throw InvalidArgumentException("omit requires array of keys");
    }

    // Create a set of keys to omit for efficient lookup
    std::set<std::string> keys_to_omit;
    for (const auto& key : keys) {
        if (!key.is_string()) {
            throw InvalidArgumentException("omit requires string keys");
        }
        keys_to_omit.insert(key.get<std::string>());
    }

    nlohmann::json result = nlohmann::json::object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (keys_to_omit.find(it.key()) == keys_to_omit.end()) {
            result[it.key()] = it.value();
        }
    }

    return result;
}

}