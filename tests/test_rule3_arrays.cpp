#include <computo.hpp>
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

class Rule3ArrayTest : public ::testing::Test {
protected:
    auto execute_script(const std::string& script_json) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script);
    }

    auto execute_script_with_input(const std::string& script_json, const json& input) -> json {
        auto script = jsom::parse_document(script_json);
        return computo::execute(script, {input});
    }
};

// Test Rule 3: Arrays with Non-String First Elements should be treated as literal arrays
// According to AI/ARRAY_HANDLING.md Rule 3: [non_string, ...] → Treat as literal array

TEST_F(Rule3ArrayTest, NumericFirstElement) {
    // Test: [1, 2, 3] should work as literal array (Rule 3)
    // Currently this fails but should work according to the spec
    EXPECT_NO_THROW({
        auto result = execute_script("[1, 2, 3]");
        EXPECT_EQ(result, json(std::vector<json>{1, 2, 3}));
    });
}

TEST_F(Rule3ArrayTest, BooleanFirstElement) {
    // Test: [true, false, "test"] should work as literal array (Rule 3)
    EXPECT_NO_THROW({
        auto result = execute_script("[true, false, \"test\"]");
        EXPECT_EQ(result, json(std::vector<json>{true, false, "test"}));
    });
}

TEST_F(Rule3ArrayTest, NullFirstElement) {
    // Test: [null, 42] should work as literal array (Rule 3)
    EXPECT_NO_THROW({
        auto result = execute_script("[null, 42]");
        EXPECT_EQ(result, json(std::vector<json>{nullptr, 42}));
    });
}

TEST_F(Rule3ArrayTest, ArrayFirstElement) {
    // Test: [[], "hello"] should work as literal array (Rule 3)
    EXPECT_NO_THROW({
        auto result = execute_script("[[], \"hello\"]");
        EXPECT_EQ(result, json(std::vector<json>{json::make_array(), "hello"}));
    });
}

TEST_F(Rule3ArrayTest, ObjectFirstElement) {
    // Test: [{"key": "value"}, 123] should work as literal array (Rule 3)
    EXPECT_NO_THROW({
        auto result = execute_script("[{\"key\": \"value\"}, 123]");
        EXPECT_EQ(result, json(std::vector<json>{json{{"key", "value"}}, 123}));
    });
}

// Test that arrays starting with known operators still work (Rule 1)
TEST_F(Rule3ArrayTest, KnownOperatorStillWorks) {
    // Test: ["+", 1, 2, 3] should still work as operator call (Rule 1)
    auto result = execute_script("[\"+\", 1, 2, 3]");
    EXPECT_EQ(result, json(6));
}

// Test that arrays starting with unknown operators fail (Rule 2)
TEST_F(Rule3ArrayTest, UnknownOperatorStillFails) {
    // Test: ["unknown", 1, 2] should still fail (Rule 2)
    EXPECT_THROW(execute_script("[\"unknown\", 1, 2]"), computo::InvalidOperatorException);
}

// Test that array objects still work (Rule 4)
TEST_F(Rule3ArrayTest, ArrayObjectsStillWork) {
    // Test: {"array": [1, 2, 3]} should still work (Rule 4)
    auto result = execute_script("{\"array\": [1, 2, 3]}");
    EXPECT_EQ(result, json(std::vector<json>{1, 2, 3}));
}

// Test mixed scenarios with Rule 3 arrays in operations
TEST_F(Rule3ArrayTest, Rule3ArraysInOperations) {
    // Test using Rule 3 arrays as inputs to operations
    // This should work: map over [1, 2, 3] directly
    EXPECT_NO_THROW({
        auto result = execute_script_with_input(
            "[\"map\", [\"$input\"], [\"lambda\", [\"x\"], [\"*\", [\"$\", \"/x\"], 2]]]",
            json(std::vector<json>{1, 2, 3}));
        // Result should be in array object format when processing arrays
        auto expected = jsom::parse_document(R"({"array":[2,4,6]})");
        EXPECT_EQ(result, expected);
    });
}

// Test functional operations with Rule 3 arrays
TEST_F(Rule3ArrayTest, Rule3ArraysWithFunctionalOps) {
    // Test cons with a Rule 3 array
    EXPECT_NO_THROW({
        auto result = execute_script("[\"cons\", 42, [1, 2, 3, 4, 5]]");
        // Result should be in array object format when processing arrays
        auto expected = jsom::parse_document(R"({"array":[42,1,2,3,4,5]})");
        EXPECT_EQ(result, expected);
    });

    // Test append with Rule 3 arrays
    EXPECT_NO_THROW({
        auto result = execute_script("[\"append\", [1, 2, 3], [4, 5, 6]]");
        // Result should be in array object format when processing arrays
        auto expected = jsom::parse_document(R"({"array":[1,2,3,4,5,6]})");
        EXPECT_EQ(result, expected);
    });
}

// Test edge cases
TEST_F(Rule3ArrayTest, EmptyArrayRule3) {
    // Test: [] should work as empty literal array
    auto result = execute_script("[]");
    EXPECT_EQ(result, json::make_array());
}

TEST_F(Rule3ArrayTest, SingleElementRule3) {
    // Test: [42] should work as single-element literal array
    auto result = execute_script("[42]");
    EXPECT_EQ(result, json(std::vector<json>{42}));
}

// Test nested scenarios
TEST_F(Rule3ArrayTest, NestedRule3Arrays) {
    // Test nested Rule 3 arrays: [[1, 2], [3, 4]]
    EXPECT_NO_THROW({
        auto result = execute_script("[[1, 2], [3, 4]]");
        EXPECT_EQ(result, json(std::vector<json>{json(std::vector<json>{1, 2}), json(std::vector<json>{3, 4})}));
    });
}
