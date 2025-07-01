#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class BasicOperatorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test literal values
TEST_F(BasicOperatorsTest, LiteralValues) {
    // Test numbers
    EXPECT_EQ(computo::execute(CB::number(42), input_data), 42);
    EXPECT_EQ(computo::execute(CB::number(3.14), input_data), 3.14);
    
    // Test strings
    EXPECT_EQ(computo::execute(CB::string("hello"), input_data), "hello");
    
    // Test booleans
    EXPECT_EQ(computo::execute(CB::boolean(true), input_data), true);
    EXPECT_EQ(computo::execute(CB::boolean(false), input_data), false);
    
    // Test null
    EXPECT_EQ(computo::execute(CB::null(), input_data), json(nullptr));
    
    // Test arrays using Builder Pattern - much cleaner!
    auto arr_script = CB::array({1, 2, 3});
    json expected_arr = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(arr_script, input_data), expected_arr);
    
    // Test objects as literals
    json obj = json{{"key", "value"}};
    EXPECT_EQ(computo::execute(obj, input_data), obj);
}

// Test $input operator
TEST_F(BasicOperatorsTest, InputOperator) {
    auto script = CB::input();
    EXPECT_EQ(computo::execute(script, input_data), input_data);
}

// Test addition operator
TEST_F(BasicOperatorsTest, AdditionOperator) {
    // Test integer addition - much cleaner than json::array({"+", 2, 3})
    auto script1 = CB::add(2, 3);
    EXPECT_EQ(computo::execute(script1, input_data), 5);
    
    // Test floating point addition
    auto script2 = CB::add(2.5, 1.5);
    EXPECT_EQ(computo::execute(script2, input_data), 4.0);
    
    // Test mixed integer/float addition
    auto script3 = CB::add(2, 1.5);
    EXPECT_EQ(computo::execute(script3, input_data), 3.5);
    
    // Test nested addition - Builder Pattern makes nesting readable!
    auto script4 = CB::add(CB::add(1, 2), 3);
    EXPECT_EQ(computo::execute(script4, input_data), 6);
}

// Test addition with no arguments
TEST_F(BasicOperatorsTest, AdditionNoArguments) {
    auto script = CB::op("+");  // Empty operator
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with one argument - now valid (returns the argument)
TEST_F(BasicOperatorsTest, AdditionOneArgument) {
    auto script = CB::op("+").arg(5);
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test addition with multiple arguments - now valid (n-ary)
TEST_F(BasicOperatorsTest, AdditionMultipleArguments) {
    auto script = CB::add({1, 2, 3, 4});  // Much cleaner than manual array construction!
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

// Test addition with string argument
TEST_F(BasicOperatorsTest, AdditionStringArgument) {
    auto script = CB::add("hello", 2);
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test addition with boolean argument
TEST_F(BasicOperatorsTest, AdditionBooleanArgument) {
    auto script = CB::add(1, true);
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// Test invalid operator
TEST_F(BasicOperatorsTest, InvalidOperator) {
    auto script = CB::op("unknown_operator").arg(1).arg(2);
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidOperatorException);
}

// Test array objects vs operator calls
TEST_F(BasicOperatorsTest, ArrayObjectsVsOperatorCalls) {
    // Array object should create literal array - Builder Pattern is so much cleaner!
    auto array_script = CB::array({1, 2, 3});
    json expected_array = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(array_script, input_data), expected_array);
    
    // Empty array object
    auto empty_script = CB::empty_array();
    json expected_empty = json::array();
    EXPECT_EQ(computo::execute(empty_script, input_data), expected_empty);
    
    // Arrays starting with numbers should now throw errors (no longer literals)
    json invalid_script = json::array({1, 2, 3});
    EXPECT_THROW(computo::execute(invalid_script, input_data), computo::InvalidArgumentException);
}

// Test exception hierarchy - InvalidArgumentException inherits from ComputoException
TEST_F(BasicOperatorsTest, InvalidArgumentExceptionHierarchy) {
    auto script = CB::add("hello", 2);
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
TEST_F(BasicOperatorsTest, InvalidOperatorExceptionHierarchy) {
    auto script = CB::op("unknown_operator").arg(1).arg(2);
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
TEST_F(BasicOperatorsTest, StdExceptionHierarchy) {
    auto script = CB::add(1, true);
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