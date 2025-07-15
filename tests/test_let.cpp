#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class LetOperatorTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(LetOperatorTest, BasicLet) {
    json script = json::array({ "let",
        json::array({ json::array({ "x", 42 }) }),
        json::array({ "$", "/x" }) });
    EXPECT_EQ(exec(script), 42);
}

TEST_F(LetOperatorTest, MultipleBindings) {
    json script = json::array({ "let",
        json::array({ json::array({ "a", 10 }),
            json::array({ "b", 20 }) }),
        json::array({ "+", json::array({ "$", "/a" }), json::array({ "$", "/b" }) }) });
    EXPECT_EQ(exec(script), 30);
}

TEST_F(LetOperatorTest, NestedLet) {
    json script = json::array({ "let",
        json::array({ json::array({ "x", 1 }) }),
        json::array({ "let",
            json::array({ json::array({ "x", 2 }) }),
            json::array({ "$", "/x" }) }) });
    EXPECT_EQ(exec(script), 2);
}

TEST_F(LetOperatorTest, LetWithComplexExpressions) {
    json script = json::array({ "let",
        json::array({ json::array({ "sum", json::array({ "+", 1, 2, 3 }) }) }),
        json::array({ "+", json::array({ "$", "/sum" }), 10 }) });
    EXPECT_EQ(exec(script), 16);
}