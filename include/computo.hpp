#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <jsom/jsom.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace computo {

// --- Exception Hierarchy ---

class ComputoException : public std::exception {
public:
    explicit ComputoException(std::string msg) : message_(std::move(msg)) {}
    [[nodiscard]] auto what() const noexcept -> const char* override { return message_.c_str(); }

private:
    std::string message_;
};

class InvalidOperatorException : public ComputoException {
public:
    explicit InvalidOperatorException(const std::string& operation)
        : ComputoException("Invalid operator: " + operation) {}

    InvalidOperatorException(const std::string& operation, const std::string& suggestion)
        : ComputoException("Invalid operator: " + operation
                           + (suggestion.empty() ? "" : ". Did you mean '" + suggestion + "'?")) {}
};

class InvalidArgumentException : public ComputoException {
public:
    explicit InvalidArgumentException(const std::string& msg)
        : ComputoException("Invalid argument: " + msg) {}

    InvalidArgumentException(const std::string& msg, const std::string& path)
        : ComputoException("Invalid argument: " + msg + " at " + path) {}
};

class DebugBreakException : public ComputoException {
public:
    explicit DebugBreakException(const std::string& location, const std::string& reason)
        : ComputoException("Debug breakpoint: " + reason + " at " + location), location_(location),
          reason_(reason) {}

    [[nodiscard]] auto get_location() const -> const std::string& { return location_; }
    [[nodiscard]] auto get_reason() const -> const std::string& { return reason_; }

private:
    std::string location_;
    std::string reason_;
};

// --- Debug Infrastructure ---

enum class DebugAction : std::uint8_t {
    CONTINUE, // Continue execution
    STEP,     // Step to next operation
    FINISH,   // Complete execution, ignore breakpoints
    BREAK     // Hit breakpoint, enter debug mode
};

struct DebugStep {
    std::string operation;                           // Current operator/expression
    std::string location;                            // Current execution path
    std::map<std::string, jsom::JsonDocument> variables; // Current scope variables
    jsom::JsonDocument expression;                       // Current expression being evaluated

    DebugStep(std::string operation_name, std::string location_name,
              std::map<std::string, jsom::JsonDocument> vars, jsom::JsonDocument expr)
        : operation(std::move(operation_name)), location(std::move(location_name)),
          variables(std::move(vars)), expression(std::move(expr)) {}
};

class DebugContext {
private:
    std::set<std::string> operator_breakpoints_; // Operators to break on (e.g., "+", "map")
    std::set<std::string> variable_breakpoints_; // Variables to break on (e.g., "x", "data")
    bool debug_enabled_{false};                  // Whether debugging is active
    bool trace_enabled_{false};                  // Whether tracing is active
    bool step_mode_{false};                      // Whether in step-by-step mode
    bool finish_mode_{false};                    // Whether to ignore breakpoints until finish
    std::vector<DebugStep> execution_trace_;     // Trace of execution steps

public:
    // Breakpoint management
    void set_operator_breakpoint(const std::string& operator_name);
    void set_variable_breakpoint(const std::string& var);
    void remove_operator_breakpoint(const std::string& operator_name);
    void remove_variable_breakpoint(const std::string& var);
    void clear_all_breakpoints();
    [[nodiscard]] auto get_operator_breakpoints() const -> const std::set<std::string>&;
    [[nodiscard]] auto get_variable_breakpoints() const -> const std::set<std::string>&;

    // Debug mode control
    void set_debug_enabled(bool enabled);
    void set_trace_enabled(bool enabled);
    void set_step_mode(bool enabled);
    void set_finish_mode(bool enabled);
    [[nodiscard]] auto is_debug_enabled() const -> bool;
    [[nodiscard]] auto is_trace_enabled() const -> bool;
    [[nodiscard]] auto is_step_mode() const -> bool;
    [[nodiscard]] auto is_finish_mode() const -> bool;

    // Execution tracking
    void record_step(const std::string& operation, const std::string& location,
                     const std::map<std::string, jsom::JsonDocument>& variables,
                     const jsom::JsonDocument& expression);
    [[nodiscard]] auto get_execution_trace() const -> const std::vector<DebugStep>&;
    [[nodiscard]] auto get_current_location() const -> std::string;

    // Breakpoint checking
    [[nodiscard]] auto should_break_on_operator(const std::string& operator_name) const -> bool;
    [[nodiscard]] auto should_break_on_variable(const std::string& var) const -> bool;
    [[nodiscard]] auto should_break() const -> bool; // Check if any break condition is met

