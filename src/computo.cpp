#include <computo/computo.hpp>
#include <permuto/permuto.hpp>
#include <iostream>

namespace computo {

// Operator registry
static std::map<std::string, OperatorFunc> operators;

// Helper function for evaluating lambda expressions
static nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, const nlohmann::json& item_value, ExecutionContext& ctx) {
    // Lambda format: ["lambda", ["var_name"], <body_expr>]
    if (!lambda_expr.is_array() || lambda_expr.size() != 3) {
        throw InvalidArgumentException("lambda must be [\"lambda\", [\"var_name\"], body_expr]");
    }
    
    if (!lambda_expr[0].is_string() || lambda_expr[0].get<std::string>() != "lambda") {
        throw InvalidArgumentException("lambda expression must start with \"lambda\"");
    }
    
    if (!lambda_expr[1].is_array() || lambda_expr[1].size() != 1) {
        throw InvalidArgumentException("lambda must have exactly one parameter: [\"var_name\"]");
    }
    
    if (!lambda_expr[1][0].is_string()) {
        throw InvalidArgumentException("lambda parameter must be a string");
    }
    
    std::string var_name = lambda_expr[1][0].get<std::string>();
    
    // Create new context with the lambda variable
    std::map<std::string, nlohmann::json> lambda_vars;
    lambda_vars[var_name] = item_value;
    ExecutionContext lambda_ctx = ctx.with_variables(lambda_vars);
    
    // Evaluate the lambda body
    return evaluate(lambda_expr[2], lambda_ctx);
}

// Initialize operators
static void initialize_operators() {
    static bool initialized = false;
    if (initialized) return;
    
    // Addition operator
    operators["+"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("+ operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("+ operator requires numeric arguments");
        }
        
        if (left.is_number_integer() && right.is_number_integer()) {
            return left.get<int64_t>() + right.get<int64_t>();
        } else {
            return left.get<double>() + right.get<double>();
        }
    };
    
    // Let operator for variable binding
    operators["let"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("let operator requires exactly 2 arguments");
        }
        
        // First argument should be an array of [var_name, expr] pairs
        if (!args[0].is_array()) {
            throw InvalidArgumentException("let operator requires bindings as an array");
        }
        
        std::map<std::string, nlohmann::json> new_vars;
        
        // Process each binding
        for (const auto& binding : args[0]) {
            if (!binding.is_array() || binding.size() != 2) {
                throw InvalidArgumentException("let binding must be [var_name, expr] pair");
            }
            
            if (!binding[0].is_string()) {
                throw InvalidArgumentException("let variable name must be a string");
            }
            
            std::string var_name = binding[0].get<std::string>();
            nlohmann::json var_value = evaluate(binding[1], ctx);
            new_vars[var_name] = var_value;
        }
        
        // Create new context with variables and evaluate body
        ExecutionContext new_ctx = ctx.with_variables(new_vars);
        return evaluate(args[1], new_ctx);
    };
    
    // Dollar operator for variable lookup
    operators["$"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("$ operator requires exactly 1 argument");
        }
        
        if (!args[0].is_string()) {
            throw InvalidArgumentException("$ operator requires a string path");
        }
        
        std::string path = args[0].get<std::string>();
        return ctx.get_variable(path);
    };
    
    // Get operator for JSON Pointer access
    operators["get"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("get operator requires exactly 2 arguments");
        }
        
        auto object = evaluate(args[0], ctx);
        
        if (!args[1].is_string()) {
            throw InvalidArgumentException("get operator requires a string JSON pointer");
        }
        
        std::string pointer = args[1].get<std::string>();
        
        try {
            return object.at(nlohmann::json::json_pointer(pointer));
        } catch (const nlohmann::json::exception& e) {
            throw InvalidArgumentException("JSON pointer access failed: " + std::string(e.what()));
        }
    };
    
    // If operator for conditional logic
    operators["if"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 3) {
            throw InvalidArgumentException("if operator requires exactly 3 arguments");
        }
        
        auto condition = evaluate(args[0], ctx);
        
        // Determine truthiness
        bool is_true = false;
        if (condition.is_boolean()) {
            is_true = condition.get<bool>();
        } else if (condition.is_number()) {
            // Numbers: 0 is false, everything else is true
            if (condition.is_number_integer()) {
                is_true = condition.get<int64_t>() != 0;
            } else {
                is_true = condition.get<double>() != 0.0;
            }
        } else if (condition.is_string()) {
            // Strings: empty string is false, non-empty is true
            is_true = !condition.get<std::string>().empty();
        } else if (condition.is_null()) {
            // null is false
            is_true = false;
        } else if (condition.is_array()) {
            // Arrays: empty array is false, non-empty is true
            is_true = !condition.empty();
        } else if (condition.is_object()) {
            // Objects: empty object is false, non-empty is true
            is_true = !condition.empty();
        }
        
        // Evaluate and return appropriate branch
        if (is_true) {
            return evaluate(args[1], ctx); // then branch
        } else {
            return evaluate(args[2], ctx); // else branch
        }
    };
    
    // Obj operator for object construction
    operators["obj"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        nlohmann::json result = nlohmann::json::object();
        
        // Each argument should be a [key, value] pair
        for (const auto& pair_expr : args) {
            if (!pair_expr.is_array() || pair_expr.size() != 2) {
                throw InvalidArgumentException("obj operator requires [key, value] pairs");
            }
            
            // First element should be a string key (literal, not evaluated)
            if (!pair_expr[0].is_string()) {
                throw InvalidArgumentException("obj operator requires string keys");
            }
            
            std::string key = pair_expr[0].get<std::string>();
            nlohmann::json value = evaluate(pair_expr[1], ctx);
            result[key] = value;
        }
        
        return result;
    };
    
    // Arr operator for array construction
    operators["arr"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        nlohmann::json result = nlohmann::json::array();
        
        // Evaluate each argument and add to result array
        for (const auto& element_expr : args) {
            nlohmann::json element = evaluate(element_expr, ctx);
            result.push_back(element);
        }
        
        return result;
    };
    
    // Permuto.apply operator for template processing
    operators["permuto.apply"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("permuto.apply operator requires exactly 2 arguments");
        }
        
        auto template_json = evaluate(args[0], ctx);
        auto context_json = evaluate(args[1], ctx);
        
        try {
            return permuto::apply(template_json, context_json, ctx.permuto_options);
        } catch (const permuto::PermutoException& e) {
            throw InvalidArgumentException("Permuto error: " + std::string(e.what()));
        }
    };
    
    // Map operator for array iteration
    operators["map"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("map operator requires exactly 2 arguments");
        }
        
        auto array = evaluate(args[0], ctx);
        
        if (!array.is_array()) {
            throw InvalidArgumentException("map operator requires an array as first argument");
        }
        
        // Second argument should be a lambda expression (not evaluated)
        const auto& lambda_expr = args[1];
        
        nlohmann::json result = nlohmann::json::array();
        
        // Apply lambda to each element
        for (const auto& item : array) {
            nlohmann::json transformed = evaluate_lambda(lambda_expr, item, ctx);
            result.push_back(transformed);
        }
        
        return result;
    };
    
    // Filter operator for array filtering
    operators["filter"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
            
            // Use the same truthiness logic as the if operator
            bool is_true = false;
            if (condition_result.is_boolean()) {
                is_true = condition_result.get<bool>();
            } else if (condition_result.is_number()) {
                if (condition_result.is_number_integer()) {
                    is_true = condition_result.get<int64_t>() != 0;
                } else {
                    is_true = condition_result.get<double>() != 0.0;
                }
            } else if (condition_result.is_string()) {
                is_true = !condition_result.get<std::string>().empty();
            } else if (condition_result.is_null()) {
                is_true = false;
            } else if (condition_result.is_array()) {
                is_true = !condition_result.empty();
            } else if (condition_result.is_object()) {
                is_true = !condition_result.empty();
            }
            
            if (is_true) {
                result.push_back(item);
            }
        }
        
        return result;
    };
    
    initialized = true;
}

