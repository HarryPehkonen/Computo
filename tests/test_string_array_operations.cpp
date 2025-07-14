#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class StringArrayOperationsTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

// Test split operator
TEST_F(StringArrayOperationsTest, SplitOperatorBasic) {
    json script = R"(["split", "hello world", " "])"_json;
    json expected = R"(["hello", "world"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorMultipleDelimiters) {
    json script = R"(["split", "a,b,c", ","])"_json;
    json expected = R"(["a", "b", "c"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorEmptyDelimiter) {
    json script = R"(["split", "abc", ""])"_json;
    json expected = R"(["a", "b", "c"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorEmptyString) {
    json script = R"(["split", "", ","])"_json;
    json expected = R"([""])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorNoDelimiterFound) {
    json script = R"(["split", "hello", ","])"_json;
    json expected = R"(["hello"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitOperatorInvalidArguments) {
    EXPECT_THROW(exec(R"(["split", "hello"])"_json), computo::InvalidArgumentException);
    EXPECT_THROW(exec(R"(["split", 123, " "])"_json), computo::InvalidArgumentException);
    EXPECT_THROW(exec(R"(["split", "hello", 123])"_json), computo::InvalidArgumentException);
}

// Test join operator
TEST_F(StringArrayOperationsTest, JoinOperatorBasic) {
    json script = R"([
        "join",
        {"array": ["hello", "world"]},
        " "
    ])"_json;
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, JoinOperatorEmptySeparator) {
    json script = R"([
        "join",
        {"array": ["a", "b", "c"]},
        ""
    ])"_json;
    EXPECT_EQ(exec(script), "abc");
}

TEST_F(StringArrayOperationsTest, JoinOperatorEmptyArray) {
    json script = R"([
        "join",
        {"array": []},
        ","
    ])"_json;
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, JoinOperatorMixedTypes) {
    json script = R"([
        "join",
        {"array": ["hello", 42, true, null]},
        ","
    ])"_json;
    EXPECT_EQ(exec(script), "hello,42,true,null");
}

TEST_F(StringArrayOperationsTest, JoinOperatorComplexElements) {
    json script = R"([
        "join",
        {"array": [[1, 2, 3], {"key": "value"}]},
        " | "
    ])"_json;
    EXPECT_EQ(exec(script), "[1,2,3] | {\"key\":\"value\"}");
}

TEST_F(StringArrayOperationsTest, JoinOperatorInvalidArguments) {
    EXPECT_THROW(exec(R"(["join", {"array": ["a"]}])"_json), computo::InvalidArgumentException);
    EXPECT_THROW(exec(R"(["join", "not_an_array", ","])"_json), computo::InvalidArgumentException);
    EXPECT_THROW(exec(R"(["join", {"array": ["a"]}, 123])"_json), computo::InvalidArgumentException);
}

// Test trim operator
TEST_F(StringArrayOperationsTest, TrimOperatorBasic) {
    json script = json::array({ "trim", "  hello world  " });
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, TrimOperatorAllWhitespace) {
    json script = json::array({ "trim", "   \t\n\r\f\v   " });
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, TrimOperatorNoWhitespace) {
    json script = json::array({ "trim", "hello" });
    EXPECT_EQ(exec(script), "hello");
}

TEST_F(StringArrayOperationsTest, TrimOperatorEmptyString) {
    json script = json::array({ "trim", "" });
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, TrimOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "trim" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "trim", 123 })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "trim", "hello", "world" })), computo::InvalidArgumentException);
}

// Test upper operator
TEST_F(StringArrayOperationsTest, UpperOperatorBasic) {
    json script = json::array({ "upper", "Hello World" });
    EXPECT_EQ(exec(script), "HELLO WORLD");
}

TEST_F(StringArrayOperationsTest, UpperOperatorAlreadyUpper) {
    json script = json::array({ "upper", "HELLO" });
    EXPECT_EQ(exec(script), "HELLO");
}

TEST_F(StringArrayOperationsTest, UpperOperatorMixedCase) {
    json script = json::array({ "upper", "HeLLo WoRLD" });
    EXPECT_EQ(exec(script), "HELLO WORLD");
}

