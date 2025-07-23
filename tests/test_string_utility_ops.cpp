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

// --- split operator tests ---

TEST_F(StringUtilityOpsTest, SplitOperatorBasic) {
    auto result = execute_script(R"(["split", "hello world", " "])");
    auto expected = json::parse(R"({"array": ["hello", "world"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SplitOperatorComma) {
    auto result = execute_script(R"(["split", "a,b,c,d", ","])");
    auto expected = json::parse(R"({"array": ["a", "b", "c", "d"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SplitOperatorEmptyDelimiter) {
    auto result = execute_script(R"(["split", "abc", ""])");
    auto expected = json::parse(R"({"array": ["a", "b", "c"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SplitOperatorNoMatch) {
    auto result = execute_script(R"(["split", "hello", "xyz"])");
    auto expected = json::parse(R"({"array": ["hello"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SplitOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["split"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["split", 123, " "])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["split", "hello", 456])"), computo::InvalidArgumentException);
}

// --- join operator tests ---

TEST_F(StringUtilityOpsTest, JoinOperatorBasic) {
    auto result = execute_script(R"(["join", {"array": ["hello", "world"]}, " "])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorDirectArray) {
    auto result = execute_script(R"(["join", {"array": ["a", "b", "c"]}, ","])");
    EXPECT_EQ(result, json("a,b,c"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorMixed) {
    auto result = execute_script(R"(["join", {"array": ["number", 42, true, null]}, "-"])");
    EXPECT_EQ(result, json("number-42-true-null"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorEmpty) {
    auto result = execute_script(R"(["join", {"array": []}, ","])");
    EXPECT_EQ(result, json(""));
}

TEST_F(StringUtilityOpsTest, JoinOperatorSingle) {
    auto result = execute_script(R"(["join", {"array": ["only"]}, ","])");
    EXPECT_EQ(result, json("only"));
}

TEST_F(StringUtilityOpsTest, JoinOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["join"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["join", "not an array", ","])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["join", {"array": ["a"]}, 123])"),
                 computo::InvalidArgumentException);
}

// --- trim operator tests ---

TEST_F(StringUtilityOpsTest, TrimOperatorBasic) {
    auto result = execute_script(R"(["trim", "  hello world  "])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, TrimOperatorLeadingOnly) {
    auto result = execute_script(R"(["trim", "   no trailing"])");
    EXPECT_EQ(result, json("no trailing"));
}

TEST_F(StringUtilityOpsTest, TrimOperatorTrailingOnly) {
    auto result = execute_script(R"(["trim", "no leading   "])");
    EXPECT_EQ(result, json("no leading"));
}

TEST_F(StringUtilityOpsTest, TrimOperatorNoWhitespace) {
    auto result = execute_script(R"(["trim", "nospace"])");
    EXPECT_EQ(result, json("nospace"));
}

TEST_F(StringUtilityOpsTest, TrimOperatorOnlyWhitespace) {
    auto result = execute_script(R"(["trim", "   "])");
    EXPECT_EQ(result, json(""));
}

TEST_F(StringUtilityOpsTest, TrimOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["trim"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["trim", 123])"), computo::InvalidArgumentException);
}

// --- upper operator tests ---

TEST_F(StringUtilityOpsTest, UpperOperatorBasic) {
    auto result = execute_script(R"(["upper", "hello world"])");
    EXPECT_EQ(result, json("HELLO WORLD"));
}

TEST_F(StringUtilityOpsTest, UpperOperatorMixed) {
    auto result = execute_script(R"(["upper", "Hello123World"])");
    EXPECT_EQ(result, json("HELLO123WORLD"));
}

TEST_F(StringUtilityOpsTest, UpperOperatorAlreadyUpper) {
    auto result = execute_script(R"(["upper", "ALREADY UPPER"])");
    EXPECT_EQ(result, json("ALREADY UPPER"));
}

TEST_F(StringUtilityOpsTest, UpperOperatorEmpty) {
    auto result = execute_script(R"(["upper", ""])");
    EXPECT_EQ(result, json(""));
}

TEST_F(StringUtilityOpsTest, UpperOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["upper"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["upper", 123])"), computo::InvalidArgumentException);
}

// --- lower operator tests ---

TEST_F(StringUtilityOpsTest, LowerOperatorBasic) {
    auto result = execute_script(R"(["lower", "HELLO WORLD"])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, LowerOperatorMixed) {
    auto result = execute_script(R"(["lower", "Hello123World"])");
    EXPECT_EQ(result, json("hello123world"));
}

TEST_F(StringUtilityOpsTest, LowerOperatorAlreadyLower) {
    auto result = execute_script(R"(["lower", "already lower"])");
    EXPECT_EQ(result, json("already lower"));
}

TEST_F(StringUtilityOpsTest, LowerOperatorEmpty) {
    auto result = execute_script(R"(["lower", ""])");
    EXPECT_EQ(result, json(""));
}

TEST_F(StringUtilityOpsTest, LowerOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["lower"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["lower", 123])"), computo::InvalidArgumentException);
}

// --- strConcat operator tests ---

TEST_F(StringUtilityOpsTest, StrConcatOperatorBasic) {
    auto result = execute_script(R"(["strConcat", "hello", " ", "world"])");
    EXPECT_EQ(result, json("hello world"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorMixed) {
    auto result = execute_script(R"(["strConcat", "count: ", 42, ", active: ", true])");
    EXPECT_EQ(result, json("count: 42, active: true"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorSingle) {
    auto result = execute_script(R"(["strConcat", "only"])");
    EXPECT_EQ(result, json("only"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorWithNull) {
    auto result = execute_script(R"(["strConcat", "value: ", null, " end"])");
    EXPECT_EQ(result, json("value: null end"));
}

TEST_F(StringUtilityOpsTest, StrConcatOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["strConcat"])"), computo::InvalidArgumentException);
}

// --- sort operator tests ---

TEST_F(StringUtilityOpsTest, SortOperatorBasic) {
    auto result = execute_script(R"(["sort", {"array": [3, 1, 4, 1, 5]}])");
    auto expected = json::parse(R"({"array": [1, 1, 3, 4, 5]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorDirectArray) {
    auto result = execute_script(R"(["sort", {"array": ["banana", "apple", "cherry"]}])");
    auto expected = json::parse(R"({"array": ["apple", "banana", "cherry"]})");
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

// --- Advanced sort operator tests ---

TEST_F(StringUtilityOpsTest, SortOperatorDescending) {
    auto result = execute_script(R"(["sort", {"array": [3, 1, 4, 1, 5]}, "desc"])");
    auto expected = json::parse(R"({"array": [5, 4, 3, 1, 1]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorExplicitAscending) {
    auto result = execute_script(R"(["sort", {"array": [3, 1, 4, 1, 5]}, "asc"])");
    auto expected = json::parse(R"({"array": [1, 1, 3, 4, 5]})");
    EXPECT_EQ(result, expected);
}

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

TEST_F(StringUtilityOpsTest, SortOperatorThreeFields) {
    auto result = execute_script(R"(["sort", {"array": [
        {"a": 1, "b": 2, "c": 3},
        {"a": 1, "b": 1, "c": 2},
        {"a": 1, "b": 2, "c": 1},
        {"a": 1, "b": 1, "c": 1}
    ]}, "/a", "/b", "/c"])");
    auto expected = json::parse(R"({"array": [
        {"a": 1, "b": 1, "c": 1},
        {"a": 1, "b": 1, "c": 2},
        {"a": 1, "b": 2, "c": 1},
        {"a": 1, "b": 2, "c": 3}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorComplexMultiField) {
    auto result = execute_script(R"(["sort", {"array": [
        {"priority": "high", "created": "2023-01-15", "assignee": "bob"},
        {"priority": "low", "created": "2023-01-10", "assignee": "alice"},
        {"priority": "high", "created": "2023-01-12", "assignee": "alice"},
        {"priority": "medium", "created": "2023-01-14", "assignee": "charlie"}
    ]}, "/priority", ["/created", "desc"], "/assignee"])");
    auto expected = json::parse(R"({"array": [
        {"priority": "high", "created": "2023-01-15", "assignee": "bob"},
        {"priority": "high", "created": "2023-01-12", "assignee": "alice"},
        {"priority": "low", "created": "2023-01-10", "assignee": "alice"},
        {"priority": "medium", "created": "2023-01-14", "assignee": "charlie"}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, SortOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["sort"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", "not an array"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", {"array": [1, 2, 3]}, "invalid_direction"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", {"array": [{}]}, []])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["sort", {"array": [{}]}, ["/field", "invalid_dir"]])"),
                 computo::InvalidArgumentException);
}

// --- reverse operator tests ---

TEST_F(StringUtilityOpsTest, ReverseOperatorBasic) {
    auto result = execute_script(R"(["reverse", {"array": [1, 2, 3, 4, 5]}])");
    auto expected = json::parse(R"({"array": [5, 4, 3, 2, 1]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorDirectArray) {
    auto result = execute_script(R"(["reverse", {"array": ["a", "b", "c"]}])");
    auto expected = json::parse(R"({"array": ["c", "b", "a"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorSingle) {
    auto result = execute_script(R"(["reverse", {"array": [42]}])");
    auto expected = json::parse(R"({"array": [42]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorEmpty) {
    auto result = execute_script(R"(["reverse", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ReverseOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["reverse"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["reverse", "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- unique operator tests ---

TEST_F(StringUtilityOpsTest, UniqueOperatorBasic) {
    auto result = execute_script(R"(["unique", {"array": [1, 1, 2, 3, 3, 3, 4]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorDirectArray) {
    auto result = execute_script(R"(["unique", {"array": ["a", "b", "a", "c", "b"]}])");
    auto expected = json::parse(R"({"array": ["a", "b", "c"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorAlreadyUnique) {
    auto result = execute_script(R"(["unique", {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorEmpty) {
    auto result = execute_script(R"(["unique", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["unique"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["unique", "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- uniqueSorted operator tests ---

TEST_F(StringUtilityOpsTest, UniqueSortedBasicFirsts) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 2, 2, 3, 4, 4]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedModeFirsts) {
    auto result
        = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 2, 2, 3, 4, 4]}, "firsts"])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedModeLasts) {
    auto result
        = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 2, 2, 3, 4, 4]}, "lasts"])");
    auto expected = json::parse(R"({"array": [1, 2, 3, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedModeSingles) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 3, 3, 4]}, "singles"])");
    auto expected = json::parse(R"({"array": [2, 4]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedModeMultiples) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 3, 3, 4]}, "multiples"])");
    auto expected = json::parse(R"({"array": [1, 1, 3, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedFieldBasedBasic) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "alice", "dept": "sales"},
        {"name": "bob", "dept": "hr"},
        {"name": "charlie", "dept": "eng"}
    ]}, "/name"])");
    auto expected = json::parse(R"({"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "bob", "dept": "hr"},
        {"name": "charlie", "dept": "eng"}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedFieldBasedLasts) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "alice", "dept": "sales"},
        {"name": "bob", "dept": "hr"}
    ]}, "/name", "lasts"])");
    auto expected = json::parse(R"({"array": [
        {"name": "alice", "dept": "sales"},
        {"name": "bob", "dept": "hr"}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedFieldBasedMultiples) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "alice", "dept": "sales"},
        {"name": "bob", "dept": "hr"},
        {"name": "charlie", "dept": "eng"}
    ]}, "/name", "multiples"])");
    auto expected = json::parse(R"({"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "alice", "dept": "sales"}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedFieldBasedSingles) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [
        {"name": "alice", "dept": "eng"},
        {"name": "alice", "dept": "sales"},
        {"name": "bob", "dept": "hr"},
        {"name": "charlie", "dept": "eng"}
    ]}, "/name", "singles"])");
    auto expected = json::parse(R"({"array": [
        {"name": "bob", "dept": "hr"},
        {"name": "charlie", "dept": "eng"}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedEmpty) {
    auto result = execute_script(R"(["uniqueSorted", {"array": []}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedSingleElement) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [42]}])");
    auto expected = json::parse(R"({"array": [42]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedSingleElementSingles) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [42]}, "singles"])");
    auto expected = json::parse(R"({"array": [42]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedSingleElementMultiples) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [42]}, "multiples"])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedAllSame) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 1, 1]}, "multiples"])");
    auto expected = json::parse(R"({"array": [1, 1, 1, 1]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedAllSameSingles) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 1, 1]}, "singles"])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedMissingField) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [
        {"name": "alice"},
        {"name": "alice"},
        {"id": 123}
    ]}, "/name"])");
    auto expected = json::parse(R"({"array": [
        {"name": "alice"},
        {"id": 123}
    ]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedDirectArray) {
    auto result = execute_script(R"(["uniqueSorted", {"array": [1, 1, 2, 3, 3]}])");
    auto expected = json::parse(R"({"array": [1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, UniqueSortedErrors) {
    EXPECT_THROW(execute_script(R"(["uniqueSorted"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["uniqueSorted", "not an array"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["uniqueSorted", {"array": [1, 2, 3]}, "invalid_mode"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(
        execute_script(R"(["uniqueSorted", {"array": [1, 2, 3]}, "/field", "invalid_mode"])"),
        computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(
                     R"(["uniqueSorted", {"array": [1, 2, 3]}, "/field", "singles", "extra_arg"])"),
                 computo::InvalidArgumentException);
}

// --- zip operator tests ---

TEST_F(StringUtilityOpsTest, ZipOperatorBasic) {
    auto result = execute_script(R"(["zip", {"array": ["a", "b", "c"]}, {"array": [1, 2, 3]}])");
    auto expected = json::parse(R"({"array": [["a", 1], ["b", 2], ["c", 3]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorDirectArrays) {
    auto result = execute_script(R"(["zip", {"array": ["x", "y"]}, {"array": [10, 20]}])");
    auto expected = json::parse(R"({"array": [["x", 10], ["y", 20]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorDifferentSizes) {
    auto result = execute_script(R"(["zip", {"array": ["a", "b", "c"]}, {"array": [1, 2]}])");
    auto expected = json::parse(R"({"array": [["a", 1], ["b", 2]]})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorEmpty) {
    auto result = execute_script(R"(["zip", {"array": []}, {"array": [1, 2]}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(StringUtilityOpsTest, ZipOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["zip"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["zip", "not an array", {"array": [1]}])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["zip", {"array": [1]}, "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- approx operator tests ---

TEST_F(StringUtilityOpsTest, ApproxOperatorBasic) {
    auto result = execute_script(R"(["approx", 1.0, 1.1, 0.2])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorFalse) {
    auto result = execute_script(R"(["approx", 1.0, 1.5, 0.2])");
    EXPECT_EQ(result, json(false));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorExact) {
    auto result = execute_script(R"(["approx", 5.0, 5.0, 0.0])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorZeroTolerance) {
    auto result = execute_script(R"(["approx", 1.0, 1.001, 0.0])");
    EXPECT_EQ(result, json(false));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorNegative) {
    auto result = execute_script(R"(["approx", -1.0, -1.05, 0.1])");
    EXPECT_EQ(result, json(true));
}

TEST_F(StringUtilityOpsTest, ApproxOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["approx"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["approx", "not a number", 1.0, 0.1])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["approx", 1.0, 1.0, -0.1])"),
                 computo::InvalidArgumentException);
}

// --- Integration tests ---

TEST_F(StringUtilityOpsTest, SplitJoinRoundtrip) {
    auto result = execute_script(R"(["join", ["split", "hello,world,test", ","], ","])");
    EXPECT_EQ(result, json("hello,world,test"));
}

TEST_F(StringUtilityOpsTest, TrimAndCase) {
    auto result = execute_script(R"(["upper", ["trim", "  hello world  "]])");
    EXPECT_EQ(result, json("HELLO WORLD"));
}

TEST_F(StringUtilityOpsTest, SortReverse) {
    auto result = execute_script(R"(["reverse", ["sort", {"array": [3, 1, 4, 1, 5]}]])");
    auto expected = json::parse(R"({"array": [5, 4, 3, 1, 1]})");
    EXPECT_EQ(result, expected);
}
