#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class ErrorHandlingEdgeCasesTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test array objects vs operator calls
TEST_F(ErrorHandlingEdgeCasesTest, ArrayObjectsVsOperatorCalls) {
    // Array object should create literal array
    json array_obj = json{{"array", json::array({1, 2, 3})}};
    json expected_array = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(array_obj, input_data), expected_array);
    
    // Empty array object
    json empty_array_obj = json{{"array", json::array()}};
    json expected_empty = json::array();
    EXPECT_EQ(computo::execute(empty_array_obj, input_data), expected_empty);
    
    // Arrays starting with numbers should now throw errors (no longer literals)
    json invalid_script = json::array({1, 2, 3});
    EXPECT_THROW(computo::execute(invalid_script, input_data), computo::InvalidArgumentException);
}

// Test exception hierarchy - InvalidArgumentException inherits from ComputoException
TEST_F(ErrorHandlingEdgeCasesTest, InvalidArgumentExceptionHierarchy) {
    json script = json::array({"+", "hello", 2});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException to be thrown";
    } catch (const computo::ComputoException& e) {
        // This should catch InvalidArgumentException since it inherits from ComputoException
        SUCCEED();
    } catch (...) {
        FAIL() << "InvalidArgumentException should inherit from ComputoException";
    }
}

// Test exception hierarchy - InvalidOperatorException inherits from ComputoException
TEST_F(ErrorHandlingEdgeCasesTest, InvalidOperatorExceptionHierarchy) {
    json script = json::array({"unknown_operator", 1, 2});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidOperatorException to be thrown";
    } catch (const computo::ComputoException& e) {
        // This should catch InvalidOperatorException since it inherits from ComputoException
        SUCCEED();
    } catch (...) {
        FAIL() << "InvalidOperatorException should inherit from ComputoException";
    }
}

// Test exception hierarchy - All exceptions inherit from std::exception
TEST_F(ErrorHandlingEdgeCasesTest, StdExceptionHierarchy) {
    json script = json::array({"+", 1, true});
    try {
        computo::execute(script, input_data);
        FAIL() << "Expected InvalidArgumentException to be thrown";
    } catch (const std::exception& e) {
        // This should catch any of our exceptions since they all inherit from std::exception
        SUCCEED();
    } catch (...) {
        FAIL() << "All Computo exceptions should inherit from std::exception";
    }
}

// Test invalid operator errors
TEST_F(ErrorHandlingEdgeCasesTest, InvalidOperatorErrors) {
    json script = json::array({"unknown_operator", 1, 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidOperatorException);
    
    // Test with completely invalid operator
    json script2 = json::array({"@#$%", 1, 2});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidOperatorException);
}

