#ifndef READ_JSON_HPP
#define READ_JSON_HPP

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>

// namespace computo {

auto read_json(std::optional<std::string> filename = std::nullopt, bool allow_comments = false) -> nlohmann::json {
    nlohmann::json result;
    
    if (filename.has_value()) {
        // Read from file
        std::ifstream file(filename.value());
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename.value());
        }
        
        try {
            if (allow_comments) {
                result = nlohmann::json::parse(file, nullptr, true, true); // allow exceptions, allow comments
            } else {
                file >> result;
            }
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error in " + filename.value() + " at byte " + std::to_string(e.byte) + ": " + e.what());
        }
    } else {
        // Read from stdin
        try {
            if (allow_comments) {
                result = nlohmann::json::parse(std::cin, nullptr, true, true); // allow exceptions, allow comments
            } else {
                std::cin >> result;
            }
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error from stdin at byte " + std::to_string(e.byte) + ": " + e.what());
        }
    }
    
    return result;
}

//} // namespace computo

#endif