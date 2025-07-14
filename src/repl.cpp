#include "read_json.hpp"
#include <chrono>
#include <climits>
#include <computo.hpp>
#include <computo_version.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <readline/history.h>
#include <readline/readline.h>
#include <set>
#include <string>
#include <vector>

// Forward declaration for benchmark function
void run_performance_benchmarks();

// Forward declaration for computo::evaluate (needed by DebugExecutionWrapper)
namespace computo {
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx);
}

#ifdef REPL
// Forward declaration
class DebugExecutionWrapper;

thread_local computo::EvaluationContext* g_current_debug_context = nullptr;
thread_local DebugExecutionWrapper* g_current_debug_wrapper = nullptr;
computo::EvaluationAction debug_pre_evaluation_hook(const computo::EvaluationContext& eval_ctx);
#endif

// Levenshtein distance algorithm for operator suggestions
int levenshtein_distance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (size_t i = 0; i <= len1; ++i)
        dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j)
        dp[0][j] = j;

    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] });
            }
        }
    }
    return dp[len1][len2];
}

// Suggest closest operator name
std::string suggest_operator(const std::string& misspelled) {
    static auto operators = computo::get_available_operators();
    int min_distance = INT_MAX;
    std::string best_match;

    for (const auto& op : operators) {
        int distance = levenshtein_distance(misspelled, op);
        if (distance < min_distance && distance <= 2) { // Max 2 edits
            min_distance = distance;
            best_match = op;
        }
    }
    return best_match;
}

// namespace for readline functions
namespace ReadLine {
inline void add_history(const char* str) { ::add_history(str); }
inline char* readline(const char* prompt) { return ::readline(prompt); }
inline void using_history() { ::using_history(); }
inline void clear_history() { ::clear_history(); }
}

// Debug session states
enum class DebugState {
    RUNNING, // Normal execution
    PAUSED, // Hit breakpoint, awaiting user command
    STEPPING, // Single-step mode
    FINISHED // Script completed
};

// Execution context for debugging
struct DebugContext {
    std::vector<std::string> execution_path;
    std::string current_operator;
    nlohmann::json current_expression;
    std::map<std::string, nlohmann::json> current_variables;
    int depth = 0;
};

class DebugExecutionWrapper {
private:
    bool trace_enabled_ = false;
    bool step_mode_ = false;
    bool profiling_enabled_ = false;
    DebugState debug_state_ = DebugState::RUNNING;

    // Breakpoint management
    std::set<std::string> operator_breakpoints_;
    std::set<std::string> variable_breakpoints_;

    // Captured state
    std::map<std::string, nlohmann::json> last_variables_;
    std::vector<std::string> execution_trace_;
    std::map<std::string, std::chrono::nanoseconds> operator_timings_;
    std::string current_script_filename_;

    // Interactive debugging state
    DebugContext current_debug_context_;
    nlohmann::json current_script_;
    std::vector<nlohmann::json> current_inputs_;
    bool debug_session_active_ = false;

    // User interaction callback (set by REPL)
    std::function<std::string()> user_input_callback_;

    void print_debug_location() {
        if (!g_current_debug_context) {
            std::cout << "No active debug session\n";
            return;
        }

        std::cout << "Location: ";
        if (g_current_debug_context->execution_path.empty()) {
            std::cout << "/";
        } else {
            for (const auto& segment : g_current_debug_context->execution_path) {
                std::cout << "/" << segment;
            }
        }
        std::cout << "\n";

        std::cout << "Operator: " << g_current_debug_context->operator_name << "\n";
        std::cout << "Expression: " << g_current_debug_context->expression.dump() << "\n";
        std::cout << "Depth: " << g_current_debug_context->depth << "\n";
    }

    void print_debug_variables() {
        if (!g_current_debug_context) {
            std::cout << "No active debug session\n";
            return;
        }

        if (g_current_debug_context->variables.empty()) {
            std::cout << "No variables in scope\n";
        } else {
            std::cout << "Variables in scope:\n";
            for (const auto& [name, value] : g_current_debug_context->variables) {
                std::cout << "  " << name << " = " << value.dump() << "\n";
            }
        }
    }

public:
    // Configuration methods
    void enable_trace(bool enabled) { trace_enabled_ = enabled; }
    void enable_step_mode(bool enabled) { step_mode_ = enabled; }
    void enable_profiling(bool enabled) { profiling_enabled_ = enabled; }

