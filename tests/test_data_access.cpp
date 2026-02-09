#include "computo.hpp"
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

TEST(DataAccessOperators, InputOperatorBasic) {
    json input_data = {{"key", "value"}, {"number", 42}}; // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$input"])"), std::vector<json>{input_data}), input_data);

    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$input"])"), std::vector<json>{123}),
              123); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$input"])"), std::vector<json>{json("hello")}), "hello");
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$input"])"), std::vector<json>{json(std::vector<json>{1, 2, 3})}),
              json(std::vector<json>{1, 2, 3}));
}

TEST(DataAccessOperators, InputOperatorErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$input", "extra_arg"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(DataAccessOperators, InputsOperatorBasic) {
    json input_data = {{"key", "value"}};
    json expected_single = json(std::vector<json>{input_data});
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs"])"), std::vector<json>{input_data}), expected_single);

    std::vector<json> multiple_inputs = {{{"first", 1}}, {{"second", 2}}, {{"third", 3}}};
    auto expected_multiple = jsom::parse_document(R"([{"first": 1}, {"second": 2}, {"third": 3}])");
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs"])"), multiple_inputs), expected_multiple);
}

TEST(DataAccessOperators, InputsOperatorEmpty) {
    std::vector<json> empty_inputs;
    auto expected_empty = jsom::parse_document(R"([])");
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs"])"), empty_inputs), expected_empty);
}

TEST(DataAccessOperators, InputsOperatorErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", "extra_arg"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
}

// New comprehensive tests for $inputs JSON Pointer functionality

TEST(DataAccessOperators, InputsOperatorJSONPointerBasic) {
    // Test basic array indexing with JSON Pointer
    std::vector<json> inputs = {json("first"), json("second"), json("third")};

    // Access first input: /0
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0"])"), inputs), json("first"));

    // Access second input: /1
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/1"])"), inputs), json("second"));

    // Access third input: /2
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/2"])"), inputs), json("third"));
}

TEST(DataAccessOperators, InputsOperatorJSONPointerDeepPath) {
    // Test deep path navigation within inputs
    std::vector<json> inputs
        = {jsom::parse_document(R"({"users": [{"name": "Alice", "age": 30}, {"name": "Bob", "age": 25}]})"),
           jsom::parse_document(
               R"({"users": [{"name": "Charlie", "age": 35}, {"name": "Diana", "age": 28}]})")};

    // Access first input's first user name: /0/users/0/name
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0/users/0/name"])"), inputs),
              json("Alice"));

    // Access first input's second user age: /0/users/1/age
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0/users/1/age"])"), inputs), json(25));

    // Access second input's first user name: /1/users/0/name
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/1/users/0/name"])"), inputs),
              json("Charlie"));

    // Access second input's second user age: /1/users/1/age
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/1/users/1/age"])"), inputs), json(28));
}

TEST(DataAccessOperators, InputsOperatorJSONPointerErrors) {
    std::vector<json> inputs = {json("first"), json("second")};

    // Test out of bounds access
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", "/10"])"), inputs),
                 computo::InvalidArgumentException);

    // Test invalid JSON Pointer format (no leading slash)
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", "0"])"), inputs),
                 computo::InvalidArgumentException);

    // Test invalid JSON Pointer format (empty string)
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", ""])"), inputs),
                 computo::InvalidArgumentException);

    // Test invalid path within valid input
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", "/0/nonexistent"])"), inputs),
                 computo::InvalidArgumentException);

    // Test integer argument (old syntax) - should now fail
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$inputs", 0])"), inputs),
                 computo::InvalidArgumentException);
}

TEST(DataAccessOperators, InputsOperatorJSONPointerSingleInput) {
    // Test with single input (common case)
    std::vector<json> single_input = {jsom::parse_document(R"({"data": {"value": 42}})")};

    // Access entire first input: /0
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0"])"), single_input),
              jsom::parse_document(R"({"data": {"value": 42}})"));

    // Access nested path: /0/data/value
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0/data/value"])"), single_input),
              json(42));
}

TEST(DataAccessOperators, InputsOperatorJSONPointerArrayElements) {
    // Test accessing array elements within inputs
    std::vector<json> inputs
        = {jsom::parse_document(R"([10, 20, 30])"), jsom::parse_document(R"({"items": [100, 200, 300]})")};

    // Access elements of first input (which is an array): /0/1
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/0/1"])"), inputs), json(20));

    // Access elements of array within second input: /1/items/2
    EXPECT_EQ(computo::execute(jsom::parse_document(R"(["$inputs", "/1/items/2"])"), inputs), json(300));
}

TEST(DataAccessOperators, VariableOperatorBasic) {
    json script
        = jsom::parse_document(R"(["let", [["x", 42]], ["$", "/x"]])"); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}),
              42); // NOLINT(readability-magic-numbers)

    script = jsom::parse_document(R"(["let", [["name", "Alice"]], ["$", "/name"]])");
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}), "Alice");

    script = jsom::parse_document(R"(["let", [["data", {"a": 1}]], ["$", "/data"]])");
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}), json({{"a", 1}}));
}

