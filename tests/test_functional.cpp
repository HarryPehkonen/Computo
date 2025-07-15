#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class FunctionalOperatorTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(FunctionalOperatorTest, CarOperator) {
    json script = R"([
        "car", 
        {"array": [1, 2, 3]}
    ])"_json;
    EXPECT_EQ(exec(script), 1);
}

TEST_F(FunctionalOperatorTest, CdrOperator) {
    json script = R"([
        "cdr", 
        {"array": [1, 2, 3]}
    ])"_json;
    json expected = R"([2, 3])"_json; // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(FunctionalOperatorTest, ConsOperator) {
    json script = R"([
        "cons", 
        0, 
        {"array": [1, 2, 3]}
    ])"_json;
    json expected = R"([0, 1, 2, 3])"_json; // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(FunctionalOperatorTest, AppendOperator) {
    json script = R"([
        "append", 
        {"array": [1, 2]}, 
        {"array": [3, 4]}
    ])"_json;
    json expected = R"([1, 2, 3, 4])"_json; // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(FunctionalOperatorTest, CarThrowsOnEmpty) {
    json script = R"([
        "car", 
        {"array": []}
    ])"_json;
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(FunctionalOperatorTest, CdrThrowsOnEmpty) {
    json script = R"([
        "cdr", 
        {"array": []}
    ])"_json;
    EXPECT_THROW(exec(script), std::exception);
}