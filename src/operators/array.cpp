#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <computo/memory_pool.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace computo {
namespace operator_modules {

// Helper function for evaluating lambda expressions with multiple parameters
static nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, const std::vector<nlohmann::json>& param_values, ExecutionContext& ctx) {
    nlohmann::json actual_lambda = lambda_expr;
    
    // Handle variable references to lambdas
    if (lambda_expr.is_array() && lambda_expr.size() == 2 && 
        lambda_expr[0].is_string() && lambda_expr[0].get<std::string>() == "$") {
        if (!lambda_expr[1].is_string()) {
            throw InvalidArgumentException("Lambda variable reference requires string path");
        }
        std::string var_path = lambda_expr[1].get<std::string>();
        try {
            actual_lambda = ctx.get_variable(var_path);
        } catch (const InvalidArgumentException& e) {
            throw InvalidArgumentException("Lambda variable resolution failed: " + std::string(e.what()));
        }
    }
    
    // Lambda format: ["lambda", ["param1", "param2", ...], <body_expr>]
    if (!actual_lambda.is_array() || actual_lambda.size() != 3) {
        throw InvalidArgumentException("lambda must be [\"lambda\", [\"param1\", \"param2\", ...], body_expr]");
    }
    
    if (!actual_lambda[0].is_string() || actual_lambda[0].get<std::string>() != "lambda") {
        throw InvalidArgumentException("lambda expression must start with \"lambda\"");
    }
    
    if (!actual_lambda[1].is_array() || actual_lambda[1].empty()) {
        throw InvalidArgumentException("lambda must have at least one parameter");
    }
    
    if (actual_lambda[1].size() != param_values.size()) {
        throw InvalidArgumentException("Lambda parameter count mismatch: expected " + std::to_string(actual_lambda[1].size()) + ", got " + std::to_string(param_values.size()));
    }
    
    // Create new context with all lambda variables
    std::map<std::string, nlohmann::json> lambda_vars;
    for (size_t i = 0; i < actual_lambda[1].size(); ++i) {
        if (!actual_lambda[1][i].is_string()) {
            throw InvalidArgumentException("Lambda parameter must be a string");
        }
        std::string var_name = actual_lambda[1][i].get<std::string>();
        lambda_vars[var_name] = param_values[i];
    }
    ExecutionContext lambda_ctx = ctx.with_variables(lambda_vars);
    
    // Evaluate the lambda body
    return evaluate(actual_lambda[2], lambda_ctx);
}

// Convenience function for single-parameter lambdas
static nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, const nlohmann::json& item_value, ExecutionContext& ctx) {
    return evaluate_lambda(lambda_expr, std::vector<nlohmann::json>{item_value}, ctx);
}

