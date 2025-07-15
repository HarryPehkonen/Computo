#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class ArrayOperatorTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(ArrayOperatorTest, MapOperator) {
    json script = json::array({ "map",
        json::object({ { "array", json::array({ 1, 2, 3 }) } }),
        json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x" }), 2 }) }) });
    json expected = json::array({ 2, 4, 6 }); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayOperatorTest, FilterOperator) {
    json script = json::array({ "filter",
        json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }), // NOLINT(readability-magic-numbers)
        json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x" }), 2 }) }) });
    json expected = json::array({ 3, 4, 5 }); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayOperatorTest, ReduceOperator) {
    json script = json::array({ "reduce",
        json::object({ { "array", json::array({ 1, 2, 3, 4 }) } }),
        json::array({ "lambda", json::array({ "args" }), json::array({ "+", json::array({ "$", "/args/0" }), json::array({ "$", "/args/1" }) }) }),
        0 });
    EXPECT_EQ(exec(script), 10); // NOLINT(readability-magic-numbers)
}

TEST_F(ArrayOperatorTest, CountOperator) {
    json script = json::array({ "count", json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }) }); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(exec(script), 5);
}

TEST_F(ArrayOperatorTest, FindOperator) {
    json script = json::array({ "find",
        json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }), // NOLINT(readability-magic-numbers)
        json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x" }), 3 }) }) });
    EXPECT_EQ(exec(script), 4);
}

TEST_F(ArrayOperatorTest, SomeOperator) {
    json script = json::array({ "some",
        json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }), // NOLINT(readability-magic-numbers)
        json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x" }), 3 }) }) });
    EXPECT_EQ(exec(script), true);
}

TEST_F(ArrayOperatorTest, EveryOperator) {
    json script = json::array({ "every",
        json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }), // NOLINT(readability-magic-numbers)
        json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x" }), 0 }) }) });
    EXPECT_EQ(exec(script), true);
}

TEST_F(ArrayOperatorTest, ZipOperator) {
    json script = json::array({ "zip",
        json::object({ { "array", json::array({ "a", "b", "c" }) } }),
        json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
    json expected = json::array({ json::array({ "a", 1 }), json::array({ "b", 2 }), json::array({ "c", 3 }) }); // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayOperatorTest, ZipOperatorUnequalLengths) {
    json script = json::array({ "zip",
        json::object({ { "array", json::array({ "a", "b", "c", "d" }) } }),
        json::object({ { "array", json::array({ 1, 2 }) } }) });
    json expected = json::array({ json::array({ "a", 1 }), json::array({ "b", 2 }) }); // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayOperatorTest, ZipOperatorEmptyArrays) {
    json script = json::array({ "zip",
        json::object({ { "array", json::array({}) } }),
        json::object({ { "array", json::array({}) } }) });
    json expected = json::array({}); // Clean array output
    EXPECT_EQ(exec(script), expected);
}
