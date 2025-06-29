#include <computo/computo.hpp>
#include <permuto/permuto.hpp>
#include <iostream>
#include <mutex>

namespace computo {

// Define static null input for ExecutionContext
const nlohmann::json ExecutionContext::null_input = nlohmann::json(nullptr);

// Operator registry
static std::map<std::string, OperatorFunc> operators;
static std::once_flag operators_init_flag;

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

// Initialize operators - thread-safe version
static void initialize_operators() {
    std::call_once(operators_init_flag, []() {
    
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
    
    // Note: let operator now handled as special form in evaluate() for TCO
    
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
    
    // Note: if operator now handled as special form in evaluate() for TCO
    
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
    
    // Reduce operator for array aggregation
    operators["reduce"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
        for (const auto& item : array) {
            // Create a lambda evaluation function that takes accumulator and item
            // Lambda format: ["lambda", ["acc", "item"], body_expr]
            if (!lambda_expr.is_array() || lambda_expr.size() != 3) {
                throw InvalidArgumentException("reduce lambda must be [\"lambda\", [\"acc\", \"item\"], body_expr]");
            }
            
            if (!lambda_expr[0].is_string() || lambda_expr[0].get<std::string>() != "lambda") {
                throw InvalidArgumentException("reduce lambda expression must start with \"lambda\"");
            }
            
            if (!lambda_expr[1].is_array() || lambda_expr[1].size() != 2) {
                throw InvalidArgumentException("reduce lambda must have exactly two parameters: [\"acc\", \"item\"]");
            }
            
            if (!lambda_expr[1][0].is_string() || !lambda_expr[1][1].is_string()) {
                throw InvalidArgumentException("reduce lambda parameters must be strings");
            }
            
            std::string acc_var = lambda_expr[1][0].get<std::string>();
            std::string item_var = lambda_expr[1][1].get<std::string>();
            
            // Create new context with both accumulator and item variables
            std::map<std::string, nlohmann::json> lambda_vars;
            lambda_vars[acc_var] = accumulator;
            lambda_vars[item_var] = item;
            ExecutionContext lambda_ctx = ctx.with_variables(lambda_vars);
            
            // Evaluate the lambda body and update accumulator
            accumulator = evaluate(lambda_expr[2], lambda_ctx);
        }
        
        return accumulator;
    };
    
    // Math operators
    operators["-"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("- operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("- operator requires numeric arguments");
        }
        
        if (left.is_number_integer() && right.is_number_integer()) {
            return left.get<int64_t>() - right.get<int64_t>();
        } else {
            return left.get<double>() - right.get<double>();
        }
    };
    
    operators["*"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("* operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("* operator requires numeric arguments");
        }
        
        if (left.is_number_integer() && right.is_number_integer()) {
            return left.get<int64_t>() * right.get<int64_t>();
        } else {
            return left.get<double>() * right.get<double>();
        }
    };
    
    operators["/"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("/ operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("/ operator requires numeric arguments");
        }
        
        double right_val = right.get<double>();
        if (right_val == 0.0) {
            throw InvalidArgumentException("Division by zero");
        }
        
        return left.get<double>() / right_val;
    };
    
    // Comparison operators
    operators[">"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("> operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("> operator requires numeric arguments");
        }
        
        return left.get<double>() > right.get<double>();
    };
    
    operators["<"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("< operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("< operator requires numeric arguments");
        }
        
        return left.get<double>() < right.get<double>();
    };
    
    operators[">="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException(">= operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException(">= operator requires numeric arguments");
        }
        
        return left.get<double>() >= right.get<double>();
    };
    
    operators["<="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("<= operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("<= operator requires numeric arguments");
        }
        
        return left.get<double>() <= right.get<double>();
    };
    
    operators["=="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("== operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        return left == right;
    };
    
    operators["!="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("!= operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        return left != right;
    };
    
    // Epsilon-based floating point equality
    operators["approx"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 3) {
            throw InvalidArgumentException("approx operator requires exactly 3 arguments: [left, right, epsilon]");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        auto epsilon = evaluate(args[2], ctx);
        
        if (!left.is_number() || !right.is_number() || !epsilon.is_number()) {
            throw InvalidArgumentException("approx operator requires numeric arguments");
        }
        
        double left_val = left.get<double>();
        double right_val = right.get<double>();
        double epsilon_val = epsilon.get<double>();
        
        if (epsilon_val < 0.0) {
            throw InvalidArgumentException("epsilon must be non-negative");
        }
        
        return std::abs(left_val - right_val) <= epsilon_val;
    };
    
    // Logical operators with short-circuit evaluation
    operators["&&"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("&& operator requires at least 1 argument");
        }
        
        // Short-circuit evaluation: return false on first false expression
        for (const auto& expr : args) {
            auto result = evaluate(expr, ctx);
            
            // Determine truthiness using same logic as if operator
            bool is_true = false;
            if (result.is_boolean()) {
                is_true = result.get<bool>();
            } else if (result.is_number()) {
                if (result.is_number_integer()) {
                    is_true = result.get<int64_t>() != 0;
                } else {
                    is_true = result.get<double>() != 0.0;
                }
            } else if (result.is_string()) {
                is_true = !result.get<std::string>().empty();
            } else if (result.is_null()) {
                is_true = false;
            } else if (result.is_array()) {
                is_true = !result.empty();
            } else if (result.is_object()) {
                is_true = !result.empty();
            }
            
            if (!is_true) {
                return false;
            }
        }
        
        return true;
    };
    
    operators["||"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("|| operator requires at least 1 argument");
        }
        
        // Short-circuit evaluation: return true on first true expression
        for (const auto& expr : args) {
            auto result = evaluate(expr, ctx);
            
            // Determine truthiness using same logic as if operator
            bool is_true = false;
            if (result.is_boolean()) {
                is_true = result.get<bool>();
            } else if (result.is_number()) {
                if (result.is_number_integer()) {
                    is_true = result.get<int64_t>() != 0;
                } else {
                    is_true = result.get<double>() != 0.0;
                }
            } else if (result.is_string()) {
                is_true = !result.get<std::string>().empty();
            } else if (result.is_array()) {
                is_true = !result.empty();
            } else if (result.is_object()) {
                is_true = !result.empty();
            }
            
            if (is_true) {
                return true;
            }
        }
        
        return false;
    };
    
    // Array utility operators
    operators["find"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
            
            // Use same truthiness logic as filter
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
                return item;
            }
        }
        
        // Return null if not found
        return nlohmann::json(nullptr);
    };
    
    operators["some"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
            
            // Use same truthiness logic as filter
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
                return true;
            }
        }
        