    // Reset state
    void reset();
};

// --- ExecutionContext ---

class ExecutionContext {
private:
    std::shared_ptr<const jsom::JsonDocument> input_ptr_;
    std::shared_ptr<const std::vector<jsom::JsonDocument>> inputs_ptr_;
    static const jsom::JsonDocument null_input_;

public:
    std::map<std::string, jsom::JsonDocument> variables;
    std::vector<std::string> path;
    std::string array_key; // Custom array wrapper key

    // Single input constructor
    explicit ExecutionContext(const jsom::JsonDocument& input, std::string array_key = "array");

    // Multiple inputs constructor
    explicit ExecutionContext(const std::vector<jsom::JsonDocument>& inputs,
                              std::string array_key = "array");

    // Accessors
    [[nodiscard]] auto input() const -> const jsom::JsonDocument& { return *input_ptr_; }
    [[nodiscard]] auto inputs() const -> const std::vector<jsom::JsonDocument>& { return *inputs_ptr_; }

    // Thread-safe context creation for scoping
    [[nodiscard]] auto with_variables(const std::map<std::string, jsom::JsonDocument>& vars) const
        -> ExecutionContext;
    [[nodiscard]] auto with_path(const std::string& segment) const -> ExecutionContext;

    [[nodiscard]] auto get_path_string() const -> std::string;
};

// --- TCO Support ---

// Continuation for tail call optimization
struct TailCall {
    jsom::JsonDocument expression;
    ExecutionContext context;

    TailCall(jsom::JsonDocument expr, ExecutionContext ctx)
        : expression(std::move(expr)), context(std::move(ctx)) {}
};

// Result type for trampoline pattern
struct EvaluationResult {
    jsom::JsonDocument value;
    bool is_tail_call;
    std::unique_ptr<TailCall> tail_call;

    // Constructor for regular result
    explicit EvaluationResult(jsom::JsonDocument val) : value(std::move(val)), is_tail_call(false) {}

    // Constructor for tail call
    EvaluationResult(jsom::JsonDocument expr, ExecutionContext ctx)
        : is_tail_call(true),
          tail_call(std::make_unique<TailCall>(std::move(expr), std::move(ctx))) {}
};

// --- Operator Function Signature ---

using OperatorFunction = std::function<EvaluationResult(const jsom::JsonDocument&, ExecutionContext&)>;

// --- Operator Registry ---

class OperatorRegistry {
private:
    std::map<std::string, OperatorFunction> operators_;
    static std::once_flag initialized_;
    static std::unique_ptr<OperatorRegistry> instance_;

    OperatorRegistry() = default;
    void initialize_operators();

public:
    static auto get_instance() -> OperatorRegistry&;
    [[nodiscard]] auto get_operator(const std::string& name) const -> OperatorFunction;
    [[nodiscard]] auto has_operator(const std::string& name) const -> bool;
    [[nodiscard]] auto get_operator_names() const -> std::vector<std::string>;
};

// --- Operator Forward Declarations ---

// Arithmetic Operators
auto op_addition(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_subtraction(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_multiplication(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_division(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_modulo(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Comparison Operators
auto op_greater_than(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_less_than(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_greater_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_less_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_not_equal(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Logical Operators
auto op_logical_and(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_logical_or(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_logical_not(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Array Operations
auto op_map(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_filter(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_reduce(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_count(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_find(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_some(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_every(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Functional Programming
auto op_car(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_cdr(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_cons(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_append(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Object Operations
auto op_obj(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_keys(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_values(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_obj_from_pairs(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_pick(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_omit(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Data Access
auto op_input(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_inputs(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_variable(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;
auto op_let(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// Control Flow
auto op_if(const jsom::JsonDocument& args, ExecutionContext& ctx) -> EvaluationResult;

// --- Core Evaluation ---

auto evaluate_internal(const jsom::JsonDocument& expr, const ExecutionContext& ctx,
                       DebugContext* debug_ctx = nullptr) -> EvaluationResult;
auto evaluate(const jsom::JsonDocument& expr, const ExecutionContext& ctx,
              DebugContext* debug_ctx = nullptr) -> jsom::JsonDocument;

// --- Public API ---

// Unified execution function - inputs vector can be empty, single element, or
// multiple elements
auto execute(const jsom::JsonDocument& script, const std::vector<jsom::JsonDocument>& inputs = {},
             DebugContext* debug_context = nullptr, std::string array_key = "array")
    -> jsom::JsonDocument;

} // namespace computo