TEST(DataAccessOperators, VariableOperatorNotFound) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$", "nonexistent_var"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
}

TEST(DataAccessOperators, VariableOperatorErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$", "x", "y"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["$", 123])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException); // NOLINT(readability-magic-numbers)
}

TEST(DataAccessOperators, LetOperatorBasic) {
    json script = jsom::parse_document(
        R"(["let", [["x", 10]], ["+", ["$", "/x"], 5]])"); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}),
              15); // NOLINT(readability-magic-numbers)

    script = jsom::parse_document(R"(["let", [["a", 1], ["b", 2]], ["*", ["$", "/a"], ["$", "/b"]]])");
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}), 2);
}

TEST(DataAccessOperators, LetOperatorNested) {
    json script = jsom::parse_document(
        R"(["let", [["x", 10]], ["let", [["y", 20]], ["+", ["$", "/x"], ["$", "/y"]]]])"); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}),
              30); // NOLINT(readability-magic-numbers)
}

TEST(DataAccessOperators, LetOperatorShadowing) {
    json script = jsom::parse_document(
        R"(["let", [["x", 10]], ["let", [["x", 20]], ["$", "/x"]]])"); // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}),
              20); // NOLINT(readability-magic-numbers)
}

TEST(DataAccessOperators, LetOperatorEvaluatedBindings) {
    json script = jsom::parse_document(R"(["let", [["x", ["+", 1, 2]]], ["$", "/x"]])");
    EXPECT_EQ(computo::execute(script, std::vector<json>{json(nullptr)}), 3);
}

TEST(DataAccessOperators, LetOperatorErrors) {
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["let"])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["let", ["x", 10]])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException); // NOLINT(readability-magic-numbers)
    EXPECT_THROW(
        computo::execute(jsom::parse_document(R"(["let", "not_an_array", ["$", "x"]])"), std::vector<json>{json(nullptr)}),
        computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(jsom::parse_document(R"(["let", [["x"]], ["$", "x"]])"), std::vector<json>{json(nullptr)}),
                 computo::InvalidArgumentException);
    EXPECT_THROW(
        computo::execute(jsom::parse_document(R"(["let", [[10, "x"]], ["$", "x"]])"), std::vector<json>{json(nullptr)}),
        computo::InvalidArgumentException); // NOLINT(readability-magic-numbers)
}

// Variable Suggestion Tests
TEST(DataAccessOperators, VariableSuggestionSingleCharacterTypo) {
    try {
        computo::execute(jsom::parse_document(R"(["let", [["users", "data"]], ["$", "/usrs"]])"),
                         std::vector<json>{json(nullptr)});
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Variable not found: 'usrs'"), std::string::npos);
        EXPECT_NE(error_msg.find("Did you mean 'users'?"), std::string::npos);
    }
}

TEST(DataAccessOperators, VariableSuggestionTransposition) {
    try {
        computo::execute(
            jsom::parse_document(R"(["let", [["data", "value"], ["count", 5]], ["$", "/dato"]])"),
            std::vector<json>{json(nullptr)});
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Variable not found: 'dato'"), std::string::npos);
        EXPECT_NE(error_msg.find("Did you mean 'data'?"), std::string::npos);
    }
}

TEST(DataAccessOperators, VariableSuggestionMultipleOptions) {
    try {
        computo::execute(
            jsom::parse_document(R"(["let", [["name", "Alice"], ["num", 42], ["age", 30]], ["$", "/nam"]])"),
            std::vector<json>{json(nullptr)});
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Variable not found: 'nam'"), std::string::npos);
        // Should suggest closest match (name over num/age)
        EXPECT_NE(error_msg.find("Did you mean 'name'?"), std::string::npos);
    }
}

TEST(DataAccessOperators, VariableSuggestionNoCloseMatch) {
    try {
        computo::execute(
            jsom::parse_document(R"(["let", [["x", 1], ["y", 2]], ["$", "/completely_different"]])"),
            std::vector<json>{json(nullptr)});
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Variable not found: 'completely_different'"), std::string::npos);
        // Should not have suggestion since no close matches
        EXPECT_EQ(error_msg.find("Did you mean"), std::string::npos);
    }
}

TEST(DataAccessOperators, VariableSuggestionEmptyScope) {
    try {
        computo::execute(jsom::parse_document(R"(["$", "/nonexistent"])"), std::vector<json>{json(nullptr)});
        FAIL() << "Expected InvalidArgumentException";
    } catch (const computo::InvalidArgumentException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Variable not found: 'nonexistent'"), std::string::npos);
        // No suggestions possible with empty scope
        EXPECT_EQ(error_msg.find("Did you mean"), std::string::npos);
    }
}