nlohmann::json evaluate(const nlohmann::json& expr, ExecutionContext& ctx) {
    initialize_operators();
    
    // Base case: if expr is not an array or is empty, it's a literal
    if (!expr.is_array() || expr.empty()) {
        return expr;
    }
    
    // Check if first element is a string (potential operator)
    if (!expr[0].is_string()) {
        // Not an operator call, treat as literal array
        return expr;
    }
    
    std::string op = expr[0].get<std::string>();
    
    // Special case: $input operator
    if (op == "$input") {
        return ctx.input;
    }
    
    // Check if this is actually a valid operator
    auto it = operators.find(op);
    if (it == operators.end()) {
        // Special case: if this looks like a literal array of similar types, 
        // treat it as literal rather than throwing an exception
        if (expr.size() > 1) {
            bool all_strings = true;
            for (size_t i = 1; i < expr.size(); ++i) {
                if (!expr[i].is_string()) {
                    all_strings = false;
                    break;
                }
            }
            // If all elements are strings, treat as literal array
            if (all_strings) {
                return expr;
            }
        }
        // Otherwise, this looks like an operator call with unknown operator
        throw InvalidOperatorException(op);
    }
    
    
    // Extract arguments (everything except the first element)
    nlohmann::json args = nlohmann::json::array();
    for (size_t i = 1; i < expr.size(); ++i) {
        args.push_back(expr[i]);
    }
    
    // Call the operator function
    return it->second(args, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate(script, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& permuto_options) {
    ExecutionContext ctx(input, permuto_options);
    return evaluate(script, ctx);
}

} // namespace computo