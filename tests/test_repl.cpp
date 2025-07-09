#include <gtest/gtest.h>
#include <computo.hpp>
#include <fstream>
#include <cstdlib>
#include <filesystem>

using json = nlohmann::json;

class REPLTest : public ::testing::Test {
protected:
    
    void SetUp() override {
    }
    
    void TearDown() override {
    }
    
    std::string find_repl_binary() {
        std::vector<std::string> paths = {
            "./build-repl/computo_repl",
            "./computo_repl"
        };
        for (const auto& path : paths) {
            if (std::filesystem::exists(path)) {
                return path;
            }
        }
        throw std::runtime_error("REPL binary not found");
    }
    
    std::string create_temp_file(const std::string& content, const std::string& suffix = ".json") {
        char *temp_name = std::tmpnam(nullptr);
        std::string filename = temp_name + suffix;
        std::ofstream file(filename);
        file << content;
        file.close();
        return filename;
    }
    
    std::string run_repl(const std::string& script_content, const std::vector<std::string>& input_contents = {}) {
        std::string cmd = find_repl_binary();

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
        if (!pipe) return "";
        
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
    EXPECT_NE(output.find("5"), std::string::npos);
}

// script with input
TEST_F(REPLTest, ScriptWithInput) {
    std::string input_content = R"("Hello, World!")";
    std::string output = run_repl(R"(["$input"])", {input_content});
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
    std::string output = run_repl({"version"});
    EXPECT_NE(output.find("Computo REPL v"), std::string::npos);
}