void init_array_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Map operator for array iteration
    ops["map"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("map operator requires exactly 2 arguments", ctx.get_path_string());
        }
        
        ExecutionContext array_ctx = ctx.with_path("array");
        auto array = evaluate(args[0], array_ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("map operator requires an array as first argument", ctx.get_path_string());
        }
        
        // Second argument should be a lambda expression (not evaluated)
        const auto& lambda_expr = args[1];
        
        nlohmann::json result = nlohmann::json::array();
        
        // Apply lambda to each element
        for (size_t i = 0; i < array.size(); ++i) {
            ExecutionContext item_ctx = ctx.with_path("lambda[" + std::to_string(i) + "]");
            nlohmann::json transformed = evaluate_lambda(lambda_expr, array[i], item_ctx);
            result.push_back(transformed);
        }
        
        return result;
    };
    
    // Filter operator for array filtering
    ops["filter"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("filter operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("filter operator requires an array as first argument");
        }
        
        // Second argument should be a lambda expression (not evaluated)
        const auto& lambda_expr = args[1];
        
        nlohmann::json result = nlohmann::json::array();
        
        // Apply lambda to each element and include if result is truthy
        for (const auto& item : array) {
            nlohmann::json condition_result = evaluate_lambda(lambda_expr, item, ctx);
            
            if (is_truthy(condition_result)) {
                result.push_back(item);
            }
        }
        
        return result;
    };
    
    // Reduce operator for array aggregation
    ops["reduce"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 3) {
            throw InvalidArgumentException("reduce operator requires exactly 3 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("reduce operator requires an array as first argument");
        }
        
        // Second argument should be a lambda expression (not evaluated)
        const auto& lambda_expr = args[1];
        
        // Third argument is the initial value (evaluated)
        nlohmann::json accumulator = evaluate(args[2], ctx);
        
        // Apply lambda to each element with accumulator
        // Lambda format: ["lambda", ["acc", "item"], body_expr]
        for (const auto& item : array) {
            accumulator = evaluate_lambda(lambda_expr, std::vector<nlohmann::json>{accumulator, item}, ctx);
        }
        
        return accumulator;
    };
    
    // Find operator - returns first element matching condition
    ops["find"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("find operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("find operator requires an array as first argument");
        }
        
        const auto& lambda_expr = args[1];
        
        // Find first element that matches condition
        for (const auto& item : array) {
            nlohmann::json condition_result = evaluate_lambda(lambda_expr, item, ctx);
            
            if (is_truthy(condition_result)) {
                return item;
            }
        }
        
        // Return null if not found
        return nlohmann::json(nullptr);
    };
    
    // Some operator - returns true if any element matches condition
    ops["some"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("some operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("some operator requires an array as first argument");
        }
        
        const auto& lambda_expr = args[1];
        
        // Return true if any element matches condition
        for (const auto& item : array) {
            nlohmann::json condition_result = evaluate_lambda(lambda_expr, item, ctx);
            
            if (is_truthy(condition_result)) {
                return true;
            }
        }
        
        return false;
    };
    
    // Every operator - returns true if all elements match condition
    ops["every"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("every operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("every operator requires an array as first argument");
        }
        
        const auto& lambda_expr = args[1];
        
        // Return true if all elements match condition
        for (const auto& item : array) {
            nlohmann::json condition_result = evaluate_lambda(lambda_expr, item, ctx);
            
            if (!is_truthy(condition_result)) {
                return false;
            }
        }
        
        return true;
    };
    
    // flatMap operator - maps and flattens result
    ops["flatMap"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("flatMap operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("flatMap operator requires an array as first argument");
        }
        
        const auto& lambda_expr = args[1];
        
        nlohmann::json result = nlohmann::json::array();
        
        // Apply lambda to each element and flatten result
        for (const auto& item : array) {
            nlohmann::json transformed = evaluate_lambda(lambda_expr, item, ctx);
            
            if (transformed.is_array()) {
                // Flatten array result
                for (const auto& nested_item : transformed) {
                    result.push_back(nested_item);
                }
            } else {
                // Single item
                result.push_back(transformed);
            }
        }
        
        return result;
    };
    
    // Count operator - returns array length
    ops["count"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("count expects exactly 1 argument");
        }
        
        nlohmann::json array_val = evaluate(args[0], ctx);
        if (!array_val.is_array()) {
            throw InvalidArgumentException("count expects an array");
        }
        
        return array_val.size();
    };
    
    // zipWith operator - combines two arrays element-wise using a lambda
    ops["zipWith"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 3) {
            throw InvalidArgumentException("zipWith operator requires exactly 3 arguments: array1, array2, lambda");
        }
        
        auto array1 = evaluate(args[0], ctx);
        auto array2 = evaluate(args[1], ctx);
        const auto& lambda_expr = args[2];
        
        if (!array1.is_array()) {
            throw InvalidArgumentException("zipWith operator requires an array as first argument");
        }
        
        if (!array2.is_array()) {
            throw InvalidArgumentException("zipWith operator requires an array as second argument");
        }
        
        nlohmann::json result = nlohmann::json::array();
        size_t min_size = std::min(array1.size(), array2.size());
        
        // Apply lambda to each pair of elements
        for (size_t i = 0; i < min_size; ++i) {
            nlohmann::json combined = evaluate_lambda(lambda_expr, std::vector<nlohmann::json>{array1[i], array2[i]}, ctx);
            result.push_back(combined);
        }
        
        return result;
    };
    
    // mapWithIndex operator - maps over array with element and index
    ops["mapWithIndex"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("mapWithIndex operator requires exactly 2 arguments: array, lambda");
        }
        
        auto array = evaluate(args[0], ctx);
        const auto& lambda_expr = args[1];
        
        if (!array.is_array()) {
            throw InvalidArgumentException("mapWithIndex operator requires an array as first argument");
        }
        
        nlohmann::json result = nlohmann::json::array();
        
        // Apply lambda to each element with its index
        for (size_t i = 0; i < array.size(); ++i) {
            nlohmann::json transformed = evaluate_lambda(lambda_expr, std::vector<nlohmann::json>{array[i], static_cast<int64_t>(i)}, ctx);
            result.push_back(transformed);
        }
        
        return result;
    };
    
    // Enumerate operator - converts array to [index, value] pairs
    ops["enumerate"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("enumerate operator requires exactly 1 argument: array");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("enumerate operator requires an array");
        }
        
        nlohmann::json result = nlohmann::json::array();
        
        // Create [index, value] pairs
        for (size_t i = 0; i < array.size(); ++i) {
            nlohmann::json pair = nlohmann::json::array();
            pair.push_back(static_cast<int64_t>(i));
            pair.push_back(array[i]);
            result.push_back(pair);
        }
        
        return result;
    };
    
    // Zip operator - combines two arrays into pairs
    ops["zip"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("zip operator requires exactly 2 arguments: array1, array2");
        }
        
        auto array1 = evaluate(args[0], ctx);
        auto array2 = evaluate(args[1], ctx);
        
        if (!array1.is_array()) {
            throw InvalidArgumentException("zip operator requires an array as first argument");
        }
        
        if (!array2.is_array()) {
            throw InvalidArgumentException("zip operator requires an array as second argument");
        }
        
        nlohmann::json result = nlohmann::json::array();
        size_t min_size = std::min(array1.size(), array2.size());
        
        // Create [element1, element2] pairs
        for (size_t i = 0; i < min_size; ++i) {
            nlohmann::json pair = nlohmann::json::array();
            pair.push_back(array1[i]);
            pair.push_back(array2[i]);
            result.push_back(pair);
        }
        
        return result;
    };
}

} // namespace operator_modules
} // namespace computo 