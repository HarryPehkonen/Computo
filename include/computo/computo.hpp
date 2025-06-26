#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <map>
#include <functional>

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

// Execution context for maintaining state during evaluation
struct ExecutionContext {
    const nlohmann::json& input;
    std::map<std::string, nlohmann::json> variables;
    
    explicit ExecutionContext(const nlohmann::json& input_data) : input(input_data) {}
};

// Type alias for operator functions
using OperatorFunc = std::function<nlohmann::json(const nlohmann::json& args, ExecutionContext& ctx)>;

// Core evaluation function
nlohmann::json evaluate(const nlohmann::json& expr, ExecutionContext& ctx);

// Main API function
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);

} // namespace computo