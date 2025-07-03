#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <computo/repl.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

using json = nlohmann::json;

// Use the real debugger (no more mocks!)
using LogLevel = computo::LogLevel;

void print_help(const char* program_name) {
    std::cout << "Computo - Safe, sandboxed, JSON-native data transformation engine\n\n";
    std::cout << "Usage: " << program_name << " [DEBUG_OPTIONS] [OPTIONS] <script.json> [input1.json [input2.json ...]]\n";
    std::cout << "       " << program_name << " --repl [DEBUG_OPTIONS] [OPTIONS]\n\n";
    
    std::cout << "Execution Modes:\n";
    std::cout << "  --repl                Start interactive REPL (no script file required)\n\n";
    
    std::cout << "Debugging Options (use BEFORE other options):\n";
    std::cout << "  --debug               Enable debugging output\n";
    std::cout << "  --debug-level=LEVEL   Set debug level (error|warning|info|debug|verbose)\n";
    std::cout << "  --trace               Enable execution tracing\n";
    std::cout << "  --profile             Enable performance profiling\n";
    std::cout << "  --interactive         Enable interactive debugging\n";
    std::cout << "  --break-on=OPERATOR   Set breakpoint on operator\n";
    std::cout << "  --watch=VARIABLE      Watch variable changes\n";
    std::cout << "  --slow-threshold=MS   Report operations slower than MS milliseconds\n\n";
    
    std::cout << "Basic Options:\n";
    std::cout << "  --help, -?            Show this help message\n";
    std::cout << "  --interpolation       Enable string interpolation in Permuto templates\n";
    std::cout << "  --diff                Generate a JSON patch between input and result\n";
    std::cout << "  --pretty=N            Pretty-print JSON with N spaces of indentation\n";
    std::cout << "  --comments            Allow comments in script files (/* */ and // style)\n\n";
    
    std::cout << "File Arguments:\n";
    std::cout << "  <script.json>         The Computo script to execute\n";
    std::cout << "  [input1.json ...]     Input data files (optional)\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  # Start REPL\n";
    std::cout << "  " << program_name << " --repl\n\n";
    std::cout << "  # REPL with debugging\n";
    std::cout << "  " << program_name << " --repl --debug --trace\n\n";
    std::cout << "  # Basic execution\n";
    std::cout << "  " << program_name << " transform.json data.json\n\n";
    std::cout << "  # Debug with tracing\n";
    std::cout << "  " << program_name << " --debug --trace transform.json data.json\n\n";
    std::cout << "  # Performance profiling\n";
    std::cout << "  " << program_name << " --debug --profile --slow-threshold=5 script.json data.json\n\n";
    std::cout << "  # Interactive debugging\n";
    std::cout << "  " << program_name << " --debug --interactive --break-on=map script.json data.json\n\n";
    std::cout << "  # Watch variables and profile\n";
    std::cout << "  " << program_name << " --debug --trace --profile --watch=users --pretty=2 script.json data.json\n\n";
    std::cout << "  # Version comparison (run twice with different Computo builds)\n";
    std::cout << "  " << program_name << " --debug --profile script.json large_data.json > profile_v1.txt\n";
    std::cout << "  " << program_name << " --debug --profile script.json large_data.json > profile_v2.txt\n";
    std::cout << "  diff profile_v1.txt profile_v2.txt  # Compare performance!\n";
}

LogLevel parse_log_level(const std::string& level_str) {
    if (level_str == "error") return LogLevel::ERROR;
    if (level_str == "warning") return LogLevel::WARNING;
    if (level_str == "info") return LogLevel::INFO;
    if (level_str == "debug") return LogLevel::DEBUG;
    if (level_str == "verbose") return LogLevel::VERBOSE;
    
    std::cerr << "Warning: Unknown debug level '" << level_str << "', using 'info'" << std::endl;
    return LogLevel::INFO;
}

std::string extract_value(const std::string& arg, const std::string& prefix) {
    if (arg.find(prefix) == 0) {
        return arg.substr(prefix.length());
    }
    return "";
}

