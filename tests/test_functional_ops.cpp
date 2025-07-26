#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class FunctionalOpsTest : public ::testing::Test {
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

// --- car operator tests ---

TEST_F(FunctionalOpsTest, CarOperatorBasic) {
    auto result = execute_script(R"(["car", {"array": [1, 2, 3, 4, 5]}])");
    EXPECT_EQ(result, json(1));
}

TEST_F(FunctionalOpsTest, CarOperatorDirectArray) {
    auto result = execute_script(R"(["car", {"array": ["first", "second", "third"]}])");
    EXPECT_EQ(result, json("first"));
}

TEST_F(FunctionalOpsTest, CarOperatorSingleElement) {
    auto result = execute_script(R"(["car", {"array": [42]}])");
    EXPECT_EQ(result, json(42));
}

TEST_F(FunctionalOpsTest, CarOperatorMixed) {
    auto result = execute_script(R"(["car", {"array": [{"name": "Alice"}, 123, true]}])");
    auto expected = json::parse(R"({"name": "Alice"})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, CarOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["car"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["car", "not an array"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["car", {"array": []}])"), computo::InvalidArgumentException);
}

// --- cdr operator tests ---

TEST_F(FunctionalOpsTest, CdrOperatorBasic) {
    auto result = execute_script(R"(["cdr", {"array": [1, 2, 3, 4, 5]}])");
    auto expected = json::parse(R"({"array": [2, 3, 4, 5]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, CdrOperatorDirectArray) {
    auto result = execute_script(R"(["cdr", {"array": ["first", "second", "third"]}])");
    auto expected = json::parse(R"({"array": ["second", "third"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, CdrOperatorSingleElement) {
    auto result = execute_script(R"(["cdr", {"array": [42]}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, CdrOperatorTwoElements) {
    auto result = execute_script(R"(["cdr", {"array": ["a", "b"]}])");
    auto expected = json::parse(R"({"array": ["b"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, CdrOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["cdr"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["cdr", "not an array"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["cdr", {"array": []}])"), computo::InvalidArgumentException);
}

// --- cons operator tests ---

TEST_F(FunctionalOpsTest, ConsOperatorBasic) {
    auto result = execute_script(R"(["cons", 0, {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": [0, 1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, ConsOperatorDirectArray) {
    auto result = execute_script(R"(["cons", "new", {"array": ["existing1", "existing2"]}])");
    auto expected = json::parse(R"({"array": ["new", "existing1", "existing2"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, ConsOperatorEmptyArray) {
    auto result = execute_script(R"(["cons", "first", {"array": []}])");
    auto expected = json::parse(R"({"array": ["first"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, ConsOperatorObject) {
    auto result = execute_script(R"(["cons", {"name": "Alice"}, {"array": [1, 2]}])");
    auto expected = json::parse(R"({"array": [{"name": "Alice"}, 1, 2]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, ConsOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["cons"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["cons", 1])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["cons", 1, "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- append operator tests ---

TEST_F(FunctionalOpsTest, AppendOperatorBasic) {
    auto result = execute_script(R"(["append", {"array": [1, 2]}, {"array": [3, 4]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorDirectArrays) {
    auto result = execute_script(R"(["append", {"array": ["a", "b"]}, {"array": ["c", "d"]}])");
    auto expected = json::parse(R"({"array": ["a", "b", "c", "d"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorMultiple) {
    auto result
        = execute_script(R"(["append", {"array": [1]}, {"array": [2, 3]}, {"array": [4, 5, 6]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4, 5, 6]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorSingle) {
    auto result = execute_script(R"(["append", {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorEmpty) {
    auto result = execute_script(R"(["append", {"array": []}, {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorMixed) {
    auto result
        = execute_script(R"(["append", {"array": []}, {"array": ["middle"]}, {"array": ["end"]}])");
    auto expected = json::parse(R"({"array": ["middle", "end"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["append"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["append", "not an array"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["append", {"array": [1]}, "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- Integration tests with other operators ---

TEST_F(FunctionalOpsTest, CarCdrChaining) {
    // Get second element using car(cdr(array))
    auto result = execute_script(R"(["car", ["cdr", {"array": [1, 2, 3, 4]}]])");
    EXPECT_EQ(result, json(2));
}

TEST_F(FunctionalOpsTest, ConsWithExpressions) {
    // cons with computed values
    auto result = execute_script(
        R"(["cons", ["+", 1, 2], ["map", {"array": [1, 2]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]])");
    auto expected = json::parse(R"({"array": [3, 2, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(FunctionalOpsTest, AppendWithTransformations) {
    // append transformed arrays
    auto result = execute_script(
        R"(["append", ["map", {"array": [1, 2]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]], ["map", {"array": [3, 4]}, ["lambda", ["x"], ["+", ["$", "/x"], 10]]]])");
    auto expected = json::parse(R"({"array": [2, 4, 13, 14]})");
    EXPECT_EQ(result, expected);
}
