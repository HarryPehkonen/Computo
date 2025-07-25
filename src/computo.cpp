#include <cmath>
#include <computo.hpp>
#include <operators/shared.hpp>
#include <optional>

namespace computo {

// Static member definitions
const nlohmann::json ExecutionContext::null_input_ = nlohmann::json(nullptr);
std::once_flag OperatorRegistry::initialized_;
std::unique_ptr<OperatorRegistry> OperatorRegistry::instance_;

// --- ExecutionContext Implementation ---

ExecutionContext::ExecutionContext(const nlohmann::json& input)
    : input_ptr_(std::make_shared<nlohmann::json>(input)),
      inputs_ptr_(
          std::make_shared<std::vector<nlohmann::json>>(std::vector<nlohmann::json>{input})) {}

ExecutionContext::ExecutionContext(const std::vector<nlohmann::json>& inputs)
    : input_ptr_(inputs.empty() ? std::make_shared<nlohmann::json>(null_input_)
                                : std::make_shared<nlohmann::json>(inputs[0])),
      inputs_ptr_(std::make_shared<std::vector<nlohmann::json>>(inputs)) {}

auto ExecutionContext::with_variables(const std::map<std::string, nlohmann::json>& vars) const
    -> ExecutionContext {
    ExecutionContext new_ctx(*inputs_ptr_);
    new_ctx.variables = variables; // Copy existing
    new_ctx.path = path;           // Copy path
    for (const auto& pair : vars) {
        new_ctx.variables[pair.first] = pair.second; // Add new
    }
    return new_ctx;
}

auto ExecutionContext::with_path(const std::string& segment) const -> ExecutionContext {
    ExecutionContext new_ctx = *this;
    new_ctx.path.push_back(segment);
    return new_ctx;
}

auto ExecutionContext::get_path_string() const -> std::string {
    if (path.empty()) {
        return "/";
    }
    std::string result;
    for (const auto& seg : path) {
        result += "/" + seg;
    }
    return result;
}

// --- Operator Registry Implementation ---

auto OperatorRegistry::get_instance() -> OperatorRegistry& {
    std::call_once(initialized_, []() {
        instance_ = std::unique_ptr<OperatorRegistry>(new OperatorRegistry());
        instance_->initialize_operators();
    });
    return *instance_;
}

auto OperatorRegistry::get_operator(const std::string& name) const -> OperatorFunction {
    auto iter = operators_.find(name);
    if (iter == operators_.end()) {
        auto available_ops = get_operator_names();
        auto suggestions = suggest_similar_names(name, available_ops, 2);

        std::string suggestion = suggestions.empty() ? "" : suggestions[0];
        throw InvalidOperatorException(name, suggestion);
    }
    return iter->second;
}

auto OperatorRegistry::has_operator(const std::string& name) const -> bool {
    return operators_.find(name) != operators_.end();
}

auto OperatorRegistry::get_operator_names() const -> std::vector<std::string> {
    std::vector<std::string> names;
    names.reserve(operators_.size());
    for (const auto& [name, _] : operators_) {
        names.push_back(name);
    }
    return names;
}

namespace operators {
// Forward declarations for operators defined in other files
auto addition(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto subtraction(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto multiplication(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto division(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto modulo(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Comparison Operators
auto greater_than(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto less_than(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto greater_equal(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto less_equal(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto equal(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto not_equal(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Data Access Operators
auto input_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto inputs_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto variable_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto let_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Logical Operators
auto logical_and(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto logical_or(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto logical_not(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Control Flow Operators
auto if_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Object Operators
auto obj_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto keys_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto values_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto objFromPairs_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto pick_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto omit_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto merge_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Array Operators
auto map_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto filter_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto reduce_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto count_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto find_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto some_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto every_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// Functional Programming Operators
auto car_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto cdr_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto cons_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto append_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;

// String and Utility Operators
auto join_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto strConcat_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto sort_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto reverse_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto unique_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto unique_sorted_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto zip_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
auto approx_operator(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
} // namespace operators

// NOLINTBEGIN(readability-function-size)
void OperatorRegistry::initialize_operators() {
    // Arithmetic Operators
    operators_["+"] = operators::addition;
    operators_["-"] = operators::subtraction;
    operators_["*"] = operators::multiplication;
    operators_["/"] = operators::division;
    operators_["%"] = operators::modulo;

    // Comparison Operators
    operators_[">"] = operators::greater_than;
    operators_["<"] = operators::less_than;
    operators_[">="] = operators::greater_equal;
    operators_["<="] = operators::less_equal;
    operators_["=="] = operators::equal;
    operators_["!="] = operators::not_equal;

    // Data Access Operators
    operators_["$input"] = operators::input_operator;
    operators_["$inputs"] = operators::inputs_operator;
    operators_["$"] = operators::variable_operator;
    operators_["let"] = operators::let_operator;

    // Logical Operators
    operators_["and"] = operators::logical_and;
    operators_["or"] = operators::logical_or;
    operators_["not"] = operators::logical_not;

    // Control Flow Operators
    operators_["if"] = operators::if_operator;

    // Object Operators
    operators_["obj"] = operators::obj_operator;
    operators_["keys"] = operators::keys_operator;
    operators_["values"] = operators::values_operator;
    operators_["objFromPairs"] = operators::objFromPairs_operator;
    operators_["pick"] = operators::pick_operator;
    operators_["omit"] = operators::omit_operator;
    operators_["merge"] = operators::merge_operator;

    // Array Operators
    operators_["map"] = operators::map_operator;
    operators_["filter"] = operators::filter_operator;
    operators_["reduce"] = operators::reduce_operator;
    operators_["count"] = operators::count_operator;
    operators_["find"] = operators::find_operator;
    operators_["some"] = operators::some_operator;
    operators_["every"] = operators::every_operator;

    // Functional Programming Operators
    operators_["car"] = operators::car_operator;
    operators_["cdr"] = operators::cdr_operator;
    operators_["cons"] = operators::cons_operator;
    operators_["append"] = operators::append_operator;

    // String and Utility Operators
    operators_["join"] = operators::join_operator;
    operators_["strConcat"] = operators::strConcat_operator;
    operators_["sort"] = operators::sort_operator;
    operators_["reverse"] = operators::reverse_operator;
    operators_["unique"] = operators::unique_operator;
    operators_["uniqueSorted"] = operators::unique_sorted_operator;
    operators_["zip"] = operators::zip_operator;
    operators_["approx"] = operators::approx_operator;
}
// NOLINTEND(readability-function-size)

// --- Core Evaluation Helper Functions ---

// Handles {"array": [...]} syntax
auto evaluate_array_object(const nlohmann::json& expr, const ExecutionContext& ctx,
                           DebugContext* debug_ctx) -> EvaluationResult {
    if (!expr["array"].is_array()) {
        throw InvalidArgumentException("Array object must contain an array", ctx.get_path_string());
    }
    // Array objects are unwrapped and returned as-is (no evaluation of elements)
    return EvaluationResult(expr["array"]);
}

static auto handle_debug_integration(const std::string& operator_name, const ExecutionContext& ctx,
                                     const nlohmann::json& expr, DebugContext* debug_ctx) -> void {
    if (debug_ctx == nullptr || !debug_ctx->is_debug_enabled()) {
        return;
    }

    // Record execution step if tracing is enabled
    if (debug_ctx->is_trace_enabled()) {
        debug_ctx->record_step(operator_name, ctx.get_path_string(), ctx.variables, expr);
    }

    // Check for operator breakpoint
    if (debug_ctx->should_break_on_operator(operator_name)) {
        throw DebugBreakException(ctx.get_path_string(), "operator breakpoint: " + operator_name);
    }

    // Check for step mode breakpoint
    if (debug_ctx->is_step_mode()) {
        debug_ctx->set_step_mode(false); // Reset step mode
        throw DebugBreakException(ctx.get_path_string(), "step mode");
    }
}

// --- Core Evaluation (Updated with Registry and TCO) ---

// Handles literal arrays like [1, 2, 3] or [true, "a"]
auto evaluate_literal_array(const nlohmann::json& expr, const ExecutionContext& ctx,
                            DebugContext* debug_ctx) -> EvaluationResult {
    // Rule 3: Empty arrays are treated as literal arrays
    if (expr.empty()) {
        return EvaluationResult(expr);
    }

    // Rule 3: Non-string first elements → treat as literal array
    // Evaluate each element and return as literal array
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < expr.size(); ++i) {
        auto element_result
            = evaluate_internal(expr[i], ctx.with_path(std::to_string(i)), debug_ctx);
        result.push_back(element_result.value);
    }
    return EvaluationResult(result);
}

// Handles operator calls like ["+", 1, 2]
auto evaluate_operator_call(const nlohmann::json& expr, const ExecutionContext& ctx,
                            DebugContext* debug_ctx) -> EvaluationResult {
    // Rule 1 & 2: String first element → operator call
    std::string operator_name = expr[0].get<std::string>();

    // Handle debug integration (breakpoints, tracing, stepping)
    handle_debug_integration(operator_name, ctx, expr, debug_ctx);

    // Extract arguments (everything after the operator name)
    nlohmann::json args = nlohmann::json::array();
    for (size_t i = 1; i < expr.size(); ++i) {
        args.push_back(expr[i]);
    }

    // Get operator from registry and execute
    auto& registry = OperatorRegistry::get_instance();
    auto operator_func = registry.get_operator(operator_name);

    // Create mutable context for operator execution
    ExecutionContext mutable_ctx = ctx;

    // Execute operator (now returns EvaluationResult directly)
    return operator_func(args, mutable_ctx);
}

auto evaluate_internal(const nlohmann::json& expr, const ExecutionContext& ctx,
                       DebugContext* debug_ctx) -> EvaluationResult {
    // 1. Handle simple literals (numbers, strings, booleans, null, objects)
    if (!expr.is_array()) {
        if (expr.is_object() && expr.size() == 1 && expr.contains("array")) {
            return evaluate_array_object(expr, ctx, debug_ctx);
        }
        return EvaluationResult(expr); // It's a literal object or scalar
    }

    // 2. From here, we know it's a JSON array. Dispatch based on its structure.
    if (expr.empty() || !expr[0].is_string()) {
        return evaluate_literal_array(expr, ctx, debug_ctx); // Rule 3
    }

    return evaluate_operator_call(expr, ctx, debug_ctx); // Rule 1 & 2
}

// Trampoline function for TCO
auto evaluate(const nlohmann::json& expr, const ExecutionContext& ctx, DebugContext* debug_ctx)
    -> nlohmann::json {
    auto result = evaluate_internal(expr, ctx, debug_ctx);

    // Keep bouncing until we get a final result
    while (result.is_tail_call) {
        result
            = evaluate_internal(result.tail_call->expression, result.tail_call->context, debug_ctx);
    }

    return result.value;
}

// --- Public API Implementation ---

// Unified execution function
auto execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs,
             DebugContext* debug_context) -> nlohmann::json {
    ExecutionContext ctx(inputs);
    return evaluate(script, ctx, debug_context);
}

} // namespace computo
