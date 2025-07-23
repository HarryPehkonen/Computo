#include "operators/shared.hpp"
#include <computo.hpp>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace computo;

class SharedUtilitiesTest : public ::testing::Test {
protected:
    void SetUp() override { ctx = ExecutionContext(json{}); }

    ExecutionContext ctx = ExecutionContext(json{});
};

// --- is_truthy() Tests ---

TEST_F(SharedUtilitiesTest, IsTruthyBoolean) {
    EXPECT_TRUE(is_truthy(json(true)));
    EXPECT_FALSE(is_truthy(json(false)));
}

TEST_F(SharedUtilitiesTest, IsTruthyNumbers) {
    EXPECT_TRUE(is_truthy(json(1)));
    EXPECT_TRUE(is_truthy(json(-1)));
    const double SMALL_POSITIVE = 0.1;
    const double SMALL_NEGATIVE = -0.1;
    const int POSITIVE_INT = 42;
    EXPECT_TRUE(is_truthy(json(SMALL_POSITIVE)));
    EXPECT_TRUE(is_truthy(json(SMALL_NEGATIVE)));
    EXPECT_TRUE(is_truthy(json(POSITIVE_INT)));

    EXPECT_FALSE(is_truthy(json(0)));
    EXPECT_FALSE(is_truthy(json(0.0)));
}

TEST_F(SharedUtilitiesTest, IsTruthyStrings) {
    EXPECT_TRUE(is_truthy(json("hello")));
    EXPECT_TRUE(is_truthy(json("0")));
    EXPECT_TRUE(is_truthy(json("false")));
    EXPECT_TRUE(is_truthy(json(" ")));

    EXPECT_FALSE(is_truthy(json("")));
}

TEST_F(SharedUtilitiesTest, IsTruthyArrays) {
    const std::vector<int> TEST_VALUES = {1, 2, 3};
    EXPECT_TRUE(is_truthy(json::array({TEST_VALUES[0], TEST_VALUES[1], TEST_VALUES[2]})));
    EXPECT_TRUE(is_truthy(json::array({0})));
    EXPECT_TRUE(is_truthy(json::array({""})));

    EXPECT_FALSE(is_truthy(json::array()));
}

TEST_F(SharedUtilitiesTest, IsTruthyObjects) {
    EXPECT_TRUE(is_truthy(json::object({{"key", "value"}})));
    EXPECT_TRUE(is_truthy(json::object({{"key", false}})));

    EXPECT_FALSE(is_truthy(json::object()));
}

TEST_F(SharedUtilitiesTest, IsTruthyNull) { EXPECT_FALSE(is_truthy(json(nullptr))); }

// --- validate_numeric_args() Tests ---

TEST_F(SharedUtilitiesTest, ValidateNumericArgsSuccess) {
    const double TWO_POINT_FIVE = 2.5;
    json args = json::array({1, TWO_POINT_FIVE, -3, 0});
    EXPECT_NO_THROW(validate_numeric_args(args, "test_op", "test_path"));
}

TEST_F(SharedUtilitiesTest, ValidateNumericArgsEmptyArray) {
    json args = json::array();
    EXPECT_NO_THROW(validate_numeric_args(args, "test_op", "test_path"));
}

