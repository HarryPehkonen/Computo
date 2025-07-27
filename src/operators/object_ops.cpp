#include "operators/shared.hpp"
#include <set>

namespace computo::operators {

auto obj_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() % 2 != 0) {
        throw InvalidArgumentException(
            "'obj' requires an even number of arguments (key-value pairs)", ctx.get_path_string());
    }

    nlohmann::json result = nlohmann::json::object();

    // Process key-value pairs
    for (size_t i = 0; i < args.size(); i += 2) {
        auto key_val = evaluate(args[i], ctx);
        auto value_val = evaluate(args[i + 1], ctx);

        if (!key_val.is_string()) {
            throw InvalidArgumentException("'obj' requires string keys", ctx.get_path_string());
        }

        std::string key = key_val.get<std::string>();
        result[key] = value_val;
    }

    return EvaluationResult(result);
}

auto keys_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'keys' requires exactly 1 argument", ctx.get_path_string());
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("'keys' requires an object argument", ctx.get_path_string());
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [key, value] : obj.items()) {
        result.push_back(key);
    }

    return EvaluationResult(nlohmann::json{{ctx.array_key, result}});
}

auto values_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'values' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto obj = evaluate(args[0], ctx);
    if (!obj.is_object()) {
        throw InvalidArgumentException("'values' requires an object argument",
                                       ctx.get_path_string());
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [key, value] : obj.items()) {
        result.push_back(value);
    }

    return EvaluationResult(nlohmann::json{{ctx.array_key, result}});
}

auto objFromPairs_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 1) {
        throw InvalidArgumentException("'objFromPairs' requires exactly 1 argument",
                                       ctx.get_path_string());
    }

    auto pairs_input = evaluate(args[0], ctx);
    auto pairs
        = extract_array_data(pairs_input, "objFromPairs", ctx.get_path_string(), ctx.array_key);

    nlohmann::json result = nlohmann::json::object();

    for (const auto& pair : pairs) {
        if (!pair.is_array() || pair.size() != 2) {
            throw InvalidArgumentException("'objFromPairs' requires an array of [key, value] pairs",
                                           ctx.get_path_string());
        }

        if (!pair[0].is_string()) {
            throw InvalidArgumentException("'objFromPairs' requires string keys",
                                           ctx.get_path_string());
        }

        std::string key = pair[0].get<std::string>();
        result[key] = pair[1];
    }

    return EvaluationResult(result);
}

// NOLINTBEGIN(readability-function-size)
auto pick_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'pick' requires exactly 2 arguments (object, keys)",
                                       ctx.get_path_string());
    }

    auto obj = evaluate(args[0], ctx);
    auto keys_input = evaluate(args[1], ctx);

    if (!obj.is_object()) {
        throw InvalidArgumentException("'pick' requires an object as first argument",
                                       ctx.get_path_string());
    }

    auto keys_to_pick
        = extract_array_data(keys_input, "pick", ctx.get_path_string(), ctx.array_key);

    nlohmann::json result = nlohmann::json::object();

    for (const auto& key_val : keys_to_pick) {
        if (!key_val.is_string()) {
            throw InvalidArgumentException("'pick' requires string keys", ctx.get_path_string());
        }

        std::string key = key_val.get<std::string>();
        if (obj.contains(key)) {
            result[key] = obj[key];
        }
    }

    return EvaluationResult(result);
}
// NOLINTEND(readability-function-size)

// NOLINTBEGIN(readability-function-size)
auto omit_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.size() != 2) {
        throw InvalidArgumentException("'omit' requires exactly 2 arguments (object, keys)",
                                       ctx.get_path_string());
    }

    auto obj = evaluate(args[0], ctx);
    auto keys_input = evaluate(args[1], ctx);

    if (!obj.is_object()) {
        throw InvalidArgumentException("'omit' requires an object as first argument",
                                       ctx.get_path_string());
    }

    auto keys_to_omit
        = extract_array_data(keys_input, "omit", ctx.get_path_string(), ctx.array_key);

    // Create set of keys to omit for O(1) lookup
    std::set<std::string> omit_keys;
    for (const auto& key_val : keys_to_omit) {
        if (!key_val.is_string()) {
            throw InvalidArgumentException("'omit' requires string keys", ctx.get_path_string());
        }
        omit_keys.insert(key_val.get<std::string>());
    }

    nlohmann::json result = nlohmann::json::object();

    for (const auto& [key, value] : obj.items()) {
        if (omit_keys.find(key) == omit_keys.end()) {
            result[key] = value;
        }
    }

    return EvaluationResult(result);
}
// NOLINTEND(readability-function-size)

auto merge_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    if (args.empty()) {
        throw InvalidArgumentException("'merge' requires at least 1 argument",
                                       ctx.get_path_string());
    }

    nlohmann::json result = nlohmann::json::object();

    for (const auto& arg_expr : args) {
        auto obj = evaluate(arg_expr, ctx);
        if (!obj.is_object()) {
            throw InvalidArgumentException("'merge' requires object arguments",
                                           ctx.get_path_string());
        }

        // Merge this object into the result
        for (const auto& [key, value] : obj.items()) {
            result[key] = value; // Later objects override earlier ones
        }
    }

    return EvaluationResult(result);
}

} // namespace computo::operators
