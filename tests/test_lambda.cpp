#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class LambdaTest : public ::testing::Test {
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

// --- Basic Lambda Operator Tests ---

TEST_F(LambdaTest, LambdaOperatorBasic) {
    auto result = execute_script(R"(["lambda", ["x"], ["+", ["$", "/x"], 1]])");
    // Lambda should return an array with [params, body]
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], json::array({"x"}));
    EXPECT_EQ(result[1], json::parse(R"(["+", ["$", "/x"], 1])"));
}

TEST_F(LambdaTest, LambdaOperatorMultipleParams) {
    auto result = execute_script(R"(["lambda", ["a", "b"], ["*", ["$", "/a"], ["$", "/b"]]])");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], json::array({"a", "b"}));
    EXPECT_EQ(result[1], json::parse(R"(["*", ["$", "/a"], ["$", "/b"]])"));
}

TEST_F(LambdaTest, LambdaOperatorEmptyParams) {
    auto result = execute_script(R"(["lambda", [], 42])");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], json::array());
    EXPECT_EQ(result[1], json(42));
}

// --- Lambda Error Handling Tests ---

TEST_F(LambdaTest, LambdaOperatorErrors) {
    // Wrong number of arguments
    EXPECT_THROW(execute_script(R"(["lambda"])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["lambda", ["x"]])"), computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["lambda", ["x"], ["+", 1, 2], "extra"])"),
                 computo::InvalidArgumentException);

    // Non-array parameters
    EXPECT_THROW(execute_script(R"(["lambda", "x", ["+", 1, 2]])"),
                 computo::InvalidArgumentException);

    // Non-string parameter names
    EXPECT_THROW(execute_script(R"(["lambda", [123], ["+", 1, 2]])"),
                 computo::InvalidArgumentException);
    EXPECT_THROW(execute_script(R"(["lambda", [["nested"]], ["+", 1, 2]])"),
                 computo::InvalidArgumentException);
}

// --- First-Class Lambda Variable Storage Tests ---

TEST_F(LambdaTest, LambdaStorageInVariable) {
    auto result = execute_script(R"([
        "let", [["doubler", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]],
        ["$", "/doubler"]
    ])");

    // Should return the stored lambda
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], json::array({"x"}));
    EXPECT_EQ(result[1], json::parse(R"(["*", ["$", "/x"], 2])"));
}

TEST_F(LambdaTest, LambdaUsageFromVariable) {
    auto result = execute_script(R"([
        "let", [["doubler", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]],
        ["map", {"array": [1, 2, 3]}, ["$", "/doubler"]]
    ])");

    auto expected = json::parse(R"({"array": [2, 4, 6]})");
    EXPECT_EQ(result, expected);
}

TEST_F(LambdaTest, MultipleLambdaVariables) {
    auto result = execute_script(R"([
        "let", [
            ["add1", ["lambda", ["x"], ["+", ["$", "/x"], 1]]],
            ["mul2", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
        ],
        [
            "map", 
            ["map", {"array": [1, 2, 3]}, ["$", "/add1"]], 
            ["$", "/mul2"]
        ]
    ])");

    // First map: [1,2,3] + 1 = [2,3,4], then * 2 = [4,6,8]
    auto expected = json::parse(R"({"array": [4, 6, 8]})");
    EXPECT_EQ(result, expected);
}

TEST_F(LambdaTest, LambdaWithReduceFromVariable) {
    auto result = execute_script(R"([
        "let", [["summer", ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]]]],
        ["reduce", {"array": [1, 2, 3, 4]}, ["$", "/summer"], 0]
    ])");

    EXPECT_EQ(result, json(10));
}

TEST_F(LambdaTest, NestedLambdaVariables) {
    auto result = execute_script(R"([
        "let", [["outer", ["lambda", ["x"], 
            ["let", [["inner", ["lambda", ["y"], ["+", ["$", "/x"], ["$", "/y"]]]]],
             ["$", "/inner"]]
        ]]],
        ["map", {"array": [1, 2, 3]}, ["$", "/outer"]]
    ])");

    // This should return an array of lambda functions
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("array"));
    EXPECT_TRUE(result["array"].is_array());
    EXPECT_EQ(result["array"].size(), 3);
}

// --- Lambda Composition Tests ---

TEST_F(LambdaTest, LambdaComposition) {
    auto result = execute_script(R"([
        "let", [
            ["compose", ["lambda", ["f", "g", "x"], 
                ["map", ["map", {"array": [["$", "/x"]]}, ["$", "/g"]], ["$", "/f"]]
            ]],
            ["add10", ["lambda", ["x"], ["+", ["$", "/x"], 10]]],
            ["mul3", ["lambda", ["x"], ["*", ["$", "/x"], 3]]]
        ],
        ["$", "/compose"]
    ])");

    // Should return the compose lambda function
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], json::array({"f", "g", "x"}));
}

// --- Complex Integration Tests ---

TEST_F(LambdaTest, LambdaInComplexPipeline) {
    auto result = execute_script(R"([
        "let", [
            ["isEven", ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]],
            ["square", ["lambda", ["x"], ["*", ["$", "/x"], ["$", "/x"]]]],
            ["data", {"array": [1, 2, 3, 4, 5, 6]}]
        ],
        ["map", 
            ["filter", ["$", "/data"], ["$", "/isEven"]], 
            ["$", "/square"]
        ]
    ])");

    // Filter evens: [2,4,6], then square: [4,16,36]
    auto expected = json::parse(R"({"array": [4, 16, 36]})");
    EXPECT_EQ(result, expected);
}

TEST_F(LambdaTest, LambdaWithDeepNesting) {
    auto input = json::parse(R"({
        "users": [
            {"name": "Alice", "scores": [85, 92, 78]},
            {"name": "Bob", "scores": [90, 88, 94]}
        ]
    })");

    auto result = execute_script(R"([
        "map", ["$input", "/users"], 
         ["lambda", ["user"], 
          ["obj", 
           "name", ["$", "/user/name"],
           "total", ["reduce", ["$", "/user/scores"],
                     ["lambda", ["acc", "score"], ["+", ["$", "/acc"], ["$", "/score"]]], 0]
          ]
         ]
    ])",
                                 input);

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("array"));
    EXPECT_EQ(result["array"].size(), 2);

    // Check Alice's total (85+92+78=255)
    EXPECT_EQ(result["array"][0]["name"], "Alice");
    EXPECT_EQ(result["array"][0]["total"], 255);

    // Check Bob's total (90+88+94=272)
    EXPECT_EQ(result["array"][1]["name"], "Bob");
    EXPECT_EQ(result["array"][1]["total"], 272);
}