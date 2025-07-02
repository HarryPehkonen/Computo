#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

namespace computo {
namespace operator_modules {

void init_string_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // str_concat operator - concatenates values as strings with automatic type conversion
    ops["strConcat"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
        if (args.size() < 1) {
            throw InvalidArgumentException("strConcat operator requires at least 1 argument");
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
}

} // namespace operator_modules
} // namespace computo 