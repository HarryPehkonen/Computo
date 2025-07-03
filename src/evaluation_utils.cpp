#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <chrono>

namespace computo {

// Forward declaration for debugger access
Debugger* get_debugger();

// Forward declaration for global debug statistics
class GlobalDebugStats;

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

// Helper function to create debug context from execution context
DebugContext create_debug_context(const std::string& operator_name, const nlohmann::json& args, const ExecutionContext& ctx) {
    DebugContext debug_ctx;
    debug_ctx.operator_name = operator_name;
    debug_ctx.execution_path = ctx.get_path_string();
    debug_ctx.arguments = args;
    debug_ctx.variables_in_scope = ctx.get_all_variables();
    debug_ctx.start_time = std::chrono::high_resolution_clock::now();
    return debug_ctx;
}

// Helper function to execute an operator with optional debugging
nlohmann::json execute_operator_with_debugging(
    const std::string& op_name,
    const std::function<nlohmann::json(const nlohmann::json&, ExecutionContext&)>& operator_func,
    const nlohmann::json& args,
    ExecutionContext& ctx) {
    
    Debugger* debugger = get_debugger();
    if (debugger) {
        DebugContext debug_ctx = create_debug_context(op_name, args, ctx);
        debugger->on_operator_enter(debug_ctx);
        
        // Update global statistics
        GlobalDebugStats::increment_operations();
        
        try {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Call the operator function
            nlohmann::json result = operator_func(args, ctx);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end_time - start_time);
            
            // Update global timing statistics
            GlobalDebugStats::add_execution_time(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
            
            debugger->on_operator_exit(debug_ctx, result, duration);
            return result;
            
        } catch (const std::exception& e) {
            GlobalDebugStats::increment_errors();
            debugger->on_error(e, debug_ctx);
            throw;
        }
    } else {
        // No debugging - direct call
        return operator_func(args, ctx);
    }
}

} // namespace computo 