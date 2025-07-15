#include "operators/declarations.hpp"
#include "operators/shared.hpp"
#include <computo.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <utility>

namespace computo {

namespace {
    using OperatorFunc = std::function<nlohmann::json(const nlohmann::json&, ExecutionContext&)>;
    std::map<std::string, OperatorFunc> op_registry;
    std::once_flag init_flag;

    void initialize_operators() {
        std::call_once(init_flag, []() {
            op_registry["+"] = computo::operators::addition;
            op_registry["-"] = computo::operators::subtraction;
            op_registry["*"] = computo::operators::multiplication;
            op_registry["/"] = computo::operators::division;
            op_registry["%"] = computo::operators::modulo;
            // comparison
            op_registry[">"] = computo::operators::greater_than;
            op_registry["<"] = computo::operators::less_than;
            op_registry[">="] = computo::operators::greater_equal;
            op_registry["<="] = computo::operators::less_equal;
            op_registry["=="] = computo::operators::equal;
            op_registry["!="] = computo::operators::not_equal;
            // logical
            op_registry["&&"] = computo::operators::logical_and;
            op_registry["||"] = computo::operators::logical_or;
            op_registry["not"] = computo::operators::logical_not;
            // data access etc.
            op_registry["$"] = computo::operators::var_access;
            op_registry["let"] = computo::operators::let_binding;
            op_registry["obj"] = computo::operators::obj_construct;
            op_registry["if"] = computo::operators::if_operator;
            // array ops
            op_registry["map"] = computo::operators::map_op;
            op_registry["filter"] = computo::operators::filter_op;
            op_registry["reduce"] = computo::operators::reduce_op;
            op_registry["count"] = computo::operators::count_op;
            op_registry["find"] = computo::operators::find_op;
            op_registry["some"] = computo::operators::some_op;
            op_registry["every"] = computo::operators::every_op;
            op_registry["zip"] = computo::operators::zip_op;
            // functional list
            op_registry["car"] = computo::operators::car_op;
            op_registry["cdr"] = computo::operators::cdr_op;
            op_registry["cons"] = computo::operators::cons_op;
            op_registry["append"] = computo::operators::append_op;
            // functional programming
            op_registry["lambda"] = computo::operators::lambda_operator;
            op_registry["call"] = computo::operators::call_operator;
            // utilities
            op_registry["strConcat"] = computo::operators::str_concat;
            op_registry["merge"] = computo::operators::merge_op;
            op_registry["approx"] = computo::operators::approx_op;
            // object operations
            op_registry["keys"] = computo::operators::keys_op;
            op_registry["values"] = computo::operators::values_op;
            op_registry["objFromPairs"] = computo::operators::objFromPairs_op;
            op_registry["pick"] = computo::operators::pick_op;
            op_registry["omit"] = computo::operators::omit_op;
            // string operations
            op_registry["split"] = computo::operators::split_op;
            op_registry["join"] = computo::operators::join_op;
            op_registry["trim"] = computo::operators::trim_op;
            op_registry["upper"] = computo::operators::upper_op;
            op_registry["lower"] = computo::operators::lower_op;
            // array operations
            op_registry["sort"] = computo::operators::sort_op;
            op_registry["reverse"] = computo::operators::reverse_op;
            op_registry["unique"] = computo::operators::unique_op;
        });
    }
} // anonymous namespace

// Forward declaration of evaluator (implementation below) – needs external linkage for operator source files

auto evaluate_lazy_tco(nlohmann::json expr, ExecutionContext ctx) -> nlohmann::json;

// Forward declarations
auto unwrap_array_objects(const nlohmann::json& value, const std::string& array_key = "array") -> nlohmann::json;
auto transform_array_keys(const nlohmann::json& value, const std::string& from_key, const std::string& to_key) -> nlohmann::json;

// Simple evaluator for Phase 1 (handles literals and $input)
auto evaluate(nlohmann::json expr, ExecutionContext ctx) -> nlohmann::json {
    // Use lazy debug TCO approach (Solution 3)
    return evaluate_lazy_tco(std::move(expr), std::move(ctx));
}

// Public API
auto execute(const nlohmann::json& script, const nlohmann::json& input) -> nlohmann::json {
    ExecutionContext ctx(input);
    nlohmann::json result = evaluate(script, ctx);

    // Apply final array unwrapping
    return unwrap_array_objects(result);
}

auto execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) -> nlohmann::json {
    ExecutionContext ctx(inputs);
    nlohmann::json result = evaluate(script, ctx);

    // Apply final array unwrapping
    return unwrap_array_objects(result);
}

// Overloads with custom array key syntax
auto execute(const nlohmann::json& script, const nlohmann::json& input, const std::string& array_key) -> nlohmann::json {
    // If using default "array" key, use regular execution
    if (array_key == "array") {
        return execute(script, input);
    }

    // Transform script and input to use "array" key internally, then process normally
    nlohmann::json transformed_script = transform_array_keys(script, array_key, "array");
    nlohmann::json transformed_input = transform_array_keys(input, array_key, "array");

    ExecutionContext ctx(transformed_input);
    nlohmann::json result = evaluate(transformed_script, ctx);

    // Apply final array unwrapping (always unwrap "array" objects)
    return unwrap_array_objects(result);
}