    // Breakpoint management
    void add_operator_breakpoint(const std::string& op) {
        operator_breakpoints_.insert(op);
    }
    void add_variable_breakpoint(const std::string& var) {
        variable_breakpoints_.insert(var);
    }
    void remove_operator_breakpoint(const std::string& op) {
        operator_breakpoints_.erase(op);
    }
    void remove_variable_breakpoint(const std::string& var) {
        variable_breakpoints_.erase(var);
    }
    void clear_all_breakpoints() {
        operator_breakpoints_.clear();
        variable_breakpoints_.clear();
    }

    // State access
    DebugState get_debug_state() const { return debug_state_; }
    void set_debug_state(DebugState state) { debug_state_ = state; }
    const std::set<std::string>& get_operator_breakpoints() const { return operator_breakpoints_; }
    const std::set<std::string>& get_variable_breakpoints() const { return variable_breakpoints_; }

    // Interactive debugging
    void set_user_input_callback(std::function<std::string()> callback) {
        user_input_callback_ = callback;
    }
    bool is_debug_session_active() const { return debug_session_active_; }
    const DebugContext& get_debug_context() const { return current_debug_context_; }

    // Debug session control
    void debug_step() { debug_state_ = DebugState::STEPPING; }
    void debug_continue() { debug_state_ = DebugState::RUNNING; }
    void debug_finish() {
        debug_state_ = DebugState::RUNNING;
        operator_breakpoints_.clear();
        variable_breakpoints_.clear();
    }

public:
    // Interactive debug method that is called by the standalone hook
    void handle_interactive_debugging() {
        debug_state_ = DebugState::PAUSED;
        debug_session_active_ = true;

        // Enter debug mode
        while (debug_state_ == DebugState::PAUSED) {
            std::cout << "(debug) ";
            std::string user_command = user_input_callback_();

            if (user_command == "step") {
                debug_state_ = DebugState::STEPPING;
            } else if (user_command == "continue") {
                debug_state_ = DebugState::RUNNING;
            } else if (user_command == "finish") {
                debug_finish();
            } else if (user_command == "where") {
                print_debug_location();
            } else if (user_command == "vars") {
                print_debug_variables();
            } else {
                std::cout << "Debug commands: step, continue, finish, where, vars\n";
            }
        }

        debug_session_active_ = false;
    }

    // Execution with debugging
    nlohmann::json execute_with_debug(const nlohmann::json& script,
        const std::vector<nlohmann::json>& inputs) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Store current script and inputs for debugging
        current_script_ = script;
        current_inputs_ = inputs;
        debug_session_active_ = false;

        // Create execution context with pre-evaluation hook
        computo::ExecutionContext ctx(inputs);

        ctx.set_pre_evaluation_hook(debug_pre_evaluation_hook);
        g_current_debug_wrapper = this;

        // Execute with debugging
        nlohmann::json result;
        try {
            result = computo::evaluate(script, ctx);
            debug_state_ = DebugState::FINISHED;
        } catch (const std::exception& e) {
            debug_state_ = DebugState::FINISHED;
            throw;
        }

        g_current_debug_wrapper = nullptr;
        g_current_debug_context = nullptr;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        if (profiling_enabled_) {
            operator_timings_["total_execution"] = duration;
        }

        debug_session_active_ = false;
        return result;
    }

    // State access for commands
    const std::map<std::string, nlohmann::json>& get_last_variables() const {
        return last_variables_;
    }
    const std::vector<std::string>& get_execution_trace() const {
        return execution_trace_;
    }
    const std::map<std::string, std::chrono::nanoseconds>& get_timings() const {
        return operator_timings_;
    }

    // Run script from file
    nlohmann::json run_script_file(const std::string& filename, const std::vector<nlohmann::json>& inputs, bool allow_comments = false) {
        current_script_filename_ = filename;
        debug_state_ = DebugState::RUNNING;

        nlohmann::json script;
        try {
            script = read_json(filename, allow_comments);
        } catch (const std::exception& e) {
            debug_state_ = DebugState::FINISHED;
            throw std::runtime_error("Failed to load script: " + std::string(e.what()));
        }

        // Extract variables for debugging
        extract_variables_from_let(script, inputs);

        // Execute the script
        return execute_with_debug(script, inputs);
    }

    // Extract and evaluate variables from let expressions
    void extract_variables_from_let(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) {
        last_variables_.clear();
        if (script.is_array() && script.size() > 0 && script[0].is_string()) {
            std::string op = script[0].get<std::string>();
            if (op == "let" && script.size() == 3 && script[1].is_array()) {
                // Create execution context for evaluating bindings
                computo::ExecutionContext ctx(inputs);

                // Extract and evaluate variable bindings from let expression
                for (const auto& binding : script[1]) {
                    if (binding.is_array() && binding.size() == 2 && binding[0].is_string()) {
                        std::string var_name = binding[0].get<std::string>();
                        try {
                            // Evaluate the binding value in the current context
                            nlohmann::json evaluated_value = computo::evaluate(binding[1], ctx);
                            last_variables_[var_name] = evaluated_value;
                            // Add to context for subsequent bindings
                            ctx.variables[var_name] = evaluated_value;
                        } catch (const std::exception& e) {
                            // If evaluation fails, store the expression
                            last_variables_[var_name] = binding[1];
                        }
                    }
                }
            }
        }
    }
};

