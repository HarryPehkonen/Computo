#pragma once

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef REPL
#include <functional>
#include <set>
#endif

namespace computo {

#ifdef REPL
// -----------------------------------------------------------------------------
// Pre-evaluation hook system (REPL builds only)
// -----------------------------------------------------------------------------
enum class EvaluationAction {
    CONTINUE, // Normal execution
    PAUSE, // Stop and wait (debugging)
    ABORT // Stop execution entirely
};

struct EvaluationContext {
    std::string operator_name;
    nlohmann::json expression;
    std::vector<std::string> execution_path;
    int depth;
    std::map<std::string, nlohmann::json> variables;
};

using PreEvaluationHook = std::function<EvaluationAction(const EvaluationContext&)>;
#endif

// -----------------------------------------------------------------------------
// Exception hierarchy
// -----------------------------------------------------------------------------
class ComputoException : public std::exception {
public:
    explicit ComputoException(std::string msg)
        : message_(std::move(msg)) { }
    [[nodiscard]] auto what() const noexcept -> const char* override { return message_.c_str(); }

private:
    std::string message_;
};

class InvalidOperatorException : public ComputoException {
public:
    explicit InvalidOperatorException(const std::string& op)
        : ComputoException("Invalid operator: " + op) { }
};

class InvalidArgumentException : public ComputoException {
public:
    explicit InvalidArgumentException(const std::string& msg)
        : ComputoException("Invalid argument: " + msg) { }
    InvalidArgumentException(const std::string& msg, const std::string& path)
        : ComputoException("Invalid argument: " + msg + " at " + path) { }
};

// -----------------------------------------------------------------------------
// ExecutionContext: thread-safe evaluation context
// -----------------------------------------------------------------------------
class ExecutionContext {
private:
    std::shared_ptr<const nlohmann::json> input_ptr_;
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr_;
    static const nlohmann::json null_input_;

#ifdef REPL
    PreEvaluationHook pre_evaluation_hook_;
#endif

public:
    std::map<std::string, nlohmann::json> variables;
    std::vector<std::string> path;

    // Single input constructor
    explicit ExecutionContext(const nlohmann::json& input)
        : input_ptr_(std::make_shared<nlohmann::json>(input))
        , inputs_ptr_(std::make_shared<std::vector<nlohmann::json>>(std::vector<nlohmann::json> { input })) { }

    // Multiple inputs constructor
    explicit ExecutionContext(const std::vector<nlohmann::json>& inputs)
        : input_ptr_(inputs.empty() ? std::make_shared<nlohmann::json>(null_input_)
                                    : std::make_shared<nlohmann::json>(inputs[0]))
        , inputs_ptr_(std::make_shared<std::vector<nlohmann::json>>(inputs)) { }

    // Accessors
    [[nodiscard]] auto input() const -> const nlohmann::json& { return *input_ptr_; }
    [[nodiscard]] auto inputs() const -> const std::vector<nlohmann::json>& { return *inputs_ptr_; }

    // Scoped copies
    [[nodiscard]] auto with_variables(const std::map<std::string, nlohmann::json>& vars) const -> ExecutionContext {
        ExecutionContext new_ctx(*inputs_ptr_);
        new_ctx.variables = variables;
        new_ctx.path = path;
        for (const auto& [k, v] : vars) {
            new_ctx.variables[k] = v;
        }
#ifdef REPL
        new_ctx.pre_evaluation_hook_ = pre_evaluation_hook_;
#endif
        return new_ctx;
    }

    [[nodiscard]] auto with_path(const std::string& segment) const -> ExecutionContext {
        ExecutionContext new_ctx = *this;
        new_ctx.path.emplace_back(segment);
        return new_ctx;
    }

    [[nodiscard]] auto get_path_string() const -> std::string {
        if (path.empty()) {
            return "/";
        }
        std::string result;
        for (const auto& seg : path) {
            result += "/" + seg;
        }
        return result;
    }

#ifdef REPL
    // Hook management
    void set_pre_evaluation_hook(const PreEvaluationHook& hook) {
        pre_evaluation_hook_ = hook;
    }

    [[nodiscard]] auto has_pre_evaluation_hook() const -> bool {
        return static_cast<bool>(pre_evaluation_hook_);
    }

    [[nodiscard]] auto call_pre_evaluation_hook(const EvaluationContext& ctx) const -> EvaluationAction {
        if (pre_evaluation_hook_) {
            return pre_evaluation_hook_(ctx);
        }
        return EvaluationAction::CONTINUE;
    }

#endif
};

// Definition of static member
inline const nlohmann::json ExecutionContext::null_input_ = nlohmann::json(nullptr);

// -----------------------------------------------------------------------------
// Public API (declarations) – implemented in src/computo.cpp
// -----------------------------------------------------------------------------
[[nodiscard]] auto execute(const nlohmann::json& script, const nlohmann::json& input) -> nlohmann::json;
[[nodiscard]] auto execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) -> nlohmann::json;

// Overloads with custom array key for unwrapping
[[nodiscard]] auto execute(const nlohmann::json& script, const nlohmann::json& input, const std::string& array_key) -> nlohmann::json;
[[nodiscard]] auto execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const std::string& array_key) -> nlohmann::json;

// Core evaluation function (exposed for advanced use cases like debugging)
[[nodiscard]] auto evaluate(nlohmann::json expr, ExecutionContext ctx) -> nlohmann::json;

// Debugging support API
[[nodiscard]] auto get_available_operators() -> std::vector<std::string>;

} // namespace computo