auto execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const std::string& array_key) -> nlohmann::json {
    // If using default "array" key, use regular execution
    if (array_key == "array") {
        return execute(script, inputs);
    }

    // Transform script and inputs to use "array" key internally
    nlohmann::json transformed_script = transform_array_keys(script, array_key, "array");
    std::vector<nlohmann::json> transformed_inputs;
    transformed_inputs.reserve(inputs.size());
    for (const auto& input : inputs) {
        transformed_inputs.push_back(transform_array_keys(input, array_key, "array"));
    }

    ExecutionContext ctx(transformed_inputs);
    nlohmann::json result = evaluate(transformed_script, ctx);

    // Apply final array unwrapping (always unwrap "array" objects)
    return unwrap_array_objects(result);
}

auto get_available_operators() -> std::vector<std::string> {
    initialize_operators();
    std::vector<std::string> operators;
    operators.reserve(op_registry.size());
    for (const auto& [name, func] : op_registry) {
        operators.push_back(name);
    }
    return operators;
}

// Import utility for TCO
using computo::operators::is_truthy;

// Enhanced evaluator with lazy debug TCO (Solution 3)
auto evaluate_lazy_tco(nlohmann::json expr, ExecutionContext ctx) -> nlohmann::json {
    // Lazy debug tracking - only track if debugging is actually needed
    bool should_debug = false;
#ifdef REPL
    should_debug = ctx.has_pre_evaluation_hook();
#endif

    // Simple trampoline loop for tail call optimization
    struct TailCall {
        nlohmann::json expression;
        ExecutionContext context;
    };

    TailCall current { expr, ctx };

    while (true) {
        initialize_operators();

        expr = current.expression;
        ctx = current.context;

        // Non-array literals or objects/arrays treated as data
        if (!expr.is_array()) {
            // Special handling for array wrapper objects
            if (expr.is_object() && expr.size() == 1 && expr.contains("array")) {
                // This is an array wrapper - evaluate contents but keep wrapper
                nlohmann::json array_content = expr["array"];
                if (array_content.is_array()) {
                    nlohmann::json evaluated_content = nlohmann::json::array();
                    for (const auto& element : array_content) {
                        // Only evaluate if it's a valid expression, otherwise keep as literal
                        if (element.is_array() && !element.empty() && element[0].is_string()) {
                            initialize_operators();
                            std::string potential_op = element[0].get<std::string>();
                            if (op_registry.find(potential_op) != op_registry.end() || potential_op == "$input" || potential_op == "$inputs") {
                                // Valid expression - evaluate it
                                evaluated_content.push_back(evaluate_lazy_tco(element, ctx));
                            } else {
                                // Not a valid expression - keep as literal
                                evaluated_content.push_back(element);
                            }
                        } else {
                            // Not an array or empty - keep as literal
                            evaluated_content.push_back(element);
                        }
                    }
                    // Return wrapper with evaluated content
                    return nlohmann::json::object({ { "array", evaluated_content } });
                }
            }
            return expr;
        }

        // Empty array is invalid
        if (expr.empty()) {
            throw InvalidArgumentException("Empty expression array", ctx.get_path_string());
        }

        // If first element is not a string, treat as literal array
        if (!expr[0].is_string()) {
            return expr;
        }

        std::string op = expr[0].get<std::string>();

        // Handle special built-in operators
        if (op == "$input") {
            if (expr.size() > 3) {
                throw InvalidArgumentException("$input takes 0, 1, or 2 arguments", ctx.get_path_string());
            }
            if (expr.size() == 1) {
                return ctx.input();
            }
            // Handle optional path argument
            auto path_expr = evaluate_lazy_tco(expr[1], ctx.with_path("path"));
            if (!path_expr.is_string()) {
                throw InvalidArgumentException("$input path must be string", ctx.get_path_string());
            }
            std::string path = path_expr.get<std::string>();
            try {
                return ctx.input().at(nlohmann::json::json_pointer(path));
            } catch (const std::exception&) {
                // If we have a default value (3rd argument), return it
                if (expr.size() == 3) {
                    return evaluate_lazy_tco(expr[2], ctx.with_path("default"));
                }
                throw InvalidArgumentException("Invalid JSON pointer or path not found: " + path, ctx.get_path_string());
            }
        }

        if (op == "$inputs") {
            if (expr.size() > 3) {
                throw InvalidArgumentException("$inputs takes 0, 1, or 2 arguments", ctx.get_path_string());
            }
            if (expr.size() == 1) {
                return nlohmann::json(ctx.inputs());
            }
            // Handle optional path argument for array indexing
            auto path_expr = evaluate_lazy_tco(expr[1], ctx.with_path("path"));
            if (!path_expr.is_string()) {
                throw InvalidArgumentException("$inputs path must be string", ctx.get_path_string());
            }
            std::string path = path_expr.get<std::string>();
            try {
                nlohmann::json inputs_array = nlohmann::json(ctx.inputs());
                return inputs_array.at(nlohmann::json::json_pointer(path));
            } catch (const std::exception&) {
                // If we have a default value (3rd argument), return it
                if (expr.size() == 3) {
                    return evaluate_lazy_tco(expr[2], ctx.with_path("default"));
                }
                throw InvalidArgumentException("Invalid JSON pointer or path not found: " + path, ctx.get_path_string());
            }
        }

        // Lazy debug hook - only called when debugging is active
        if (should_debug) {
#ifdef REPL
            EvaluationContext hook_ctx;
            hook_ctx.operator_name = op;
            hook_ctx.expression = expr;
            hook_ctx.execution_path = ctx.path;
            hook_ctx.depth = static_cast<int>(ctx.path.size());
            hook_ctx.variables = ctx.variables;

            EvaluationAction action = ctx.call_pre_evaluation_hook(hook_ctx);
            if (action == EvaluationAction::ABORT) {
                throw std::runtime_error("Execution aborted by pre-evaluation hook");
            }
#endif
        }

        // Handle tail-call optimizable operators
        if (op == "if") {
            if (expr.size() != 4) {
                throw InvalidArgumentException("if requires exactly 3 arguments", ctx.get_path_string());
            }

            // Evaluate condition
            auto condition = evaluate_lazy_tco(expr[1], ctx.with_path("condition"));
            bool is_true = is_truthy(condition);

            // Tail call to the appropriate branch
            current.expression = is_true ? expr[2] : expr[3];
            current.context = ctx.with_path(is_true ? "then" : "else");
            continue; // Tail call optimization!
        }

        if (op == "let") {
            if (expr.size() != 3) {
                throw InvalidArgumentException("let requires exactly 2 arguments", ctx.get_path_string());
            }
            if (!expr[1].is_array()) {
                throw InvalidArgumentException("let bindings must be array", ctx.get_path_string());
            }

            // Evaluate all bindings
            std::map<std::string, nlohmann::json> new_vars;
            for (const auto& binding : expr[1]) {
                if (!binding.is_array() || binding.size() != 2) {
                    throw InvalidArgumentException("let binding must be [var, value]", ctx.get_path_string());
                }
                if (!binding[0].is_string()) {
                    throw InvalidArgumentException("let variable name must be string", ctx.get_path_string());
                }
                std::string var_name = binding[0].get<std::string>();
                new_vars[var_name] = evaluate_lazy_tco(binding[1], ctx.with_path("binding"));
            }

            // Tail call to body with new variables
            current.expression = expr[2];
            current.context = ctx.with_path("body").with_variables(new_vars);
            continue; // Tail call optimization!
        }

        // Regular operators - not tail-call optimizable
        auto it = op_registry.find(op);
        if (it == op_registry.end()) {
            throw InvalidOperatorException(op);
        }

        // Build argument list (leave unevaluated; operator implementations will evaluate as needed)
        nlohmann::json arg_exprs = nlohmann::json::array();
        for (size_t i = 1; i < expr.size(); ++i) {
            arg_exprs.push_back(expr[i]);
        }

        return it->second(arg_exprs, ctx);
    }
}

