#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class ObjectOperationsTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

// Test keys operator
TEST_F(ObjectOperationsTest, KeysOperator) {
    json script = json::array({ "keys", json { { "a", 1 }, { "b", 2 }, { "c", 3 } } });
    json expected = json::object({ { "array", json::array({ "a", "b", "c" }) } });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, KeysOperatorEmptyObject) {
    json script = json::array({ "keys", json::object() });
    json expected = json::object({ { "array", json::array() } });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, KeysOperatorInvalidArgument) {
    json script = json::array({ "keys", json::array({ 1, 2, 3 }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, KeysOperatorWithVariables) {
    json script = json::array({ "let",
        json::array({ json::array({ "obj", json { { "x", 10 }, { "y", 20 } } }) }),
        json::array({ "keys", json::array({ "$", "/obj" }) }) });
    json expected = json::object({ { "array", json::array({ "x", "y" }) } });
    EXPECT_EQ(exec(script), expected);
}

// Test values operator
TEST_F(ObjectOperationsTest, ValuesOperator) {
    json script = json::array({ "values", json { { "a", 1 }, { "b", 2 }, { "c", 3 } } });
    json expected = json::object({ { "array", json::array({ 1, 2, 3 }) } });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ValuesOperatorEmptyObject) {
    json script = json::array({ "values", json::object() });
    json expected = json::object({ { "array", json::array() } });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ValuesOperatorInvalidArgument) {
    json script = json::array({ "values", json::array({ 1, 2, 3 }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ValuesOperatorWithComplexValues) {
    json script = json::array({ "values", json { { "data", json::array({ 1, 2, 3 }) }, { "meta", json { { "version", "1.0" } } }, { "flag", true } } });
    auto result = exec(script);

    // Check that result is an array container
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("array"));
    ASSERT_TRUE(result["array"].is_array());

    // Check that all expected values are present (order doesn't matter)
    auto values = result["array"];
    EXPECT_EQ(values.size(), 3);

    bool found_data = false, found_meta = false, found_flag = false;
    for (const auto& value : values) {
        if (value == json::array({ 1, 2, 3 }))
            found_data = true;
        else if (value == json { { "version", "1.0" } })
            found_meta = true;
        else if (value == true)
            found_flag = true;
    }

    EXPECT_TRUE(found_data);
    EXPECT_TRUE(found_meta);
    EXPECT_TRUE(found_flag);
}

// Test objFromPairs operator
TEST_F(ObjectOperationsTest, ObjFromPairsOperator) {
    json script = json::array({ "objFromPairs", json::object({ { "array", json::array({ json::array({ "a", 1 }), json::array({ "b", 2 }), json::array({ "c", 3 }) }) } }) });
    json expected = json { { "a", 1 }, { "b", 2 }, { "c", 3 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorEmptyArray) {
    json script = json::array({ "objFromPairs", json::object({ { "array", json::array() } }) });
    json expected = json::object();
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorInvalidPair) {
    json script = json::array({ "objFromPairs", json::object({ { "array", json::array({ json::array({ "a", 1, 2 }) }) } }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorNonStringKey) {
    json script = json::array({ "objFromPairs", json::object({ { "array", json::array({ json::array({ 1, "value" }) }) } }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorWithZip) {
    json script = json::array({ "objFromPairs", json::array({ "zip", json::object({ { "array", json::array({ "name", "age", "city" }) } }), json::object({ { "array", json::array({ "Alice", 30, "NYC" }) } }) }) });
    json expected = json { { "name", "Alice" }, { "age", 30 }, { "city", "NYC" } };
    EXPECT_EQ(exec(script), expected);
}

// Test pick operator
TEST_F(ObjectOperationsTest, PickOperator) {
    json script = json::array({ "pick",
        json { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } },
        json::object({ { "array", json::array({ "a", "c" }) } }) });
    json expected = json { { "a", 1 }, { "c", 3 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, PickOperatorNonexistentKeys) {
    json script = json::array({ "pick",
        json { { "a", 1 }, { "b", 2 } },
        json::object({ { "array", json::array({ "a", "x", "y" }) } }) });
    json expected = json { { "a", 1 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, PickOperatorEmptyKeys) {
    json script = json::array({ "pick",
        json { { "a", 1 }, { "b", 2 } },
        json::object({ { "array", json::array() } }) });
    json expected = json::object();
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, PickOperatorInvalidArgument) {
    json script = json::array({ "pick",
        json::array({ 1, 2, 3 }),
        json::object({ { "array", json::array({ "a" }) } }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

// Test omit operator
TEST_F(ObjectOperationsTest, OmitOperator) {
    json script = json::array({ "omit",
        json { { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } },
        json::object({ { "array", json::array({ "b", "d" }) } }) });
    json expected = json { { "a", 1 }, { "c", 3 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, OmitOperatorNonexistentKeys) {
    json script = json::array({ "omit",
        json { { "a", 1 }, { "b", 2 } },
        json::object({ { "array", json::array({ "x", "y" }) } }) });
    json expected = json { { "a", 1 }, { "b", 2 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, OmitOperatorEmptyKeys) {
    json script = json::array({ "omit",
        json { { "a", 1 }, { "b", 2 } },
        json::object({ { "array", json::array() } }) });
    json expected = json { { "a", 1 }, { "b", 2 } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, OmitOperatorInvalidArgument) {
    json script = json::array({ "omit",
        json::array({ 1, 2, 3 }),
        json::object({ { "array", json::array({ "a" }) } }) });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

// Test complex scenarios
TEST_F(ObjectOperationsTest, ComplexObjectTransformation) {
    json script = json::array({ "let",
        json::array({ json::array({ "user", json { { "name", "Alice" }, { "age", 30 }, { "email", "alice@example.com" }, { "password", "secret" } } }) }),
        json::array({ "objFromPairs",
            json::array({ "zip",
                json::array({ "keys", json::array({ "omit", json::array({ "$", "/user" }), json::object({ { "array", json::array({ "password" }) } }) }) }),
                json::array({ "values", json::array({ "omit", json::array({ "$", "/user" }), json::object({ { "array", json::array({ "password" }) } }) }) }) }) }) });
    json expected = json { { "name", "Alice" }, { "age", 30 }, { "email", "alice@example.com" } };
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, DynamicObjectConstruction) {
    json script = json::array({ "let",
        json::array({ json::array({ "data", json { { "items", json::object({ { "array", json::array({ "a", "b", "c" }) } }) }, { "prefix", "item_" } } }) }),
        json::array({ "objFromPairs",
            json::array({ "zip",
                json::array({ "map",
                    json::array({ "$", "/data/items" }),
                    json::array({ "lambda", json::array({ "x" }), json::array({ "strConcat", json::array({ "$", "/data/prefix" }), json::array({ "$", "/x" }) }) }) }),
                json::array({ "$", "/data/items" }) }) }) });
    json expected = json { { "item_a", "a" }, { "item_b", "b" }, { "item_c", "c" } };
    EXPECT_EQ(exec(script), expected);
}

// Test argument validation
TEST_F(ObjectOperationsTest, ArgumentValidation) {
    // Test wrong number of arguments
    EXPECT_THROW(exec(json::array({ "keys" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "keys", json::object(), json::object() })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "values" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "objFromPairs" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "pick", json::object() })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "omit", json::object() })), computo::InvalidArgumentException);
}