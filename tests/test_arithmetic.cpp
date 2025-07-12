#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class ArithmeticLogicalTest : public ::testing::Test {
protected:
    json input = json(nullptr);
    auto exec(const json& script) { return computo::execute(script, input); }
};

TEST_F(ArithmeticLogicalTest, Addition) {
    json script = json::array({ "+", 1, 2, 3 });
    EXPECT_EQ(exec(script), 6);
}

TEST_F(ArithmeticLogicalTest, Subtraction) {
    EXPECT_EQ(exec(json::array({ "-", 10, 3, 2 })), 5);
    EXPECT_EQ(exec(json::array({ "-", 5 })), -5);
}

TEST_F(ArithmeticLogicalTest, Multiplication) {
    EXPECT_EQ(exec(json::array({ "*", 2, 3, 4 })), 24);
}

TEST_F(ArithmeticLogicalTest, Division) {
    EXPECT_EQ(exec(json::array({ "/", 20, 2, 2 })), 5);
    EXPECT_EQ(exec(json::array({ "/", 4 })), 0.25);
}

TEST_F(ArithmeticLogicalTest, Modulo) {
    EXPECT_EQ(exec(json::array({ "%", 20, 6 })), 2);
}

TEST_F(ArithmeticLogicalTest, LogicalAndOrNot) {
    EXPECT_EQ(exec(json::array({ "&&", true, 1, "non-empty" })), true);
    EXPECT_EQ(exec(json::array({ "||", false, 0, "" })), false);
    EXPECT_EQ(exec(json::array({ "||", false, 0, 3 })), true);
    EXPECT_EQ(exec(json::array({ "not", false })), true);
}

TEST_F(ArithmeticLogicalTest, AdditionInvalidType) {
    EXPECT_THROW(exec(json::array({ "+", "str", 1 })), computo::InvalidArgumentException);
}