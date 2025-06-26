#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " [--interpolation] <script.json> <input.json>" << std::endl;
        std::cerr << "  --interpolation: Enable string interpolation in Permuto templates" << std::endl;
        return 1;
    }
    
    bool enable_interpolation = false;
    int script_arg = 1;
    int input_arg = 2;
    
    // Check for --interpolation flag
    if (argc == 4 && std::string(argv[1]) == "--interpolation") {
        enable_interpolation = true;
        script_arg = 2;
        input_arg = 3;
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
        
        // Read input file
        std::ifstream input_file(argv[input_arg]);
        if (!input_file.is_open()) {
            std::cerr << "Error: Cannot open input file: " << argv[input_arg] << std::endl;
            return 1;
        }
        json input;
        input_file >> input;
        input_file.close();
        
        // Execute script with Permuto options
        json result;
        if (enable_interpolation) {
            permuto::Options opts;
            opts.enable_interpolation = true;
            result = computo::execute(script, input, opts);
        } else {
            result = computo::execute(script, input);
        }
        
        // Output result
        std::cout << result.dump(2) << std::endl;
        
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