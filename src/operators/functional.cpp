#include "declarations.hpp"
#include "shared.hpp"

namespace computo::operators {
using computo::evaluate;

nlohmann::json car_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) throw InvalidArgumentException("car requires exactly 1 argument");
    auto array_val = evaluate(args[0], ctx);
    if (array_val.is_object() && array_val.contains("array")) array_val = array_val["array"];
    if (!array_val.is_array() || array_val.empty()) throw InvalidArgumentException("car requires non-empty array");
    return array_val[0];
}

nlohmann::json cdr_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 1) throw InvalidArgumentException("cdr requires exactly 1 argument");
    auto array_val = evaluate(args[0], ctx);
    if (array_val.is_object() && array_val.contains("array")) array_val = array_val["array"];
    if (!array_val.is_array() || array_val.empty()) throw InvalidArgumentException("cdr requires non-empty array");
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 1; i < array_val.size(); ++i) result.push_back(array_val[i]);
    return nlohmann::json::object({{"array", result}});
}

nlohmann::json cons_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) throw InvalidArgumentException("cons requires exactly 2 arguments");
    auto head = evaluate(args[0], ctx);
    auto array_val = evaluate(args[1], ctx);
    if (array_val.is_object() && array_val.contains("array")) array_val = array_val["array"];
    if (!array_val.is_array()) throw InvalidArgumentException("cons second argument must be array");
    nlohmann::json result = nlohmann::json::array();
    result.push_back(head);
    for (const auto& item : array_val) result.push_back(item);
    return nlohmann::json::object({{"array", result}});
}

nlohmann::json append_op(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) throw InvalidArgumentException("append requires exactly 2 arguments");
    auto arr1 = evaluate(args[0], ctx);
    auto arr2 = evaluate(args[1], ctx);
    if (arr1.is_object() && arr1.contains("array")) arr1 = arr1["array"];
    if (arr2.is_object() && arr2.contains("array")) arr2 = arr2["array"];
    if (!arr1.is_array() || !arr2.is_array()) throw InvalidArgumentException("append arguments must be arrays");
    nlohmann::json result = nlohmann::json::array();
    for (const auto& item : arr1) result.push_back(item);
    for (const auto& item : arr2) result.push_back(item);
    return nlohmann::json::object({{"array", result}});
}

// Lambda operator - creates a callable function
nlohmann::json lambda_operator(const nlohmann::json& args, [[maybe_unused]] ExecutionContext& ctx) {
    if (args.size() != 2) {
        throw InvalidArgumentException("lambda requires exactly 2 arguments: [params, body]");
    }
    
    // Validate parameters
    if (!args[0].is_array()) {
        throw InvalidArgumentException("lambda parameters must be an array");
    }
    
    // Validate that parameters array has exactly one string element
    if (args[0].size() != 1 || !args[0][0].is_string()) {
        throw InvalidArgumentException("lambda must have exactly one string parameter");
    }
    
    // Return the lambda as a callable object
    return nlohmann::json::array({"lambda", args[0], args[1]});
}

// Call operator - invokes a lambda function
nlohmann::json call_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) {
        throw InvalidArgumentException("call requires at least 2 arguments: [function, arg1, arg2, ...]");
    }
    
    // Evaluate the function
    auto func = evaluate(args[0], ctx);
    if (!func.is_array() || func.size() != 3 || func[0] != "lambda") {
        throw InvalidArgumentException("First argument must be a lambda function");
    }
    
    // Build arguments array from remaining arguments
    nlohmann::json func_args = nlohmann::json::array();
    for (size_t i = 1; i < args.size(); ++i) {
        func_args.push_back(evaluate(args[i], ctx));
    }
    
    // Call the lambda using existing evaluate_lambda function
    return evaluate_lambda(func, func_args, ctx);
}

} 