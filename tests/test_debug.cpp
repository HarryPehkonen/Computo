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
    json script = json::array({ "let",
        json::array({ json::array({ "x", 42 }) }),
        json::array({ "$", "/x" }) });
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, LambdaInMap) {
    json script = json::array({ "map",
        json::object({ { "array", json::array({ 1 }) } }),
        json::array({ "lambda", json::array({ "x" }), json::array({ "$", "/x" }) }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 1 }) } }));
}

TEST_F(DebugTest, DirectLambdaCall) {
    json script = json::array({ "let",
        json::array({ json::array({ "f", json::array({ "lambda", json::array({ "y" }), json::array({ "$", "/x" }) }) }),
            json::array({ "x", 42 }) }),
        json::array({ "call", json::array({ "$", "/f" }), 10 }) });
    // This should work now - lambda returns 42 (from x), ignoring the parameter y
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, SimpleLambdaReturn) {
    json script = json::array({ "map",
        json::object({ { "array", json::array({ 1 }) } }),
        json::array({ "lambda", json::array({ "x" }), 42 }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 42 }) } }));
}

// New tests for lambda and call functionality
TEST_F(DebugTest, LambdaWithParameters) {
    json script = json::array({ "let",
        json::array({ json::array({ "add", json::array({ "lambda", json::array({ "x" }), json::array({ "+", json::array({ "$", "/x/0" }), json::array({ "$", "/x/1" }) }) }) }) }),
        json::array({ "call", json::array({ "$", "/add" }), 5, 3 }) });
    EXPECT_EQ(exec(script), 8);
}

TEST_F(DebugTest, LambdaWithMultipleParameters) {
    json script = json::array({ "let",
        json::array({ json::array({ "multiply", json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x/0" }), json::array({ "$", "/x/1" }) }) }) }) }),
        json::array({ "call", json::array({ "$", "/multiply" }), 4, 7 }) });
    EXPECT_EQ(exec(script), 28);
}

TEST_F(DebugTest, LambdaVariableInMap) {
    json script = json::array({ "let",
        json::array({ json::array({ "double", json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x" }), 2 }) }) }) }),
        json::array({ "map", json::object({ { "array", json::array({ 1, 2, 3 }) } }), json::array({ "$", "/double" }) }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 2, 4, 6 }) } }));
}

TEST_F(DebugTest, LambdaVariableInFilter) {
    json script = json::array({ "let",
        json::array({ json::array({ "is_even", json::array({ "lambda", json::array({ "x" }), json::array({ "==", json::array({ "%", json::array({ "$", "/x" }), 2 }), 0 }) }) }) }),
        json::array({ "filter", json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }), json::array({ "$", "/is_even" }) }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 2, 4 }) } }));
}

TEST_F(DebugTest, LambdaVariableInReduce) {
    json script = json::array({ "let",
        json::array({ json::array({ "add", json::array({ "lambda", json::array({ "x" }), json::array({ "+", json::array({ "$", "/x/0" }), json::array({ "$", "/x/1" }) }) }) }) }),
        json::array({ "reduce", json::object({ { "array", json::array({ 1, 2, 3, 4 }) } }), json::array({ "$", "/add" }), 0 }) });
    EXPECT_EQ(exec(script), 10);
}

