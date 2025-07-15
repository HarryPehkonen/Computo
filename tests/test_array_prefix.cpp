#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class ArrayPrefixTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }

    static auto exec_with_prefix(const json& script, const json& input = json(nullptr), [[maybe_unused]] const std::string& prefix = "$") {
        // TODO: This will use the new prefix-aware execute function
        return computo::execute(script, input);
    }
};

// Basic array unwrapping tests
TEST_F(ArrayPrefixTest, BasicStringArray) {
    json script = json::object({ { "array", json::array({ "one", "two", "three" }) } });
    json expected = json::array({ "one", "two", "three" });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayPrefixTest, BasicNumberArray) {
    json script = json::object({ { "array", json::array({ 1, 2, 3 }) } });
    json expected = json::array({ 1, 2, 3 });
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayPrefixTest, EmptyArray) {
    json script = json::object({ { "array", json::array({}) } });
    json expected = json::array({});
    EXPECT_EQ(exec(script), expected);
}

TEST_F(ArrayPrefixTest, MixedTypeArray) {
    json script = json::object({ { "array", json::array({ "hello", 42, true, json(nullptr) }) } });
    json expected = json::array({ "hello", 42, true, json(nullptr) });
    EXPECT_EQ(exec(script), expected);
}

// Nested array tests
TEST_F(ArrayPrefixTest, NestedArrays) {
    json script = json::object({ { "array", json::array({ json::object({ { "array", json::array({ 1, 2 }) } }), json::object({ { "array", json::array({ 3, 4 }) } }) }) } });
    json expected = json::array({ json::array({ 1, 2 }),
        json::array({ 3, 4 }) });
    EXPECT_EQ(exec(script), expected);
}

// Objects with array should not unwrap if they have other keys
TEST_F(ArrayPrefixTest, ObjectWithMultipleKeys) {
    json script = json::object({ { "array", json::array({ 1, 2, 3 }) },
        { "other", "value" } });
    json expected = json::object({ { "array", json::array({ 1, 2, 3 }) },
        { "other", "value" } });
    EXPECT_EQ(exec(script), expected);
}

// Regular objects should not be affected
TEST_F(ArrayPrefixTest, RegularObjectsUnchanged) {
    json script = json::object({ { "data", json::array({ 1, 2, 3 }) } });
    json expected = json::object({ { "data", json::array({ 1, 2, 3 }) } });
    EXPECT_EQ(exec(script), expected);
}

// Arrays in expressions should work correctly
TEST_F(ArrayPrefixTest, ArrayInMapOperator) {
    json script = json::array({ "map",
        json::object({ { "array", json::array({ 1, 2, 3 }) } }),
        json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x" }), 2 }) }) });
    json expected = json::array({ 2, 4, 6 });
    EXPECT_EQ(exec(script), expected);
}

// Dynamically created array objects should ALSO unwrap consistently
TEST_F(ArrayPrefixTest, DynamicallyCreatedArrayObjectShouldAlsoUnwrap) {
    json script = json::array({ "let",
        json::array({ json::array({ "obj_with_array", json::object({ { "array", json::array({ 1, 2, 3 }) } }) }) }),
        json::object({ { "array", json::array({ json::array({ "$", "/obj_with_array" }) }) } }) });
    // The variable contains {"array": [1,2,3]} which should unwrap to [1,2,3]
    // The outer {"array": [...]} unwraps to an array containing that unwrapped result
    json expected = json::array({ json::array({ 1, 2, 3 }) });
    EXPECT_EQ(exec(script), expected);
}

// Test custom prefix functionality (when implemented)
TEST_F(ArrayPrefixTest, CustomPrefix) {
    json script = json::object({ { "@array", json::array({ "one", "two", "three" }) } });
    json expected = json::array({ "one", "two", "three" });
    // TODO: This will use exec_with_prefix when prefix support is implemented
    // EXPECT_EQ(exec_with_prefix(script, json(nullptr), "@"), expected);
}

// Test that array in real data doesn't get unwrapped when using different syntax
TEST_F(ArrayPrefixTest, ConflictResolutionWithCustomPrefix) {
    // When using @array syntax, regular "array" should be treated as regular data
    json script = json::object({ { "array", json::array({ "this", "should", "not", "unwrap" }) } });
    json expected = json::object({ { "array", json::array({ "this", "should", "not", "unwrap" }) } });
    // TODO: This will test that @array syntax means "array" is treated as data
    // EXPECT_EQ(exec_with_array_syntax(script, json(nullptr), "@array"), expected);
}