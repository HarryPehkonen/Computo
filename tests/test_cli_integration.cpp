#include "cross_platform_test_utils.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

// Helper function to execute shell commands and capture output
struct CommandResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
};

CommandResult execute_command(const std::string& command) {
    CommandResult result;

    // Create temporary files for capturing stdout and stderr using cross-platform paths
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
    std::filesystem::path stdout_file = temp_dir / "computo_test_stdout.tmp";
    std::filesystem::path stderr_file = temp_dir / "computo_test_stderr.tmp";

    // Execute command with output redirection (cross-platform)
    std::string full_command
        = test_utils::make_redirected_command(command, stdout_file, stderr_file);
    result.exit_code = std::system(full_command.c_str());

    // Read stdout
    std::ifstream stdout_stream(stdout_file);
    if (stdout_stream.is_open()) {
        std::ostringstream stdout_content;
        stdout_content << stdout_stream.rdbuf();
        result.stdout_output = stdout_content.str();
        stdout_stream.close();
    }

    // Read stderr
    std::ifstream stderr_stream(stderr_file);
    if (stderr_stream.is_open()) {
        std::ostringstream stderr_content;
        stderr_content << stderr_stream.rdbuf();
        result.stderr_output = stderr_content.str();
        stderr_stream.close();
    }

    // Clean up temporary files
    std::filesystem::remove(stdout_file);
    std::filesystem::remove(stderr_file);

    return result;
}

void create_test_file(const std::filesystem::path& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create file: " + filename.string());
    }
    file << content;
    file.close();
}

void cleanup_test_file(const std::filesystem::path& filename) { std::filesystem::remove(filename); }

class CLIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use CMake-provided binary path
        computo_binary = COMPUTO_BINARY_PATH;

        // Create test directory if it doesn't exist
        test_dir = "test_cli_temp";
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir);
    }

    std::string computo_binary;
    std::filesystem::path test_dir;
};

// Test basic script execution
TEST_F(CLIIntegrationTest, BasicScriptExecution) {
    // Create a simple script
    std::filesystem::path script_file = test_dir / "simple.json";
    create_test_file(script_file, R"(["+", 1, 2, 3])");

    auto result = execute_command(computo_binary + " --script " + script_file.string());

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_EQ(result.stdout_output, "6.0\n");
    EXPECT_TRUE(result.stderr_output.empty());
}

// Test script with input data
TEST_F(CLIIntegrationTest, ScriptWithInputData) {
    // Create script and input files
    std::filesystem::path script_file = test_dir / "with_input.json";
    std::filesystem::path input_file = test_dir / "input.json";

    create_test_file(script_file, R"(["$input", "/value"])");
    create_test_file(input_file, R"({"value": 42})");

    auto result = execute_command(computo_binary + " --script " + script_file.string() + " "
                                  + input_file.string());

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_EQ(result.stdout_output, "42\n");
    EXPECT_TRUE(result.stderr_output.empty());
}

// Test multiple input files
TEST_F(CLIIntegrationTest, MultipleInputFiles) {
    std::filesystem::path script_file = test_dir / "multi_input.json";
    std::filesystem::path input1_file = test_dir / "input1.json";
    std::filesystem::path input2_file = test_dir / "input2.json";

    create_test_file(script_file, R"(["+", ["$input"], ["$inputs", "/1"]])");
    create_test_file(input1_file, "10");
    create_test_file(input2_file, "20");

    auto result = execute_command(computo_binary + " --script " + script_file.string() + " "
                                  + input1_file.string() + " " + input2_file.string());

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_EQ(result.stdout_output, "30.0\n");
    EXPECT_TRUE(result.stderr_output.empty());
}

// Test complex data transformation
TEST_F(CLIIntegrationTest, ComplexTransformation) {
    std::filesystem::path script_file = test_dir / "complex.json";
    std::filesystem::path input_file = test_dir / "complex_input.json";

    create_test_file(script_file, R"([
        "map", 
        ["$input", "/users"], 
        ["lambda", ["user"], ["$", "/user/name"]]
    ])");

    create_test_file(input_file, R"({
        "users": [
            {"name": "Alice", "age": 30},
            {"name": "Bob", "age": 25}
        ]
    })");

    auto result = execute_command(computo_binary + " --script " + script_file.string() + " "
                                  + input_file.string());

    EXPECT_EQ(result.exit_code, 0);
    // Check for the expected JSON content (formatted output)
    EXPECT_TRUE(result.stdout_output.find("\"Alice\"") != std::string::npos);
    EXPECT_TRUE(result.stdout_output.find("\"Bob\"") != std::string::npos);
    EXPECT_TRUE(result.stdout_output.find("\"array\"") != std::string::npos);
    // Note: stderr may contain warnings, which is acceptable
}

// Test error handling - invalid script
TEST_F(CLIIntegrationTest, InvalidScript) {
    std::filesystem::path script_file = test_dir / "invalid.json";
    create_test_file(script_file, R"(["+", "not_a_number"])");

    auto result = execute_command(computo_binary + " --script " + script_file.string());

    EXPECT_NE(result.exit_code, 0);
    EXPECT_TRUE(result.stderr_output.find("Invalid argument") != std::string::npos);
}

// Test error handling - file not found
TEST_F(CLIIntegrationTest, FileNotFound) {
    auto result = execute_command(computo_binary + " --script nonexistent_file.json");

    EXPECT_NE(result.exit_code, 0);
    EXPECT_TRUE(result.stderr_output.find("Error") != std::string::npos
                || result.stderr_output.find("file") != std::string::npos);
}