TEST_F(StringArrayOperationsTest, UpperOperatorEmptyString) {
    json script = json::array({ "upper", "" });
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, UpperOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "upper" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "upper", 123 })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "upper", "hello", "world" })), computo::InvalidArgumentException);
}

// Test lower operator
TEST_F(StringArrayOperationsTest, LowerOperatorBasic) {
    json script = json::array({ "lower", "Hello World" });
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, LowerOperatorAlreadyLower) {
    json script = json::array({ "lower", "hello" });
    EXPECT_EQ(exec(script), "hello");
}

TEST_F(StringArrayOperationsTest, LowerOperatorMixedCase) {
    json script = json::array({ "lower", "HeLLo WoRLD" });
    EXPECT_EQ(exec(script), "hello world");
}

TEST_F(StringArrayOperationsTest, LowerOperatorEmptyString) {
    json script = json::array({ "lower", "" });
    EXPECT_EQ(exec(script), "");
}

TEST_F(StringArrayOperationsTest, LowerOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "lower" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "lower", 123 })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "lower", "hello", "world" })), computo::InvalidArgumentException);
}

// Test sort operator - updated for new API
TEST_F(StringArrayOperationsTest, SortOperatorBasic) {
    json script = R"([
        "sort",
        {"array": ["charlie", "alice", "bob"]}
    ])"_json;
    json expected = R"(["alice", "bob", "charlie"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorNumbers) {
    json script = R"([
        "sort",
        {"array": [3, 1, 2]}
    ])"_json;
    json expected = R"([1, 2, 3])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorDescending) {
    json script = R"([
        "sort",
        {"array": [3, 1, 2]},
        "desc"
    ])"_json;
    json expected = R"([3, 2, 1])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorAscendingExplicit) {
    json script = R"([
        "sort",
        {"array": ["charlie", "alice", "bob"]},
        "asc"
    ])"_json;
    json expected = R"(["alice", "bob", "charlie"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorTypeAware) {
    // Type ordering: null < numbers < strings < booleans < arrays < objects
    json script = R"([
        "sort",
        {"array": ["b", 2, null, true, [1], "a", 1]}
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 7);
    // Check ordering: null first, then numbers, then strings, then booleans, then arrays
    EXPECT_TRUE(result[0].is_null());
    EXPECT_TRUE(result[1].is_number());
    EXPECT_TRUE(result[2].is_number());
    EXPECT_TRUE(result[3].is_string());
    EXPECT_TRUE(result[4].is_string());
    EXPECT_TRUE(result[5].is_boolean());
    EXPECT_TRUE(result[6].is_array());
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectSingleField) {
    json script = R"([
        "sort",
        {"array": [
            {"name": "charlie", "age": 30},
            {"name": "alice", "age": 25},
            {"name": "bob", "age": 35}
        ]},
        "/name"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]["name"], "alice");
    EXPECT_EQ(result[1]["name"], "bob");
    EXPECT_EQ(result[2]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectSingleFieldDesc) {
    json script = R"([
        "sort",
        {"array": [
            {"name": "charlie", "age": 30},
            {"name": "alice", "age": 25},
            {"name": "bob", "age": 35}
        ]},
        ["/name", "desc"]
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]["name"], "charlie");
    EXPECT_EQ(result[1]["name"], "bob");
    EXPECT_EQ(result[2]["name"], "alice");
}

TEST_F(StringArrayOperationsTest, SortOperatorObjectMultiField) {
    json script = R"([
        "sort",
        {"array": [
            {"name": "alice", "age": 30},
            {"name": "bob", "age": 25},
            {"name": "alice", "age": 25}
        ]},
        "/name",
        ["/age", "desc"]
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    // First alice (age 30), then alice (age 25), then bob (age 25)
    EXPECT_EQ(result[0]["name"], "alice");
    EXPECT_EQ(result[0]["age"], 30);
    EXPECT_EQ(result[1]["name"], "alice");
    EXPECT_EQ(result[1]["age"], 25);
    EXPECT_EQ(result[2]["name"], "bob");
    EXPECT_EQ(result[2]["age"], 25);
}

