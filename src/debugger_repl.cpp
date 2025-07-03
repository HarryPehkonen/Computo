#include <computo/debugger.hpp>
#include <computo/computo.hpp>
#include <iostream>

namespace computo {

// Implementation of REPL evaluation for the debugger
nlohmann::json Debugger::evaluate_repl_expression(const std::string& input, const DebugContext& context) const {
    try {
        bool substitution_made = false;
        std::string substitution_info;
        auto expr = parse_repl_expression(input, substitution_made, substitution_info);
        
        if (substitution_made) {
            std::cerr << "INFO: " << substitution_info << std::endl;
        }
        
        // Create isolated execution context with dummy input data
        // We use an empty object as placeholder since we're only evaluating expressions
        ExecutionContext repl_ctx(nlohmann::json{});
        
        // Copy all variables from the debugging context
        for (const auto& [name, value] : context.variables_in_scope) {
            repl_ctx.variables[name] = value;
        }
        
        // Set the current execution path for proper context
        repl_ctx.evaluation_path = {context.execution_path};
        
        // Evaluate the expression (this uses the main evaluate function)
        return evaluate(expr, repl_ctx);
        
    } catch (const std::exception& e) {
        return nlohmann::json{{"_repl_error", e.what()}};
    }
}

} // namespace computo 