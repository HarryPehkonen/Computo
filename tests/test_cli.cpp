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

    std::string create_temp_file(const std::string& content, const std::string& suffix = ".json") {
        static int file_counter = 0;
        std::string filename = temp_dir_ + "/test" + std::to_string(file_counter++) + suffix;
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }

    std::string run_cli(const std::vector<std::string>& args) {
        std::string cmd = COMPUTO_CLI_PATH;

        for (const auto& arg : args) {
            cmd += " " + arg;
        }

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe)
            return "";

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
    EXPECT_NE(output.find("3"), std::string::npos);
}

TEST_F(CLITest, ScriptWithInput) {
    std::string script_content = R"(["$input"])";
    std::string script_file = create_temp_file(script_content);

    std::string input_content = R"("Hello, World!")";
    std::string input_file = create_temp_file(input_content);

    std::string output = run_cli({ input_file, script_file });
    EXPECT_NE(output.find("Hello, World!"), std::string::npos);
}
