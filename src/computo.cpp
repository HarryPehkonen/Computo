#include <computo/computo.hpp>
#include <computo/operators.hpp>
#include <computo/debugger.hpp>
#include <permuto/permuto.hpp>
#include <iostream>
#include <mutex>

namespace computo {

// Define static null input for ExecutionContext
const nlohmann::json ExecutionContext::null_input = nlohmann::json(nullptr);

// =============================================================================
// Operator Registry Management
// =============================================================================

namespace {
    // Operator registry - internal to this module
    std::map<std::string, OperatorFunc> operators;
    std::once_flag operators_init_flag;
    
    // Initialize operators - thread-safe version with modular approach
    void initialize_operators() {
        std::call_once(operators_init_flag, []() {
            // Initialize operators from separate modules
            operator_modules::init_arithmetic_operators(operators);
            operator_modules::init_logical_operators(operators);  
            operator_modules::init_comparison_operators(operators);
            operator_modules::init_array_operators(operators);
            operator_modules::init_utility_operators(operators);
            operator_modules::init_list_operators(operators);
            operator_modules::init_string_operators(operators);
            
            // Note: let and if operators handled as special forms in evaluate() for TCO
        });
    }
}

// =============================================================================
// Core Evaluation Engine
// =============================================================================

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
            // Evaluate each element in the array - exception-safe move operations
            nlohmann::json result = nlohmann::json::array();
            auto& array_ref = expr["array"];
            
            // Pre-reserve space to minimize reallocation exceptions
            result.get_ref<nlohmann::json::array_t&>().reserve(array_ref.size());
            
            for (size_t i = 0; i < array_ref.size(); ++i) {
                ExecutionContext element_ctx = ctx.with_path("array[" + std::to_string(i) + "]");
                
                // Exception-safe: move only when we're sure push_back will succeed
                nlohmann::json element = evaluate_move(std::move(array_ref[i]), element_ctx);
                
                // This should not throw due to pre-reservation, but if it does,
                // element is already moved so we're in a consistent state
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
                
                // Exception-safe variable binding
                nlohmann::json var_value;
                try {
                    // Special case: don't evaluate lambda expressions, store them as-is
                    if (binding[1].is_array() && binding[1].size() == 3 &&
                        binding[1][0].is_string() && binding[1][0].get<std::string>() == "lambda") {
                        var_value = std::move(binding[1]);  // Move lambda as-is
                    } else {
                        // Include previously bound variables in the evaluation context
                        ExecutionContext value_ctx = binding_ctx.with_path("value").with_variables(new_vars);
                        var_value = evaluate_move(std::move(binding[1]), value_ctx);  // Move evaluate with sequential context
                    }
                    
                    // Only add to new_vars if evaluation succeeded
                    // This ensures new_vars is in a consistent state if later bindings fail
                    new_vars[var_name] = std::move(var_value);
                } catch (...) {
                    // If evaluation fails, binding[1] is already moved, but that's okay
                    // because we're about to propagate the exception and abandon this let operation
                    throw;
                }
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
        
        // Extract arguments (everything except the first element) - exception-safe move
        nlohmann::json args = nlohmann::json::array();
        
        // Pre-reserve space to minimize reallocation exceptions
        if (expr.size() > 1) {
            args.get_ref<nlohmann::json::array_t&>().reserve(expr.size() - 1);
        }
        
        for (size_t i = 1; i < expr.size(); ++i) {
            // Exception-safe: move element and add to args
            // If push_back throws, element is moved but we're propagating exception anyway
            args.push_back(std::move(expr[i]));
        }
        
        // Execute operator with debugging support
        return execute_operator_with_debugging(op, it->second, args, op_ctx);
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
            
            // Pre-reserve space to minimize reallocation exceptions
            result.get_ref<nlohmann::json::array_t&>().reserve(expr["array"].size());
            
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
                    // Include previously bound variables in the evaluation context
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
        
        // Pre-reserve space to minimize reallocation exceptions
        if (expr.size() > 1) {
            args.get_ref<nlohmann::json::array_t&>().reserve(expr.size() - 1);
        }
        
        for (size_t i = 1; i < expr.size(); ++i) {
            args.push_back(expr[i]);
        }
        
        // Execute operator with debugging support
        return execute_operator_with_debugging(op, it->second, args, op_ctx);
    }
}

// =============================================================================
// Public API Functions
// =============================================================================

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