#include "repl.hpp"
#include <computo.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#ifdef COMPUTO_USE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

namespace computo {

// --- File Utilities (used by REPL) ---

auto load_json_file(const std::string& filename, bool enable_comments) -> jsom::JsonDocument {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    try {
        if (enable_comments) {
            return jsom::parse_document(content, jsom::ParsePresets::Comments);
        } else {
            return jsom::parse_document(content);
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("JSON parse error in " + filename + ": " + e.what());
    }
}

auto load_input_files(const std::vector<std::string>& filenames, bool enable_comments)
    -> std::vector<jsom::JsonDocument> {
    std::vector<jsom::JsonDocument> inputs;
    inputs.reserve(filenames.size());

    for (const auto& filename : filenames) {
        inputs.push_back(load_json_file(filename, enable_comments));
    }

    return inputs;
}

// --- REPL Command Structure ---

enum class ReplCommandType {
    UNKNOWN,
    HELP,
    VARS,
    DEBUG_TOGGLE,
    TRACE_TOGGLE,
    HISTORY,
    CLEAR,
    QUIT,
    BREAK,
    NOBREAK,
    BREAKS,
    RUN,
    SET,
    // Debug mode commands
    STEP,
    CONTINUE,
    FINISH,
    WHERE,
    JSON_SCRIPT
};

struct ReplCommand {
    ReplCommandType type;
    std::vector<std::string> args;
    std::string raw_input;
};

class ReplCommandParser {
private:
    static const std::unordered_map<std::string, ReplCommandType> command_map;

public:
    static auto parse(const std::string& input) -> ReplCommand {
        ReplCommand cmd;
        cmd.raw_input = input;

        if (input.empty()) {
            cmd.type = ReplCommandType::UNKNOWN;
            return cmd;
        }

        // Check if it's a JSON script (starts with '[', '{', '"', digit, or boolean keywords)
        char first_char = input[0];
        if (first_char == '[' || first_char == '{' || first_char == '"' || std::isdigit(first_char)
            || input.find("true") == 0 || input.find("false") == 0 || input.find("null") == 0) {
            cmd.type = ReplCommandType::JSON_SCRIPT;
            return cmd;
        }

        // Parse command and arguments
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        std::string arg;
        while (iss >> arg) {
            cmd.args.push_back(arg);
        }

        // Look up command type from static map
        auto it = command_map.find(command);
        cmd.type = (it != command_map.end()) ? it->second : ReplCommandType::UNKNOWN;

        return cmd;
    }
};

// Static command mapping table
const std::unordered_map<std::string, ReplCommandType> ReplCommandParser::command_map = {
    {"help", ReplCommandType::HELP},
    {"vars", ReplCommandType::VARS},
    {"debug", ReplCommandType::DEBUG_TOGGLE},
    {"trace", ReplCommandType::TRACE_TOGGLE},
    {"history", ReplCommandType::HISTORY},
    {"clear", ReplCommandType::CLEAR},
    {"quit", ReplCommandType::QUIT},
    {"exit", ReplCommandType::QUIT},
    {"break", ReplCommandType::BREAK},
    {"nobreak", ReplCommandType::NOBREAK},
    {"breaks", ReplCommandType::BREAKS},
    {"run", ReplCommandType::RUN},
    {"set", ReplCommandType::SET},
    {"step", ReplCommandType::STEP},
    {"s", ReplCommandType::STEP},
    {"continue", ReplCommandType::CONTINUE},
    {"c", ReplCommandType::CONTINUE},
    {"finish", ReplCommandType::FINISH},
    {"f", ReplCommandType::FINISH},
    {"where", ReplCommandType::WHERE},
    {"w", ReplCommandType::WHERE}
};

// --- REPL Implementation ---

// Create a struct to hold REPL state
struct ReplState {
    std::vector<jsom::JsonDocument> inputs;
    computo::DebugContext debug_context;
    std::vector<std::string> command_history;
    bool in_debug_mode = false;
    std::map<std::string, jsom::JsonDocument> repl_variables;
    const ComputoArgs* args;
};

// Helper function to get input line
auto get_input_line(bool in_debug_mode, std::vector<std::string>& command_history) -> std::string {
    // Choose prompt based on debug mode
    const char* prompt = in_debug_mode ? "(debug) " : "computo> ";
    std::string line_str;

#ifdef COMPUTO_USE_READLINE
    char* line_c_str = readline(prompt);
    if (line_c_str == nullptr) { // EOF or Ctrl-D
        return ""; // Signal EOF
    }
    line_str = line_c_str;
    if (!line_str.empty()) {
        add_history(line_c_str);
        command_history.push_back(line_str);
    }
    free(line_c_str);
#else
    std::cout << prompt;
    if (!std::getline(std::cin, line_str)) {
        return ""; // Signal EOF
    }
    if (!line_str.empty()) {
        command_history.push_back(line_str);
    }
#endif
    return line_str;
}

// Create handler functions for each command
void handle_help_command(const ReplCommand& /*cmd*/, ReplState& /*state*/) {
    std::cout << R"(REPL Commands:
  help                     Show this help message
  vars                     Show variables in current scope
  debug                    Toggle debug mode
  trace                    Toggle trace mode
  history                  Show command history
  clear                    Clear command history
  quit, exit               Exit the REPL

Script Execution:
  run <file>               Execute JSON script file
  ["+", 1, 2]              Execute JSON expression directly

Variables:
  set <name> <value>       Set a REPL variable (e.g., set x 10)

Breakpoints:
  break <operator>         Break on operator (e.g., "break +")
  break <variable>         Break on variable access (e.g., "break /users")
  nobreak <target>         Remove specific breakpoint
  nobreak                  Remove all breakpoints
  breaks                   List all active breakpoints

Debug Mode (when at breakpoint):
  step, s                  Execute next operation
  continue, c              Continue until next breakpoint
  finish, f                Complete execution, ignore breakpoints
  where, w                 Show current execution location
  vars                     Show variables in current scope
)";
}

void handle_vars_command(const ReplCommand& /*cmd*/, ReplState& state) {
    std::cout << "Variables in current scope:\n";
    
    // Show REPL persistent variables
    if (!state.repl_variables.empty()) {
        std::cout << "  REPL variables:\n";
        for (const auto& [name, value] : state.repl_variables) {
            std::cout << "    " << name << ": " << value.to_json() << "\n";
        }
    }
    
    // Always show input variables
    std::cout << "  Input variables:\n";
    if (!state.inputs.empty()) {
        std::cout << "    $input: " << state.inputs[0].to_json() << "\n";
        if (state.inputs.size() > 1) {
            std::cout << "    $inputs: array of " << state.inputs.size() << " elements\n";
            for (size_t i = 0; i < state.inputs.size(); ++i) {
                std::cout << "      $inputs[" << i << "]: " << state.inputs[i].to_json() << "\n";
            }
        }
    } else {
        std::cout << "    $input: null (no input files loaded)\n";
        std::cout << "    $inputs: [] (empty array)\n";
    }
    
    // Show debug trace variables if available
    if (state.debug_context.is_debug_enabled() && state.debug_context.is_trace_enabled()) {
        auto trace = state.debug_context.get_execution_trace();
        if (!trace.empty()) {
            // Find the most recent step with variables
            const DebugStep* step_with_vars = nullptr;
            for (auto it = trace.rbegin(); it != trace.rend(); ++it) {
                if (!it->variables.empty()) {
                    step_with_vars = &(*it);
                    break;
                }
            }
            
            if (step_with_vars) {
                std::cout << "  Local variables from recent execution:\n";
                for (const auto& [name, value] : step_with_vars->variables) {
                    std::cout << "    " << name << ": " << value.to_json() << "\n";
                }
                std::cout << "    (from step: " << step_with_vars->operation 
                         << " at " << step_with_vars->location << ")\n";
            } else {
                std::cout << "  Local variables: (none in recent execution)\n";
            }
        }
    }
    
    // Show status message if debug/trace not enabled
    if (!state.debug_context.is_debug_enabled()) {
        std::cout << "  Note: Enable debug mode ('debug on') and trace mode ('trace on') to see execution variables\n";
    } else if (!state.debug_context.is_trace_enabled()) {
        std::cout << "  Note: Enable trace mode ('trace on') to see execution variables\n";
    }
}

void handle_debug_toggle_command(const ReplCommand& /*cmd*/, ReplState& state) {
    state.debug_context.set_debug_enabled(!state.debug_context.is_debug_enabled());
    std::cout << "Debug mode "
              << (state.debug_context.is_debug_enabled() ? "enabled" : "disabled") << "\n";
}

void handle_trace_toggle_command(const ReplCommand& /*cmd*/, ReplState& state) {
    state.debug_context.set_trace_enabled(!state.debug_context.is_trace_enabled());
    std::cout << "Trace mode "
              << (state.debug_context.is_trace_enabled() ? "enabled" : "disabled") << "\n";
}

void handle_history_command(const ReplCommand& /*cmd*/, ReplState& state) {
    std::cout << "Command history:\n";
    for (size_t i = 0; i < state.command_history.size(); ++i) {
        std::cout << "  " << (i + 1) << ": " << state.command_history[i] << "\n";
    }
}

void handle_clear_command(const ReplCommand& /*cmd*/, ReplState& state) {
    state.command_history.clear();
#ifdef COMPUTO_USE_READLINE
    clear_history();
#endif
    std::cout << "Command history cleared\n";
}

void handle_break_command(const ReplCommand& cmd, ReplState& state) {
    if (cmd.args.empty()) {
        std::cout << "Usage: break <operator|variable>\n";
        std::cout << "Examples: break +, break map, break /users\n";
    } else {
        const auto& target = cmd.args[0];
        if (target.find("/") == 0) {
            state.debug_context.set_variable_breakpoint(target);
            std::cout << "Set variable breakpoint: " << target << "\n";
        } else {
            state.debug_context.set_operator_breakpoint(target);
            std::cout << "Set operator breakpoint: " << target << "\n";
        }
    }
}

void handle_nobreak_command(const ReplCommand& cmd, ReplState& state) {
    if (cmd.args.empty()) {
        state.debug_context.clear_all_breakpoints();
        std::cout << "All breakpoints removed\n";
    } else {
        const auto& target = cmd.args[0];
        if (target.find("/") == 0) {
            state.debug_context.remove_variable_breakpoint(target);
            std::cout << "Removed variable breakpoint: " << target << "\n";
        } else {
            state.debug_context.remove_operator_breakpoint(target);
            std::cout << "Removed operator breakpoint: " << target << "\n";
        }
    }
}

void handle_breaks_command(const ReplCommand& /*cmd*/, ReplState& state) {
    auto op_breaks = state.debug_context.get_operator_breakpoints();
    auto var_breaks = state.debug_context.get_variable_breakpoints();

    if (op_breaks.empty() && var_breaks.empty()) {
        std::cout << "No active breakpoints\n";
    } else {
        std::cout << "Active breakpoints:\n";
        for (const auto& op : op_breaks) {
            std::cout << "  Operator: " << op << "\n";
        }
        for (const auto& var : var_breaks) {
            std::cout << "  Variable: " << var << "\n";
        }
    }
}

void handle_run_command(const ReplCommand& cmd, ReplState& state) {
    if (cmd.args.empty()) {
        std::cout << "Usage: run <script_file>\n";
    } else {
        try {
            auto script = load_json_file(cmd.args[0], state.args->enable_comments);
            
            // Create execution context with REPL variables
            computo::ExecutionContext ctx(state.inputs, state.args->array_key);
            auto ctx_with_vars = ctx.with_variables(state.repl_variables);
            auto result = computo::evaluate(script, ctx_with_vars, &state.debug_context);
            
            std::cout << result.to_json(true) << "\n";
        } catch (const computo::DebugBreakException& e) {
            std::cout << "\nBreakpoint hit: " << e.get_reason() << "\n";
            std::cout << "Location: " << e.get_location() << "\n";
            std::cout << "Use 'step', 'continue', or 'finish' to proceed\n";
            state.in_debug_mode = true;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            if (state.debug_context.is_debug_enabled()) {
                std::cout << "Entering debug mode due to error\n";
                state.in_debug_mode = true;
            }
        }
    }
}

void handle_json_script(const ReplCommand& cmd, ReplState& state) {
    try {
        auto script = jsom::parse_document(cmd.raw_input);
        
        // Create execution context with REPL variables
        computo::ExecutionContext ctx(state.inputs, state.args->array_key);
        auto ctx_with_vars = ctx.with_variables(state.repl_variables);
        auto result = computo::evaluate(script, ctx_with_vars, &state.debug_context);
        
        std::cout << result.to_json(true) << "\n";
    } catch (const computo::DebugBreakException& e) {
        std::cout << "\nBreakpoint hit: " << e.get_reason() << "\n";
        std::cout << "Location: " << e.get_location() << "\n";
        std::cout << "Use 'step', 'continue', or 'finish' to proceed\n";
        state.in_debug_mode = true;
    } catch (const std::runtime_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        if (state.debug_context.is_debug_enabled()) {
            std::cout << "Entering debug mode due to error\n";
            state.in_debug_mode = true;
        }
    }
}

void handle_set_command(const ReplCommand& cmd, ReplState& state) {
    if (cmd.args.size() < 2) {
        std::cout << "Usage: set <variable_name> <json_value>\n";
        std::cout << "Examples: set x 10, set name \"Alice\", set data {\"key\": \"value\"}\n";
    } else {
        const auto& var_name = cmd.args[0];
        
        // Join remaining args as JSON value
        std::string json_str;
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            if (i > 1) json_str += " ";
            json_str += cmd.args[i];
        }
        
        try {
            auto value = jsom::parse_document(json_str);
            state.repl_variables[var_name] = value;
            std::cout << "Set " << var_name << " = " << value.to_json() << "\n";
        } catch (const std::runtime_error& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
            std::cout << "Try: set " << var_name << " \"" << json_str << "\" (for strings)\n";
        }
    }
}

