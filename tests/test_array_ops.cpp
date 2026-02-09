#include <computo.hpp>
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

class ArrayOpsTest : public ::testing::Test {
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

// --- map operator tests ---

TEST_F(ArrayOpsTest, MapOperatorBasic) {
    auto result = execute_script(
        R"(["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])");
    auto expected = jsom::parse_document(R"({"array": [2, 4, 6]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, MapOperatorDirectArray) {
    auto result = execute_script(
        R"(["map", {"array": [10, 20, 30]}, ["lambda", ["x"], ["+", ["$", "/x"], 5]]])");
    auto expected = jsom::parse_document(R"({"array": [15, 25, 35]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, MapOperatorEmpty) {
    auto result = execute_script(R"(["map", {"array": []}, ["lambda", ["x"], ["$", "/x"]]])");
    auto expected = jsom::parse_document(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, MapOperatorStringTransform) {
    auto result = execute_script(
        R"(["map", {"array": ["hello", "world"]}, ["lambda", ["s"], ["strConcat", "prefix_", ["$", "/s"]]]])");
    auto expected = jsom::parse_document(R"({"array": ["prefix_hello", "prefix_world"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, MapOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["map"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["map", "not an array", ["lambda", ["x"], ["$", "/x"]]])"),
                 computo::InvalidArgumentException);
}

// --- filter operator tests ---

TEST_F(ArrayOpsTest, FilterOperatorBasic) {
    auto result = execute_script(
        R"(["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]])");
    auto expected = jsom::parse_document(R"({"array": [4, 5]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, FilterOperatorDirectArray) {
    auto result = execute_script(
        R"(["filter", {"array": [10, 15, 20, 25]}, ["lambda", ["x"], ["==", ["%", ["$", "/x"], 5], 0]]])");
    auto expected = jsom::parse_document(R"({"array": [10, 15, 20, 25]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, FilterOperatorEmpty) {
    auto result = execute_script(R"(["filter", {"array": []}, ["lambda", ["x"], true]])");
    auto expected = jsom::parse_document(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, FilterOperatorNoneMatch) {
    auto result = execute_script(
        R"(["filter", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 10]]])");
    auto expected = jsom::parse_document(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(ArrayOpsTest, FilterOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["filter"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["filter", "not an array", ["lambda", ["x"], true]])"),
                 computo::InvalidArgumentException);
}

// --- reduce operator tests ---

TEST_F(ArrayOpsTest, ReduceOperatorBasic) {
    auto result = execute_script(
        R"(["reduce", {"array": [1, 2, 3, 4]}, ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0])");
    EXPECT_EQ(result, json(10));
}

TEST_F(ArrayOpsTest, ReduceOperatorDirectArray) {
    auto result = execute_script(
        R"(["reduce", {"array": [2, 3, 4]}, ["lambda", ["acc", "x"], ["*", ["$", "/acc"], ["$", "/x"]]], 1])");
    EXPECT_EQ(result, json(24));
}

TEST_F(ArrayOpsTest, ReduceOperatorStringConcat) {
    auto result = execute_script(
        R"(["reduce", {"array": ["a", "b", "c"]}, ["lambda", ["acc", "x"], ["strConcat", ["$", "/acc"], ["$", "/x"]]], ""])");
    EXPECT_EQ(result, json("abc"));
}

TEST_F(ArrayOpsTest, ReduceOperatorEmpty) {
    auto result = execute_script(
        R"(["reduce", {"array": []}, ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 42])");
    EXPECT_EQ(result, json(42));
}

TEST_F(ArrayOpsTest, ReduceOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["reduce"])"), computo::InvalidArgumentException);
    EXPECT_THROW(
        execute_script(R"(["reduce", "not an array", ["lambda", ["acc", "x"], ["$", "/acc"]], 0])"),
        computo::InvalidArgumentException);
}

// --- count operator tests ---

TEST_F(ArrayOpsTest, CountOperatorBasic) {
    auto result = execute_script(R"(["count", {"array": [1, 2, 3, 4, 5]}])");
    EXPECT_EQ(result, json(5));
}

TEST_F(ArrayOpsTest, CountOperatorDirectArray) {
    auto result = execute_script(R"(["count", {"array": ["a", "b", "c"]}])");
    EXPECT_EQ(result, json(3));
}

TEST_F(ArrayOpsTest, CountOperatorEmpty) {
    auto result = execute_script(R"(["count", {"array": []}])");
    EXPECT_EQ(result, json(0));
}

TEST_F(ArrayOpsTest, CountOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["count"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["count", "not an array"])"), computo::InvalidArgumentException);
}

// --- find operator tests ---

TEST_F(ArrayOpsTest, FindOperatorBasic) {
    auto result = execute_script(
        R"(["find", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]])");
    EXPECT_EQ(result, json(4));
}

TEST_F(ArrayOpsTest, FindOperatorDirectArray) {
    auto result = execute_script(
        R"(["find", {"array": ["apple", "banana", "cherry"]}, ["lambda", ["s"], ["==", ["$", "/s"], "banana"]]])");
    EXPECT_EQ(result, json("banana"));
}

TEST_F(ArrayOpsTest, FindOperatorNotFound) {
    auto result = execute_script(
        R"(["find", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 10]]])");
    EXPECT_EQ(result, json(nullptr));
}

TEST_F(ArrayOpsTest, FindOperatorEmpty) {
    auto result = execute_script(R"(["find", {"array": []}, ["lambda", ["x"], true]])");
    EXPECT_EQ(result, json(nullptr));
}

TEST_F(ArrayOpsTest, FindOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["find"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["find", "not an array", ["lambda", ["x"], true]])"),
                 computo::InvalidArgumentException);
}

// --- some operator tests ---

TEST_F(ArrayOpsTest, SomeOperatorBasic) {
    auto result = execute_script(
        R"(["some", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]])");
    EXPECT_EQ(result, json(true));
}

TEST_F(ArrayOpsTest, SomeOperatorDirectArray) {
    auto result = execute_script(
        R"(["some", {"array": [1, 2, 3]}, ["lambda", ["x"], ["==", ["$", "/x"], 2]]])");
    EXPECT_EQ(result, json(true));
}

TEST_F(ArrayOpsTest, SomeOperatorFalse) {
    auto result = execute_script(
        R"(["some", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 10]]])");
    EXPECT_EQ(result, json(false));
}

TEST_F(ArrayOpsTest, SomeOperatorEmpty) {
    auto result = execute_script(R"(["some", {"array": []}, ["lambda", ["x"], true]])");
    EXPECT_EQ(result, json(false));
}

TEST_F(ArrayOpsTest, SomeOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["some"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["some", "not an array", ["lambda", ["x"], true]])"),
                 computo::InvalidArgumentException);
}

// --- every operator tests ---

TEST_F(ArrayOpsTest, EveryOperatorBasic) {
    auto result = execute_script(
        R"(["every", {"array": [2, 4, 6, 8]}, ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]])");
    EXPECT_EQ(result, json(true));
}

TEST_F(ArrayOpsTest, EveryOperatorDirectArray) {
    auto result = execute_script(
        R"(["every", {"array": [10, 20, 30]}, ["lambda", ["x"], [">", ["$", "/x"], 5]]])");
    EXPECT_EQ(result, json(true));
}

TEST_F(ArrayOpsTest, EveryOperatorFalse) {
    auto result = execute_script(
        R"(["every", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]])");
    EXPECT_EQ(result, json(false));
}

TEST_F(ArrayOpsTest, EveryOperatorEmpty) {
    auto result = execute_script(R"(["every", {"array": []}, ["lambda", ["x"], false]])");
    EXPECT_EQ(result, json(true));
}

TEST_F(ArrayOpsTest, EveryOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["every"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["every", "not an array", ["lambda", ["x"], true]])"),
                 computo::InvalidArgumentException);
}
