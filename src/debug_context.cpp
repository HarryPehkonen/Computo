#include "computo.hpp"

namespace computo {

// --- DebugContext Implementation ---

void DebugContext::set_operator_breakpoint(const std::string& operator_name) {
    operator_breakpoints_.insert(operator_name);
}

void DebugContext::set_variable_breakpoint(const std::string& var) {
    variable_breakpoints_.insert(var);
}

void DebugContext::remove_operator_breakpoint(const std::string& operator_name) {
    operator_breakpoints_.erase(operator_name);
}

void DebugContext::remove_variable_breakpoint(const std::string& var) {
    variable_breakpoints_.erase(var);
}

void DebugContext::clear_all_breakpoints() {
    operator_breakpoints_.clear();
    variable_breakpoints_.clear();
}

auto DebugContext::get_operator_breakpoints() const -> const std::set<std::string>& {
    return operator_breakpoints_;
}

auto DebugContext::get_variable_breakpoints() const -> const std::set<std::string>& {
    return variable_breakpoints_;
}

void DebugContext::set_debug_enabled(bool enabled) { debug_enabled_ = enabled; }

void DebugContext::set_trace_enabled(bool enabled) { trace_enabled_ = enabled; }

void DebugContext::set_step_mode(bool enabled) { step_mode_ = enabled; }

void DebugContext::set_finish_mode(bool enabled) { finish_mode_ = enabled; }

auto DebugContext::is_debug_enabled() const -> bool { return debug_enabled_; }

auto DebugContext::is_trace_enabled() const -> bool { return trace_enabled_; }

auto DebugContext::is_step_mode() const -> bool { return step_mode_; }

auto DebugContext::is_finish_mode() const -> bool { return finish_mode_; }

void DebugContext::record_step(const std::string& operation, const std::string& location,
                               const std::map<std::string, nlohmann::json>& variables,
                               const nlohmann::json& expression) {
    if (trace_enabled_) {
        execution_trace_.emplace_back(operation, location, variables, expression);
    }
}

auto DebugContext::get_execution_trace() const -> const std::vector<DebugStep>& {
    return execution_trace_;
}

auto DebugContext::get_current_location() const -> std::string {
    if (execution_trace_.empty()) {
        return "start";
    }
    return execution_trace_.back().location;
}

auto DebugContext::should_break_on_operator(const std::string& operator_name) const -> bool {
    if (finish_mode_) {
        return false;
    }
    return operator_breakpoints_.count(operator_name) > 0;
}

auto DebugContext::should_break_on_variable(const std::string& var) const -> bool {
    if (finish_mode_) {
        return false;
    }
    return variable_breakpoints_.count(var) > 0;
}

auto DebugContext::should_break() const -> bool {
    if (finish_mode_) {
        return false;
    }
    return step_mode_ || !operator_breakpoints_.empty() || !variable_breakpoints_.empty();
}

void DebugContext::reset() {
    step_mode_ = false;
    finish_mode_ = false;
    execution_trace_.clear();
}

} // namespace computo
