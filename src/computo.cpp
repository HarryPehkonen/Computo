#include <computo.hpp>
#include <mutex>
#include <functional>
#include <map>
#include "operators/declarations.hpp"
#include "operators/shared.hpp"

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
        op_registry["get"] = computo::operators::get_ptr;
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
    });
}
} // anonymous namespace

// Forward declaration of evaluator (implementation below) – needs external linkage for operator source files
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx);
nlohmann::json evaluate_lazy_tco(nlohmann::json expr, ExecutionContext ctx);

// Simple evaluator for Phase 1 (handles literals and $input)
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    // Use lazy debug TCO approach (Solution 3)
    return evaluate_lazy_tco(expr, ctx);
}

// Public API
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate(script, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) {
    ExecutionContext ctx(inputs);
    return evaluate(script, ctx);
}

std::vector<std::string> get_available_operators() {
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
nlohmann::json evaluate_lazy_tco(nlohmann::json expr, ExecutionContext ctx) {
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
    
    TailCall current{expr, ctx};
    
    while (true) {
        initialize_operators();
        
        expr = current.expression;
        ctx = current.context;
        
        // Non-array literals or objects/arrays treated as data
        if (!expr.is_array()) {
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
            if (expr.size() != 1) {
                throw InvalidArgumentException("$input takes no arguments", ctx.get_path_string());
            }
            return ctx.input();
        }
        
        if (op == "$inputs") {
            if (expr.size() != 1) {
                throw InvalidArgumentException("$inputs takes no arguments", ctx.get_path_string());
            }
            return nlohmann::json(ctx.inputs());
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
            continue;  // Tail call optimization!
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
            continue;  // Tail call optimization!
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

} // namespace computo 