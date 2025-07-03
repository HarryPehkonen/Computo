// Thread Safety Guarantees:
// - All evaluation functions are thread-safe due to thread-local storage
// - Each thread maintains its own debugger instance and memory pool
// - ExecutionContext is stack-based and thread-local
// - Operator registry uses std::once_flag for safe initialization
// - No shared mutable state between threads

#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <permuto/permuto.hpp>
#include "builder.hpp"
#include "memory_pool.hpp"

namespace computo {

// Exception hierarchy
class ComputoException : public std::exception {
public:
    explicit ComputoException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class InvalidScriptException : public ComputoException {
public:
    explicit InvalidScriptException(const std::string& message) 
        : ComputoException("Invalid script: " + message) {}
};

class InvalidOperatorException : public ComputoException {
public:
    explicit InvalidOperatorException(const std::string& op) 
        : ComputoException("Invalid operator: " + op) {}
};

class InvalidArgumentException : public ComputoException {
public:
    explicit InvalidArgumentException(const std::string& message) 
        : ComputoException("Invalid argument: " + message) {}
    
    explicit InvalidArgumentException(const std::string& message, const std::string& path) 
        : ComputoException("Invalid argument: " + message + " at " + path) {}
};

class PatchFailedException : public ComputoException {
public:
    explicit PatchFailedException(const std::string& message) 
        : ComputoException("Failed to apply JSON patch: " + message) {}
};

// Execution context for maintaining state during evaluation
// Uses smart pointers for memory safety and assignability (required for TCO)
class ExecutionContext {
private:
    std::shared_ptr<const nlohmann::json> input_ptr;
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr;
    static const nlohmann::json null_input;  // Static null input for empty inputs case
    
public:
    std::map<std::string, nlohmann::json> variables;
    permuto::Options permuto_options;
    std::vector<std::string> evaluation_path;  // Path tracking for error reporting
    
    // Constructor for single input (backward compatibility)
    explicit ExecutionContext(const nlohmann::json& input_data) 
        : input_ptr(std::make_shared<nlohmann::json>(input_data)),
          inputs_ptr(std::make_shared<std::vector<nlohmann::json>>(std::vector<nlohmann::json>{input_data})) {}
    
    // Constructor for single input with options (backward compatibility)
    explicit ExecutionContext(const nlohmann::json& input_data, const permuto::Options& options) 
        : input_ptr(std::make_shared<nlohmann::json>(input_data)),
          inputs_ptr(std::make_shared<std::vector<nlohmann::json>>(std::vector<nlohmann::json>{input_data})),
          permuto_options(options) {}
    
    // Constructor for multiple inputs
    explicit ExecutionContext(const std::vector<nlohmann::json>& input_data) 
        : input_ptr(input_data.empty() ? 
                    std::make_shared<nlohmann::json>(null_input) : 
                    std::make_shared<nlohmann::json>(input_data[0])),
          inputs_ptr(std::make_shared<std::vector<nlohmann::json>>(input_data)) {}
    
    // Constructor for multiple inputs with options
    explicit ExecutionContext(const std::vector<nlohmann::json>& input_data, const permuto::Options& options) 
        : input_ptr(input_data.empty() ? 
                    std::make_shared<nlohmann::json>(null_input) : 
                    std::make_shared<nlohmann::json>(input_data[0])),
          inputs_ptr(std::make_shared<std::vector<nlohmann::json>>(input_data)),
          permuto_options(options) {}
    
    // Accessors for backward compatibility
    const nlohmann::json& input() const { return *input_ptr; }
    const std::vector<nlohmann::json>& inputs() const { return *inputs_ptr; }
    
    // Create a new context with additional variables for let scoping
    ExecutionContext with_variables(const std::map<std::string, nlohmann::json>& new_vars) const {
        ExecutionContext new_ctx(*inputs_ptr, permuto_options);
        new_ctx.variables = variables; // Copy existing variables
        new_ctx.evaluation_path = evaluation_path; // Copy path
        // Add/override with new variables
        for (const auto& pair : new_vars) {
            new_ctx.variables[pair.first] = pair.second;
        }
        return new_ctx;
    }
    
    // Create a new context with additional path segment
    ExecutionContext with_path(const std::string& segment) const {
        ExecutionContext new_ctx = *this;
        new_ctx.evaluation_path.push_back(segment);
        return new_ctx;
    }
    
    // Get current evaluation path as string
    std::string get_path_string() const {
        if (evaluation_path.empty()) return "/";
        std::string result;
        for (const auto& segment : evaluation_path) {
            result += "/" + segment;
        }
        return result;
    }
    
    // Lookup a variable by path (e.g., "/var_name")
    nlohmann::json get_variable(const std::string& path) const {
        if (path.empty() || path[0] != '/') {
            throw InvalidArgumentException("Variable path must start with '/'");
        }
        
        std::string var_name = path.substr(1); // Remove leading '/'
        auto it = variables.find(var_name);
        if (it == variables.end()) {
            throw InvalidArgumentException("Variable not found: " + path);
        }
        return it->second;
    }
    
    // Get all variables (for debugging)
    const std::map<std::string, nlohmann::json>& get_all_variables() const {
        return variables;
    }
    
    // Now assignable for TCO!
    ExecutionContext& operator=(const ExecutionContext&) = default;
    ExecutionContext(const ExecutionContext&) = default;
    ExecutionContext& operator=(ExecutionContext&&) = default;
    ExecutionContext(ExecutionContext&&) = default;
};

// Type alias for operator functions
using OperatorFunc = std::function<nlohmann::json(const nlohmann::json& args, ExecutionContext& ctx)>;

// Core evaluation function (now uses pass-by-value for TCO)
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx);

// Move-optimized evaluation function for large JSON objects
nlohmann::json evaluate_move(nlohmann::json&& expr, ExecutionContext ctx);

// Main API functions - existing single input API
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& permuto_options);

// New multiple inputs API
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& permuto_options);

// Memory-optimized execute functions using move semantics
nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input);
nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input, const permuto::Options& permuto_options);
nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& permuto_options);

// Debugging support
class Debugger;
void set_debugger(std::unique_ptr<Debugger> debugger);
Debugger* get_debugger();

// Internal functions for RAII debugger management
void push_debugger_state();
void pop_debugger_state();

// RAII wrapper for debugger lifecycle management
class DebuggerScope {
private:
    bool state_pushed_;
    
public:
    // Constructor: sets a new debugger and saves the previous one
    explicit DebuggerScope(std::unique_ptr<Debugger> debugger);
    
    // Destructor: restores the previous debugger
    ~DebuggerScope();
    
    // Non-copyable, non-moveable (RAII guard)
    DebuggerScope(const DebuggerScope&) = delete;
    DebuggerScope& operator=(const DebuggerScope&) = delete;
    DebuggerScope(DebuggerScope&&) = delete;
    DebuggerScope& operator=(DebuggerScope&&) = delete;
    
    // Get the current debugger
    Debugger* get() const;
    
    // Check if debugger is set
    bool is_set() const;
};

// Utility functions for evaluation
bool is_truthy(const nlohmann::json& value);

// Forward declarations for debugging
struct DebugContext;
DebugContext create_debug_context(const std::string& operator_name, const nlohmann::json& args, const ExecutionContext& ctx);

// Helper function to execute an operator with optional debugging
nlohmann::json execute_operator_with_debugging(
    const std::string& op_name,
    const std::function<nlohmann::json(const nlohmann::json&, ExecutionContext&)>& operator_func,
    const nlohmann::json& args,
    ExecutionContext& ctx);

} // namespace computo