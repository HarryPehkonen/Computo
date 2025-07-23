#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class ObjectOpsTest : public ::testing::Test {
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

// --- obj operator tests ---

TEST_F(ObjectOpsTest, ObjOperatorBasic) {
    auto result = execute_script(R"(["obj", "name", "Alice", "age", 30])");
    auto expected = json::parse(R"({"name": "Alice", "age": 30})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjOperatorWithExpressions) {
    auto result = execute_script(
        R"(["obj", ["strConcat", "user_", "name"], "Bob", "score", ["+", 10, 5]])");
    auto expected = json::parse(R"({"user_name": "Bob", "score": 15})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjOperatorEmpty) {
    auto result = execute_script(R"(["obj"])");
    auto expected = json::parse(R"({})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["obj", "key"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["obj", 123, "value"])"), computo::InvalidArgumentException);
}

// --- keys operator tests ---

TEST_F(ObjectOpsTest, KeysOperatorBasic) {
    auto result = execute_script(R"(["keys", {"a": 1, "b": 2, "c": 3}])");
    auto expected = json::parse(R"({"array": ["a", "b", "c"]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, KeysOperatorEmpty) {
    auto result = execute_script(R"(["keys", {}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, KeysOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["keys"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["keys", [1, 2, 3]])"), computo::InvalidArgumentException);
}

// --- values operator tests ---

TEST_F(ObjectOpsTest, ValuesOperatorBasic) {
    auto result = execute_script(R"(["values", {"a": 1, "b": 2, "c": 3}])");
    auto expected = json::parse(R"({"array": [1, 2, 3]})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ValuesOperatorEmpty) {
    auto result = execute_script(R"(["values", {}])");
    auto expected = json::parse(R"({"array": []})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ValuesOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["values"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["values", "not an object"])"),
                 computo::InvalidArgumentException);
}

// --- objFromPairs operator tests ---

TEST_F(ObjectOpsTest, ObjFromPairsOperatorBasic) {
    auto result = execute_script(R"(["objFromPairs", {"array": [["a", 1], ["b", 2]]}])");
    auto expected = json::parse(R"({"a": 1, "b": 2})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjFromPairsOperatorDirectArray) {
    auto result = execute_script(R"(["objFromPairs", {"array": [["x", 10], ["y", 20]]}])");
    auto expected = json::parse(R"({"x": 10, "y": 20})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjFromPairsOperatorEmpty) {
    auto result = execute_script(R"(["objFromPairs", {"array": []}])");
    auto expected = json::parse(R"({})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, ObjFromPairsOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["objFromPairs"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["objFromPairs", "not an array"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["objFromPairs", {"array": [["incomplete"]]}])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["objFromPairs", {"array": [[123, "value"]]}])"),
                 computo::InvalidArgumentException);
}

// --- pick operator tests ---

TEST_F(ObjectOpsTest, PickOperatorBasic) {
    auto result = execute_script(R"(["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}])");
    auto expected = json::parse(R"({"a": 1, "c": 3})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, PickOperatorDirectArray) {
    auto result = execute_script(R"(["pick", {"x": 10, "y": 20, "z": 30}, {"array": ["x", "z"]}])");
    auto expected = json::parse(R"({"x": 10, "z": 30})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, PickOperatorNonExistentKeys) {
    auto result = execute_script(R"(["pick", {"a": 1}, {"array": ["a", "missing"]}])");
    auto expected = json::parse(R"({"a": 1})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, PickOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["pick"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["pick", "not an object", {"array": ["key"]}])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["pick", {"a": 1}, "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- omit operator tests ---

TEST_F(ObjectOpsTest, OmitOperatorBasic) {
    auto result = execute_script(R"(["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}])");
    auto expected = json::parse(R"({"a": 1, "c": 3})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, OmitOperatorDirectArray) {
    auto result = execute_script(R"(["omit", {"x": 10, "y": 20, "z": 30}, {"array": ["y", "z"]}])");
    auto expected = json::parse(R"({"x": 10})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, OmitOperatorNonExistentKeys) {
    auto result = execute_script(R"(["omit", {"a": 1, "b": 2}, {"array": ["missing"]}])");
    auto expected = json::parse(R"({"a": 1, "b": 2})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, OmitOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["omit"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["omit", "not an object", {"array": ["key"]}])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["omit", {"a": 1}, "not an array"])"),
                 computo::InvalidArgumentException);
}

// --- merge operator tests ---

TEST_F(ObjectOpsTest, MergeOperatorBasic) {
    auto result = execute_script(R"(["merge", {"a": 1, "b": 2}, {"c": 3, "d": 4}])");
    auto expected = json::parse(R"({"a": 1, "b": 2, "c": 3, "d": 4})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, MergeOperatorOverwrite) {
    auto result = execute_script(R"(["merge", {"a": 1, "b": 2}, {"b": 20, "c": 3}])");
    auto expected = json::parse(R"({"a": 1, "b": 20, "c": 3})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, MergeOperatorMultiple) {
    auto result = execute_script(R"(["merge", {"a": 1}, {"b": 2}, {"c": 3}])");
    auto expected = json::parse(R"({"a": 1, "b": 2, "c": 3})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, MergeOperatorSingle) {
    auto result = execute_script(R"(["merge", {"a": 1, "b": 2}])");
    auto expected = json::parse(R"({"a": 1, "b": 2})");
    EXPECT_EQ(result, expected);
}

TEST_F(ObjectOpsTest, MergeOperatorErrors) {
    EXPECT_THROW(execute_script(R"(["merge"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["merge", "not an object"])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["merge", {"a": 1}, [1, 2, 3]])"),
                 computo::InvalidArgumentException);
}