#ifdef REPL
computo::EvaluationAction debug_pre_evaluation_hook(const computo::EvaluationContext& eval_ctx) {
    g_current_debug_context = const_cast<computo::EvaluationContext*>(&eval_ctx);

    bool should_break = false;
    if (g_current_debug_wrapper) {
        const auto& op_breakpoints = g_current_debug_wrapper->get_operator_breakpoints();
        if (op_breakpoints.count(eval_ctx.operator_name)) {
            should_break = true;
        }

        if (g_current_debug_wrapper->get_debug_state() == DebugState::STEPPING) {
            should_break = true;
        }

        if (should_break) {
            std::cout << "\n*** Breakpoint hit ***\n";
            std::cout << "Location: ";
            if (eval_ctx.execution_path.empty()) {
                std::cout << "/";
            } else {
                for (const auto& segment : eval_ctx.execution_path) {
                    std::cout << "/" << segment;
                }
            }
            std::cout << "\n";
            std::cout << "Operator: " << eval_ctx.operator_name << "\n";
            std::cout << "Expression: " << eval_ctx.expression.dump() << "\n";
            std::cout << "Depth: " << eval_ctx.depth << "\n";

            g_current_debug_wrapper->handle_interactive_debugging();
        }
    }

    return computo::EvaluationAction::CONTINUE;
}

#endif

class ComputoREPL {
private:
    bool debug_mode_ = false;
    bool trace_mode_ = false;
    bool comments_enabled_ = false;
    std::vector<std::string> command_history_;
    std::vector<nlohmann::json> input_data_;
    DebugExecutionWrapper debug_wrapper_;

public:
    ComputoREPL() {
        // Enable history expansion
        rl_bind_key('\t', rl_complete);
        ReadLine::using_history();

        // Set up debug callback for user input
        debug_wrapper_.set_user_input_callback([]() -> std::string {
            char* line = ReadLine::readline("");
            if (!line)
                return "continue"; // EOF defaults to continue
            std::string input(line);
            free(line);
            return input;
        });
    }

    static void print_version() {
        std::cout << "Computo REPL v" << COMPUTO_VERSION << "\n"
                  << "JSON-native data transformation engine" << std::endl;
    }