void handle_step_command(const ReplCommand& /*cmd*/, ReplState& state) {
    if (state.in_debug_mode) {
        state.debug_context.set_step_mode(true);
        std::cout << "Stepping to next operation...\n";
        state.in_debug_mode = false; // Exit debug mode temporarily
    } else {
        std::cout << "Not in debug mode. Use 'debug' to enable debugging.\n";
    }
}

void handle_continue_command(const ReplCommand& /*cmd*/, ReplState& state) {
    if (state.in_debug_mode) {
        state.debug_context.set_step_mode(false);
        std::cout << "Continuing execution...\n";
        state.in_debug_mode = false; // Exit debug mode temporarily
    } else {
        std::cout << "Not in debug mode.\n";
    }
}

void handle_finish_command(const ReplCommand& /*cmd*/, ReplState& state) {
    if (state.in_debug_mode) {
        state.debug_context.set_finish_mode(true);
        std::cout << "Finishing execution, ignoring breakpoints...\n";
        state.in_debug_mode = false; // Exit debug mode
    } else {
        std::cout << "Not in debug mode.\n";
    }
}

void handle_where_command(const ReplCommand& /*cmd*/, ReplState& state) {
    if (state.in_debug_mode) {
        std::cout << "Current location: " << state.debug_context.get_current_location()
                  << "\n";
    } else {
        std::cout << "Not in debug mode.\n";
    }
}

