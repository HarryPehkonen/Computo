#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace computo {

// Forward declarations
class ExecutionContext;

enum class LogLevel {
    ERROR = 0,
    WARNING = 1,
    INFO = 2,
    DEBUG = 3,
    VERBOSE = 4
};

enum class BreakpointAction {
    CONTINUE,
    STEP_INTO,
    STEP_OVER,
    STOP
};

/**
 * Debug context provides rich information about current execution state
 */
struct DebugContext {
    std::string operator_name;
    std::string execution_path;
    nlohmann::json arguments;
    std::map<std::string, nlohmann::json> variables_in_scope;
    std::vector<std::string> call_stack;
    std::chrono::high_resolution_clock::time_point start_time;
    
    nlohmann::json get_argument(size_t index) const {
        if (arguments.is_array() && index < arguments.size()) {
            return arguments[index];
        }
        return nlohmann::json(nullptr);
    }
    
    bool has_variable(const std::string& name) const {
        return variables_in_scope.find(name) != variables_in_scope.end();
    }
    
    nlohmann::json get_variable(const std::string& name) const {
        auto it = variables_in_scope.find(name);
        return it != variables_in_scope.end() ? it->second : nlohmann::json(nullptr);
    }
};

/**
 * Performance statistics for operator execution
 */
struct OperatorStats {
    std::string name;
    std::chrono::duration<double, std::milli> total_time{0};
    size_t call_count = 0;
    std::chrono::duration<double, std::milli> min_time{std::chrono::duration<double, std::milli>::max()};
    std::chrono::duration<double, std::milli> max_time{0};
    
    void add_execution(std::chrono::duration<double, std::milli> duration) {
        total_time += duration;
        call_count++;
        if (duration < min_time) min_time = duration;
        if (duration > max_time) max_time = duration;
    }
    
    std::chrono::duration<double, std::milli> average_time() const {
        return call_count > 0 ? total_time / call_count : std::chrono::duration<double, std::milli>{0};
    }
};

/**
 * Breakpoint definition
 */
struct Breakpoint {
    enum Type { OPERATOR, PATH, CONDITIONAL };
    
    Type type;
    std::string target;  // operator name or path
    std::function<bool(const DebugContext&)> condition;  // for conditional breakpoints
    bool enabled = true;
    size_t hit_count = 0;
    
    static Breakpoint on_operator(const std::string& op_name) {
        return {OPERATOR, op_name, nullptr, true, 0};
    }
    
    static Breakpoint on_path(const std::string& path) {
        return {PATH, path, nullptr, true, 0};
    }
    
    static Breakpoint conditional(const std::string& op_name, 
                                 std::function<bool(const DebugContext&)> cond) {
        return {CONDITIONAL, op_name, cond, true, 0};
    }
    
    bool should_break(const DebugContext& context) {
        if (!enabled) return false;
        
        switch (type) {
            case OPERATOR:
                return context.operator_name == target;
            case PATH:
                return context.execution_path == target;
            case CONDITIONAL:
                return context.operator_name == target && condition && condition(context);
        }
        return false;
    }
};

/**
 * Working debugger implementation
 */
class Debugger {
private:
    LogLevel log_level_ = LogLevel::INFO;
    bool execution_trace_enabled_ = false;
    bool performance_profiling_enabled_ = false;
    bool variable_tracking_enabled_ = false;
    bool interactive_mode_ = false;
    
    std::vector<Breakpoint> breakpoints_;
    std::vector<std::string> watched_variables_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point execution_start_;
    std::map<std::string, OperatorStats> operator_stats_;
    std::chrono::duration<double, std::milli> slow_operation_threshold_{10};
    
    // Execution state
    std::vector<std::string> execution_log_;
    std::map<std::string, std::vector<nlohmann::json>> variable_history_;
    std::vector<std::string> call_stack_;
    
    // Current debugging session state
    bool in_breakpoint_ = false;
    bool step_mode_ = false;
    
public:
    // Configuration methods
    Debugger& set_log_level(LogLevel level) {
        log_level_ = level;
        return *this;
    }
    
    Debugger& enable_execution_trace(bool enabled = true) {
        execution_trace_enabled_ = enabled;
        return *this;
    }
    
    Debugger& enable_performance_profiling(bool enabled = true) {
        performance_profiling_enabled_ = enabled;
        return *this;
    }
    
    Debugger& enable_variable_tracking(bool enabled = true) {
        variable_tracking_enabled_ = enabled;
        return *this;
    }
    
    Debugger& set_interactive_mode(bool enabled = true) {
        interactive_mode_ = enabled;
        return *this;
    }
    
    Debugger& set_slow_operation_threshold(std::chrono::duration<double, std::milli> threshold) {
        slow_operation_threshold_ = threshold;
        return *this;
    }
    
    // Breakpoint methods
    Debugger& set_breakpoint_on_operator(const std::string& operator_name) {
        breakpoints_.push_back(Breakpoint::on_operator(operator_name));
        return *this;
    }
    
