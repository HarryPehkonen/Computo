#include <computo.hpp>
#include <gtest/gtest.h>

using namespace computo;

class ArrayKeyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default array key behavior
TEST_F(ArrayKeyTest, DefaultArrayKey) {
    jsom::JsonDocument script = jsom::parse_document(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    std::vector<jsom::JsonDocument> inputs = {42};

    auto result = execute(script, inputs);
    auto expected = jsom::parse_document(R"({"array": [2.0, 4.0, 6.0]})");

    EXPECT_EQ(result, expected);
}

// Test custom array key functionality
TEST_F(ArrayKeyTest, CustomArrayKey) {
    jsom::JsonDocument script = jsom::parse_document(
        R"(["map", {"@array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    std::vector<jsom::JsonDocument> inputs = {42};

    auto result = execute(script, inputs, nullptr, "@array");
    auto expected = jsom::parse_document(R"({"@array": [2.0, 4.0, 6.0]})");

    EXPECT_EQ(result, expected);
}

// Test ability to output literal "array" objects with custom key
TEST_F(ArrayKeyTest, LiteralArrayOutput) {
    jsom::JsonDocument script = jsom::parse_document(R"({"@array": [{"array": [1, 2, 3]}]})");
    std::vector<jsom::JsonDocument> inputs = {42};

    auto result = execute(script, inputs, nullptr, "@array");
    auto expected = jsom::parse_document(R"([{"array": [1, 2, 3]}])");

    EXPECT_EQ(result, expected);
}

// Test mixed usage scenario
TEST_F(ArrayKeyTest, MixedScenario) {
    jsom::JsonDocument script = jsom::parse_document(
        R"(["obj", "data", {"$array": [1, 2, 3]}, "metadata", {"array": ["original", "preserved"]}])");
    std::vector<jsom::JsonDocument> inputs = {42};

    auto result = execute(script, inputs, nullptr, "$array");
    auto expected = jsom::parse_document(R"({
        "data": [1, 2, 3],
        "metadata": {"array": ["original", "preserved"]}
    })");

    EXPECT_EQ(result, expected);
}

// Test all array-returning operators use custom key
TEST_F(ArrayKeyTest, AllOperatorsUseCustomKey) {
    std::vector<jsom::JsonDocument> inputs = {42};
    std::string custom_key = "@test";

    // Test functional operators
    auto result1 = execute(jsom::parse_document(R"(["cdr", {"@test": [1, 2, 3]}])"), inputs,
                           nullptr, custom_key);
    EXPECT_EQ(result1["@test"], jsom::parse_document("[2, 3]"));

    auto result2 = execute(jsom::parse_document(R"(["cons", 0, {"@test": [1, 2, 3]}])"), inputs,
                           nullptr, custom_key);
    EXPECT_EQ(result2["@test"], jsom::parse_document("[0, 1, 2, 3]"));

    auto result3
        = execute(jsom::parse_document(R"(["append", {"@test": [1, 2]}, {"@test": [3, 4]}])"),
                  inputs, nullptr, custom_key);
    EXPECT_EQ(result3["@test"], jsom::parse_document("[1, 2, 3, 4]"));

    // Test array operators
    auto result4 = execute(
        jsom::parse_document(R"(["map", {"@test": [1, 2]}, ["lambda", ["x"], ["$", "/x"]]])"),
        inputs, nullptr, custom_key);
    EXPECT_EQ(result4["@test"], jsom::parse_document("[1, 2]"));

    auto result5 = execute(
        jsom::parse_document(
            R"(["filter", {"@test": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 1]]])"),
        inputs, nullptr, custom_key);
    EXPECT_EQ(result5["@test"], jsom::parse_document("[2, 3]"));

    // Test string utility operators
    auto result6 = execute(jsom::parse_document(R"(["reverse", {"@test": [1, 2, 3]}])"), inputs,
                           nullptr, custom_key);
    EXPECT_EQ(result6["@test"], jsom::parse_document("[3, 2, 1]"));

    auto result7 = execute(jsom::parse_document(R"(["unique", {"@test": [1, 2, 2, 3]}])"), inputs,
                           nullptr, custom_key);
    EXPECT_EQ(result7["@test"], jsom::parse_document("[1, 2, 3]"));

    // Test object operators
    auto result8 = execute(jsom::parse_document(R"(["keys", {"name": "test", "value": 42}])"),
                           inputs, nullptr, custom_key);
    EXPECT_EQ(result8["@test"], jsom::parse_document(R"(["name", "value"])"));

    auto result9 = execute(jsom::parse_document(R"(["values", {"name": "test", "value": 42}])"),
                           inputs, nullptr, custom_key);
    EXPECT_EQ(result9["@test"], jsom::parse_document(R"(["test", 42])"));
}

// Test error handling with invalid array key
TEST_F(ArrayKeyTest, MismatchedArrayKey) {
    jsom::JsonDocument script = jsom::parse_document(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    std::vector<jsom::JsonDocument> inputs = {42};

    // Using wrong key should cause error
    EXPECT_THROW(execute(script, inputs, nullptr, "@different"), InvalidArgumentException);
}