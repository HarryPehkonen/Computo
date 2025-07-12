#include "declarations.hpp"
#include "shared.hpp"

namespace computo::operators {
using computo::evaluate;

nlohmann::json map_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("map requires exactly 2 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    bool is_array_object = false;
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
        is_array_object = true;
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("map first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& item : array_val) {
        auto lambda_result = evaluate_lambda(lambda_expr, item, ctx);
        result.push_back(lambda_result);
    }

    if (is_array_object) {
        return nlohmann::json::object({ { "array", result } });
    }
    return result;
}

nlohmann::json filter_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("filter requires exactly 2 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    bool is_array_object = false;
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
        is_array_object = true;
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("filter first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& item : array_val) {
        if (is_truthy(evaluate_lambda(lambda_expr, item, ctx))) {
            result.push_back(item);
        }
    }

    if (is_array_object) {
        return nlohmann::json::object({ { "array", result } });
    }
    return result;
}

nlohmann::json reduce_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 3)
        throw InvalidArgumentException("reduce requires exactly 3 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("reduce first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    nlohmann::json accumulator = evaluate(args[2], ctx);
    for (const auto& item : array_val) {
        nlohmann::json args_for_lambda = nlohmann::json::array({ accumulator, item });
        accumulator = evaluate_lambda(lambda_expr, args_for_lambda, ctx);
    }
    return accumulator;
}

nlohmann::json count_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1)
        throw InvalidArgumentException("count requires exactly 1 argument");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("count argument must be array");
    return static_cast<int>(array_val.size());
}

nlohmann::json find_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("find requires exactly 2 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("find first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    for (const auto& item : array_val) {
        if (is_truthy(evaluate_lambda(lambda_expr, item, ctx))) {
            return item;
        }
    }
    return nlohmann::json(nullptr);
}

nlohmann::json some_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("some requires exactly 2 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("some first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    for (const auto& item : array_val) {
        if (is_truthy(evaluate_lambda(lambda_expr, item, ctx))) {
            return true;
        }
    }
    return false;
}

nlohmann::json every_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("every requires exactly 2 arguments");
    auto array_val = evaluate(args[0], ctx);

    // Handle {"array": [...]} syntax
    if (array_val.is_object() && array_val.contains("array")) {
        array_val = array_val["array"];
    }

    if (!array_val.is_array())
        throw InvalidArgumentException("every first argument must be array");

    // Evaluate the lambda expression (supports both inline and variable references)
    auto lambda_expr = evaluate(args[1], ctx);

    for (const auto& item : array_val) {
        if (!is_truthy(evaluate_lambda(lambda_expr, item, ctx))) {
            return false;
        }
    }
    return true;
}

nlohmann::json zip_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2)
        throw InvalidArgumentException("zip requires exactly 2 arguments");

    auto array1 = evaluate(args[0], ctx);
    auto array2 = evaluate(args[1], ctx);

    // Handle {"array": [...]} syntax
    bool is_array_object = false;
    if (array1.is_object() && array1.contains("array")) {
        array1 = array1["array"];
        is_array_object = true;
    }
    if (array2.is_object() && array2.contains("array")) {
        array2 = array2["array"];
        is_array_object = true;
    }

    if (!array1.is_array() || !array2.is_array())
        throw InvalidArgumentException("zip requires two arrays");

    size_t min_size = std::min(array1.size(), array2.size());
    nlohmann::json result = nlohmann::json::array();

    for (size_t i = 0; i < min_size; ++i) {
        nlohmann::json pair = nlohmann::json::array({ array1[i], array2[i] });
        result.push_back(pair);
    }

    if (is_array_object) {
        return nlohmann::json::object({ { "array", result } });
    }
    return result;
}

}