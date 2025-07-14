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
    json script = R"([
        "keys", 
        {"a": 1, "b": 2, "c": 3}
    ])"_json;
    json expected = R"(["a", "b", "c"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, KeysOperatorEmptyObject) {
    json script = R"([
        "keys", 
        {}
    ])"_json;
    json expected = R"([])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, KeysOperatorInvalidArgument) {
    json script = R"([
        "keys", 
        [1, 2, 3]
    ])"_json;
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, KeysOperatorWithVariables) {
    json script = R"([
        "let",
        [["obj", {"x": 10, "y": 20}]],
        ["keys", ["$", "/obj"]]
    ])"_json;
    json expected = R"(["x", "y"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

// Test values operator
TEST_F(ObjectOperationsTest, ValuesOperator) {
    json script = R"([
        "values", 
        {"a": 1, "b": 2, "c": 3}
    ])"_json;
    json expected = R"([1, 2, 3])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ValuesOperatorEmptyObject) {
    json script = R"([
        "values", 
        {}
    ])"_json;
    json expected = R"([])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ValuesOperatorInvalidArgument) {
    json script = R"([
        "values", 
        [1, 2, 3]
    ])"_json;
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ValuesOperatorWithComplexValues) {
    json script = R"([
        "values", 
        {
            "data": [1, 2, 3], 
            "meta": {"version": "1.0"}, 
            "flag": true
        }
    ])"_json;
    auto result = exec(script);

    // Check that result is a clean array
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 3);

    // Check that all expected values are present (order doesn't matter)
    bool found_data = false, found_meta = false, found_flag = false;
    for (const auto& value : result) {
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
    json script = R"([
        "objFromPairs", 
        {"array": [["a", 1], ["b", 2], ["c", 3]]}
    ])"_json;
    json expected = R"({"a": 1, "b": 2, "c": 3})"_json;
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorEmptyArray) {
    json script = R"([
        "objFromPairs", 
        {"array": []}
    ])"_json;
    json expected = R"({})"_json;
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorInvalidPair) {
    json script = R"([
        "objFromPairs", 
        {"array": [["a", 1, 2]]}
    ])"_json;
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorNonStringKey) {
    json script = R"([
        "objFromPairs", 
        {"array": [[1, "value"]]}
    ])"_json;
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(ObjectOperationsTest, ObjFromPairsOperatorWithZip) {
    json script = R"([
        "objFromPairs", 
        ["zip", 
            {"array": ["name", "age", "city"]}, 
            {"array": ["Alice", 30, "NYC"]}
        ]
    ])"_json;
    json expected = R"({"name": "Alice", "age": 30, "city": "NYC"})"_json;
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