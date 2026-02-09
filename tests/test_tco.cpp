#include <computo.hpp>
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

class TCOTest : public ::testing::Test {
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

// --- TCO (Tail Call Optimization) Tests ---

TEST_F(TCOTest, DeepRecursionIf) {
    // Test that deep recursion with if operator doesn't cause stack overflow
    // Create a deeply nested if expression: if(true, if(true, if(true, ...,
    // "result"), "else"), "else")
    std::string deep_if = R"("result")";

    // Build nested if expressions (100 levels deep)
    constexpr int DEEP_IF_NESTING_LEVELS = 100;
    for (int i = 0; i < DEEP_IF_NESTING_LEVELS; ++i) {
        std::string temp = R"(["if", true, )";
        temp += deep_if;
        temp += R"(, "else"])";
        deep_if = std::move(temp);
    }

    // This should not cause stack overflow thanks to TCO
    EXPECT_EQ(execute_script(deep_if), json("result"));
}

TEST_F(TCOTest, DeepRecursionLet) {
    // Test that deep recursion with let operator doesn't cause stack overflow
    // Create deeply nested let expressions: let({"x": 1}, let({"y": 2},
    // let({"z": 3}, ..., "result")))
    std::string deep_let = R"("result")";

    // Build nested let expressions (100 levels deep)
    constexpr int DEEP_LET_NESTING_LEVELS = 100;
    for (int i = 0; i < DEEP_LET_NESTING_LEVELS; ++i) {
        std::string temp
            = R"(["let", {"x)" + std::to_string(i) + R"(": )" + std::to_string(i) + R"(}, )";
        temp += deep_let;
        temp += R"(])";
        deep_let = std::move(temp);
    }

    // This should not cause stack overflow thanks to TCO
    EXPECT_EQ(execute_script(deep_let), json("result"));
}

TEST_F(TCOTest, DeepRecursionMixed) {
    // Test mixed deep recursion with if and let operators
    std::string deep_mixed = R"("final")";

    // Build mixed nested expressions (50 levels deep)
    constexpr int MIXED_NESTING_LEVELS = 50;
    for (int i = 0; i < MIXED_NESTING_LEVELS; ++i) {
        std::string temp;
        if (i % 2 == 0) {
            temp = R"(["if", true, )";
            temp += deep_mixed;
            temp += R"(, "else"])";
        } else {
            temp = R"(["let", {"x)" + std::to_string(i) + R"(": )" + std::to_string(i) + R"(}, )";
            temp += deep_mixed;
            temp += R"(])";
        }
        deep_mixed = std::move(temp);
    }

    // This should not cause stack overflow thanks to TCO
    EXPECT_EQ(execute_script(deep_mixed), json("final"));
}

TEST_F(TCOTest, DeepRecursionConditional) {
    // Test conditional deep recursion that actually evaluates different
    // branches
    std::string deep_conditional = R"("success")";

    // Build nested if expressions where condition depends on iteration
    constexpr int CONDITIONAL_NESTING_LEVELS = 50;
    constexpr int CONDITION_MODULO = 10;
    constexpr int CONDITION_REMAINDER = 9;
    for (int i = 0; i < CONDITIONAL_NESTING_LEVELS; ++i) {
        bool condition = (i % CONDITION_MODULO != CONDITION_REMAINDER); // Most conditions are true,
                                                                        // except every 10th
        std::string cond_str = condition ? "true" : "false";
        std::string else_value = condition ? R"("fail")" : deep_conditional;
        std::string temp = R"(["if", )";
        temp += cond_str;
        temp += R"(, )";
        temp += deep_conditional;
        temp += R"(, )";
        temp += else_value;
        temp += R"(])";
        deep_conditional = std::move(temp);
    }

    // This should not cause stack overflow and should return "success"
    EXPECT_EQ(execute_script(deep_conditional), json("success"));
}