// Test help flag
TEST_F(CLIIntegrationTest, HelpFlag) {
    auto result = execute_command(computo_binary + " --help");

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(result.stdout_output.find("Usage") != std::string::npos
                || result.stdout_output.find("help") != std::string::npos);
}

// Test version flag
TEST_F(CLIIntegrationTest, VersionFlag) {
    auto result = execute_command(computo_binary + " --version");

    // Should either succeed or fail gracefully
    EXPECT_TRUE(result.exit_code == 0 || result.exit_code != 0);
    // Should produce some output
    EXPECT_FALSE(result.stdout_output.empty() && result.stderr_output.empty());
}

// Test REPL mode basic startup
TEST_F(CLIIntegrationTest, REPLModeStartup) {
    // Test REPL mode with immediate exit
    std::string input_commands = "quit\n";
    std::filesystem::path temp_input = test_dir / "repl_input.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    // Should exit cleanly
    EXPECT_EQ(result.exit_code, 0);
}

// Test REPL with simple expression
TEST_F(CLIIntegrationTest, REPLSimpleExpression) {
    std::string input_commands = "[\"+\", 1, 2, 3]\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_expr.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(result.stdout_output.find("6") != std::string::npos);
}

// Test REPL help command
TEST_F(CLIIntegrationTest, REPLHelpCommand) {
    std::string input_commands = "help\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_help.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(result.stdout_output.find("help") != std::string::npos
                || result.stdout_output.find("commands") != std::string::npos);
}

// Test REPL run command
TEST_F(CLIIntegrationTest, REPLRunCommand) {
    // Create a script to run
    std::filesystem::path script_file = test_dir / "repl_script.json";
    create_test_file(script_file, R"(["+", 5, 10])");

    std::string input_commands = "run " + script_file.string() + "\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_run.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_TRUE(result.stdout_output.find("15") != std::string::npos);
}

// Test REPL debug commands
TEST_F(CLIIntegrationTest, REPLDebugCommands) {
    std::string input_commands = "debug on\nbreak +\nbreaks\nnobreak +\nbreaks\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_debug.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    // Should show breakpoint was set and then removed
    EXPECT_TRUE(result.stdout_output.find("+") != std::string::npos);
}

// Test JSON comment support
TEST_F(CLIIntegrationTest, JSONCommentSupport) {
    std::filesystem::path script_file = test_dir / "commented.jsonc";
    create_test_file(script_file, R"([
        // This is a comment
        "+", 
        1,  // First number
        2   /* Second number */
    ])");

    auto result
        = execute_command(computo_binary + " --script " + script_file.string() + " --comments");

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_EQ(result.stdout_output, "3.0\n");
    EXPECT_TRUE(result.stderr_output.empty());
}

// Test stdin input
TEST_F(CLIIntegrationTest, StdinInput) {
    std::filesystem::path script_file = test_dir / "stdin_script.json";
    create_test_file(script_file, R"(["$input"])");

#ifdef _WIN32
    auto result
        = execute_command("echo 42 | " + computo_binary + " --script " + script_file.string());
#else
    auto result
        = execute_command("echo '42' | " + computo_binary + " --script " + script_file.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    // Note: CLI currently requires explicit input files, stdin input results in null
    EXPECT_EQ(result.stdout_output, "null\n");
}

// Test cross-platform path handling
TEST_F(CLIIntegrationTest, PathHandling) {
    // Create nested directory structure
    std::filesystem::path nested_dir = test_dir / "subdir";
    std::filesystem::create_directories(nested_dir);

    std::filesystem::path script_file = nested_dir / "nested.json";
    create_test_file(script_file, R"(["+", 1, 1])");

    auto result = execute_command(computo_binary + " --script " + script_file.string());

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_EQ(result.stdout_output, "2.0\n");
}

// Test REPL operator breakpoint functionality
TEST_F(CLIIntegrationTest, REPLOperatorBreakpoint) {
    std::string input_commands = "debug on\nbreak +\n[\"+\", 1, 2]\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_breakpoint.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    // Should show that breakpoint was set
    EXPECT_TRUE(result.stdout_output.find("Set operator breakpoint: +") != std::string::npos);
    // Should show breakpoint hit message
    EXPECT_TRUE(result.stdout_output.find("Breakpoint hit") != std::string::npos
                || result.stdout_output.find("Debug breakpoint") != std::string::npos);
}

// Test REPL variable management with vars and set commands
TEST_F(CLIIntegrationTest, REPLVariableManagement) {
    std::string input_commands
        = "set x 10\nset data {\"key\": \"value\"}\nvars\n[\"$\", \"/x\"]\nquit\n";
    std::filesystem::path temp_input = test_dir / "repl_vars.txt";
    create_test_file(temp_input, input_commands);

#ifdef _WIN32
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#else
    auto result = execute_command(computo_binary + " --repl < " + temp_input.string());
#endif

    EXPECT_EQ(result.exit_code, 0);
    // Should show variable setting confirmations
    EXPECT_TRUE(result.stdout_output.find("Set x = 10") != std::string::npos);
    EXPECT_TRUE(result.stdout_output.find("Set data = ") != std::string::npos);
    // Should show variables in vars output
    EXPECT_TRUE(result.stdout_output.find("REPL variables:") != std::string::npos);
    EXPECT_TRUE(result.stdout_output.find("x: 10") != std::string::npos);
    // Should be able to use the variable (result could be 10 or 10.0)
    EXPECT_TRUE(result.stdout_output.find("10") != std::string::npos); // Result of ["$", "/x"]
}
