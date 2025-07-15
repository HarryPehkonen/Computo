#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class ComparisonTest : public ::testing::Test {
protected:
    static auto exec(const json& s) { return computo::execute(s, json(nullptr)); }
};

TEST_F(ComparisonTest, GreaterThan) {
    EXPECT_EQ(exec(json::array({ ">", 5, 3, 1 })), true);
    EXPECT_EQ(exec(json::array({ ">", 5, 5 })), false);
}

TEST_F(ComparisonTest, LessEqual) {
    EXPECT_EQ(exec(json::array({ "<=", 1, 2, 2 })), true);
}

TEST_F(ComparisonTest, Equal) {
    EXPECT_EQ(exec(json::array({ "==", 3, 3, 3 })), true);
    EXPECT_EQ(exec(json::array({ "==", 3, 4 })), false);
}

TEST_F(ComparisonTest, NotEqual) {
    EXPECT_EQ(exec(json::array({ "!=", 1, 2 })), true);
    EXPECT_EQ(exec(json::array({ "!=", 2, 2 })), false);
}