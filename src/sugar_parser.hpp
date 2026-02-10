#pragma once

#include <jsom/json_document.hpp>
#include <stdexcept>
#include <string>

namespace computo {

struct SugarParseOptions {
    std::string array_key = "array";
};

class SugarParseError : public std::runtime_error {
public:
    int line;
    int column;
    SugarParseError(const std::string& msg, int line, int column)
        : std::runtime_error(msg + " at line " + std::to_string(line)
                             + ", column " + std::to_string(column)),
          line(line), column(column) {}
};

class SugarParser {
public:
    static auto parse(const std::string& source,
                      const SugarParseOptions& options = {}) -> jsom::JsonDocument;
};

} // namespace computo