        return false;
    };
    
    operators["every"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
            
            // Use same truthiness logic as filter
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
            
            if (!is_true) {
                return false;
            }
        }
        
        return true;
    };
    
    operators["flatMap"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    operators["count"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("count expects exactly 1 argument");
        }
        
        nlohmann::json array_val = evaluate(args[0], ctx);
        if (!array_val.is_array()) {
            throw InvalidArgumentException("count expects an array");
        }
        
        return array_val.size();
    };
    
    // Diff operator for generating JSON patches
    operators["diff"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("diff operator requires exactly 2 arguments");
        }
        
        auto document_a = evaluate(args[0], ctx);
        auto document_b = evaluate(args[1], ctx);
        
        try {
            return nlohmann::json::diff(document_a, document_b);
        } catch (const nlohmann::json::exception& e) {
            throw InvalidArgumentException("Failed to generate JSON diff: " + std::string(e.what()));
        }
    };
    
    // Patch operator for applying JSON patches
    operators["patch"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("patch operator requires exactly 2 arguments");
        }
        
        auto document_to_patch = evaluate(args[0], ctx);
        auto patch_array = evaluate(args[1], ctx);
        
        if (!patch_array.is_array()) {
            throw InvalidArgumentException("patch operator requires an array as second argument");
        }
        
        try {
            return document_to_patch.patch(patch_array);
        } catch (const nlohmann::json::exception& e) {
            throw PatchFailedException(std::string(e.what()));
        }
    };
    
    // Car operator - returns first element of array
    operators["car"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    operators["cdr"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    });
}

nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    initialize_operators();
    
    // Trampoline loop for Tail Call Optimization
    while (true) {
        // Check for array object syntax: {"array": [...]}
        if (expr.is_object() && expr.contains("array")) {
            if (!expr["array"].is_array()) {
                throw InvalidArgumentException("array object must contain an actual array");
            }
            // Evaluate each element in the array
            nlohmann::json result = nlohmann::json::array();
            for (const auto& element_expr : expr["array"]) {
                nlohmann::json element = evaluate(element_expr, ctx);
                result.push_back(element);
            }
            return result;
        }
        
        // Base case: if expr is not an array or is empty, it's a literal
        if (!expr.is_array() || expr.empty()) {
            return expr;
        }
        
        // Arrays are now always operator calls - no ambiguity!
        // Check if first element is a string (operator name)
        if (!expr[0].is_string()) {
            throw InvalidArgumentException("Array must start with operator name (string). For literal arrays, use {\"array\": [...]} syntax.");
        }
        
        std::string op = expr[0].get<std::string>();
        
        // Special case: $input operator
        if (op == "$input") {
            return ctx.input();
        }
        
        // Special case: $inputs operator
        if (op == "$inputs") {
            nlohmann::json result = nlohmann::json::array();
            for (const auto& input_doc : ctx.inputs()) {
                result.push_back(input_doc);
            }
            return result;
        }
        
        // --- TCO Special Forms ---
        
        if (op == "if") {
            if (expr.size() != 4) {
                throw InvalidArgumentException("if operator requires exactly 3 arguments");
            }
            
            // Evaluate condition
            auto condition = evaluate(expr[1], ctx);
            
            // Determine truthiness (same logic as current implementation)
            bool is_true = false;
            if (condition.is_boolean()) {
                is_true = condition.get<bool>();
            } else if (condition.is_number()) {
                if (condition.is_number_integer()) {
                    is_true = condition.get<int64_t>() != 0;
                } else {
                    is_true = condition.get<double>() != 0.0;
                }
            } else if (condition.is_string()) {
                is_true = !condition.get<std::string>().empty();
            } else if (condition.is_null()) {
                is_true = false;
            } else if (condition.is_array()) {
                is_true = !condition.empty();
            } else if (condition.is_object()) {
                is_true = !condition.empty();
            }
            
            // TCO: Replace current expression with chosen branch and continue
            expr = is_true ? expr[2] : expr[3];
            continue;
        }
        
        if (op == "let") {
            if (expr.size() != 3) {
                throw InvalidArgumentException("let operator requires exactly 2 arguments");
            }
            
            // First argument should be an array of [var_name, expr] pairs
            if (!expr[1].is_array()) {
                throw InvalidArgumentException("let operator requires bindings as an array");
            }
            
            std::map<std::string, nlohmann::json> new_vars;
            
            // Process each binding
            for (const auto& binding : expr[1]) {
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
            
            // TCO: Update context and set next expression to the body
            ctx = ctx.with_variables(new_vars);
            expr = expr[2];
            continue;
        }
        
        // --- Normal Functions ---
        
        // Look up operator in registry
        auto it = operators.find(op);
        if (it == operators.end()) {
            throw InvalidOperatorException(op);
        }
        
        // Extract arguments (everything except the first element)
        nlohmann::json args = nlohmann::json::array();
        for (size_t i = 1; i < expr.size(); ++i) {
            args.push_back(expr[i]);
        }
        
        // Call the operator function and return (exits the loop)
        return it->second(args, ctx);
    }
}

nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate(script, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& permuto_options) {
    ExecutionContext ctx(input, permuto_options);
    return evaluate(script, ctx);
}

// New multiple inputs API
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) {
    ExecutionContext ctx(inputs);
    return evaluate(script, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& permuto_options) {
    ExecutionContext ctx(inputs, permuto_options);
    return evaluate(script, ctx);
}

} // namespace computo