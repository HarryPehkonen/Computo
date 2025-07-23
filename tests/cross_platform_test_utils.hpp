#pragma once

#include <filesystem>
#include <sstream>
#include <string>

// Cross-platform utilities for testing

namespace test_utils {

/**
 * Create a cross-platform shell command with proper output redirection
 */
inline std::string make_redirected_command(const std::string& command,
                                           const std::filesystem::path& stdout_file,
                                           const std::filesystem::path& stderr_file) {
#ifdef _WIN32
    // Windows command prompt style
    return command + " >" + stdout_file.string() + " 2>" + stderr_file.string();
#else
    // Unix/Linux shell style
    return command + " >" + stdout_file.string() + " 2>" + stderr_file.string();
#endif
}

/**
 * Create a cross-platform shell command with input redirection
 */
inline std::string make_input_redirected_command(const std::string& command,
                                                 const std::filesystem::path& input_file) {
#ifdef _WIN32
    // Windows: cmd.exe style input redirection
    return command + " <" + input_file.string();
#else
    // Unix/Linux shell style
    return command + " <" + input_file.string();
#endif
}

/**
 * Create a cross-platform pipe command
 */
inline std::string make_pipe_command(const std::string& source_cmd, const std::string& dest_cmd) {
#ifdef _WIN32
    // Windows: Use echo without quotes for simple cases
    if (source_cmd.find("echo ") == 0) {
        return source_cmd + " | " + dest_cmd;
    }
    return source_cmd + " | " + dest_cmd;
#else
    // Unix/Linux: Standard pipe
    return source_cmd + " | " + dest_cmd;
#endif
}

/**
 * Create a safe echo command for different platforms
 */
inline std::string make_echo_command(const std::string& content) {
#ifdef _WIN32
    // Windows cmd.exe: no quotes needed for simple content
    return "echo " + content;
#else
    // Unix/Linux: use single quotes for shell safety
    return "echo '" + content + "'";
#endif
}

/**
 * Get the expected executable extension for the current platform
 */
inline std::string get_executable_extension() {
#ifdef _WIN32
    return ".exe";
#else
    return "";
#endif
}

/**
 * Normalize line endings for cross-platform comparison
 */
inline std::string normalize_line_endings(const std::string& text) {
    std::string result = text;
    // Convert Windows CRLF to LF
    size_t pos = 0;
    while ((pos = result.find("\r\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "\n");
        pos += 1;
    }
    return result;
}

/**
 * Check if a command is available on the current platform
 */
inline bool is_command_available(const std::string& command) {
#ifdef _WIN32
    std::string check_cmd = "where " + command + " >nul 2>nul";
#else
    std::string check_cmd = "which " + command + " >/dev/null 2>&1";
#endif
    return std::system(check_cmd.c_str()) == 0;
}

} // namespace test_utils
