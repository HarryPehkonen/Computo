#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class FunctionalOperatorTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(FunctionalOperatorTest, CarOperator) {
    json script = json::array({ "car", json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
    EXPECT_EQ(exec(script), 1);
}

TEST_F(FunctionalOperatorTest, CdrOperator) {
    json script = json::array({ "cdr", json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 2, 3 }) } }));
}

TEST_F(FunctionalOperatorTest, ConsOperator) {
    json script = json::array({ "cons", 0, json::object({ { "array", json::array({ 1, 2, 3 }) } }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 0, 1, 2, 3 }) } }));
}

TEST_F(FunctionalOperatorTest, AppendOperator) {
    json script = json::array({ "append", json::object({ { "array", json::array({ 1, 2 }) } }), json::object({ { "array", json::array({ 3, 4 }) } }) });
    EXPECT_EQ(exec(script), json::object({ { "array", json::array({ 1, 2, 3, 4 }) } }));
}

TEST_F(FunctionalOperatorTest, CarThrowsOnEmpty) {
    json script = json::array({ "car", json::object({ { "array", json::array({}) } }) });
    EXPECT_THROW(exec(script), std::exception);
}

TEST_F(FunctionalOperatorTest, CdrThrowsOnEmpty) {
    json script = json::array({ "cdr", json::object({ { "array", json::array({}) } }) });
    EXPECT_THROW(exec(script), std::exception);
}