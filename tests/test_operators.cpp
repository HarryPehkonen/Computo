#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ComputoTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test literal values
TEST_F(ComputoTest, LiteralValues) {
    // Test numbers
    EXPECT_EQ(computo::execute(42, input_data), 42);
    EXPECT_EQ(computo::execute(3.14, input_data), 3.14);
    
    // Test strings
    EXPECT_EQ(computo::execute("hello", input_data), "hello");
    
    // Test booleans
    EXPECT_EQ(computo::execute(true, input_data), true);
    EXPECT_EQ(computo::execute(false, input_data), false);
    
    // Test null
    EXPECT_EQ(computo::execute(json(nullptr), input_data), json(nullptr));
    
    // Test arrays as literals
    json arr = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(arr, input_data), arr);
    
    // Test objects as literals
    json obj = json{{"key", "value"}};
    EXPECT_EQ(computo::execute(obj, input_data), obj);
}

// Test $input operator
TEST_F(ComputoTest, InputOperator) {
    json script = json::array({"$input"});
    EXPECT_EQ(computo::execute(script, input_data), input_data);
}

// Test addition operator
TEST_F(ComputoTest, AdditionOperator) {
    // Test integer addition
    json script1 = json::array({"+", 2, 3});
    EXPECT_EQ(computo::execute(script1, input_data), 5);
    
    // Test floating point addition
    json script2 = json::array({"+", 2.5, 1.5});
    EXPECT_EQ(computo::execute(script2, input_data), 4.0);
    
    // Test mixed integer/float addition
    json script3 = json::array({"+", 2, 1.5});
    EXPECT_EQ(computo::execute(script3, input_data), 3.5);
    
    // Test nested addition
    json script4 = json::array({"+", json::array({"+", 1, 2}), 3});
    EXPECT_EQ(computo::execute(script4, input_data), 6);
}

// Test addition with no arguments
TEST_F(ComputoTest, AdditionNoArguments) {
    json script = json::array({"+"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with one argument
TEST_F(ComputoTest, AdditionOneArgument) {
    json script = json::array({"+", 1});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with too many arguments
TEST_F(ComputoTest, AdditionTooManyArguments) {
    json script = json::array({"+", 1, 2, 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with string argument
TEST_F(ComputoTest, AdditionStringArgument) {
    json script = json::array({"+", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with boolean argument
TEST_F(ComputoTest, AdditionBooleanArgument) {
    json script = json::array({"+", 1, true});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test invalid operator
TEST_F(ComputoTest, InvalidOperator) {
    json script = json::array({"unknown_operator", 1, 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidOperatorException);
}

// Test arrays with non-string first element
TEST_F(ComputoTest, NonOperatorArrays) {
    // Array starting with number should be treated as literal
    json script1 = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(script1, input_data), script1);
    
    // Empty array should be treated as literal
    json script2 = json::array();
    EXPECT_EQ(computo::execute(script2, input_data), script2);
}

// Test exception hierarchy - InvalidArgumentException inherits from ComputoException
TEST_F(ComputoTest, InvalidArgumentExceptionHierarchy) {
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
TEST_F(ComputoTest, InvalidOperatorExceptionHierarchy) {
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
TEST_F(ComputoTest, StdExceptionHierarchy) {
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