TEST_F(StringArrayOperationsTest, SortOperatorEmptyArray) {
    json script = R"([
        "sort",
        {"array": []}
    ])"_json;
    json expected = R"([])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SortOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "sort" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "sort", "not_an_array" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "sort", json::object({ { "array", json::array({ 1, 2, 3 }) } }), "invalid_direction" })), computo::InvalidArgumentException);
}

// Test reverse operator
TEST_F(StringArrayOperationsTest, ReverseOperatorBasic) {
    json script = R"([
        "reverse",
        {"array": [1, 2, 3, 4]}
    ])"_json;
    json expected = R"([4, 3, 2, 1])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorStrings) {
    json script = R"([
        "reverse",
        {"array": ["a", "b", "c"]}
    ])"_json;
    json expected = R"(["c", "b", "a"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorEmptyArray) {
    json script = R"([
        "reverse",
        {"array": []}
    ])"_json;
    json expected = R"([])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorSingleElement) {
    json script = R"([
        "reverse",
        {"array": ["only"]}
    ])"_json;
    json expected = R"(["only"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, ReverseOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "reverse" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "reverse", "not_an_array" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "reverse", json::object({ { "array", json::array({ "a" }) } }), "extra" })), computo::InvalidArgumentException);
}

// Test unique operator
// Test enhanced unique operator - requires pre-sorted data
TEST_F(StringArrayOperationsTest, UniqueOperatorBasicFirsts) {
    // Pre-sorted array for unique processing: ["a", "a", "b", "c", "c"]
    json script = R"([
        "unique",
        {"array": ["a", "a", "b", "c", "c"]}
    ])"_json;
    json expected = R"(["a", "b", "c"])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorLasts) {
    json script = R"([
        "unique",
        {"array": [1, 1, 2, 3, 3, 3]},
        "lasts"
    ])"_json;
    json expected = R"([1, 2, 3])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorSingles) {
    json script = R"([
        "unique",
        {"array": [1, 1, 2, 3, 3, 3]},
        "singles"
    ])"_json;
    json expected = R"([2])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorMultiples) {
    json script = R"([
        "unique",
        {"array": [1, 1, 2, 3, 3, 3]},
        "multiples"
    ])"_json;
    json expected = R"([1, 3])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorObjectByField) {
    json script = R"([
        "unique",
        {"array": [
            {"name": "alice", "dept": "eng"},
            {"name": "alice", "dept": "sales"},
            {"name": "bob", "dept": "hr"},
            {"name": "charlie", "dept": "eng"}
        ]},
        "/name"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]["name"], "alice");
    EXPECT_EQ(result[1]["name"], "bob");
    EXPECT_EQ(result[2]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, UniqueOperatorObjectFieldSingles) {
    json script = R"([
        "unique",
        {"array": [
            {"dept": "eng"},
            {"dept": "eng"},
            {"dept": "hr"},
            {"dept": "sales"},
            {"dept": "sales"}
        ]},
        "/dept",
        "singles"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["dept"], "hr");
}

TEST_F(StringArrayOperationsTest, UniqueOperatorEmptyArray) {
    json script = R"([
        "unique",
        {"array": []}
    ])"_json;
    json expected = R"([])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorSingleElement) {
    json script = R"([
        "unique",
        {"array": [42]},
        "singles"
    ])"_json;
    json expected = R"([42])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, UniqueOperatorInvalidArguments) {
    EXPECT_THROW(exec(json::array({ "unique" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "unique", "not_an_array" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "unique", json::object({ { "array", json::array({ "a" }) } }), "invalid_mode" })), computo::InvalidArgumentException);
    EXPECT_THROW(exec(json::array({ "unique", json::object({ { "array", json::array({ "a" }) } }), "/field", "invalid_mode" })), computo::InvalidArgumentException);
}