    Debugger& set_breakpoint_on_path(const std::string& path) {
        breakpoints_.push_back(Breakpoint::on_path(path));
        return *this;
    }
    
    Debugger& set_conditional_breakpoint(const std::string& operator_name,
                                        std::function<bool(const DebugContext&)> condition) {
        breakpoints_.push_back(Breakpoint::conditional(operator_name, condition));
        return *this;
    }
    
    // Variable watching
    Debugger& add_variable_watch(const std::string& variable_name) {
        watched_variables_.push_back(variable_name);
        enable_variable_tracking();
        return *this;
    }
    
    // Core debugging hooks (called by evaluate() function)
    void on_operator_enter(const DebugContext& context) {
        call_stack_.push_back(context.execution_path);
        
        if (execution_trace_enabled_) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - execution_start_);
            
            std::ostringstream ss;
            ss << "Operator '" << context.operator_name << "' at '" << context.execution_path << "'";
            if (context.arguments.is_array() && !context.arguments.empty()) {
                ss << " with " << context.arguments.size() << " argument(s)";
            }
            
            execution_log_.push_back("✓ TRACE [" + std::to_string(duration.count()) + "ms]: " + ss.str());
        }
        
        // Check breakpoints
        for (auto& bp : breakpoints_) {
            if (bp.should_break(context)) {
                bp.hit_count++;
                if (interactive_mode_) {
                    handle_breakpoint(context, bp);
                } else {
                    std::cerr << "BREAKPOINT: " << context.operator_name << " at " << context.execution_path << std::endl;
                }
            }
        }
        
        // Watch variables
        if (variable_tracking_enabled_) {
            for (const auto& var_name : watched_variables_) {
                if (context.has_variable(var_name)) {
                    auto value = context.get_variable(var_name);
                    variable_history_[var_name].push_back(value);
                    
                    if (execution_trace_enabled_) {
                        execution_log_.push_back("👁️  WATCH: '" + var_name + "' = " + value.dump());
                    }
                }
            }
        }
    }
    
    void on_operator_exit(const DebugContext& context, const nlohmann::json& result, 
                         std::chrono::duration<double, std::milli> duration) {
        if (!call_stack_.empty()) {
            call_stack_.pop_back();
        }
        
        // Performance tracking
        if (performance_profiling_enabled_) {
            operator_stats_[context.operator_name].add_execution(duration);
            
            if (duration > slow_operation_threshold_) {
                execution_log_.push_back("🐌 SLOW: " + context.operator_name + " took " + 
                                       std::to_string(duration.count()) + "ms");
            }
        }
        
        if (execution_trace_enabled_ && log_level_ >= LogLevel::VERBOSE) {
            auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - execution_start_);
            
            std::string result_preview = result.dump();
            if (result_preview.length() > 50) {
                result_preview = result_preview.substr(0, 47) + "...";
            }
            
            execution_log_.push_back("  └─ Result [" + std::to_string(total_duration.count()) + "ms]: " + result_preview);
        }
    }
    
    void on_error(const std::exception& error, const DebugContext& context) {
        std::ostringstream ss;
        ss << "ERROR in '" << context.operator_name << "' at '" << context.execution_path << "': " << error.what();
        execution_log_.push_back("❌ " + ss.str());
        
        if (!call_stack_.empty()) {
            execution_log_.push_back("📍 Call stack: " + join_call_stack());
        }
        
        if (context.arguments.is_array() && !context.arguments.empty()) {
            execution_log_.push_back("📝 Arguments: " + context.arguments.dump());
        }
        
        if (!context.variables_in_scope.empty()) {
            execution_log_.push_back("📊 Variables in scope:");
            for (const auto& [name, value] : context.variables_in_scope) {
                std::string value_str = value.dump();
                if (value_str.length() > 100) value_str = value_str.substr(0, 97) + "...";
                execution_log_.push_back("   " + name + ": " + value_str);
            }
        }
    }
    
    // Interactive debugging
    void handle_breakpoint(const DebugContext& context, const Breakpoint& bp) {
        in_breakpoint_ = true;
        
        std::cerr << "\n🛑 BREAKPOINT: " << context.operator_name << " at " << context.execution_path << std::endl;
        std::cerr << "Hit count: " << bp.hit_count << std::endl;
        
        if (context.arguments.is_array() && !context.arguments.empty()) {
            std::cerr << "Arguments: " << context.arguments.dump(2) << std::endl;
        }
        
        if (!context.variables_in_scope.empty()) {
            std::cerr << "\nVariables in scope:" << std::endl;
            for (const auto& [name, value] : context.variables_in_scope) {
                std::cerr << "  " << name << ": " << value.dump() << std::endl;
            }
        }
        
        interactive_session(context);
        in_breakpoint_ = false;
    }
    
    void interactive_session(const DebugContext& context) {
        std::string command;
        
        std::cerr << "\nDebug Commands:" << std::endl;
        std::cerr << "  (c)ontinue  (s)tep  (i)nspect <var>  (e)val <expr>  (h)elp  (q)uit" << std::endl;
        
        while (true) {
            std::cerr << "> ";
            std::getline(std::cin, command);
            
            if (command.empty()) continue;
            
            char cmd = command[0];
            switch (cmd) {
                case 'c':
                    std::cerr << "Continuing execution..." << std::endl;
                    return;
                    
                case 's':
                    std::cerr << "Step mode enabled" << std::endl;
                    step_mode_ = true;
                    return;
                    
                case 'i': {
                    if (command.length() > 2) {
                        std::string var_name = command.substr(2);
                        if (context.has_variable(var_name)) {
                            std::cerr << var_name << " = " << context.get_variable(var_name).dump(2) << std::endl;
                        } else {
                            std::cerr << "Variable '" << var_name << "' not found" << std::endl;
                        }
                    } else {
                        std::cerr << "Usage: i <variable_name>" << std::endl;
                    }
                    break;
                }
                
                case 'e':
                    std::cerr << "Expression evaluation not yet implemented" << std::endl;
                    break;
                    
                case 'h':
                    std::cerr << "Available commands:" << std::endl;
                    std::cerr << "  c - Continue execution" << std::endl;
                    std::cerr << "  s - Step to next operation" << std::endl;
                    std::cerr << "  i <var> - Inspect variable" << std::endl;
                    std::cerr << "  e <expr> - Evaluate expression (future)" << std::endl;
                    std::cerr << "  h - Show this help" << std::endl;
                    std::cerr << "  q - Quit debugging session" << std::endl;
                    break;
                    
                case 'q':
                    std::cerr << "Exiting debugger." << std::endl;
                    std::exit(0);
                    
                default:
                    std::cerr << "Unknown command. Type 'h' for help." << std::endl;
            }
        }
    }
    
    // Reporting
    std::string get_execution_report() const {
        std::ostringstream ss;
        for (const auto& entry : execution_log_) {
            ss << entry << "\n";
        }
        return ss.str();
    }
    
    std::string get_performance_report() const {
        if (!performance_profiling_enabled_) return "";
        
        std::ostringstream ss;
        ss << "COMPUTO PERFORMANCE PROFILE\n";
        ss << "===========================\n";
        
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - execution_start_);
        ss << "Total execution time: " << total_time.count() << "ms\n\n";
        
        if (!operator_stats_.empty()) {
            ss << "Operator Performance:\n";
            
            // Sort by total time
            std::vector<std::pair<std::string, OperatorStats>> sorted_stats(
                operator_stats_.begin(), operator_stats_.end());
            std::sort(sorted_stats.begin(), sorted_stats.end(),
                [](const auto& a, const auto& b) {
                    return a.second.total_time > b.second.total_time;
                });
            
            for (const auto& [name, stats] : sorted_stats) {
                double percentage = total_time.count() > 0 ? 
                    (stats.total_time.count() / total_time.count()) * 100 : 0;
                
                ss << "  " << name << "  " << stats.total_time.count() << "ms (" 
                   << std::fixed << std::setprecision(1) << percentage << "%)  "
                   << "[" << stats.call_count << " calls, avg: " 
                   << stats.average_time().count() << "ms]\n";
            }
            ss << "\n";
        }
        
        // Report slow operations
        auto slow_ops = get_slow_operations();
        if (!slow_ops.empty()) {
            ss << "🐌 Slow Operations (>" << slow_operation_threshold_.count() << "ms threshold):\n";
            for (const auto& op : slow_ops) {
                ss << "  " << op << "\n";
            }
            ss << "\n";
        }
        
        return ss.str();
    }
    
    void start_execution() {
        execution_start_ = std::chrono::high_resolution_clock::now();
        call_stack_.clear();
        execution_log_.clear();
        operator_stats_.clear();
        variable_history_.clear();
    }
    
    // State queries
    bool is_tracing_enabled() const { return execution_trace_enabled_; }
    bool is_profiling_enabled() const { return performance_profiling_enabled_; }
    bool is_interactive_enabled() const { return interactive_mode_; }
    LogLevel get_log_level() const { return log_level_; }
    
private:
    std::string join_call_stack() const {
        std::ostringstream ss;
        for (size_t i = 0; i < call_stack_.size(); ++i) {
            if (i > 0) ss << " -> ";
            ss << call_stack_[i];
        }
        return ss.str();
    }
    
    std::vector<std::string> get_slow_operations() const {
        std::vector<std::string> slow_ops;
        for (const auto& [name, stats] : operator_stats_) {
            if (stats.max_time > slow_operation_threshold_) {
                slow_ops.push_back(name + ": " + std::to_string(stats.max_time.count()) + "ms");
            }
        }
        return slow_ops;
    }
};

} // namespace computo 