TEST_F(DebugTest, MultipleLambdaDefinitions) {
    json script = json::array({ "let",
        json::array({ json::array({ "add", json::array({ "lambda", json::array({ "x" }), json::array({ "+", json::array({ "$", "/x/0" }), json::array({ "$", "/x/1" }) }) }) }),
            json::array({ "multiply", json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x/0" }), json::array({ "$", "/x/1" }) }) }) }) }),
        json::array({ "call", json::array({ "$", "/add" }),
            json::array({ "call", json::array({ "$", "/multiply" }), 3, 4 }),
            5 }) });
    EXPECT_EQ(exec(script), 17); // (3 * 4) + 5 = 12 + 5 = 17
}

TEST_F(DebugTest, LambdaWithComplexBody) {
    json script = json::array({ "let",
        json::array({ json::array({ "complex", json::array({ "lambda", json::array({ "x" }), json::array({ "let", json::array({ json::array({ "a", json::array({ "$", "/x/0" }) }), json::array({ "b", json::array({ "$", "/x/1" }) }) }), json::array({ "+", json::array({ "*", json::array({ "$", "/a" }), 2 }), json::array({ "$", "/b" }) }) }) }) }) }),
        json::array({ "call", json::array({ "$", "/complex" }), 5, 3 }) });
    EXPECT_EQ(exec(script), 13); // (5 * 2) + 3 = 10 + 3 = 13
}

TEST_F(DebugTest, LambdaParameterAccess) {
    json script = json::array({ "let",
        json::array({ json::array({ "access", json::array({ "lambda", json::array({ "x" }), json::array({ "$", "/x/2" }) }) }) }),
        json::array({ "call", json::array({ "$", "/access" }), 1, 2, 3, 4, 5 }) });
    EXPECT_EQ(exec(script), 3); // Third parameter (index 2)
}

TEST_F(DebugTest, LambdaErrorHandling) {
    // Test lambda with wrong number of arguments
    json script = json::array({ "lambda", json::array({ "x", "y" }), json::array({ "$", "/x" }) });
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(DebugTest, CallErrorHandling) {
    // Test call with non-lambda first argument
    json script = json::array({ "call", 42, 1, 2, 3 });
    EXPECT_THROW(exec(script), std::exception);
}

// Tests for if operator functionality
TEST_F(DebugTest, IfOperatorBasic) {
    json script = json::array({ "if", true, 42, 0 });
    EXPECT_EQ(exec(script), 42);
}

TEST_F(DebugTest, IfOperatorFalse) {
    json script = json::array({ "if", false, 42, 0 });
    EXPECT_EQ(exec(script), 0);
}

TEST_F(DebugTest, IfOperatorWithExpressions) {
    json script = json::array({ "if",
        json::array({ ">", 5, 3 }),
        json::array({ "+", 10, 5 }),
        json::array({ "-", 10, 5 }) });
    EXPECT_EQ(exec(script), 15); // 5 > 3 is true, so 10 + 5 = 15
}

TEST_F(DebugTest, IfOperatorWithComplexCondition) {
    json script = json::array({ "if",
        json::array({ "&&", json::array({ ">", 5, 3 }), json::array({ "<", 2, 4 }) }),
        "true_branch",
        "false_branch" });
    EXPECT_EQ(exec(script), "true_branch"); // Both conditions are true
}

TEST_F(DebugTest, IfOperatorWithVariables) {
    json script = json::array({ "let",
        json::array({ json::array({ "x", 10 }),
            json::array({ "y", 5 }) }),
        json::array({ "if",
            json::array({ ">", json::array({ "$", "/x" }), json::array({ "$", "/y" }) }),
            json::array({ "+", json::array({ "$", "/x" }), json::array({ "$", "/y" }) }),
            json::array({ "-", json::array({ "$", "/x" }), json::array({ "$", "/y" }) }) }) });
    EXPECT_EQ(exec(script), 15); // 10 > 5 is true, so 10 + 5 = 15
}

TEST_F(DebugTest, IfOperatorNested) {
    json script = json::array({ "let",
        json::array({ json::array({ "value", 7 }) }),
        json::array({ "if",
            json::array({ ">", json::array({ "$", "/value" }), 5 }),
            json::array({ "if",
                json::array({ ">", json::array({ "$", "/value" }), 10 }),
                "very_large",
                "medium" }),
            "small" }) });
    EXPECT_EQ(exec(script), "medium"); // 7 > 5 but 7 <= 10
}

TEST_F(DebugTest, IfOperatorWithArrays) {
    json script = json::array({ "if",
        true,
        json::array({ 1, 2, 3 }),
        json::array({ 4, 5, 6 }) });
    EXPECT_EQ(exec(script), json::array({ 1, 2, 3 }));
}

TEST_F(DebugTest, IfOperatorWithObjects) {
    json script = json::array({ "if",
        false,
        json::object({ { "a", 1 }, { "b", 2 } }),
        json::object({ { "c", 3 }, { "d", 4 } }) });
    EXPECT_EQ(exec(script), json::object({ { "c", 3 }, { "d", 4 } }));
}

TEST_F(DebugTest, IfOperatorErrorHandling) {
    // Test if with wrong number of arguments
    json script = json::array({ "if", true, 42 });
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(DebugTest, IfOperatorWithLambda) {
    json script = json::array({ "let",
        json::array({ json::array({ "test_func", json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x/0" }), 5 }) }) }) }),
        json::array({ "if",
            json::array({ "call", json::array({ "$", "/test_func" }), 10 }),
            "passed",
            "failed" }) });
    EXPECT_EQ(exec(script), "passed"); // 10 > 5 is true
}