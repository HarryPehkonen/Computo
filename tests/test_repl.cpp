#include <computo.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using json = nlohmann::json;

class REPLTest : public ::testing::Test {
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

    auto run_repl(const std::string& script_content, const std::vector<std::string>& input_contents = {}) -> std::string {
        std::string cmd = COMPUTO_REPL_PATH;

        // save script to a temp file
        // yes, it's fine because we'll pipe the script to the repl
        std::string script_file = create_temp_file(script_content);

        if (!input_contents.empty()) {
            for (const auto& input_content : input_contents) {
                std::string filename = create_temp_file(input_content);
                cmd += " " + filename;
            }
        }
        cmd += " < " + script_file;

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

TEST_F(REPLTest, Help) {
    std::string output = run_repl("help");
    EXPECT_NE(output.find("Commands:"), std::string::npos);
}

TEST_F(REPLTest, SimpleScript) {
    std::string output = run_repl("[\"+\", 2, 3]");
    EXPECT_NE(output.find('5'), std::string::npos);
}

// script with input
TEST_F(REPLTest, ScriptWithInput) {
    std::string input_content = R"("Hello, World!")";
    std::string output = run_repl(R"(["$input"])", { input_content });
    EXPECT_NE(output.find("Hello, World!"), std::string::npos);
}

// historical variables
TEST_F(REPLTest, HistoricalVariables) {
    std::string script = R"(["+", 1, 2]
    ["+", _1, 2])";
    std::string output = run_repl(script);
    EXPECT_NE(output.find("\n3.0\n"), std::string::npos);
    EXPECT_NE(output.find("\n5.0\n"), std::string::npos);
}

TEST_F(REPLTest, VersionOption) {
    std::string output = run_repl({ "version" });
    EXPECT_NE(output.find("Computo REPL v"), std::string::npos);
}