// Test complex scenarios
TEST_F(StringArrayOperationsTest, ComplexStringProcessing) {
    json script = json::array({ "let",
        json::array({ json::array({ "text", "  Hello, World! How are you?  " }) }),
        json::array({ "join",
            json::array({ "reverse",
                json::array({ "unique",
                    json::array({ "sort",
                        json::array({ "map",
                            json::array({ "split",
                                json::array({ "trim",
                                    json::array({ "$", "/text" }) }),
                                " " }),
                            json::array({ "lambda", json::array({ "word" }), json::array({ "lower", json::array({ "$", "/word" }) }) }) }) }) }) }),
            " " }) });

    // Should: trim -> split -> map(lower) -> sort -> unique -> reverse -> join
    // "Hello, World! How are you?" -> ["hello,", "world!", "how", "are", "you?"] -> sort -> unique -> reverse -> join
    auto result = exec(script);
    ASSERT_TRUE(result.is_string());

    // The result should contain all the words in processed form
    std::string result_str = result.get<std::string>();
    EXPECT_TRUE(result_str.find("hello") != std::string::npos);
    EXPECT_TRUE(result_str.find("world") != std::string::npos);
}

TEST_F(StringArrayOperationsTest, ArrayProcessingPipeline) {
    json script = R"([
        "let",
        [["data", {"array": [3, 1, 4, 1, 5, 9, 2, 6, 5, 3]}]],
        ["unique",
            ["sort",
                ["$", "/data"]
            ]
        ]
    ])"_json;

    // Should: sort -> unique (firsts mode)
    // [3,1,4,1,5,9,2,6,5,3] -> [1,1,2,3,3,4,5,5,6,9] -> [1,2,3,4,5,6,9]
    json expected = R"([1, 2, 3, 4, 5, 6, 9])"_json;  // Clean array output
    EXPECT_EQ(exec(script), expected);
}

TEST_F(StringArrayOperationsTest, SplitJoinRoundTrip) {
    json script = json::array({ "let",
        json::array({ json::array({ "text", "apple,banana,cherry" }) }),
        json::array({ "join",
            json::array({ "split",
                json::array({ "$", "/text" }),
                "," }),
            "," }) });

    // Should be identical to the original
    EXPECT_EQ(exec(script), "apple,banana,cherry");
}

// Additional sort tests for enhanced API
TEST_F(StringArrayOperationsTest, SortOperatorNestedField) {
    json script = R"([
        "sort",
        {"array": [
            {"user": {"name": "charlie"}, "score": 90},
            {"user": {"name": "alice"}, "score": 85},
            {"user": {"name": "bob"}, "score": 95}
        ]},
        "/user/name"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]["user"]["name"], "alice");
    EXPECT_EQ(result[1]["user"]["name"], "bob");
    EXPECT_EQ(result[2]["user"]["name"], "charlie");
}

TEST_F(StringArrayOperationsTest, SortOperatorMissingField) {
    json script = R"([
        "sort",
        {"array": [
            {"name": "alice", "age": 30},
            {"name": "bob"},
            {"name": "charlie", "age": 25}
        ]},
        "/age"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 3);
    // Objects with missing fields should sort first (null < numbers)
    EXPECT_EQ(result[0]["name"], "bob");
    EXPECT_EQ(result[1]["name"], "charlie");
    EXPECT_EQ(result[2]["name"], "alice");
}

TEST_F(StringArrayOperationsTest, SortOperatorComplexMultiField) {
    json script = R"([
        "sort",
        {"array": [
            {"dept": "eng", "level": 3, "salary": 90000},
            {"dept": "hr", "level": 2, "salary": 70000},
            {"dept": "eng", "level": 2, "salary": 80000},
            {"dept": "hr", "level": 3, "salary": 85000}
        ]},
        "/dept",
        ["/level", "desc"],
        "/salary"
    ])"_json;
    auto result = exec(script);
    ASSERT_TRUE(result.is_array());  // Clean array output
    EXPECT_EQ(result.size(), 4);
    // Sort by: dept asc, level desc, salary asc
    EXPECT_EQ(result[0]["dept"], "eng");
    EXPECT_EQ(result[0]["level"], 3);
    EXPECT_EQ(result[1]["dept"], "eng");
    EXPECT_EQ(result[1]["level"], 2);
    EXPECT_EQ(result[2]["dept"], "hr");
    EXPECT_EQ(result[2]["level"], 3);
    EXPECT_EQ(result[3]["dept"], "hr");
    EXPECT_EQ(result[3]["level"], 2);
}