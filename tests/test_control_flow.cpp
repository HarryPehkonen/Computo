#include <computo.hpp>
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

class ControlFlowTest : public ::testing::Test {
protected:
    void SetUp() override { input_data = json{{"test", "value"}}; }

    auto execute_script(const std::string& script_json) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input});
    }

    json input_data;
};

// --- If Operator Tests ---

TEST_F(ControlFlowTest, IfOperatorTrue) {
    // Test if with true condition: ["if", true, "then_value", "else_value"]
    auto result = execute_script(R"(["if", true, "then_value", "else_value"])");
    EXPECT_EQ(result, json("then_value"));
}

TEST_F(ControlFlowTest, IfOperatorFalse) {
    // Test if with false condition: ["if", false, "then_value", "else_value"]
    auto result = execute_script(R"(["if", false, "then_value", "else_value"])");
    EXPECT_EQ(result, json("else_value"));
}

TEST_F(ControlFlowTest, IfOperatorTruthiness) {
    // Test various truthy/falsy values
    EXPECT_EQ(execute_script(R"(["if", 0, "then", "else"])"), json("else"));
    EXPECT_EQ(execute_script(R"(["if", 1, "then", "else"])"), json("then"));
    EXPECT_EQ(execute_script(R"(["if", "", "then", "else"])"), json("else"));
    EXPECT_EQ(execute_script(R"(["if", "hello", "then", "else"])"), json("then"));
    EXPECT_EQ(execute_script(R"(["if", null, "then", "else"])"), json("else"));
    // Use array object syntax for empty array test
    EXPECT_EQ(execute_script(R"(["if", {"array": []}, "then", "else"])"), json("else"));
    EXPECT_EQ(execute_script(R"(["if", {"array": [1]}, "then", "else"])"), json("then"));
}

TEST_F(ControlFlowTest, IfOperatorWrongArgCount) {
    // Test if with wrong number of arguments
    EXPECT_THROW(execute_script(R"(["if", true, "then"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["if", true])"), computo::InvalidArgumentException);
}

TEST_F(ControlFlowTest, IfOperatorNested) {
    // Test nested if expression
    auto result = execute_script(
        R"(["if", true, ["if", false, "nested_then", "nested_else"], "outer_else"])");
    EXPECT_EQ(result, json("nested_else"));
}

// --- Let and Variable Tests ---

TEST_F(ControlFlowTest, LetOperatorBasic) {
    // Test basic let binding: ["let", {"x": 42}, ["$", "/x"]]
    auto result = execute_script(R"(["let", {"x": 42}, ["$", "/x"]])");
    EXPECT_EQ(result, json(42));
}

TEST_F(ControlFlowTest, LetOperatorMultipleBindings) {
    // Test multiple bindings: ["let", {"x": 10, "y": 20}, ["$", "/x"]]
    auto result = execute_script(R"(["let", {"x": 10, "y": 20}, ["$", "/x"]])");
    EXPECT_EQ(result, json(10));

    // Test accessing second binding
    result = execute_script(R"(["let", {"x": 10, "y": 20}, ["$", "/y"]])");
    EXPECT_EQ(result, json(20));
}

TEST_F(ControlFlowTest, LetOperatorNestedExpressions) {
    // Test let with nested expressions in body
    auto result
        = execute_script(R"(["let", {"x": true}, ["if", ["$", "/x"], "then_val", "else_val"]])");
    EXPECT_EQ(result, json("then_val"));
}

TEST_F(ControlFlowTest, LetOperatorEvaluatedBindings) {
    // Test let with evaluated binding values
    auto result = execute_script(R"(["let", {"x": ["if", true, 42, 0]}, ["$", "/x"]])");
    EXPECT_EQ(result, json(42));
}

TEST_F(ControlFlowTest, LetOperatorWrongArgCount) {
    // Test let with wrong number of arguments
    EXPECT_THROW(execute_script(R"(["let", {"x": 42}])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["let"])"), computo::InvalidArgumentException);
}

TEST_F(ControlFlowTest, LetOperatorInvalidBindings) {
    // Test let with non-object bindings
    EXPECT_THROW(execute_script(R"(["let", "not_an_object", ["$", "x"]])"),
                 computo::InvalidArgumentException);
}

TEST_F(ControlFlowTest, VariableOperatorNotFound) {
    // Test $ with non-existent variable
    EXPECT_THROW(execute_script(R"(["$", "nonexistent"])"), computo::InvalidArgumentException);
}

TEST_F(ControlFlowTest, VariableOperatorWrongArgCount) {
    // Test $ with wrong number of arguments
    EXPECT_THROW(execute_script(R"(["$"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["$", "x", "y"])"), computo::InvalidArgumentException);
}

TEST_F(ControlFlowTest, VariableOperatorInvalidArgument) {
    // Test $ with non-string argument
    EXPECT_THROW(execute_script(R"(["$", 42])"), computo::InvalidArgumentException);
}
