#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

// Tests for CLI array key functionality
class CLIArrayKeyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get the binary path
        binary_path = COMPUTO_BINARY_PATH;

        // Create temporary test files
        test_script_default = "test_script_default_" + std::to_string(getpid()) + ".json";
        test_script_custom = "test_script_custom_" + std::to_string(getpid()) + ".json";
        test_input = "test_input_" + std::to_string(getpid()) + ".json";

        // Write test files
        std::ofstream script_default(test_script_default);
        script_default
            << R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])";
        script_default.close();

        std::ofstream script_custom(test_script_custom);
        script_custom
            << R"(["map", {"@test": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])";
        script_custom.close();

        std::ofstream input(test_input);
        input << "42";
        input.close();
    }

    void TearDown() override {
        // Clean up temporary files
        std::filesystem::remove(test_script_default);
        std::filesystem::remove(test_script_custom);
        std::filesystem::remove(test_input);
    }

    std::string exec_command(const std::string& cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            return "";
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    std::string binary_path;
    std::string test_script_default;
    std::string test_script_custom;
    std::string test_input;
};

TEST_F(CLIArrayKeyTest, DefaultArrayKeyViaCLI) {
    std::string cmd = binary_path + " --script " + test_script_default + " " + test_input;
    std::string output = exec_command(cmd);

    // Output is unwrapped: [2, 4, 6] not {"array": [...]}
    EXPECT_NE(output.find("["), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("4"), std::string::npos);
    EXPECT_NE(output.find("6"), std::string::npos);
}

TEST_F(CLIArrayKeyTest, CustomArrayKeyViaCLI) {
    std::string cmd
        = binary_path + " --script " + test_script_custom + " " + test_input + " --array=@test";
    std::string output = exec_command(cmd);

    // Output is unwrapped: [2, 4, 6] - array key is internal
    EXPECT_NE(output.find("["), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("4"), std::string::npos);
    EXPECT_NE(output.find("6"), std::string::npos);
}

TEST_F(CLIArrayKeyTest, HelpShowsArrayOption) {
    std::string cmd = binary_path + " --help";
    std::string output = exec_command(cmd);

    // Should show the array option in help
    EXPECT_NE(output.find("--array=<key>"), std::string::npos);
    EXPECT_NE(output.find("Use custom array wrapper key"), std::string::npos);
}

TEST_F(CLIArrayKeyTest, MismatchedKeyProducesError) {
    // Using default script with custom key should fail
    std::string cmd = binary_path + " --script " + test_script_default + " " + test_input
                      + " --array=@test 2>&1";
    std::string output = exec_command(cmd);

    // Should contain error message
    EXPECT_NE(output.find("Error"), std::string::npos);
}