// Enhanced error formatting with real debugging
void print_enhanced_error(const std::exception& e, computo::Debugger* debugger) {
    std::cerr << "\nERROR: EXECUTION FAILED\n";
    std::cerr << "===================\n\n";
    
    std::cerr << "ERROR: " << e.what() << "\n\n";
    
    if (debugger && debugger->is_tracing_enabled()) {
        std::cerr << "EXECUTION TRACE:\n";
        std::cerr << "===================\n";
        std::cerr << debugger->get_execution_report() << std::endl;
    }
    
    std::cerr << "DEBUGGING TIPS:\n";
    std::cerr << "   1. Use --trace to see execution flow\n";
    std::cerr << "   2. Use --debug-level=verbose for more details\n";
    std::cerr << "   3. Check your JSON syntax and field names\n";
    std::cerr << "   4. Verify input data structure matches script expectations\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    // Debugging options  
    bool debug_enabled = false;
    std::unique_ptr<computo::Debugger> debugger = nullptr;
    
    // Basic options
    bool enable_interpolation = false;
    bool diff_mode = false;
    bool allow_comments = false;
    bool repl_mode = false;
    int pretty_indent = -1;
    
    // Parse arguments - DEBUG OPTIONS FIRST, then other options, then files
    std::vector<std::string> file_args;
    bool parsing_options = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (!parsing_options) {
            // We're in file arguments section
            file_args.push_back(arg);
            continue;
        }
        
        if (arg == "--help" || arg == "-?") {
            print_help(argv[0]);
            return 0;
        }
        // EXECUTION MODES
        else if (arg == "--repl") {
            repl_mode = true;
        }
        // DEBUG OPTIONS (parsed first)
        else if (arg == "--debug") {
            debug_enabled = true;
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
        }
        else if (arg == "--trace") {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            debugger->enable_execution_trace(true);
            debug_enabled = true;
        }
        else if (arg == "--profile") {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            debugger->enable_performance_profiling(true);
            debug_enabled = true;
        }
        else if (arg == "--interactive") {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            debugger->set_interactive_mode(true);
            debug_enabled = true;
        }
        else if (arg.find("--debug-level=") == 0) {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            std::string level_str = extract_value(arg, "--debug-level=");
            debugger->set_log_level(parse_log_level(level_str));
            debug_enabled = true;
        }
        else if (arg.find("--break-on=") == 0) {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            std::string op = extract_value(arg, "--break-on=");
            debugger->set_breakpoint_on_operator(op);
            debug_enabled = true;
        }
        else if (arg.find("--watch=") == 0) {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            std::string var = extract_value(arg, "--watch=");
            debugger->add_variable_watch(var);
            debug_enabled = true;
        }
        else if (arg.find("--slow-threshold=") == 0) {
            if (!debugger) {
                debugger = std::make_unique<computo::Debugger>();
            }
            std::string threshold_str = extract_value(arg, "--slow-threshold=");
            try {
                int threshold_ms = std::stoi(threshold_str);
                if (threshold_ms < 0) {
                    std::cerr << "Error: --slow-threshold must be >= 0" << std::endl;
                    return 1;
                }
                debugger->set_slow_operation_threshold(std::chrono::milliseconds(threshold_ms));
                debugger->enable_performance_profiling(true);
                debug_enabled = true;
            } catch (const std::exception&) {
                std::cerr << "Error: Invalid --slow-threshold value: " << threshold_str << std::endl;
                return 1;
            }
        }
        // BASIC OPTIONS
        else if (arg == "--interpolation") {
            enable_interpolation = true;
        }
        else if (arg == "--diff") {
            diff_mode = true;
        }
        else if (arg == "--comments") {
            allow_comments = true;
        }
        else if (arg.find("--pretty=") == 0) {
            std::string indent_str = extract_value(arg, "--pretty=");
            try {
                pretty_indent = std::stoi(indent_str);
                if (pretty_indent < 0) {
                    std::cerr << "Error: --pretty indent must be >= 0" << std::endl;
                    return 1;
                }
            } catch (const std::exception&) {
                std::cerr << "Error: Invalid --pretty value: " << indent_str << std::endl;
                return 1;
            }
        }
        // FILE ARGUMENTS (anything that doesn't start with --)
        else if (arg.find("--") != 0) {
            // This is the start of file arguments
            parsing_options = false;
            file_args.push_back(arg);
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return 1;
        }
    }
    
    // Handle REPL mode
    if (repl_mode) {
        if (debug_enabled && debugger) {
            std::cerr << "DEBUG: Debug mode enabled";
            if (debugger->is_tracing_enabled()) std::cerr << " [TRACE]";
            if (debugger->is_profiling_enabled()) std::cerr << " [PROFILE]";
            if (debugger->is_interactive_enabled()) std::cerr << " [INTERACTIVE]";
            std::cerr << "\n\n";
        }
        
        computo::ComputoREPL repl(debugger.get());
        repl.run();
        return 0;
    }
    
    if (file_args.empty()) {
        std::cerr << "Error: Script file not specified" << std::endl;
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] <script.json> [input1.json ...]" << std::endl;
        std::cerr << "       " << argv[0] << " --repl [OPTIONS]" << std::endl;
        return 1;
    }
    
    std::string script_file = file_args[0];
    std::vector<std::string> input_files(file_args.begin() + 1, file_args.end());
    
    // Validate combinations
    if (diff_mode && input_files.size() != 1) {
        std::cerr << "Error: --diff mode requires exactly one input file" << std::endl;
        return 1;
    }
    
    try {
        // Read script file
        std::ifstream script_stream(script_file);
        if (!script_stream.is_open()) {
            std::cerr << "Error: Cannot open script file: " << script_file << std::endl;
            return 1;
        }
        json script;
        if (allow_comments) {
            script = json::parse(script_stream, nullptr, true, true);
        } else {
            script_stream >> script;
        }
        script_stream.close();
        
        // Read input files
        std::vector<json> inputs;
        for (const auto& input_file : input_files) {
            std::ifstream input_stream(input_file);
            if (!input_stream.is_open()) {
                std::cerr << "Error: Cannot open input file: " << input_file << std::endl;
                return 1;
            }
            json input;
            input_stream >> input;
            input_stream.close();
            inputs.push_back(input);
        }
        
        // Show debug configuration and set up the debugger
        if (debug_enabled && debugger) {
            std::cerr << "DEBUG: Debug mode enabled";
            if (debugger->is_tracing_enabled()) std::cerr << " [TRACE]";
            if (debugger->is_profiling_enabled()) std::cerr << " [PROFILE]";
            if (debugger->is_interactive_enabled()) std::cerr << " [INTERACTIVE]";
            std::cerr << "\n\n";
            
            // Set the global debugger for evaluate() to use
            computo::set_debugger(std::move(debugger));
        }
        
        // Execute with timing
        auto start_time = std::chrono::high_resolution_clock::now();
        
        json result;
        try {
            // Execute script with full debugging support
            if (enable_interpolation) {
                permuto::Options opts;
                opts.enable_interpolation = true;
                
                if (inputs.empty()) {
                    result = computo::execute(script, json(nullptr), opts);
                } else {
                    result = computo::execute(script, inputs, opts);
                }
            } else {
                if (inputs.empty()) {
                    result = computo::execute(script, json(nullptr));
                } else {
                    result = computo::execute(script, inputs);
                }
            }
            
        } catch (const computo::ComputoException& e) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            if (debug_enabled) {
                print_enhanced_error(e, computo::get_debugger());
                
                if (computo::get_debugger() && computo::get_debugger()->is_profiling_enabled()) {
                    std::cerr << "\nPERFORMANCE ANALYSIS:\n";
                    std::cerr << "=========================\n";
                    std::cerr << "Failed after: " << duration.count() << "ms\n";
                    std::cerr << computo::get_debugger()->get_performance_report() << std::endl;
                }
            } else {
                std::cerr << "Computo error: " << e.what() << std::endl;
                std::cerr << "Tip: Use --debug --trace for detailed execution information" << std::endl;
            }
            return 1;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Debug output after successful execution
        if (debug_enabled) {
            std::cerr << "SUCCESS: EXECUTION SUCCESSFUL in " << duration.count() << "ms\n";
            std::cerr << "==========================================\n";
            
            auto* current_debugger = computo::get_debugger();
            if (current_debugger && current_debugger->is_tracing_enabled()) {
                std::cerr << "\nEXECUTION TRACE:\n";
                std::cerr << "===================\n";
                std::cerr << current_debugger->get_execution_report() << std::endl;
            }
            
            if (current_debugger && current_debugger->is_profiling_enabled()) {
                std::cerr << "\nPERFORMANCE PROFILE:\n";
                std::cerr << "========================\n";
                std::cerr << current_debugger->get_performance_report() << std::endl;
            }
            
            std::cerr << "\nRESULT:\n";
            std::cerr << "==========\n";
        }
        
        // Output result
        if (diff_mode) {
            json patch = nlohmann::json::diff(inputs[0], result);
            if (pretty_indent >= 0) {
                std::cout << patch.dump(pretty_indent) << std::endl;
            } else {
                std::cout << patch.dump() << std::endl;
            }
        } else {
            if (pretty_indent >= 0) {
                std::cout << result.dump(pretty_indent) << std::endl;
            } else {
                std::cout << result.dump() << std::endl;
            }
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error in " << script_file << ": " << e.what() << std::endl;
        if (debug_enabled) {
            std::cerr << "\nJSON DEBUGGING TIPS:\n";
            std::cerr << "   1. Validate JSON syntax with: python3 -m json.tool " << script_file << "\n";
            std::cerr << "   2. Use --comments flag if your JSON contains comments\n";
            std::cerr << "   3. Check for trailing commas, unquoted strings, etc.\n";
        }
        return 1;
    } catch (const std::exception& e) {
        if (debug_enabled) {
            std::cerr << "\nERROR: UNEXPECTED ERROR\n";
            std::cerr << "===================\n";
            std::cerr << "Error: " << e.what() << std::endl;
            std::cerr << "\nPlease report this as a bug with your script and input files.\n";
        } else {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 1;
    }
    
    return 0;
}