TEST_F(SharedUtilitiesTest, ValidateNumericArgsFailureString) {
    json args = json::array({1, "not_a_number", 3});
    EXPECT_THROW(
        {
            try {
                validate_numeric_args(args, "test_op", "test_path");
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: test_op requires numeric "
                             "arguments, got string at argument 1 at test_path",
                             e.what());
                EXPECT_TRUE(std::string(e.what()).find("test_path") != std::string::npos);
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, ValidateNumericArgsFailureArray) {
    const std::vector<int> TEST_ARRAY_VALUES = {2, 3};
    json args = json::array({1, json::array({TEST_ARRAY_VALUES[0], TEST_ARRAY_VALUES[1]})});
    EXPECT_THROW(
        {
            try {
                validate_numeric_args(args, "test_op", "test_path");
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: test_op requires numeric "
                             "arguments, got array at argument 1 at test_path",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, ValidateNumericArgsFailureNull) {
    json args = json::array({1, nullptr, 3});
    EXPECT_THROW(
        {
            try {
                validate_numeric_args(args, "test_op", "test_path");
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: test_op requires numeric "
                             "arguments, got null at argument 1 at test_path",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

// --- evaluate_lambda() Tests ---

TEST_F(SharedUtilitiesTest, EvaluateLambdaSimple) {
    // Lambda: [["x"], ["$", "/x"]]
    json lambda = json::array({json::array({"x"}), json::array({"$", "/x"})});

    const int TEST_VALUE = 42;
    std::vector<json> args = {json(TEST_VALUE)};
    auto lambda_result = evaluate_lambda(lambda, args, ctx);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result.is_tail_call) {
        lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                          lambda_result.tail_call->context);
    }

    EXPECT_EQ(lambda_result.value, json(TEST_VALUE));
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaMultipleParams) {
    // Lambda: [["x", "y"], ["+", ["$", "/x"], ["$", "/y"]]]
    json lambda
        = json::array({json::array({"x", "y"}),
                       json::array({"+", json::array({"$", "/x"}), json::array({"$", "/y"})})});

    const int FIRST_VALUE = 10;
    const int SECOND_VALUE = 20;
    std::vector<json> args = {json(FIRST_VALUE), json(SECOND_VALUE)};
    auto lambda_result = evaluate_lambda(lambda, args, ctx);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result.is_tail_call) {
        lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                          lambda_result.tail_call->context);
    }

    const int EXPECTED_SUM = 30;
    EXPECT_EQ(lambda_result.value, json(EXPECTED_SUM));
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaNoParams) {
    // Lambda: [[], 42]
    const int TEST_VALUE = 42;
    json lambda = json::array({json::array(), json(TEST_VALUE)});

    std::vector<json> args = {};
    auto lambda_result = evaluate_lambda(lambda, args, ctx);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result.is_tail_call) {
        lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                          lambda_result.tail_call->context);
    }

    EXPECT_EQ(lambda_result.value, json(TEST_VALUE));
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaInvalidFormat) {
    // Not an array
    const int BODY_VALUE = 42;
    json lambda = json::object({{"params", json::array()}, {"body", BODY_VALUE}});
    std::vector<json> args = {};

    EXPECT_THROW(
        {
            try {
                evaluate_lambda(lambda, args, ctx);
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: Lambda must be an array with 2 "
                             "elements: [params, body] at /",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaWrongSize) {
    // Too many elements
    const int BODY_VALUE = 42;
    const int EXTRA_VALUE = 43;
    json lambda = json::array({json::array({"x"}), json(BODY_VALUE), json(EXTRA_VALUE)});
    const int FIRST_VALUE = 10;
    std::vector<json> args = {json(FIRST_VALUE)};

    EXPECT_THROW(
        {
            try {
                evaluate_lambda(lambda, args, ctx);
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: Lambda must be an array with 2 "
                             "elements: [params, body] at /",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaInvalidParams) {
    // Parameters not an array
    const int BODY_VALUE = 42;
    json lambda = json::array({json("x"), json(BODY_VALUE)});
    const int FIRST_VALUE = 10;
    std::vector<json> args = {json(FIRST_VALUE)};

    EXPECT_THROW(
        {
            try {
                evaluate_lambda(lambda, args, ctx);
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: Lambda parameters must be an array at /", e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaParamCountMismatch) {
    // Lambda expects 2 params, got 1
    const int BODY_VALUE = 42;
    json lambda = json::array({json::array({"x", "y"}), json(BODY_VALUE)});
    const int FIRST_VALUE = 10;
    std::vector<json> args = {json(FIRST_VALUE)};

    EXPECT_THROW(
        {
            try {
                evaluate_lambda(lambda, args, ctx);
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: Lambda expects 2 arguments, got 1 at /", e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

TEST_F(SharedUtilitiesTest, EvaluateLambdaNonStringParam) {
    // Parameter name is not a string
    const int PARAM_VALUE = 42;
    const int BODY_VALUE = 42;
    json lambda = json::array({json::array({PARAM_VALUE}), json(BODY_VALUE)});
    const int FIRST_VALUE = 10;
    std::vector<json> args = {json(FIRST_VALUE)};

    EXPECT_THROW(
        {
            try {
                evaluate_lambda(lambda, args, ctx);
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: Lambda parameter names must be "
                             "strings at /",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

// --- to_numeric() Tests ---

TEST_F(SharedUtilitiesTest, ToNumericSuccess) {
    const int TEST_INT = 42;
    const double PI = 3.14; // NOLINT(readability-identifier-length)
    EXPECT_EQ(to_numeric(json(TEST_INT), "test_op", "test_path"), 42.0);
    EXPECT_EQ(to_numeric(json(PI), "test_op", "test_path"), PI);
    EXPECT_EQ(to_numeric(json(-1), "test_op", "test_path"), -1.0);
    EXPECT_EQ(to_numeric(json(0), "test_op", "test_path"), 0.0);
}

TEST_F(SharedUtilitiesTest, ToNumericFailure) {
    EXPECT_THROW(
        {
            try {
                to_numeric(json("42"), "test_op", "test_path");
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: test_op requires numeric "
                             "argument, got string at test_path",
                             e.what());
                EXPECT_TRUE(std::string(e.what()).find("test_path") != std::string::npos);
                throw;
            }
        },
        InvalidArgumentException);

    const std::vector<int> TEST_ARRAY_VALUES = {1, 2};
    EXPECT_THROW(
        {
            try {
                to_numeric(json::array({TEST_ARRAY_VALUES[0], TEST_ARRAY_VALUES[1]}), "test_op",
                           "test_path");
            } catch (const InvalidArgumentException& e) {
                EXPECT_STREQ("Invalid argument: test_op requires numeric "
                             "argument, got array at test_path",
                             e.what());
                throw;
            }
        },
        InvalidArgumentException);
}

// --- get_type_name() Tests ---

TEST_F(SharedUtilitiesTest, GetTypeName) {
    const int TEST_INT = 42;
    const double PI = 3.14; // NOLINT(readability-identifier-length)
    EXPECT_EQ(get_type_name(json(nullptr)), "null");
    EXPECT_EQ(get_type_name(json(true)), "boolean");
    EXPECT_EQ(get_type_name(json(false)), "boolean");
    EXPECT_EQ(get_type_name(json(TEST_INT)), "integer");
    EXPECT_EQ(get_type_name(json(PI)), "number");
    EXPECT_EQ(get_type_name(json("hello")), "string");
    EXPECT_EQ(get_type_name(json::array()), "array");
    EXPECT_EQ(get_type_name(json::object()), "object");
}

// --- Integration Tests ---

TEST_F(SharedUtilitiesTest, LambdaWithComplexExpression) {
    // Lambda that filters even numbers: [["x"], ["==", ["%", ["$", "/x"], 2],
    // 0]]
    json lambda = json::array(
        {json::array({"x"}),
         json::array({"==", json::array({"%", json::array({"$", "/x"}), json(2)}), json(0)})});

    // Test with even number
    const int EVEN_VALUE = 4;
    std::vector<json> args_even = {json(EVEN_VALUE)};
    auto lambda_result_even = evaluate_lambda(lambda, args_even, ctx);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result_even.is_tail_call) {
        lambda_result_even = evaluate_internal(lambda_result_even.tail_call->expression,
                                               lambda_result_even.tail_call->context);
    }

    EXPECT_EQ(lambda_result_even.value, json(true));

    // Test with odd number
    const int ODD_VALUE = 3;
    std::vector<json> args_odd = {json(ODD_VALUE)};
    auto lambda_result_odd = evaluate_lambda(lambda, args_odd, ctx);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result_odd.is_tail_call) {
        lambda_result_odd = evaluate_internal(lambda_result_odd.tail_call->expression,
                                              lambda_result_odd.tail_call->context);
    }

    EXPECT_EQ(lambda_result_odd.value, json(false));
}

TEST_F(SharedUtilitiesTest, LambdaVariableShadowing) {
    // Create context with existing variable "x"
    const int OUTER_VALUE = 100;
    std::map<std::string, json> vars = {{"x", json(OUTER_VALUE)}};
    auto ctx_with_vars = ctx.with_variables(vars);

    // Lambda: [["x"], ["$", "/x"]] - should use lambda parameter, not outer
    // variable
    json lambda = json::array({json::array({"x"}), json::array({"$", "/x"})});

    const int TEST_VALUE = 42;
    std::vector<json> args = {json(TEST_VALUE)};
    auto lambda_result = evaluate_lambda(lambda, args, ctx_with_vars);

    // Resolve any tail calls from lambda evaluation
    while (lambda_result.is_tail_call) {
        lambda_result = evaluate_internal(lambda_result.tail_call->expression,
                                          lambda_result.tail_call->context);
    }

    EXPECT_EQ(lambda_result.value,
              json(TEST_VALUE)); // Should be lambda parameter, not outer variable
}

// --- Levenshtein Distance Tests ---

TEST_F(SharedUtilitiesTest, LevenshteinDistanceIdentical) {
    EXPECT_EQ(calculate_levenshtein_distance("hello", "hello"), 0);
    EXPECT_EQ(calculate_levenshtein_distance("", ""), 0);
    EXPECT_EQ(calculate_levenshtein_distance("a", "a"), 0);
}

TEST_F(SharedUtilitiesTest, LevenshteinDistanceEmptyStrings) {
    EXPECT_EQ(calculate_levenshtein_distance("", "hello"), 5);
    EXPECT_EQ(calculate_levenshtein_distance("hello", ""), 5);
}

TEST_F(SharedUtilitiesTest, LevenshteinDistanceSingleCharacter) {
    EXPECT_EQ(calculate_levenshtein_distance("a", "b"), 1); // substitution
    EXPECT_EQ(calculate_levenshtein_distance("a", ""), 1);  // deletion
    EXPECT_EQ(calculate_levenshtein_distance("", "a"), 1);  // insertion
}

TEST_F(SharedUtilitiesTest, LevenshteinDistanceTypicalCases) {
    EXPECT_EQ(calculate_levenshtein_distance("kitten", "sitting"), 3);
    EXPECT_EQ(calculate_levenshtein_distance("map", "mpa"), 2);       // two swaps
    EXPECT_EQ(calculate_levenshtein_distance("reduce", "redcue"), 2); // two swaps
    EXPECT_EQ(calculate_levenshtein_distance("filter", "filer"), 1);  // one deletion
    EXPECT_EQ(calculate_levenshtein_distance("users", "usrs"), 1);    // one deletion
}

TEST_F(SharedUtilitiesTest, LevenshteinDistanceOperatorTypos) {
    // Common operator typos from documentation examples
    EXPECT_EQ(calculate_levenshtein_distance("mpa", "map"), 2);
    EXPECT_EQ(calculate_levenshtein_distance("redcue", "reduce"), 2);
    EXPECT_EQ(calculate_levenshtein_distance("filer", "filter"), 1);
    EXPECT_EQ(calculate_levenshtein_distance("stConcat", "strConcat"), 1);
}

// --- Suggestion System Tests ---

TEST_F(SharedUtilitiesTest, SuggestSimilarNamesBasic) {
    std::vector<std::string> candidates = {"map", "filter", "reduce", "count"};

    auto suggestions = suggest_similar_names("mpa", candidates, 2);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0], "map"); // closest match

    suggestions = suggest_similar_names("filer", candidates, 2);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0], "filter"); // closest match
}

TEST_F(SharedUtilitiesTest, SuggestSimilarNamesDistanceFiltering) {
    std::vector<std::string> candidates = {"map", "filter", "reduce", "count"};

    // Should find matches within distance 2
    auto suggestions = suggest_similar_names("mpa", candidates, 2);
    EXPECT_FALSE(suggestions.empty());

    // Should not find matches with distance > 2
    suggestions = suggest_similar_names("completely_different", candidates, 2);
    EXPECT_TRUE(suggestions.empty());
}

TEST_F(SharedUtilitiesTest, SuggestSimilarNamesSorting) {
    std::vector<std::string> candidates = {"apple", "apply", "appl", "application"};

    // All have distance 1, should be sorted alphabetically
    auto suggestions = suggest_similar_names("app", candidates, 2);
    ASSERT_GE(suggestions.size(), 2);

    // Check that lower distances come first, then alphabetically
    std::vector<int> distances;
    for (const auto& suggestion : suggestions) {
        distances.push_back(calculate_levenshtein_distance("app", suggestion));
    }

    // Should be sorted by distance first
    for (size_t i = 1; i < distances.size(); ++i) {
        EXPECT_LE(distances[i - 1], distances[i]);
    }
}

TEST_F(SharedUtilitiesTest, SuggestSimilarNamesEmptyCandidates) {
    std::vector<std::string> candidates = {};

    auto suggestions = suggest_similar_names("test", candidates, 2);
    EXPECT_TRUE(suggestions.empty());
}

TEST_F(SharedUtilitiesTest, SuggestSimilarNamesVariableExamples) {
    std::vector<std::string> candidates = {"users", "data", "count", "name", "value"};

    // Variable typo examples
    auto suggestions = suggest_similar_names("usrs", candidates, 2);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0], "users");

    suggestions = suggest_similar_names("dato", candidates, 2);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0], "data");

    suggestions = suggest_similar_names("nam", candidates, 2);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0], "name");
}

// --- Operator Suggestion Integration Tests ---

TEST_F(SharedUtilitiesTest, OperatorSuggestionIntegration) {
    // Test invalid operator throws exception with suggestion
    try {
        computo::execute(json::parse(R"(["mpa", [1, 2, 3], ["x"], ["*", ["$", "/x"], 2]])"),
                         {json(nullptr)});
        FAIL() << "Expected InvalidOperatorException";
    } catch (const computo::InvalidOperatorException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Invalid operator: mpa"), std::string::npos);
        EXPECT_NE(error_msg.find("Did you mean 'map'?"), std::string::npos);
    }
}

TEST_F(SharedUtilitiesTest, OperatorSuggestionNoCloseMatch) {
    // Test invalid operator with no close matches
    try {
        computo::execute(json::parse(R"(["completely_nonexistent_operator", 1, 2])"),
                         {json(nullptr)});
        FAIL() << "Expected InvalidOperatorException";
    } catch (const computo::InvalidOperatorException& e) {
        std::string error_msg(e.what());
        EXPECT_NE(error_msg.find("Invalid operator: completely_nonexistent_operator"),
                  std::string::npos);
        // Should not have suggestion since no close matches
        EXPECT_EQ(error_msg.find("Did you mean"), std::string::npos);
    }
}
