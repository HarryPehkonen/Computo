#include <computo.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using json = nlohmann::json;

class CLITest : public ::testing::Test {
protected:
    std::string temp_dir_;

    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path().string() + "/computo_test_";
        temp_dir_ += std::to_string(getpid());
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir_);
    }

    auto create_temp_file(const std::string& content, const std::string& suffix = ".json") -> std::string {
        static int file_counter = 0;
        std::string filename = temp_dir_ + "/test" + std::to_string(file_counter++) + suffix;
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }

    static auto run_cli(const std::vector<std::string>& args) -> std::string {
        std::string cmd = COMPUTO_CLI_PATH;

        for (const auto& arg : args) {
            cmd += " " + arg;
        }
        cmd += " 2>&1";

        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe == nullptr) {
            return "";
        }

        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        return result;
    }
};

TEST_F(CLITest, HelpOption) {
    std::string output = run_cli({ "--help" });
    EXPECT_NE(output.find("Usage:"), std::string::npos);
}

TEST_F(CLITest, VersionOption) {
    std::string output = run_cli({ "--version" });
    EXPECT_NE(output.find("Computo CLI v"), std::string::npos);
}

TEST_F(CLITest, SimpleScriptFile) {
    std::string script_content = R"(["+", 1, 2])";
    std::string script_file = create_temp_file(script_content);

    std::string output = run_cli({ script_file });
    EXPECT_NE(output.find('3'), std::string::npos);
}

TEST_F(CLITest, ScriptWithInput) {
    std::string script_content = R"(["$input"])";
    std::string script_file = create_temp_file(script_content);

    std::string input_content = R"("Hello, World!")";
    std::string input_file = create_temp_file(input_content);

    std::string output = run_cli({ script_file, input_file });
    EXPECT_NE(output.find("Hello, World!"), std::string::npos);
}

TEST_F(CLITest, InputVsInputsRegression) {
    // Regression test for CLI bug where $input and $inputs returned same result

    // Test $input returns the first input object directly
    std::string input_script = R"(["$input"])";
    std::string input_script_file = create_temp_file(input_script);

    std::string test_data = R"({"name": "test", "value": 42})";
    std::string test_data_file = create_temp_file(test_data);

    std::string input_output = run_cli({ input_script_file, test_data_file });

    // Parse the output to verify it's the object directly, not wrapped in array
    json input_result = json::parse(input_output);
    EXPECT_TRUE(input_result.is_object());
    EXPECT_EQ(input_result["name"], "test");
    EXPECT_EQ(input_result["value"], 42);

    // Test $inputs returns array of all inputs
    std::string inputs_script = R"(["$inputs"])";
    std::string inputs_script_file = create_temp_file(inputs_script);

    std::string inputs_output = run_cli({ inputs_script_file, test_data_file });

    // Parse the output to verify it's an array containing the object
    json inputs_result = json::parse(inputs_output);
    EXPECT_TRUE(inputs_result.is_array());
    EXPECT_EQ(inputs_result.size(), 1);
    EXPECT_TRUE(inputs_result[0].is_object());
    EXPECT_EQ(inputs_result[0]["name"], "test");
    EXPECT_EQ(inputs_result[0]["value"], 42);

    // Test with multiple inputs to ensure $input gets first, $inputs gets all
    std::string test_data2 = R"({"name": "second", "value": 100})";
    std::string test_data2_file = create_temp_file(test_data2);

    std::string multi_input_output = run_cli({ input_script_file, test_data_file, test_data2_file });
    json multi_input_result = json::parse(multi_input_output);
    EXPECT_TRUE(multi_input_result.is_object());
    EXPECT_EQ(multi_input_result["name"], "test"); // Should be first input

    std::string multi_inputs_output = run_cli({ inputs_script_file, test_data_file, test_data2_file });
    json multi_inputs_result = json::parse(multi_inputs_output);
    EXPECT_TRUE(multi_inputs_result.is_array());
    EXPECT_EQ(multi_inputs_result.size(), 2);
    EXPECT_EQ(multi_inputs_result[0]["name"], "test");
    EXPECT_EQ(multi_inputs_result[1]["name"], "second");
}

// Test CLI with comments support
TEST_F(CLITest, CLIWithCommentsFlag) {
    std::string script_content = R"([
        "+",  // Addition operator
        1,    /* first number */
        2,    /* second number */
        3     // third number
    ])";
    std::string script_file = create_temp_file(script_content);

    std::string output = run_cli({ "--comments", script_file });
    json result = json::parse(output);
    EXPECT_EQ(result, 6);
}

// Test CLI rejects comments without --comments flag
TEST_F(CLITest, CLIRejectsCommentsWithoutFlag) {
    std::string script_content = R"([
        "+",  // Addition operator
        1,    /* first number */
        2,    /* second number */
        3     // third number
    ])";
    std::string script_file = create_temp_file(script_content);

    std::string output = run_cli({ script_file });
    // Should contain error message
    EXPECT_NE(output.find("Error"), std::string::npos);
}
