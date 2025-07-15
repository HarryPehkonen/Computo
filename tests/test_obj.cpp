#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class ObjOperatorTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(ObjOperatorTest, BasicObject) {
    json script = json::array({ "obj",
        json::array({ "name", "Alice" }),
        json::array({ "age", 30 }) });
    json expected = json { { "name", "Alice" }, { "age", 30 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjOperatorTest, VariableKeys) {
    json script = json::array({ "let",
        json::array({ json::array({ "key", "title" }),
            json::array({ "value", "Manager" }) }),
        json::array({ "obj",
            json::array({ json::array({ "$", "/key" }), json::array({ "$", "/value" }) }) }) });
    json expected = json { { "title", "Manager" } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjOperatorTest, MixedStaticAndVariableKeys) {
    json script = json::array({ "let",
        json::array({ json::array({ "dynamic_key", "salary" }) }),
        json::array({ "obj",
            json::array({ "name", "Bob" }),
            json::array({ json::array({ "$", "/dynamic_key" }), 50000 }) }) });
    json expected = json { { "name", "Bob" }, { "salary", 50000 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjOperatorTest, InvalidKeyTypeThrows) {
    json script = json::array({ "obj",
        json::array({ 42, "value" }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}
