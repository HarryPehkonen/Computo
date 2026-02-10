#pragma once

#include <jsom/json_document.hpp>
#include <string>

namespace computo {

struct SugarWriterOptions {
    std::string array_key = "array";
    int indent_width = 2;
};

class SugarWriter {
public:
    static auto write(const jsom::JsonDocument& doc,
                      const SugarWriterOptions& options = {}) -> std::string;
};

} // namespace computo