    void run(std::vector<std::string> input_filenames, bool comments_enabled = false) {
        comments_enabled_ = comments_enabled;
        print_version();
        std::cout << "Type 'help' for commands, 'quit' to exit\n";
        std::cout << "Use _1, _2, etc. to reference previous commands\n\n";

        // Load input files and store for execution context
        std::vector<nlohmann::json> loaded_inputs;
        if (!input_filenames.empty()) {
            for (const auto& input_filename : input_filenames) {
                loaded_inputs.push_back(read_json(input_filename, false)); // Input files never allow comments
            }
        }

        // Store the loaded inputs for use in execute_expression
        input_data_ = std::move(loaded_inputs);

        while (true) {

            char* line = ReadLine::readline("computo> ");
            if (!line)
                break; // EOF (Ctrl+D)
            std::string input(line);
            free(line);

            if (input.empty())
                continue;

            // Process history expansion (_1, _2, etc.)
            std::string processed_input = process_history_expansion(input);

            ReadLine::add_history(processed_input.c_str());
            command_history_.push_back(processed_input);

            try {
                if (processed_input == "quit" || processed_input == "exit") {
                    break;
                } else if (processed_input == "help") {
                    print_help();
                } else if (processed_input == "version") {
                    print_version();
                } else if (processed_input == "debug") {
                    debug_mode_ = !debug_mode_;
                    std::cout << "Debug mode: " << (debug_mode_ ? "ON" : "OFF") << "\n";
                } else if (processed_input == "trace") {
                    trace_mode_ = !trace_mode_;
                    std::cout << "Trace mode: " << (trace_mode_ ? "ON" : "OFF") << "\n";
                } else if (processed_input == "clear") {
                    ReadLine::clear_history();
                    command_history_.clear();
                    std::cout << "History cleared\n";
                } else if (processed_input == "history") {
                    print_history();
                } else if (processed_input == "vars") {
                    print_variables();
                } else if (processed_input.size() > 4 && processed_input.substr(0, 4) == "run ") {
                    std::string filename = processed_input.substr(4);

                    // remove surrounding whitespace
                    filename = trim_whitespace(filename);

                    // remove quotes if present
                    if (filename.size() > 1 && filename[0] == '"' && filename[filename.size() - 1] == '"') {
                        filename = filename.substr(1, filename.size() - 2);
                    }

                    run_script_file(filename);
                } else if (processed_input.size() > 6 && processed_input.substr(0, 6) == "break ") {
                    std::string target = processed_input.substr(6);
                    add_breakpoint(target);
                } else if (processed_input.size() > 8 && processed_input.substr(0, 8) == "nobreak ") {
                    std::string target = processed_input.substr(8);
                    remove_breakpoint(target);
                } else if (processed_input == "nobreak") {
                    remove_all_breakpoints();
                } else if (processed_input == "breaks") {
                    list_breakpoints();
                } else if (processed_input == "continue") {
                    continue_execution();
                } else if (processed_input == "step") {
                    step_execution();
                } else if (processed_input == "finish") {
                    finish_execution();
                } else if (processed_input == "where") {
                    show_execution_location();
                } else {
                    execute_expression(processed_input);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        std::cout << "Goodbye!\n";
    }

private:
    std::string trim_whitespace(const std::string& str) {

        // spaces, tabs, newlines, etc.
        std::string whitespace = " \t\n\r\f\v";
        size_t first = str.find_first_not_of(whitespace);
        size_t last = str.find_last_not_of(whitespace);
        return first == std::string::npos ? "" : str.substr(first, last - first + 1);
    }

    std::string process_history_expansion(const std::string& input) {
        std::string result = input;

        // Simple history expansion: _N where N is a number
        size_t pos = 0;
        while ((pos = result.find("_", pos)) != std::string::npos) {
            if (pos > 0 && (std::isalnum(result[pos - 1]) || result[pos - 1] == '_')) {
                pos++;
                continue; // Not a history reference
            }

            size_t start = pos;
            pos++; // Skip the underscore

            // Find the number
            size_t num_start = pos;
            while (pos < result.size() && std::isdigit(result[pos])) {
                pos++;
            }

            if (pos == num_start) {
                continue; // No number after underscore
            }

            std::string num_str = result.substr(num_start, pos - num_start);
            int history_index = std::stoi(num_str) - 1;

            if (history_index >= 0 && history_index < static_cast<int>(command_history_.size())) {
                std::string replacement = command_history_[history_index];
                result.replace(start, pos - start, replacement);
                pos = start + replacement.size();
            } else {
                std::cerr << "Warning: History reference _" << history_index << " not found\n";
                pos = start + 1;
            }
        }

        return result;
    }

    void print_history() {
        if (command_history_.empty()) {
            std::cout << "No command history\n";
            return;
        }

        std::cout << "Command history:\n";
        for (size_t i = 0; i < command_history_.size(); ++i) {
            std::cout << "  " << (i + 1) << ": " << command_history_[i] << "\n";
        }
    }

    void print_help() {
        std::cout << "Commands:\n";
        std::cout << "  help         Show this help\n";
        std::cout << "  debug        Toggle debug mode\n";
        std::cout << "  trace        Toggle trace mode\n";
        std::cout << "  clear        Clear history\n";
        std::cout << "  history      Show command history\n";
        std::cout << "  vars         Show variables in scope\n";
        std::cout << "  quit         Exit REPL\n";
        std::cout << "\n";
        std::cout << "Script execution:\n";
        std::cout << "  run FILE     Load and execute script file\n";
        std::cout << "\n";
        std::cout << "Breakpoint management:\n";
        std::cout << "  break OP     Set breakpoint on operator (e.g., 'break map')\n";
        std::cout << "  break /VAR   Set breakpoint on variable (e.g., 'break /users')\n";
        std::cout << "  nobreak OP   Remove operator breakpoint\n";
        std::cout << "  nobreak /VAR Remove variable breakpoint\n";
        std::cout << "  nobreak      Remove all breakpoints\n";
        std::cout << "  breaks       List active breakpoints\n";
        std::cout << "\n";
        std::cout << "Debug session (when paused):\n";
        std::cout << "  step         Execute next operation\n";
        std::cout << "  continue     Continue until next breakpoint\n";
        std::cout << "  finish       Run to completion, ignoring breakpoints\n";
        std::cout << "  where        Show current execution location\n";
        std::cout << "\n";
        std::cout << "History expansion:\n";
        std::cout << "  _1           Reference the last command\n";
        std::cout << "  _2           Reference the second-to-last command\n";
        std::cout << "  etc.         Use _N to reference command N from the end\n";
        std::cout << "\n";
        std::cout << "Enter JSON expressions to evaluate.\n";
        std::cout << "Examples:\n";
        std::cout << "  [\"+\", 1, 2]\n";
        std::cout << "  [\"map\", {\"array\": [1, 2, 3]}, [\"lambda\", [\"x\"], [\"*\", [\"$\", \"/x\"], 2]]]\n";
        std::cout << "  run script.json\n";
        std::cout << "  break map\n";
    }

    void print_variables() {
        const auto& variables = debug_wrapper_.get_last_variables();
        if (variables.empty()) {
            std::cout << "No variables in scope\n";
            std::cout << "(Execute a 'let' expression to create variables)\n";
            return;
        }

        std::cout << "Variables in scope:\n";
        for (const auto& [name, value] : variables) {
            std::cout << "  " << name << " = " << value.dump() << "\n";
        }
    }

    void run_script_file(const std::string& filename) {
        try {
            // Check if file exists
            if (!std::filesystem::exists(filename)) {
                std::cerr << "Error: File not found: " << filename << "\n";
                return;
            }

            // Configure debug wrapper
            debug_wrapper_.enable_trace(trace_mode_);
            debug_wrapper_.enable_profiling(debug_mode_);

            // Run the script
            auto result = debug_wrapper_.run_script_file(filename, input_data_, comments_enabled_);

            // Output result
            std::cout << result.dump(2) << "\n";

        } catch (const computo::InvalidOperatorException& e) {
            std::cerr << "Error: " << e.what();

            // Try to suggest a similar operator
            std::string op_name = std::string(e.what()).substr(18); // Remove "Invalid operator: "
            std::string suggestion = suggest_operator(op_name);
            if (!suggestion.empty()) {
                std::cerr << ". Did you mean '" << suggestion << "'?";
            }
            std::cerr << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    void add_breakpoint(const std::string& target) {
        if (!target.empty() && target[0] == '/') {
            debug_wrapper_.add_variable_breakpoint(target);
            std::cout << "Added variable breakpoint: " << target << "\n";
        } else {
            debug_wrapper_.add_operator_breakpoint(target);
            std::cout << "Added operator breakpoint: " << target << "\n";
        }
    }

    void remove_breakpoint(const std::string& target) {
        if (!target.empty() && target[0] == '/') {
            debug_wrapper_.remove_variable_breakpoint(target);
            std::cout << "Removed variable breakpoint: " << target << "\n";
        } else {
            debug_wrapper_.remove_operator_breakpoint(target);
            std::cout << "Removed operator breakpoint: " << target << "\n";
        }
    }

    void remove_all_breakpoints() {
        debug_wrapper_.clear_all_breakpoints();
        std::cout << "All breakpoints cleared\n";
    }

    void list_breakpoints() {
        const auto& op_breaks = debug_wrapper_.get_operator_breakpoints();
        const auto& var_breaks = debug_wrapper_.get_variable_breakpoints();

        if (op_breaks.empty() && var_breaks.empty()) {
            std::cout << "No breakpoints set\n";
            return;
        }

        std::cout << "Active breakpoints:\n";
        for (const auto& op : op_breaks) {
            std::cout << "  Operator: " << op << "\n";
        }
        for (const auto& var : var_breaks) {
            std::cout << "  Variable: " << var << "\n";
        }
    }

    void continue_execution() {
        if (debug_wrapper_.is_debug_session_active()) {
            debug_wrapper_.debug_continue();
            std::cout << "Continuing execution...\n";
        } else {
            std::cout << "No active debug session\n";
        }
    }

    void step_execution() {
        if (debug_wrapper_.is_debug_session_active()) {
            debug_wrapper_.debug_step();
            std::cout << "Stepping to next operation...\n";
        } else {
            // Enable stepping for next execution
            debug_wrapper_.debug_step();
            std::cout << "Step mode enabled. Next script execution will step through.\n";
        }
    }

    void finish_execution() {
        if (debug_wrapper_.is_debug_session_active()) {
            debug_wrapper_.debug_finish();
            std::cout << "Finishing execution (clearing all breakpoints)...\n";
        } else {
            debug_wrapper_.debug_finish();
            std::cout << "All breakpoints cleared\n";
        }
    }

    void show_execution_location() {
        if (debug_wrapper_.is_debug_session_active()) {
            const auto& ctx = debug_wrapper_.get_debug_context();
            std::cout << "Current execution location:\n";
            std::cout << "  Operator: " << ctx.current_operator << "\n";
            std::cout << "  Expression: " << ctx.current_expression.dump() << "\n";
            std::cout << "  Depth: " << ctx.depth << "\n";
            std::cout << "  Path: ";
            if (ctx.execution_path.empty()) {
                std::cout << "/";
            } else {
                for (const auto& segment : ctx.execution_path) {
                    std::cout << "/" << segment;
                }
            }
            std::cout << "\n";
        } else {
            std::cout << "No active debug session\n";
        }
    }

    void execute_expression(const std::string& input) {
        // Parse JSON input
        nlohmann::json script;
        try {
            if (comments_enabled_) {
                script = nlohmann::json::parse(input, nullptr, true, true); // allow exceptions, allow comments
            } else {
                script = nlohmann::json::parse(input);
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid JSON: " << e.what() << "\n";
            return;
        }

        // Configure debug wrapper based on current modes
        debug_wrapper_.enable_trace(trace_mode_);
        debug_wrapper_.enable_profiling(debug_mode_);

        // Extract variables from let expressions for debugging
        debug_wrapper_.extract_variables_from_let(script, input_data_);

        // Execute with debug wrapper
        try {
            auto result = debug_wrapper_.execute_with_debug(script, input_data_);

            // Output result
            std::cout << result.dump(2) << "\n";

        } catch (const computo::InvalidOperatorException& e) {
            std::cerr << "Error: " << e.what();

            // Try to suggest a similar operator
            std::string op_name = std::string(e.what()).substr(18); // Remove "Invalid operator: "
            std::string suggestion = suggest_operator(op_name);
            if (!suggestion.empty()) {
                std::cerr << ". Did you mean '" << suggestion << "'?";
            }
            std::cerr << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
};

struct REPLOptions {
    bool help = false;
    bool version = false;
    bool perf = false;
    bool comments = false;
    std::string bad_option;
    std::vector<std::string> input_filenames;
};

void print_cmdline_help(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [INPUT_FILENAME [INPUT_FILENAME]...]\n"
              << "Options:\n"
              << "  -h, --help     Show this help message\n"
              << "  -v, --version  Show version information\n"
              << "  --comments     Allow comments in script files and direct input\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    REPLOptions options;

    // parse command line options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "-v" || arg == "--version") {
            options.version = true;
        } else if (arg == "-p" || arg == "--perf") {
            options.perf = true;
        } else if (arg == "--comments") {
            options.comments = true;
        } else if (arg[0] == '-') {
            options.bad_option = arg;
            break;
        } else {
            options.input_filenames.push_back(arg);
        }
    }

    if (!options.bad_option.empty()) {
        std::cerr << "Error: Invalid option: " << options.bad_option << std::endl;
        print_cmdline_help(argv[0]);
        return 1;
    }

    if (options.perf) {
        run_performance_benchmarks();
        return 0;
    }

    if (options.help) {
        print_cmdline_help(argv[0]);
        return 0;
    }

    if (options.version) {
        ComputoREPL::print_version();
        return 0;
    }

    // make sure filenames exist before running
    for (const auto& filename : options.input_filenames) {
        if (!std::filesystem::exists(filename)) {
            std::cerr << "Error: File not found: " << filename << std::endl;
            return 1;
        }
    }

    ComputoREPL repl;
    repl.run(options.input_filenames, options.comments);
    return 0;
}
