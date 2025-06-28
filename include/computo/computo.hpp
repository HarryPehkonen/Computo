#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <map>
#include <functional>
#include <permuto/permuto.hpp>

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
};

class PatchFailedException : public ComputoException {
public:
    explicit PatchFailedException(const std::string& message) 
        : ComputoException("Failed to apply JSON patch: " + message) {}
};

// Execution context for maintaining state during evaluation
struct ExecutionContext {
private:
    static const nlohmann::json null_input;  // Static null input for empty inputs case
    
public:
    const nlohmann::json& input;  // Kept for backward compatibility
    const std::vector<nlohmann::json> inputs;  // New multiple inputs support
    std::map<std::string, nlohmann::json> variables;
    permuto::Options permuto_options;
    
    // Constructor for single input (backward compatibility)
    explicit ExecutionContext(const nlohmann::json& input_data) 
        : input(input_data), inputs({input_data}) {}
    
    // Constructor for single input with options (backward compatibility)
    explicit ExecutionContext(const nlohmann::json& input_data, const permuto::Options& options) 
        : input(input_data), inputs({input_data}), permuto_options(options) {}
    
    // Constructor for multiple inputs
    explicit ExecutionContext(const std::vector<nlohmann::json>& input_data) 
        : input(input_data.empty() ? null_input : input_data[0]), inputs(input_data) {}
    
    // Constructor for multiple inputs with options
    explicit ExecutionContext(const std::vector<nlohmann::json>& input_data, const permuto::Options& options) 
        : input(input_data.empty() ? null_input : input_data[0]), inputs(input_data), permuto_options(options) {}
    
    // Create a new context with additional variables for let scoping
    ExecutionContext with_variables(const std::map<std::string, nlohmann::json>& new_vars) const {
        ExecutionContext new_ctx(inputs, permuto_options);
        new_ctx.variables = variables; // Copy existing variables
        // Add/override with new variables
        for (const auto& pair : new_vars) {
            new_ctx.variables[pair.first] = pair.second;
        }
        return new_ctx;
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
};

// Type alias for operator functions
using OperatorFunc = std::function<nlohmann::json(const nlohmann::json& args, ExecutionContext& ctx)>;

// Core evaluation function
nlohmann::json evaluate(const nlohmann::json& expr, ExecutionContext& ctx);

// Main API functions - existing single input API
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& permuto_options);

// New multiple inputs API
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& permuto_options);

} // namespace computo