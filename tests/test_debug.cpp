#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class DebugTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(DebugTest, SimpleLambda) {
    json script = R"([
        "let",
        [["x", 42]],
        ["$", "/x"]
    ])"_json;
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, LambdaInMap) {
    json script = R"([
        "map",
        {"array": [1]},
        ["lambda", ["x"], ["$", "/x"]]
    ])"_json;
    json expected = R"([1])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(DebugTest, DirectLambdaCall) {
    json script = R"([
        "let",
        [["f", ["lambda", ["y"], ["$", "/x"]]],
         ["x", 42]],
        ["call", ["$", "/f"], 10]
    ])"_json;
    // This should work now - lambda returns 42 (from x), ignoring the parameter y
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, SimpleLambdaReturn) {
    json script = R"([
        "map",
        {"array": [1]},
        ["lambda", ["x"], 42]
    ])"_json;
    json expected = R"([42])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

// New tests for lambda and call functionality
TEST_F(DebugTest, LambdaWithParameters) {
    json script = R"([
        "let",
        [["add", ["lambda", ["x"], ["+", ["$", "/x/0"], ["$", "/x/1"]]]]],
        ["call", ["$", "/add"], 5, 3]
    ])"_json;
    EXPECT_EQ(exec(script), 8);
}

TEST_F(DebugTest, LambdaWithMultipleParameters) {
    json script = R"([
        "let",
        [["multiply", ["lambda", ["x"], ["*", ["$", "/x/0"], ["$", "/x/1"]]]]],
        ["call", ["$", "/multiply"], 4, 7]
    ])"_json;
    EXPECT_EQ(exec(script), 28);
}

TEST_F(DebugTest, LambdaVariableInMap) {
    json script = R"([
        "let",
        [["double", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]],
        ["map", {"array": [1, 2, 3]}, ["$", "/double"]]
    ])"_json;
    json expected = R"([2, 4, 6])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(DebugTest, LambdaVariableInFilter) {
    json script = R"([
        "let",
        [["is_even", ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]]],
        ["filter", {"array": [1, 2, 3, 4, 5]}, ["$", "/is_even"]]
    ])"_json;
    json expected = R"([2, 4])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(DebugTest, LambdaVariableInReduce) {
    json script = R"([
        "let",
        [["add", ["lambda", ["x"], ["+", ["$", "/x/0"], ["$", "/x/1"]]]]],
        ["reduce", {"array": [1, 2, 3, 4]}, ["$", "/add"], 0]
    ])"_json;
    EXPECT_EQ(exec(script), 10);
}

TEST_F(DebugTest, MultipleLambdaDefinitions) {
    json script = R"([
        "let",
        [["add", ["lambda", ["x"], ["+", ["$", "/x/0"], ["$", "/x/1"]]]],
         ["multiply", ["lambda", ["x"], ["*", ["$", "/x/0"], ["$", "/x/1"]]]]],
        ["call", ["$", "/add"],
            ["call", ["$", "/multiply"], 3, 4],
            5]
    ])"_json;
    EXPECT_EQ(exec(script), 17); // (3 * 4) + 5 = 12 + 5 = 17
}

TEST_F(DebugTest, LambdaWithComplexBody) {
    json script = R"([
        "let",
        [["complex", [
            "lambda", ["x"], 
            ["let", 
                [["a", ["$", "/x/0"]], ["b", ["$", "/x/1"]]],
                ["+", ["*", ["$", "/a"], 2], ["$", "/b"]]
            ]
        ]]],
        ["call", ["$", "/complex"], 5, 3]
    ])"_json;
    EXPECT_EQ(exec(script), 13); // (5 * 2) + 3 = 10 + 3 = 13
}

TEST_F(DebugTest, LambdaParameterAccess) {
    json script = R"([
        "let",
        [["access", ["lambda", ["x"], ["$", "/x/2"]]]],
        ["call", ["$", "/access"], 1, 2, 3, 4, 5]
    ])"_json;
    EXPECT_EQ(exec(script), 3); // Third parameter (index 2)
}

TEST_F(DebugTest, LambdaErrorHandling) {
    // Test lambda with wrong number of arguments
    json script = R"(["lambda", ["x", "y"], ["$", "/x"]])"_json;
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(DebugTest, CallErrorHandling) {
    // Test call with non-lambda first argument
    json script = R"(["call", 42, 1, 2, 3])"_json;
    EXPECT_THROW(exec(script), std::exception);
}

// Tests for if operator functionality
TEST_F(DebugTest, IfOperatorBasic) {
    json script = R"(["if", true, 42, 0])"_json;
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, IfOperatorFalse) {
    json script = R"(["if", false, 42, 0])"_json;
    EXPECT_EQ(exec(script), 0);
}

TEST_F(DebugTest, IfOperatorWithExpressions) {
    json script = R"([
        "if",
        [">", 5, 3],
        ["+", 10, 5],
        ["-", 10, 5]
    ])"_json;
    EXPECT_EQ(exec(script), 15); // 5 > 3 is true, so 10 + 5 = 15
}

TEST_F(DebugTest, IfOperatorWithComplexCondition) {
    json script = R"([
        "if",
        ["&&", [">", 5, 3], ["<", 2, 4]],
        "true_branch",
        "false_branch"
    ])"_json;
    EXPECT_EQ(exec(script), "true_branch"); // Both conditions are true
}

TEST_F(DebugTest, IfOperatorWithVariables) {
    json script = R"([
        "let",
        [["x", 10], ["y", 5]],
        ["if",
            [">", ["$", "/x"], ["$", "/y"]],
            ["+", ["$", "/x"], ["$", "/y"]],
            ["-", ["$", "/x"], ["$", "/y"]]
        ]
    ])"_json;
    EXPECT_EQ(exec(script), 15); // 10 > 5 is true, so 10 + 5 = 15
}

TEST_F(DebugTest, IfOperatorNested) {
    json script = R"([
        "let",
        [["value", 7]],
        ["if",
            [">", ["$", "/value"], 5],
            ["if",
                [">", ["$", "/value"], 10],
                "very_large",
                "medium"
            ],
            "small"
        ]
    ])"_json;
    EXPECT_EQ(exec(script), "medium"); // 7 > 5 but 7 <= 10
}

TEST_F(DebugTest, IfOperatorWithArrays) {
    json script = R"([
        "if",
        true,
        [1, 2, 3],
        [4, 5, 6]
    ])"_json;
    json expected = R"([1, 2, 3])"_json;
    EXPECT_EQ(exec(script), expected);
}

TEST_F(DebugTest, IfOperatorWithObjects) {
    json script = R"([
        "if",
        false,
        {"a": 1, "b": 2},
        {"c": 3, "d": 4}
    ])"_json;
    json expected = R"({"c": 3, "d": 4})"_json;
    EXPECT_EQ(exec(script), expected);
}

TEST_F(DebugTest, IfOperatorErrorHandling) {
    // Test if with wrong number of arguments
    json script = R"(["if", true, 42])"_json;
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(DebugTest, IfOperatorWithLambda) {
    json script = R"([
        "let",
        [["test_func", ["lambda", ["x"], [">", ["$", "/x/0"], 5]]]],
        ["if",
            ["call", ["$", "/test_func"], 10],
            "passed",
            "failed"
        ]
    ])"_json;
    EXPECT_EQ(exec(script), "passed"); // 10 > 5 is true
}