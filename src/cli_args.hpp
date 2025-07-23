#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace computo {

struct ComputoArgs {
    enum class Mode : std::uint8_t { SCRIPT, REPL };
    Mode mode;
    std::string script_file; // Only valid in SCRIPT mode
    std::vector<std::string> input_files;
    bool enable_comments = false;
    bool debug_mode = false;
    bool show_help = false;
    bool show_version = false;
};

class ArgumentError : public std::runtime_error {
public:
    explicit ArgumentError(const std::string& msg) : std::runtime_error(msg) {}
};

class ArgumentParser {
public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static auto parse(int argc, char* const argv[]) -> ComputoArgs;
    static void print_help();
    static void print_version();
};

} // namespace computo
