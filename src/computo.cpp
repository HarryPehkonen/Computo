#include <computo/computo.hpp>
#include <permuto/permuto.hpp>

namespace computo {

// Operator registry
static std::map<std::string, OperatorFunc> operators;

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
    
    initialized = true;
}

nlohmann::json evaluate(const nlohmann::json& expr, ExecutionContext& ctx) {
    initialize_operators();
    
    // Base case: if expr is not an array or is empty, it's a literal
    if (!expr.is_array() || expr.empty()) {
        return expr;
    }
    
    // Check if first element is a string (operator)
    if (!expr[0].is_string()) {
        // Not an operator call, treat as literal array
        return expr;
    }
    
    std::string op = expr[0].get<std::string>();
    
    // Special case: $input operator
    if (op == "$input") {
        return ctx.input;
    }
    
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
    
    // Call the operator function
    return it->second(args, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate(script, ctx);
}

} // namespace computo