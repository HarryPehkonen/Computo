#ifndef READ_JSON_HPP
#define READ_JSON_HPP

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// namespace computo {

nlohmann::json read_json_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    nlohmann::json result;
    file >> result;
    return result;
}

nlohmann::json read_json_from_stdin() {
    nlohmann::json result;
    std::cin >> result;
    return result;
}

nlohmann::json read_json_with_comments_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    nlohmann::json result;
    try {
        result = nlohmann::json::parse(file, nullptr, true, true); // allow exceptions, allow comments
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + filename + " at byte " + std::to_string(e.byte) + ": " + e.what());
    }
    return result;
}

//} // namespace computo

#endif