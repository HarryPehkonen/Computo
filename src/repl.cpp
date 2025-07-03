#include <computo/repl.hpp>
#include <iostream>
#include <regex>
#include <memory>

namespace computo {

ComputoREPL::ComputoREPL(Debugger* debugger) 
    : context_(nlohmann::json(nullptr)), debugger_(debugger) {}

std::string ComputoREPL::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

nlohmann::json ComputoREPL::parse_repl_expression(const std::string& input) {
    std::string trimmed = trim(input);
    
    if (trimmed.empty()) {
        return nlohmann::json(nullptr);
    }
    
    // Handle history references
    if (trimmed == "_1" && !history_.empty()) {
        return history_.back();
    }
    if (trimmed == "_2" && history_.size() >= 2) {
        return history_[history_.size() - 2];
    }
    if (trimmed == "_3" && history_.size() >= 3) {
        return history_[history_.size() - 3];
    }
    
    // Handle simple variable references (no spaces, starts with letter)
    if (std::regex_match(trimmed, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
        return nlohmann::json::array({"$", "/" + trimmed});
    }
    
    // Handle simple arithmetic (basic cases)
    std::regex arithmetic_regex(R"(^([a-zA-Z_]\w*)\s*([\+\-\*\/])\s*(\d+)$)");
    std::smatch match;
    if (std::regex_match(trimmed, match, arithmetic_regex)) {
        std::string var = match[1].str();
        std::string op = match[2].str();
        int value = std::stoi(match[3].str());
        return nlohmann::json::array({op, nlohmann::json::array({"$", "/" + var}), value});
    }
    
    // Replace history references in the string before parsing
    std::string processed = trimmed;
    
    // Replace _1, _2, _3 with their actual values in the JSON string
    if (processed.find("_1") != std::string::npos && !history_.empty()) {
        std::string replacement = history_.back().dump();
        size_t pos = 0;
        while ((pos = processed.find("_1", pos)) != std::string::npos) {
            processed.replace(pos, 2, replacement);
            pos += replacement.length();
        }
    }
    if (processed.find("_2") != std::string::npos && history_.size() >= 2) {
        std::string replacement = history_[history_.size() - 2].dump();
        size_t pos = 0;
        while ((pos = processed.find("_2", pos)) != std::string::npos) {
            processed.replace(pos, 2, replacement);
            pos += replacement.length();
        }
    }
    if (processed.find("_3") != std::string::npos && history_.size() >= 3) {
        std::string replacement = history_[history_.size() - 3].dump();
        size_t pos = 0;
        while ((pos = processed.find("_3", pos)) != std::string::npos) {
            processed.replace(pos, 2, replacement);
            pos += replacement.length();
        }
    }
    
    // Try to parse as JSON
    try {
        return nlohmann::json::parse(processed);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Invalid expression: " + std::string(e.what()));
    }
}

void ComputoREPL::print_history_label(size_t index) {
    std::cout << "[" << index << "] ";
}

void ComputoREPL::print_help() {
    std::cout << "\nComputo REPL Commands:\n";
    std::cout << "  help, ?               Show this help message\n";
    std::cout << "  quit, exit            Exit the REPL\n";
    std::cout << "  history               Show evaluation history\n";
    std::cout << "  clear                 Clear history\n";
    std::cout << "  _1, _2, _3            Reference previous results\n\n";
    std::cout << "Expression Examples:\n";
    std::cout << "  42                    Literal number\n";
    std::cout << "  \"hello world\"         Literal string\n";
    std::cout << "  [1, 2, 3]             Literal array (use {\"array\": [1, 2, 3]} for evaluation)\n";
    std::cout << "  {\"name\": \"Alice\"}      JSON object\n";
    std::cout << "  [\"+\", 1, 2]           Addition operator\n";
    std::cout << "  [\"let\", [[\"x\", 5]], [\"*\", [\"$\", \"/x\"], 2]]  Let binding\n";
    std::cout << "  users                 Variable reference (becomes [\"$\", \"/users\"])\n";
    std::cout << "  count + 1             Simple arithmetic (becomes [\"+\", [\"$\", \"/count\"], 1])\n\n";
    std::cout << "Try pasting some JSON data and then working with it!\n";
}

void ComputoREPL::print_history() {
    if (history_.empty()) {
        std::cout << "No history available.\n";
        return;
    }
    
    std::cout << "\nEvaluation History:\n";
    for (size_t i = 0; i < history_.size(); ++i) {
        std::cout << "[" << (i + 1) << "] ";
        if (history_[i].is_string() && history_[i].get<std::string>().size() > 50) {
            std::cout << history_[i].dump().substr(0, 50) << "...\n";
        } else {
            std::cout << history_[i].dump() << "\n";
        }
    }
    std::cout << "\nUse _1 for most recent, _2 for second most recent, etc.\n";
}

void ComputoREPL::run() {
    std::cout << "Computo REPL v1.0 - Interactive JSON Processing\n";
    std::cout << "Type 'help' for commands, 'quit' to exit\n\n";
    
    std::string line;
    while (true) {
        std::cout << "computo> ";
        std::getline(std::cin, line);
        
        if (std::cin.eof()) {
            std::cout << "\nGoodbye!\n";
            break;
        }
        
        std::string trimmed = trim(line);
        
        if (trimmed.empty()) {
            continue;
        }
        
        // Handle commands
        if (trimmed == "quit" || trimmed == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        if (trimmed == "help" || trimmed == "?") {
            print_help();
            continue;
        }
        
        if (trimmed == "history") {
            print_history();
            continue;
        }
        
        if (trimmed == "clear") {
            history_.clear();
            std::cout << "History cleared.\n";
            continue;
        }
        
        // Evaluate expression
        try {
            auto expr = parse_repl_expression(trimmed);
            
            // Skip evaluation of null expressions
            if (expr.is_null()) {
                continue;
            }
            
            // Set debugger if available
            if (debugger_) {
                set_debugger(std::unique_ptr<Debugger>(debugger_));
            }
            
            auto result = evaluate(expr, context_);
            
            // Reset debugger (don't actually move it)
            if (debugger_) {
                set_debugger(nullptr);
            }
            
            // Add to history
            history_.push_back(result);
            
            // Print result with history label
            print_history_label(history_.size());
            
            // Pretty print result
            if (result.is_string()) {
                std::cout << result.dump() << std::endl;
            } else {
                std::cout << result.dump(2) << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            // Don't add errors to history
        }
    }
}

} // namespace computo 