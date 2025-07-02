#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace computo {
namespace operator_modules {

// Helper function for evaluating lambda expressions 
static nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, const nlohmann::json& item_value, ExecutionContext& ctx) {
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
    
    // Lambda format: ["lambda", ["param"], <body_expr>]
    if (!actual_lambda.is_array() || actual_lambda.size() != 3) {
        throw InvalidArgumentException("lambda must be [\"lambda\", [\"param\"], body_expr]");
    }
    
    if (!actual_lambda[0].is_string() || actual_lambda[0].get<std::string>() != "lambda") {
        throw InvalidArgumentException("lambda expression must start with \"lambda\"");
    }
    
    if (!actual_lambda[1].is_array() || actual_lambda[1].size() != 1) {
        throw InvalidArgumentException("lambda must have exactly one parameter");
    }
    
    if (!actual_lambda[1][0].is_string()) {
        throw InvalidArgumentException("Lambda parameter must be a string");
    }
    
    // Create new context with lambda variable
    std::string var_name = actual_lambda[1][0].get<std::string>();
    std::map<std::string, nlohmann::json> lambda_vars;
    lambda_vars[var_name] = item_value;
    ExecutionContext lambda_ctx = ctx.with_variables(lambda_vars);
    
    // Evaluate the lambda body
    return evaluate(actual_lambda[2], lambda_ctx);
}

void init_list_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Car operator - returns first element of array
    ops["car"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("car expects exactly 1 argument");
        }
        
        nlohmann::json array_val = evaluate(args[0], ctx);
        if (!array_val.is_array()) {
            throw InvalidArgumentException("car expects an array");
        }
        
        if (array_val.empty()) {
            throw InvalidArgumentException("car cannot be applied to empty array");
        }
        
        return array_val[0];
    };
    
    // Cdr operator - returns all but first element of array
    ops["cdr"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("cdr expects exactly 1 argument");
        }
        
        nlohmann::json array_val = evaluate(args[0], ctx);
        if (!array_val.is_array()) {
            throw InvalidArgumentException("cdr expects an array");
        }
        
        if (array_val.empty()) {
            return nlohmann::json::array();  // Return empty array for empty input
        }
        
        nlohmann::json result = nlohmann::json::array();
        for (size_t i = 1; i < array_val.size(); ++i) {
            result.push_back(array_val[i]);
        }
        return result;
    };
    
    // Cons operator - prepends item to array
    ops["cons"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("cons operator requires exactly 2 arguments");
        }
        
        auto item = evaluate(args[0], ctx);
        auto array = evaluate(args[1], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("cons operator requires an array as second argument");
        }
        
        nlohmann::json result = nlohmann::json::array();
        result.push_back(item);
        for (const auto& element : array) {
            result.push_back(element);
        }
        
        return result;
    };
    
    // Append operator - concatenates arrays
    ops["append"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("append operator requires at least 1 argument");
        }
        
        nlohmann::json result = nlohmann::json::array();
        
        for (const auto& arg : args) {
            auto array = evaluate(arg, ctx);
            
            if (!array.is_array()) {
                throw InvalidArgumentException("append operator requires all arguments to be arrays");
            }
            
            for (const auto& element : array) {
                result.push_back(element);
            }
        }
        
        return result;
    };
    
    // Chunk operator - splits array into chunks of specified size
    ops["chunk"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("chunk operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        auto size_val = evaluate(args[1], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("chunk operator requires an array as first argument");
        }
        
        if (!size_val.is_number_integer() || size_val.get<int64_t>() <= 0) {
            throw InvalidArgumentException("chunk operator requires a positive integer as second argument");
        }
        
        int64_t chunk_size = size_val.get<int64_t>();
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < array.size(); i += chunk_size) {
            nlohmann::json chunk = nlohmann::json::array();
            for (size_t j = i; j < std::min(i + chunk_size, array.size()); ++j) {
                chunk.push_back(array[j]);
            }
            result.push_back(chunk);
        }
        
        return result;
    };
    
    // Partition operator - splits array into two arrays based on predicate
    ops["partition"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("partition operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("partition operator requires an array as first argument");
        }
        
        const auto& lambda_expr = args[1];
        
        nlohmann::json truthy_items = nlohmann::json::array();
        nlohmann::json falsy_items = nlohmann::json::array();
        
        for (const auto& item : array) {
            nlohmann::json condition_result = evaluate_lambda(lambda_expr, item, ctx);
            
            if (is_truthy(condition_result)) {
                truthy_items.push_back(item);
            } else {
                falsy_items.push_back(item);
            }
        }
        
        // Return array containing [truthy_items, falsy_items]
        nlohmann::json result = nlohmann::json::array();
        result.push_back(truthy_items);
        result.push_back(falsy_items);
        
        return result;
    };
}

} // namespace operator_modules
} // namespace computo 