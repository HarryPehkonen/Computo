#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

void print_help(const char* program_name) {
    std::cout << "Computo - Safe, sandboxed, JSON-native data transformation engine\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] <script.json> [input1.json [input2.json ...]]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -?        Show this help message\n";
    std::cout << "  --interpolation   Enable string interpolation in Permuto templates\n";
    std::cout << "  --diff            Generate a JSON patch between input and result\n";
    std::cout << "  --pretty=N        Pretty-print JSON with N spaces of indentation\n";
    std::cout << "  --comments        Allow comments in script files (/* */ and // style)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " transform.json data.json\n";
    std::cout << "  " << program_name << " --pretty=2 script.json input1.json input2.json\n";
    std::cout << "  " << program_name << " --interpolation --diff transform.json data.json\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    bool enable_interpolation = false;
    bool diff_mode = false;
    bool allow_comments = false;
    int pretty_indent = -1;  // -1 means compact output
    int script_arg = 1;
    
    // Parse flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-?") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--interpolation") {
            enable_interpolation = true;
            script_arg = i + 1;
        } else if (arg == "--diff") {
            diff_mode = true;
            script_arg = i + 1;
        } else if (const std::string pretty_prefix = "--pretty="; arg.find(pretty_prefix) == 0) {
            std::string indent_str = arg.substr(pretty_prefix.length());
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
            script_arg = i + 1;
        } else if (arg == "--comments") {
            allow_comments = true;
            script_arg = i + 1;
        } else {
            // Found non-flag argument, this is the script
            script_arg = i;
            break;
        }
    }
    
    if (script_arg >= argc) {
        std::cerr << "Error: Script file not specified" << std::endl;
        return 1;
    }
    
    // Validate --diff usage
    if (diff_mode) {
        int num_inputs = argc - script_arg - 1;
        if (num_inputs != 1) {
            std::cerr << "Error: --diff mode requires exactly one input file" << std::endl;
            return 1;
        }
    }
    
    try {
        // Read script file
        std::ifstream script_file(argv[script_arg]);
        if (!script_file.is_open()) {
            std::cerr << "Error: Cannot open script file: " << argv[script_arg] << std::endl;
            return 1;
        }
        json script;
        if (allow_comments) {
            script = json::parse(script_file, nullptr, true, true);
        } else {
            script_file >> script;
        }
        script_file.close();
        
        // Read input files
        std::vector<json> inputs;
        for (int i = script_arg + 1; i < argc; ++i) {
            std::ifstream input_file(argv[i]);
            if (!input_file.is_open()) {
                std::cerr << "Error: Cannot open input file: " << argv[i] << std::endl;
                return 1;
            }
            json input;
            input_file >> input;
            input_file.close();
            inputs.push_back(input);
        }
        
        // Execute script
        json result;
        if (enable_interpolation) {
            permuto::Options opts;
            opts.enable_interpolation = true;
            
            if (inputs.empty()) {
                // No inputs - use old API with null input for backward compatibility
                result = computo::execute(script, json(nullptr), opts);
            } else {
                result = computo::execute(script, inputs, opts);
            }
        } else {
            if (inputs.empty()) {
                // No inputs - use old API with null input for backward compatibility
                result = computo::execute(script, json(nullptr));
            } else {
                result = computo::execute(script, inputs);
            }
        }
        
        // Handle output
        if (diff_mode) {
            // Generate diff between original input and result
            json patch = nlohmann::json::diff(inputs[0], result);
            if (pretty_indent >= 0) {
                std::cout << patch.dump(pretty_indent) << std::endl;
            } else {
                std::cout << patch.dump() << std::endl;
            }
        } else {
            // Output transformation result
            if (pretty_indent >= 0) {
                std::cout << result.dump(pretty_indent) << std::endl;
            } else {
                std::cout << result.dump() << std::endl;
            }
        }
        
    } catch (const computo::ComputoException& e) {
        std::cerr << "Computo error: " << e.what() << std::endl;
        return 1;
    } catch (const json::exception& e) {
        std::cerr << "JSON error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}