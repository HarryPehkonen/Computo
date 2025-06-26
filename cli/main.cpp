#include <computo/computo.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <script.json> <input.json>" << std::endl;
        return 1;
    }
    
    try {
        // Read script file
        std::ifstream script_file(argv[1]);
        if (!script_file.is_open()) {
            std::cerr << "Error: Cannot open script file: " << argv[1] << std::endl;
            return 1;
        }
        json script;
        script_file >> script;
        script_file.close();
        
        // Read input file
        std::ifstream input_file(argv[2]);
        if (!input_file.is_open()) {
            std::cerr << "Error: Cannot open input file: " << argv[2] << std::endl;
            return 1;
        }
        json input;
        input_file >> input;
        input_file.close();
        
        // Execute script
        json result = computo::execute(script, input);
        
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