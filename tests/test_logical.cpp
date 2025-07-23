#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class LogicalOperatorsTest : public ::testing::Test {
protected:
    void SetUp() override { input_data = json{{"test", "value"}}; }

    auto execute_script(const std::string& script_json) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input});
    }

    json input_data;
};

// --- Logical AND Operator Tests ---

TEST_F(LogicalOperatorsTest, LogicalAndBasic) {
    // Test basic logical AND
    EXPECT_EQ(execute_script(R"(["and", true, true])"), json(true));
    EXPECT_EQ(execute_script(R"(["and", true, false])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", false, true])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", false, false])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalAndNary) {
    // Test n-ary logical AND
    EXPECT_EQ(execute_script(R"(["and", true, true, true])"), json(true));
    EXPECT_EQ(execute_script(R"(["and", true, true, false])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", false, true, true])"), json(false));

    // Test single argument
    EXPECT_EQ(execute_script(R"(["and", true])"), json(true));
    EXPECT_EQ(execute_script(R"(["and", false])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalAndTruthiness) {
    // Test various truthy/falsy values
    EXPECT_EQ(execute_script(R"(["and", 1, "hello", true])"), json(true));
    EXPECT_EQ(execute_script(R"(["and", 1, 0, true])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", "", "hello"])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", null, true])"), json(false));
    EXPECT_EQ(execute_script(R"(["and", {"array": [1]}, 42])"), json(true));
    EXPECT_EQ(execute_script(R"(["and", {"array": []}, 42])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalAndErrors) {
    // Test error conditions
    EXPECT_THROW(execute_script(R"(["and"])"), computo::InvalidArgumentException);
}

// --- Logical OR Operator Tests ---

TEST_F(LogicalOperatorsTest, LogicalOrBasic) {
    // Test basic logical OR
    EXPECT_EQ(execute_script(R"(["or", true, true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", true, false])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", false, true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", false, false])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalOrNary) {
    // Test n-ary logical OR
    EXPECT_EQ(execute_script(R"(["or", false, false, true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", false, false, false])"), json(false));
    EXPECT_EQ(execute_script(R"(["or", true, false, false])"), json(true));

    // Test single argument
    EXPECT_EQ(execute_script(R"(["or", true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", false])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalOrTruthiness) {
    // Test various truthy/falsy values
    EXPECT_EQ(execute_script(R"(["or", 0, "", true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", 0, "", false])"), json(false));
    EXPECT_EQ(execute_script(R"(["or", null, "hello"])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", null, 0, ""])"), json(false));
    EXPECT_EQ(execute_script(R"(["or", {"array": []}, 42])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", {"array": []}, null])"), json(false));
}

TEST_F(LogicalOperatorsTest, LogicalOrErrors) {
    // Test error conditions
    EXPECT_THROW(execute_script(R"(["or"])"), computo::InvalidArgumentException);
}

// --- Logical NOT Operator Tests ---

TEST_F(LogicalOperatorsTest, LogicalNotBasic) {
    // Test basic logical NOT
    EXPECT_EQ(execute_script(R"(["not", true])"), json(false));
    EXPECT_EQ(execute_script(R"(["not", false])"), json(true));
}

TEST_F(LogicalOperatorsTest, LogicalNotTruthiness) {
    // Test various truthy/falsy values
    EXPECT_EQ(execute_script(R"(["not", 1])"), json(false));
    EXPECT_EQ(execute_script(R"(["not", 0])"), json(true));
    EXPECT_EQ(execute_script(R"(["not", "hello"])"), json(false));
    EXPECT_EQ(execute_script(R"(["not", ""])"), json(true));
    EXPECT_EQ(execute_script(R"(["not", null])"), json(true));
    EXPECT_EQ(execute_script(R"(["not", {"array": [1]}])"), json(false));
    EXPECT_EQ(execute_script(R"(["not", {"array": []}])"), json(true));
}

TEST_F(LogicalOperatorsTest, LogicalNotErrors) {
    // Test error conditions (unary only)
    EXPECT_THROW(execute_script(R"(["not"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["not", true, false])"), computo::InvalidArgumentException);
}

// --- Complex Logical Operations Tests ---

TEST_F(LogicalOperatorsTest, LogicalOperatorsComplex) {
    // Test nested logical operations
    EXPECT_EQ(execute_script(R"(["and", ["not", false], true])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", ["not", true], false])"), json(false));
    EXPECT_EQ(execute_script(R"(["not", ["and", false, true]])"), json(true));

    // Test complex combinations
    EXPECT_EQ(execute_script(R"(["and", ["or", true, false], ["not", false]])"), json(true));
    EXPECT_EQ(execute_script(R"(["or", ["and", false, true], ["not", false]])"), json(true));
}