// Test addition with invalid argument types
TEST_F(ErrorHandlingEdgeCasesTest, AdditionInvalidTypes) {
    // Test addition with string argument
    json script = json::array({"+", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    // Test addition with boolean argument
    json script2 = json::array({"+", 1, true});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Test addition with null argument
    json script3 = json::array({"+", 1, json(nullptr)});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    // Test addition with array argument
    json script4 = json::array({"+", 1, json::array({1, 2})});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
}

// Test subtraction with invalid argument types
TEST_F(ErrorHandlingEdgeCasesTest, SubtractionInvalidTypes) {
    json script = json::array({"-", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"-", 1, true});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test multiplication with invalid argument types
TEST_F(ErrorHandlingEdgeCasesTest, MultiplicationInvalidTypes) {
    json script = json::array({"*", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"*", 1, json::array({1, 2})});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test division with invalid argument types
TEST_F(ErrorHandlingEdgeCasesTest, DivisionInvalidTypes) {
    json script = json::array({"/", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"/", 1, json::object()});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test division by zero
TEST_F(ErrorHandlingEdgeCasesTest, DivisionByZero) {
    json script = json::array({"/", 10, 0});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"/", 10.5, 0.0});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test modulo with invalid argument types
TEST_F(ErrorHandlingEdgeCasesTest, ModuloInvalidTypes) {
    json script = json::array({"%", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"%", 1.5, 2});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test modulo by zero
TEST_F(ErrorHandlingEdgeCasesTest, ModuloByZero) {
    json script = json::array({"%", 10, 0});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test operators with wrong argument counts
TEST_F(ErrorHandlingEdgeCasesTest, WrongArgumentCounts) {
    // Addition with no arguments
    json script1 = json::array({"+"});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Subtraction with no arguments
    json script2 = json::array({"-"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Division with one argument
    json script3 = json::array({"/", 5});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    // Subtraction with too many arguments
    json script4 = json::array({"-", 1, 2, 3});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
}

// Test let operator errors
TEST_F(ErrorHandlingEdgeCasesTest, LetOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"let", json::array({json::array({"x", 5})})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Non-array bindings
    json script2 = json::array({"let", "not_array", json::array({"$", "/x"})});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Invalid binding format
    json script3 = json::array({
        "let",
        json::array({json::array({"x"})}), // Missing value
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test $ operator errors
TEST_F(ErrorHandlingEdgeCasesTest, DollarOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"$"});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"$", "/x", "/y"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-string variable name
    json script3 = json::array({"$", 123});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    // Variable not found
    json script4 = json::array({"$", "/nonexistent"});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
}

// Test get operator errors
TEST_F(ErrorHandlingEdgeCasesTest, GetOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"get", json::object()});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"get", json::object(), "/path", "extra"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-string path
    json script3 = json::array({"get", json::object(), 123});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    // Invalid JSON pointer
    json script4 = json::array({"get", json::object(), "invalid_path"});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
    
    // Path not found
    json script5 = json::array({"get", json::object(), "/nonexistent"});
    EXPECT_THROW(computo::execute(script5, input_data), computo::InvalidArgumentException);
}

// Test if operator errors
TEST_F(ErrorHandlingEdgeCasesTest, IfOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"if", true});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({"if", true, "then", "else", "extra"});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test obj operator errors
TEST_F(ErrorHandlingEdgeCasesTest, ObjOperatorErrors) {
    // Non-array field specification
    json script1 = json::array({"obj", "not_array"});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Invalid field format
    json script2 = json::array({"obj", json::array({json::array({"key"})})});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-string key
    json script3 = json::array({"obj", json::array({json::array({123, "value"})})});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test map operator errors
TEST_F(ErrorHandlingEdgeCasesTest, MapOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"map", json::array()});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Non-array input
    json script2 = json::array({
        "map",
        "not_array",
        json::array({"lambda", json::array({"x"}), json::array({"$", "/x"})})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Invalid lambda
    json script3 = json::array({
        "map",
        json::array({1, 2, 3}),
        "not_lambda"
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test filter operator errors
TEST_F(ErrorHandlingEdgeCasesTest, FilterOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"filter", json::array()});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Non-array input
    json script2 = json::array({
        "filter",
        "not_array",
        json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), 0})})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test lambda operator errors
TEST_F(ErrorHandlingEdgeCasesTest, LambdaOperatorErrors) {
    // Wrong argument count
    json script1 = json::array({"lambda", json::array({"x"})});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Non-array parameters
    json script2 = json::array({
        "lambda",
        "not_array",
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Non-string parameter
    json script3 = json::array({
        "lambda",
        json::array({123}),
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test nested error propagation
TEST_F(ErrorHandlingEdgeCasesTest, NestedErrorPropagation) {
    // Error in nested expression should propagate up
    json script = json::array({
        "+",
        1,
        json::array({"/", 10, 0})  // Division by zero
    });
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    // Error in let binding should propagate
    json script2 = json::array({
        "let",
        json::array({json::array({"x", json::array({"/", 1, 0})})}),
        json::array({"$", "/x"})
    });
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

// Test malformed JSON operations
TEST_F(ErrorHandlingEdgeCasesTest, MalformedOperations) {
    // Empty array should throw error
    json script1 = json::array();
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    // Array with non-string operator
    json script2 = json::array({123, 1, 2});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Array with null operator
    json script3 = json::array({json(nullptr), 1, 2});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}

// Test complex nested error scenarios
TEST_F(ErrorHandlingEdgeCasesTest, ComplexNestedErrors) {
    // Multiple levels of nesting with error deep inside
    json script = json::array({
        "let",
        json::array({
            json::array({"a", 1}),
            json::array({"b", json::array({
                "if",
                true,
                json::array({"+", 1, 2}),
                json::array({"/", 1, 0})  // This would cause error if evaluated
            })})
        }),
        json::array({
            "map",
            json::array({json::array({1, 2, 3})}),
            json::array({
                "lambda",
                json::array({"x"}),
                json::array({
                    "*",
                    json::array({"$", "/a"}),
                    json::array({"$", "/b"}),
                    json::array({"/", json::array({"$", "/x"}), 0})  // Division by zero
                })
            })
        })
    });
    
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test edge cases with comparison operators
TEST_F(ErrorHandlingEdgeCasesTest, ComparisonOperatorEdgeCases) {
    // Comparing incompatible types
    json script1 = json::array({"<", "hello", 5});
    EXPECT_THROW(computo::execute(script1, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({">", json::array({1, 2}), 5});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Wrong argument count
    json script3 = json::array({"<", 5});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
}