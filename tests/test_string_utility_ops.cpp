#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class StringUtilityOpsTest : public ::testing::Test {
protected:
    void SetUp() override { input_data = json{{"test", "value"}}; }

    auto execute_script(const std::string& script_json) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input_data});
    }

    static auto execute_script(const std::string& script_json, const json& input) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input});
    }

    json input_data;
};

// --- join operator tests ---

TEST_F(StringUtilityOpsTest, JoinOperatorBasic) {
    auto result = execute_script(R"(["join", {"array": ["hello", "world"]}, " "])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorComma) {
    auto result = execute_script(R"(["join", {"array": ["a", "b", "c", "d"]}, ","])");
    EXPECT_EQ(result, json("a,b,c,d"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorEmpty) {
    auto result = execute_script(R"(["join", {"array": ["a", "b", "c"]}, ""])");
    EXPECT_EQ(result, json("abc"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorSingleElement) {
    auto result = execute_script(R"(["join", {"array": ["hello"]}, ","])");
    EXPECT_EQ(result, json("hello"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorEmptyArray) {
    auto result = execute_script(R"(["join", {"array": []}, ","])");
    EXPECT_EQ(result, json(""));
}

TEST_F(StringUtilityOpsTest, JoinOperatorMixedTypes) {
    auto result = execute_script(R"(["join", {"array": ["hello", 42, true, null]}, " | "])");
    EXPECT_EQ(result, json("hello | 42 | true | null"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["join"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["join", {"array": ["a", "b"]}])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["join", {"array": ["a", "b"]}, 123])"), computo::InvalidArgumentException);
}

// --- strConcat operator tests ---

TEST_F(StringUtilityOpsTest, StrConcatOperatorBasic) {
    auto result = execute_script(R"(["strConcat", "hello", " ", "world"])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorSingle) {
    auto result = execute_script(R"(["strConcat", "hello"])");
    EXPECT_EQ(result, json("hello"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorMixedTypes) {
    auto result = execute_script(R"(["strConcat", "Count: ", 42, " items"])");
    EXPECT_EQ(result, json("Count: 42 items"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorBooleans) {
    auto result = execute_script(R"(["strConcat", "Status: ", true, " and ", false])");
    EXPECT_EQ(result, json("Status: true and false"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorNull) {
    auto result = execute_script(R"(["strConcat", "Value: ", null])");
    EXPECT_EQ(result, json("Value: null"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorEmpty) {
    auto result = execute_script(R"(["strConcat", "", "hello", ""])");
    EXPECT_EQ(result, json("hello"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["strConcat"])"), computo::InvalidArgumentException);
}

// --- sort operator tests ---

TEST_F(StringUtilityOpsTest, SortOperatorBasicStrings) {
    auto result = execute_script(R"(["sort", {"array": ["zebra", "apple", "banana"]}])");
    auto expected = json::parse(R"({"array": ["apple", "banana", "zebra"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorDescending) {
    auto result = execute_script(R"(["sort", {"array": ["apple", "banana", "zebra"]}, "desc"])");
    auto expected = json::parse(R"({"array": ["zebra", "banana", "apple"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorNumbers) {
    auto result = execute_script(R"(["sort", {"array": [3, 1, 4, 1, 5]}])");
    auto expected = json::parse(R"({"array": [1, 1, 3, 4, 5]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorMixed) {
    auto result = execute_script(R"(["sort", {"array": [true, false, 1, 0, "z", "a", null]}])");
    auto expected = json::parse(R"({"array": [null, 0, 1, "a", "z", false, true]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorEmpty) {
    auto result = execute_script(R"(["sort", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorExplicitAscending) {
    auto result = execute_script(R"(["sort", {"array": [3, 1, 4, 1, 5]}, "asc"])");
    auto expected = json::parse(R"({"array": [1, 1, 3, 4, 5]})");
    EXPECT_EQ(result, expected);
}

// --- Object sorting tests ---

TEST_F(StringUtilityOpsTest, SortOperatorObjectSingleField) {
    auto result = execute_script(R"(["sort", {"array": [
        {"name": "charlie", "age": 30},
        {"name": "alice", "age": 25},
        {"name": "bob", "age": 35}
    ]}, "/name"])");
    auto expected = json::parse(R"({"array": [
        {"name": "alice", "age": 25},
        {"name": "bob", "age": 35},
        {"name": "charlie", "age": 30}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorObjectFieldDescending) {
    auto result = execute_script(R"(["sort", {"array": [
        {"name": "alice", "age": 25},
        {"name": "bob", "age": 35},
        {"name": "charlie", "age": 30}
    ]}, ["/age", "desc"]])");
    auto expected = json::parse(R"({"array": [
        {"name": "bob", "age": 35},
        {"name": "charlie", "age": 30},
        {"name": "alice", "age": 25}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorMultiField) {
    auto result = execute_script(R"(["sort", {"array": [
        {"dept": "engineering", "salary": 90000},
        {"dept": "marketing", "salary": 75000},
        {"dept": "engineering", "salary": 85000},
        {"dept": "marketing", "salary": 80000}
    ]}, "/dept", ["/salary", "desc"]])");
    auto expected = json::parse(R"({"array": [
        {"dept": "engineering", "salary": 90000},
        {"dept": "engineering", "salary": 85000},
        {"dept": "marketing", "salary": 80000},
        {"dept": "marketing", "salary": 75000}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorNestedField) {
    auto result = execute_script(R"(["sort", {"array": [
        {"user": {"profile": {"score": 85}}},
        {"user": {"profile": {"score": 92}}},
        {"user": {"profile": {"score": 78}}}
    ]}, "/user/profile/score"])");
    auto expected = json::parse(R"({"array": [
        {"user": {"profile": {"score": 78}}},
        {"user": {"profile": {"score": 85}}},
        {"user": {"profile": {"score": 92}}}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorMissingField) {
    auto result = execute_script(R"(["sort", {"array": [
        {"name": "alice", "age": 25},
        {"name": "bob"},
        {"name": "charlie", "age": 30}
    ]}, "/age"])");
    auto expected = json::parse(R"({"array": [
        {"name": "bob"},
        {"name": "alice", "age": 25},
        {"name": "charlie", "age": 30}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorFieldWithMixedTypes) {
    auto result = execute_script(R"(["sort", {"array": [
        {"value": "hello"},
        {"value": 42},
        {"value": null},
        {"value": true}
    ]}, "/value"])");
    auto expected = json::parse(R"({"array": [
        {"value": null},
        {"value": 42},
        {"value": "hello"},
        {"value": true}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["sort"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", "not an array"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", {"array": [1, 2, 3]}, "invalid_direction"])"),
                 computo::InvalidArgumentException);
}

// --- reverse operator tests ---

TEST_F(StringUtilityOpsTest, ReverseOperatorBasic) {
    auto result = execute_script(R"(["reverse", {"array": ["a", "b", "c"]}])");
    auto expected = json::parse(R"({"array": ["c", "b", "a"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorNumbers) {
    auto result = execute_script(R"(["reverse", {"array": [1, 2, 3, 4, 5]}])");
    auto expected = json::parse(R"({"array": [5, 4, 3, 2, 1]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorMixedTypes) {
    auto result = execute_script(R"(["reverse", {"array": [true, 42, "hello", null]}])");
    auto expected = json::parse(R"({"array": [null, "hello", 42, true]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorSingleElement) {
    auto result = execute_script(R"(["reverse", {"array": ["only"]}])");
    auto expected = json::parse(R"({"array": ["only"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorEmpty) {
    auto result = execute_script(R"(["reverse", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["reverse"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["reverse", "not an array"])"), computo::InvalidArgumentException);
}

// --- unique operator tests ---

TEST_F(StringUtilityOpsTest, UniqueOperatorBasic) {
    auto result = execute_script(R"(["unique", {"array": ["a", "b", "a", "c", "b"]}])");
    auto expected = json::parse(R"({"array": ["a", "b", "c"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorNumbers) {
    auto result = execute_script(R"(["unique", {"array": [3, 1, 4, 1, 5, 3]}])");
    auto expected = json::parse(R"({"array": [3, 1, 4, 5]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorMixedTypes) {
    auto result = execute_script(R"(["unique", {"array": [1, "1", true, 1, false, "1"]}])");
    auto expected = json::parse(R"({"array": [1, "1", true, false]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorEmpty) {
    auto result = execute_script(R"(["unique", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorAllSame) {
    auto result = execute_script(R"(["unique", {"array": ["x", "x", "x"]}])");
    auto expected = json::parse(R"({"array": ["x"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["unique"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["unique", "not an array"])"), computo::InvalidArgumentException);
}


// --- zip operator tests ---

TEST_F(StringUtilityOpsTest, ZipOperatorBasic) {
    auto result = execute_script(R"(["zip", {"array": ["a", "b", "c"]}, {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": [["a", 1], ["b", 2], ["c", 3]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorUnequalLengths) {
    auto result = execute_script(R"(["zip", {"array": ["a", "b"]}, {"array": [1, 2, 3, 4]}])");
    auto expected = json::parse(R"({"array": [["a", 1], ["b", 2]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorComplexElements) {
    auto result = execute_script(R"(["zip", {"array": [{"name": "alice"}, {"name": "bob"}]}, {"array": [25, 30]}])");
    auto expected = json::parse(R"({"array": [[{"name": "alice"}, 25], [{"name": "bob"}, 30]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorEmptyArrays) {
    auto result = execute_script(R"(["zip", {"array": []}, {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorOneEmpty) {
    auto result = execute_script(R"(["zip", {"array": []}, {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["zip"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["zip", {"array": [1, 2]}])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["zip", "not an array", {"array": [1, 2]}])"), computo::InvalidArgumentException);
}

// --- approx operator tests ---

TEST_F(StringUtilityOpsTest, ApproxOperatorBasic) {
    auto result = execute_script(R"(["approx", 1.0, 1.1, 0.2])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorFalse) {
    auto result = execute_script(R"(["approx", 1.0, 2.0, 0.5])");
    EXPECT_EQ(result, json(false));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorExact) {
    auto result = execute_script(R"(["approx", 5.0, 5.0, 0.001])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorNegative) {
    auto result = execute_script(R"(["approx", -3.0, -2.9, 0.2])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorZeroTolerance) {
    auto result = execute_script(R"(["approx", 1.0, 1.0, 0.0])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorZeroDifference) {
    auto result = execute_script(R"(["approx", 0.0, 0.0, 0.1])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorBoundaryCase) {
    auto result = execute_script(R"(["approx", 1.0, 1.5, 0.5])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorBoundaryFalse) {
    auto result = execute_script(R"(["approx", 1.0, 1.51, 0.5])");
    EXPECT_EQ(result, json(false));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["approx"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["approx", 1.0])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["approx", 1.0, 2.0])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["approx", "not a number", 1.0, 0.1])"), computo::InvalidArgumentException);
}