// Transform array keys recursively (e.g., convert "@array" to "array" for internal processing)
auto transform_array_keys(const nlohmann::json& value, const std::string& from_key, const std::string& to_key) -> nlohmann::json {
    if (value.is_object()) {
        if (value.size() == 1 && value.contains(from_key)) {
            // Transform this object key
            nlohmann::json result = nlohmann::json::object();
            result[to_key] = transform_array_keys(value[from_key], from_key, to_key);
            return result;
        } // Process each value in the object
        nlohmann::json result = nlohmann::json::object();
        for (auto it = value.begin(); it != value.end(); ++it) {
            result[it.key()] = transform_array_keys(it.value(), from_key, to_key);
        }
        return result;
    }
    if (value.is_array()) {
        // Process each element in the array
        nlohmann::json result = nlohmann::json::array();
        for (const auto& element : value) {
            result.push_back(transform_array_keys(element, from_key, to_key));
        }
        return result;
    } // Primitive values are returned as-is
    return value;
}

// Clean final-pass array unwrapping (after all evaluation is complete)
auto unwrap_array_objects(const nlohmann::json& value, const std::string& array_key) -> nlohmann::json {
    if (value.is_object() && value.size() == 1 && value.contains(array_key)) {
        // This is an array object - unwrap it and recursively process the content
        const nlohmann::json& unwrapped = value[array_key];
        return unwrap_array_objects(unwrapped, array_key);
    }
    if (value.is_array()) {
        // Process each element in the array
        nlohmann::json result = nlohmann::json::array();
        for (const auto& element : value) {
            result.push_back(unwrap_array_objects(element, array_key));
        }
        return result;
    }
    if (value.is_object()) {
        // Process each value in the object (but don't unwrap multi-key objects)
        nlohmann::json result = nlohmann::json::object();
        for (auto it = value.begin(); it != value.end(); ++it) {
            result[it.key()] = unwrap_array_objects(it.value(), array_key);
        }
        return result;
    } // Primitive values are returned as-is
    return value;
}

} // namespace computo