void handle_unknown_command(const ReplCommand& cmd, ReplState& /*state*/) {
    std::cout << "Unknown command: " << cmd.raw_input << "\n";
    std::cout << "Type 'help' for available commands\n";
}

// The new, clean main loop
auto run_repl_mode(const ComputoArgs& args) -> int {
    try {
        // Initialize state
        ReplState state;
        state.args = &args;
        
        // Load input files if provided
        state.inputs = load_input_files(args.input_files, args.enable_comments);
        if (!args.input_files.empty()) {
            std::cout << "Loaded " << state.inputs.size() << " input file(s)\n";
        }

        std::cout << "Computo REPL v1.0.0\n";
        std::cout << "Type 'help' for commands, 'quit' to exit\n";
        if (args.debug_mode) {
            std::cout << "Debug mode enabled\n";
            state.debug_context.set_debug_enabled(true);
        }
        std::cout << "\n";

        while (true) {
            auto line_str = get_input_line(state.in_debug_mode, state.command_history);
            if (line_str.empty()) {
                break; // EOF
            }

            // Parse command
            auto cmd = ReplCommandParser::parse(line_str);

            // Dispatch to handlers
            switch (cmd.type) {
            case ReplCommandType::QUIT:
                std::cout << "\nGoodbye!\n";
                return 0;
            case ReplCommandType::HELP:         handle_help_command(cmd, state); break;
            case ReplCommandType::VARS:         handle_vars_command(cmd, state); break;
            case ReplCommandType::DEBUG_TOGGLE: handle_debug_toggle_command(cmd, state); break;
            case ReplCommandType::TRACE_TOGGLE: handle_trace_toggle_command(cmd, state); break;
            case ReplCommandType::HISTORY:      handle_history_command(cmd, state); break;
            case ReplCommandType::CLEAR:        handle_clear_command(cmd, state); break;
            case ReplCommandType::BREAK:        handle_break_command(cmd, state); break;
            case ReplCommandType::NOBREAK:      handle_nobreak_command(cmd, state); break;
            case ReplCommandType::BREAKS:       handle_breaks_command(cmd, state); break;
            case ReplCommandType::RUN:          handle_run_command(cmd, state); break;
            case ReplCommandType::JSON_SCRIPT:  handle_json_script(cmd, state); break;
            case ReplCommandType::SET:          handle_set_command(cmd, state); break;
            case ReplCommandType::STEP:         handle_step_command(cmd, state); break;
            case ReplCommandType::CONTINUE:     handle_continue_command(cmd, state); break;
            case ReplCommandType::FINISH:       handle_finish_command(cmd, state); break;
            case ReplCommandType::WHERE:        handle_where_command(cmd, state); break;
            case ReplCommandType::UNKNOWN:
            default:                            handle_unknown_command(cmd, state); break;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0; // Should not reach here, but prevents compiler warning
}

} // namespace computo
