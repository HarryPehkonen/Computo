#include <computo/computo.hpp>
#include <computo/operators.hpp>
#include <permuto/permuto.hpp>
#include <iostream>
#include <mutex>

namespace computo {

// Define static null input for ExecutionContext
const nlohmann::json ExecutionContext::null_input = nlohmann::json(nullptr);

// Operator registry
static std::map<std::string, OperatorFunc> operators;
static std::once_flag operators_init_flag;

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

// Helper function for consistent truthiness evaluation across all operators
bool is_truthy(const nlohmann::json& value) {
    if (value.is_boolean()) {
        return value.get<bool>();
    } else if (value.is_number()) {
        if (value.is_number_integer()) {
            return value.get<int64_t>() != 0;
        } else {
            return value.get<double>() != 0.0;
        }
    } else if (value.is_string()) {
        return !value.get<std::string>().empty();
    } else if (value.is_null()) {
        return false;
    } else if (value.is_array()) {
        return !value.empty();
    } else if (value.is_object()) {
        return !value.empty();
    }
    return false; // Default case
}

// Initialize operators - thread-safe version with modular approach
static void initialize_operators() {
    std::call_once(operators_init_flag, []() {
        // Initialize operators from separate modules
        operator_modules::init_arithmetic_operators(operators);
        operator_modules::init_logical_operators(operators);  
        operator_modules::init_comparison_operators(operators);
        
        // TODO: Migrate remaining operators to separate modules
        // For now, keep the remaining operators inline until full migration
        
        // Variable lookup operator
        operators["$"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("+ operator requires at least 1 argument");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("+ operator requires numeric arguments");
            }
            values.push_back(value);
        }
        
        // Start with first value
        bool all_integers = values[0].is_number_integer();
        int64_t int_result = all_integers ? values[0].get<int64_t>() : 0;
        double double_result = values[0].get<double>();
        
        // Add remaining values
        for (size_t i = 1; i < values.size(); ++i) {
            if (all_integers && values[i].is_number_integer()) {
                int_result += values[i].get<int64_t>();
                double_result += values[i].get<double>();
            } else {
                all_integers = false;
                double_result += values[i].get<double>();
            }
        }
        
        return all_integers ? nlohmann::json(int_result) : nlohmann::json(double_result);
    };
    
    // Note: let operator now handled as special form in evaluate() for TCO
    
    // Dollar operator for variable lookup
    operators["$"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("$ operator requires exactly 1 argument", ctx.get_path_string());
        }
        
        if (!args[0].is_string()) {
            throw InvalidArgumentException("$ operator requires a string path", ctx.get_path_string());
        }
        
        std::string path = args[0].get<std::string>();
        try {
            return ctx.get_variable(path);
        } catch (const InvalidArgumentException& e) {
            throw InvalidArgumentException(std::string(e.what()), ctx.get_path_string());
        }
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
            
            // Evaluate the first element to get the key
            nlohmann::json key_val = evaluate(pair_expr[0], ctx);
            
            // Key must evaluate to a string
            if (!key_val.is_string()) {
                throw InvalidArgumentException("obj operator keys must evaluate to strings");
            }
            
            std::string key = key_val.get<std::string>();
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
            
            if (is_truthy(condition_result)) {
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
        // Lambda format: ["lambda", ["acc", "item"], body_expr]
        for (const auto& item : array) {
            accumulator = evaluate_lambda(lambda_expr, std::vector<nlohmann::json>{accumulator, item}, ctx);
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
        if (args.size() < 1) {
            throw InvalidArgumentException("* operator requires at least 1 argument");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("* operator requires numeric arguments");
            }
            values.push_back(value);
        }
        
        // Start with first value
        bool all_integers = values[0].is_number_integer();
        int64_t int_result = all_integers ? values[0].get<int64_t>() : 0;
        double double_result = values[0].get<double>();
        
        // Multiply remaining values
        for (size_t i = 1; i < values.size(); ++i) {
            if (all_integers && values[i].is_number_integer()) {
                int_result *= values[i].get<int64_t>();
                double_result *= values[i].get<double>();
            } else {
                all_integers = false;
                double_result *= values[i].get<double>();
            }
        }
        
        return all_integers ? nlohmann::json(int_result) : nlohmann::json(double_result);
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
    
    // Modulo operator
    operators["%"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("% operator requires exactly 2 arguments");
        }
        
        auto left = evaluate(args[0], ctx);
        auto right = evaluate(args[1], ctx);
        
        if (!left.is_number() || !right.is_number()) {
            throw InvalidArgumentException("% operator requires numeric arguments");
        }
        
        // For modulo, we need integers
        if (!left.is_number_integer() || !right.is_number_integer()) {
            throw InvalidArgumentException("% operator requires integer arguments");
        }
        
        int64_t right_val = right.get<int64_t>();
        if (right_val == 0) {
            throw InvalidArgumentException("Modulo by zero");
        }
        
        return left.get<int64_t>() % right_val;
    };
    
    // Comparison operators
    operators[">"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("> operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("> operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a > b > c means a > b && b > c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] > values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    operators["<"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("< operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("< operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a < b < c means a < b && b < c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] < values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    operators[">="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException(">= operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException(">= operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a >= b >= c means a >= b && b >= c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] >= values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    operators["<="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("<= operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<double> values;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (!value.is_number()) {
                throw InvalidArgumentException("<= operator requires numeric arguments");
            }
            values.push_back(value.get<double>());
        }
        
        // Check chained comparison: a <= b <= c means a <= b && b <= c
        for (size_t i = 0; i < values.size() - 1; ++i) {
            if (!(values[i] <= values[i + 1])) {
                return false;
            }
        }
        
        return true;
    };
    
    operators["=="] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 2) {
            throw InvalidArgumentException("== operator requires at least 2 arguments");
        }
        
        // Evaluate all arguments first
        std::vector<nlohmann::json> values;
        for (const auto& arg : args) {
            values.push_back(evaluate(arg, ctx));
        }
        
        // Check that all values are equal to the first
        const auto& first = values[0];
        for (size_t i = 1; i < values.size(); ++i) {
            if (first != values[i]) {
                return false;
            }
        }
        
        return true;
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
            
            if (!is_truthy(result)) {
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
            
            if (is_truthy(result)) {
                return true;
            }
        }
        
        return false;
    };
    
    // Logical not operator
    operators["not"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("not operator requires exactly 1 argument");
        }
        
        auto result = evaluate(args[0], ctx);
        
        return !is_truthy(result);
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
            
            if (is_truthy(condition_result)) {
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
            
            if (is_truthy(condition_result)) {
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
            
            if (!is_truthy(condition_result)) {
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
    
    // str_concat operator - concatenates values as strings with automatic type conversion
    operators["str_concat"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("str_concat operator requires at least 1 argument");
        }
        
        std::string result;
        for (const auto& arg : args) {
            auto value = evaluate(arg, ctx);
            if (value.is_string()) {
                result += value.get<std::string>();
            } else if (value.is_null()) {
                // null values contribute nothing to concatenation
                continue;
            } else {
                // Convert non-strings to their JSON representation without quotes for primitives
                if (value.is_number() || value.is_boolean()) {
                    result += value.dump();
                } else {
                    // For objects and arrays, use JSON dump
                    result += value.dump();
                }
            }
        }
        
        return result;
    };
    
    // zipWith operator - combines two arrays element-wise using a lambda
    operators["zipWith"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    operators["mapWithIndex"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // enumerate operator - converts array to [index, value] pairs
    operators["enumerate"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // zip operator - combines two arrays into pairs
    operators["zip"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    
    // Cons operator - prepends item to array
    operators["cons"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    operators["append"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    operators["chunk"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    operators["partition"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Lambda operator for creating lambda expressions
    operators["lambda"] = [](const nlohmann::json& args, ExecutionContext& /* unused */) -> nlohmann::json {
        if (args.size() != 2) {
            throw InvalidArgumentException("lambda operator requires exactly 2 arguments: parameters and body");
        }
        
        if (!args[0].is_array()) {
            throw InvalidArgumentException("lambda operator requires first argument to be parameter array");
        }
        
        // Validate parameter names are strings
        for (const auto& param : args[0]) {
            if (!param.is_string()) {
                throw InvalidArgumentException("lambda parameters must be strings");
            }
        }
        
        // Return the lambda as-is (it's a data structure)
        return nlohmann::json::array({"lambda", args[0], args[1]});
    };
    
    });
}

// Move-optimized evaluation function for large JSON objects
nlohmann::json evaluate_move(nlohmann::json&& expr, ExecutionContext ctx) {
    initialize_operators();
    
    // Trampoline loop for Tail Call Optimization
    while (true) {
        // Check for array object syntax: {"array": [...]}
        if (expr.is_object() && expr.contains("array")) {
            if (!expr["array"].is_array()) {
                throw InvalidArgumentException("array object must contain an actual array", ctx.get_path_string());
            }
            // Evaluate each element in the array - use move when possible
            nlohmann::json result = nlohmann::json::array();
            auto& array_ref = expr["array"];
            for (size_t i = 0; i < array_ref.size(); ++i) {
                ExecutionContext element_ctx = ctx.with_path("array[" + std::to_string(i) + "]");
                nlohmann::json element = evaluate_move(std::move(array_ref[i]), element_ctx);
                result.push_back(std::move(element));
            }
            return result;
        }
        
        // Base case: if expr is not an array, return it (move)
        if (!expr.is_array()) {
            return std::move(expr);
        }
        
        // Empty arrays are invalid as scripts
        if (expr.empty()) {
            throw InvalidArgumentException("Empty array is not a valid script. For literal arrays, use {\"array\": []} syntax.", ctx.get_path_string());
        }
        
        // Arrays are now always operator calls - no ambiguity!
        // Check if first element is a string (operator name)
        if (!expr[0].is_string()) {
            throw InvalidArgumentException("Array must start with operator name (string). For literal arrays, use {\"array\": [...]} syntax.", ctx.get_path_string());
        }
        
        std::string op = expr[0].get<std::string>();
        ExecutionContext op_ctx = ctx.with_path(op);
        
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
                throw InvalidArgumentException("if operator requires exactly 3 arguments", op_ctx.get_path_string());
            }
            
            // Evaluate condition
            ExecutionContext condition_ctx = op_ctx.with_path("condition");
            auto condition = evaluate_move(std::move(expr[1]), condition_ctx);
            
            // Determine truthiness using helper function
            bool condition_is_true = is_truthy(condition);
            
            // TCO: Replace current expression with chosen branch and continue
            ctx = op_ctx.with_path(condition_is_true ? "then" : "else");
            expr = std::move(condition_is_true ? expr[2] : expr[3]);
            continue;
        }
        
        if (op == "let") {
            if (expr.size() != 3) {
                throw InvalidArgumentException("let operator requires exactly 2 arguments", op_ctx.get_path_string());
            }
            
            // First argument should be an array of [var_name, expr] pairs
            if (!expr[1].is_array()) {
                throw InvalidArgumentException("let operator requires bindings as an array", op_ctx.get_path_string());
            }
            
            std::map<std::string, nlohmann::json> new_vars;
            
            // Process each binding sequentially, allowing later bindings to reference earlier ones
            auto& bindings = expr[1];
            for (size_t i = 0; i < bindings.size(); ++i) {
                auto& binding = bindings[i];
                ExecutionContext binding_ctx = op_ctx.with_path("bindings[" + std::to_string(i) + "]");
                
                if (!binding.is_array() || binding.size() != 2) {
                    throw InvalidArgumentException("let binding must be [var_name, expr] pair", binding_ctx.get_path_string());
                }
                
                if (!binding[0].is_string()) {
                    throw InvalidArgumentException("let variable name must be a string", binding_ctx.get_path_string());
                }
                
                std::string var_name = binding[0].get<std::string>();
                
                // Special case: don't evaluate lambda expressions, store them as-is
                nlohmann::json var_value;
                if (binding[1].is_array() && binding[1].size() == 3 &&
                    binding[1][0].is_string() && binding[1][0].get<std::string>() == "lambda") {
                    var_value = std::move(binding[1]);  // Move lambda as-is
                } else {
                    // FIXED: Include previously bound variables in the evaluation context
                    ExecutionContext value_ctx = binding_ctx.with_path("value").with_variables(new_vars);
                    var_value = evaluate_move(std::move(binding[1]), value_ctx);  // Move evaluate with sequential context
                }
                new_vars[var_name] = std::move(var_value);
            }
            
            // TCO: Update context and set next expression to the body
            ctx = op_ctx.with_path("body").with_variables(new_vars);
            expr = std::move(expr[2]);
            continue;
        }
        
        // --- Normal Functions ---
        
        // Look up operator in registry
        auto it = operators.find(op);
        if (it == operators.end()) {
            throw InvalidOperatorException(op);
        }
        
        // Extract arguments (everything except the first element) - use move
        nlohmann::json args = nlohmann::json::array();
        for (size_t i = 1; i < expr.size(); ++i) {
            args.push_back(std::move(expr[i]));
        }
        
        // Call the operator function and return (exits the loop)
        return it->second(args, op_ctx);
    }
}

nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    initialize_operators();
    
    // Trampoline loop for Tail Call Optimization
    while (true) {
        // Check for array object syntax: {"array": [...]}
        if (expr.is_object() && expr.contains("array")) {
            if (!expr["array"].is_array()) {
                throw InvalidArgumentException("array object must contain an actual array", ctx.get_path_string());
            }
            // Evaluate each element in the array
            nlohmann::json result = nlohmann::json::array();
            for (size_t i = 0; i < expr["array"].size(); ++i) {
                ExecutionContext element_ctx = ctx.with_path("array[" + std::to_string(i) + "]");
                nlohmann::json element = evaluate(expr["array"][i], element_ctx);
                result.push_back(element);
            }
            return result;
        }
        
        // Base case: if expr is not an array, it's a literal
        if (!expr.is_array()) {
            return expr;
        }
        
        // Empty arrays are invalid as scripts
        if (expr.empty()) {
            throw InvalidArgumentException("Empty array is not a valid script. For literal arrays, use {\"array\": []} syntax.", ctx.get_path_string());
        }
        
        // Arrays are now always operator calls - no ambiguity!
        // Check if first element is a string (operator name)
        if (!expr[0].is_string()) {
            throw InvalidArgumentException("Array must start with operator name (string). For literal arrays, use {\"array\": [...]} syntax.", ctx.get_path_string());
        }
        
        std::string op = expr[0].get<std::string>();
        ExecutionContext op_ctx = ctx.with_path(op);
        
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
                throw InvalidArgumentException("if operator requires exactly 3 arguments", op_ctx.get_path_string());
            }
            
            // Evaluate condition
            ExecutionContext condition_ctx = op_ctx.with_path("condition");
            auto condition = evaluate(expr[1], condition_ctx);
            
            // Determine truthiness using helper function
            bool condition_is_true = is_truthy(condition);
            
            // TCO: Replace current expression with chosen branch and continue
            ctx = op_ctx.with_path(condition_is_true ? "then" : "else");
            expr = condition_is_true ? expr[2] : expr[3];
            continue;
        }
        
        if (op == "let") {
            if (expr.size() != 3) {
                throw InvalidArgumentException("let operator requires exactly 2 arguments", op_ctx.get_path_string());
            }
            
            // First argument should be an array of [var_name, expr] pairs
            if (!expr[1].is_array()) {
                throw InvalidArgumentException("let operator requires bindings as an array", op_ctx.get_path_string());
            }
            
            std::map<std::string, nlohmann::json> new_vars;
            
            // Process each binding sequentially, allowing later bindings to reference earlier ones
            for (size_t i = 0; i < expr[1].size(); ++i) {
                const auto& binding = expr[1][i];
                ExecutionContext binding_ctx = op_ctx.with_path("bindings[" + std::to_string(i) + "]");
                
                if (!binding.is_array() || binding.size() != 2) {
                    throw InvalidArgumentException("let binding must be [var_name, expr] pair", binding_ctx.get_path_string());
                }
                
                if (!binding[0].is_string()) {
                    throw InvalidArgumentException("let variable name must be a string", binding_ctx.get_path_string());
                }
                
                std::string var_name = binding[0].get<std::string>();
                
                // Special case: don't evaluate lambda expressions, store them as-is
                nlohmann::json var_value;
                if (binding[1].is_array() && binding[1].size() == 3 &&
                    binding[1][0].is_string() && binding[1][0].get<std::string>() == "lambda") {
                    var_value = binding[1];  // Store lambda as-is
                } else {
                    // FIXED: Include previously bound variables in the evaluation context
                    ExecutionContext value_ctx = binding_ctx.with_path("value").with_variables(new_vars);
                    var_value = evaluate(binding[1], value_ctx);  // Evaluate with sequential context
                }
                new_vars[var_name] = var_value;
            }
            
            // TCO: Update context and set next expression to the body
            ctx = op_ctx.with_path("body").with_variables(new_vars);
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
        return it->second(args, op_ctx);
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

// Memory-optimized execute functions using move semantics
nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate_move(std::move(script), ctx);
}

nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input, const permuto::Options& permuto_options) {
    ExecutionContext ctx(input, permuto_options);
    return evaluate_move(std::move(script), ctx);
}

nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs) {
    ExecutionContext ctx(inputs);
    return evaluate_move(std::move(script), ctx);
}

nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& permuto_options) {
    ExecutionContext ctx(inputs, permuto_options);
    return evaluate_move(std::move(script), ctx);
}

} // namespace computo