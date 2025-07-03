#pragma once

#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#ifdef COMPUTO_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace computo {

class ComputoREPL {
private:
    std::vector<nlohmann::json> history_;
    ExecutionContext context_;
    Debugger* debugger_;
    
    // Trim whitespace from string
    std::string trim(const std::string& str);
    
    // Parse REPL input with syntactic sugar support
    nlohmann::json parse_repl_expression(const std::string& input);
    
    // Print history label for results
    void print_history_label(size_t index);
    
    // Print help message
    void print_help();
    
    // Print evaluation history
    void print_history();
    
    // Get input with optional readline support for command history
    std::string get_input(const std::string& prompt);
    
public:
    explicit ComputoREPL(Debugger* debugger = nullptr);
    
    // Run the REPL loop
    void run();
};

} // namespace computo 