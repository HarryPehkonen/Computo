#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [--interpolation] [--diff] <script.json> [input1.json [input2.json ...]]" << std::endl;
        std::cerr << "  --interpolation: Enable string interpolation in Permuto templates" << std::endl;
        std::cerr << "  --diff: Generate a JSON patch between the input and the transformation result" << std::endl;
        return 1;
    }
    
    bool enable_interpolation = false;
    bool diff_mode = false;
    int script_arg = 1;
    
    // Parse flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--interpolation") {
            enable_interpolation = true;
            script_arg = i + 1;
        } else if (arg == "--diff") {
            diff_mode = true;
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
        script_file >> script;
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
            std::cout << patch.dump(2) << std::endl;
        } else {
            // Output transformation result
            std::cout << result.dump(2) << std::endl;
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