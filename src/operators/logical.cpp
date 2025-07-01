#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

namespace computo {
namespace operator_modules {

// Helper function is_truthy is declared in computo.hpp

void init_logical_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Logical AND with short-circuit evaluation
    ops["&&"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Logical OR with short-circuit evaluation
    ops["||"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Logical NOT operator
    ops["not"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() != 1) {
            throw InvalidArgumentException("not operator requires exactly 1 argument");
        }
        
        auto result = evaluate(args[0], ctx);
        
        return !is_truthy(result);
    };
}

} // namespace operator_modules
} // namespace computo 