#include <computo.hpp>
#include <gtest/gtest.h>

using json = jsom::JsonDocument;
using namespace computo;

class JsonPointerTest : public ::testing::Test {
protected:
    void SetUp() override { input_data = json{{"test", "value"}}; }

    auto execute_script(const std::string& script_json) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input});
    }

    json input_data;
};

// --- JSON Pointer Variable Access Tests ---

TEST_F(JsonPointerTest, SimpleVariableAccess) {
    // Test simple variable access with JSON Pointer syntax
    auto result = execute_script(R"(["let", {"x": 42}, ["$", "/x"]])");
    EXPECT_EQ(result, json(42));
}

TEST_F(JsonPointerTest, NestedObjectAccess) {
    // Test accessing nested object properties
    auto result
        = execute_script(R"(["let", {"user": {"name": "Alice", "age": 30}}, ["$", "/user/name"]])");
    EXPECT_EQ(result, json("Alice"));

    result
        = execute_script(R"(["let", {"user": {"name": "Alice", "age": 30}}, ["$", "/user/age"]])");
    EXPECT_EQ(result, json(30));
}

TEST_F(JsonPointerTest, ArrayIndexAccess) {
    // Test accessing array elements by index
    auto result = execute_script(R"(["let", {"items": [10, 20, 30]}, ["$", "/items/0"]])");
    EXPECT_EQ(result, json(10));

    result = execute_script(R"(["let", {"items": [10, 20, 30]}, ["$", "/items/2"]])");
    EXPECT_EQ(result, json(30));
}

TEST_F(JsonPointerTest, DeepNestedAccess) {
    // Test deep nested access combining objects and arrays
    auto result = execute_script(
        R"(["let", {"data": {"users": [{"name": "Bob", "id": 1}, {"name": "Carol", "id": 2}]}}, ["$", "/data/users/1/name"]])");
    EXPECT_EQ(result, json("Carol"));

    result = execute_script(
        R"(["let", {"config": {"database": {"connection": {"host": "localhost", "port": 5432}}}}, ["$", "/config/database/connection/host"]])");
    EXPECT_EQ(result, json("localhost"));
}

TEST_F(JsonPointerTest, ComplexExpressionWithPointers) {
    // Test using JSON Pointer access in complex expressions
    auto result = execute_script(
        R"(["let", {"a": {"value": 10}, "b": {"value": 20}}, ["+", ["$", "/a/value"], ["$", "/b/value"]]])");
    EXPECT_EQ(result, json(30));
}

TEST_F(JsonPointerTest, ErrorHandling) {
    // Test error handling for invalid JSON Pointers
    EXPECT_THROW(
        { execute_script(R"(["let", {"x": 42}, ["$", "/y"]])"); }, InvalidArgumentException);

    EXPECT_THROW(
        { execute_script(R"(["let", {"x": {"a": 1}}, ["$", "/x/b"]])"); },
        InvalidArgumentException);

    EXPECT_THROW(
        { execute_script(R"(["let", {"x": [1, 2]}, ["$", "/x/5"]])"); }, InvalidArgumentException);
}

TEST_F(JsonPointerTest, RequiresSlashPrefix) {
    // Test that variable access requires JSON Pointer format
    EXPECT_THROW(
        { execute_script(R"(["let", {"x": 42}, ["$", "x"]])"); }, InvalidArgumentException);
}
