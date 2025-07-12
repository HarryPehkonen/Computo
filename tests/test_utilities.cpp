#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class UtilityOperatorTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(UtilityOperatorTest, StrConcatOperator) {
    json script = json::array({ "strConcat",
        "Hello, ",
        "World!",
        " How are you?" });
    EXPECT_EQ(exec(script), "Hello, World! How are you?");
}

TEST_F(UtilityOperatorTest, MergeOperator) {
    json script = json::array({ "merge",
        json::object({ { "a", 1 }, { "b", 2 } }),
        json::object({ { "c", 3 }, { "d", 4 } }),
        json::object({ { "e", 5 } }) });
    json expected = json::object({ { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 }, { "e", 5 } });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(UtilityOperatorTest, ApproxOperator) {
    json script = json::array({ "approx",
        3.14159,
        3.14159 });
    EXPECT_EQ(exec(script), true);
}

TEST_F(UtilityOperatorTest, ApproxOperatorClose) {
    json script = json::array({ "approx",
        3.14159,
        3.1415900001 });
    EXPECT_EQ(exec(script), true);
}

TEST_F(UtilityOperatorTest, ApproxOperatorDifferent) {
    json script = json::array({ "approx",
        3.14159,
        3.15 });
    EXPECT_EQ(exec(script), false);
}

TEST_F(UtilityOperatorTest, StrConcatInvalidType) {
    json script = json::array({ "strConcat",
        "Hello",
        42 });
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(UtilityOperatorTest, MergeInvalidType) {
    json script = json::array({ "merge",
        json::object({ { "a", 1 } }),
        "not an object" });
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(UtilityOperatorTest, ApproxInvalidType) {
    json script = json::array({ "approx",
        "not a number",
        3.14159 });
    EXPECT_THROW(exec(script), std::exception);
}