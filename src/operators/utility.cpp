#include <computo/operators.hpp>
#include <computo/computo.hpp>
#include <permuto/permuto.hpp>
#include <nlohmann/json.hpp>

namespace computo {
namespace operator_modules {

void init_utility_operators(std::map<std::string, OperatorFunc>& ops) {
    
    // Dollar operator for variable lookup
    ops["$"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    ops["get"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Obj operator for object construction
    ops["obj"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    ops["permuto.apply"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Diff operator for generating JSON patches
    ops["diff"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    ops["patch"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
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
    
    // Lambda operator for creating lambda expressions
    ops["lambda"] = [](const nlohmann::json& args, ExecutionContext& /* unused */) -> nlohmann::json {
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
}

} // namespace operator_modules
} // namespace computo 