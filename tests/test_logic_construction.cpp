#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class LogicConstructionTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test if operator with boolean conditions
TEST_F(LogicConstructionTest, IfOperatorBoolean) {
    // Builder Pattern makes conditional logic crystal clear!
    auto script1 = CB::if_then_else(true, "yes", "no");
    EXPECT_EQ(computo::execute(script1, input_data), "yes");
    
    auto script2 = CB::if_then_else(false, "yes", "no");
    EXPECT_EQ(computo::execute(script2, input_data), "no");
}

// Test if operator with numeric conditions
TEST_F(LogicConstructionTest, IfOperatorNumeric) {
    // Non-zero is true
    auto script1 = CB::if_then_else(1, "truthy", "falsy");
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // Zero is false
    auto script2 = CB::if_then_else(0, "truthy", "falsy");
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
    
    // Non-zero float is true
    auto script3 = CB::if_then_else(3.14, "truthy", "falsy");
    EXPECT_EQ(computo::execute(script3, input_data), "truthy");
    
    // Zero float is false
    auto script4 = CB::if_then_else(0.0, "truthy", "falsy");
    EXPECT_EQ(computo::execute(script4, input_data), "falsy");
}

// Test if operator with string conditions
TEST_F(LogicConstructionTest, IfOperatorString) {
    // Non-empty string is true
    auto script1 = CB::if_then_else("hello", "truthy", "falsy");
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // Empty string is false
    auto script2 = CB::if_then_else("", "truthy", "falsy");
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with null condition
TEST_F(LogicConstructionTest, IfOperatorNull) {
    // Null is false
    auto script = CB::if_then_else(json(nullptr), "truthy", "falsy");
    EXPECT_EQ(computo::execute(script, input_data), "falsy");
}

// Test if operator with array conditions
TEST_F(LogicConstructionTest, IfOperatorArray) {
    // Non-empty array is true - Builder Pattern makes this so much cleaner!
    auto script1 = CB::if_then_else(CB::array({1, 2}), "truthy", "falsy");
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // Empty array is false
    auto script2 = CB::if_then_else(CB::empty_array(), "truthy", "falsy");
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with object conditions
TEST_F(LogicConstructionTest, IfOperatorObject) {
    // Non-empty object is true
    auto script1 = CB::if_then_else(json{{"key", "value"}}, "truthy", "falsy");
    EXPECT_EQ(computo::execute(script1, input_data), "truthy");
    
    // Empty object is false
    auto script2 = CB::if_then_else(json::object(), "truthy", "falsy");
    EXPECT_EQ(computo::execute(script2, input_data), "falsy");
}

// Test if operator with expression evaluation
TEST_F(LogicConstructionTest, IfOperatorWithExpressions) {
    // Builder Pattern makes complex conditional expressions readable
    auto script = CB::if_then_else(
        CB::add(1, 1),    // condition evaluates to 2 (truthy)
        CB::add(10, 5),   // then branch
        CB::add(20, 5)    // else branch
    );
    EXPECT_EQ(computo::execute(script, input_data), 15);
}

// Test obj operator basic functionality
TEST_F(LogicConstructionTest, ObjOperatorBasic) {
    // Builder Pattern makes object construction beautiful!
    auto script = CB::obj()
        .add_field("name", "John")
        .add_field("age", 30);
        
    json expected = json{{"name", "John"}, {"age", 30}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator with expression values
TEST_F(LogicConstructionTest, ObjOperatorWithExpressions) {
    // Object with computed values - Builder Pattern shines here!
    auto script = CB::obj()
        .add_field("sum", CB::add(2, 3))
        .add_field("input", CB::input());
        
    json expected = json{{"sum", 5}, {"input", input_data}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test obj operator empty object
TEST_F(LogicConstructionTest, ObjOperatorEmpty) {
    auto script = CB::obj();  // No fields added
    json expected = json::object();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object basic functionality
TEST_F(LogicConstructionTest, ArrayObjectBasic) {
    // Builder Pattern makes mixed-type arrays clean
    auto script = CB::array({1, "hello", true});
    json expected = json::array({1, "hello", true});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object with expressions
TEST_F(LogicConstructionTest, ArrayObjectWithExpressions) {
    // Array with computed elements - much cleaner than manual JSON construction!
    auto script = CB::array({
        CB::add(1, 2),
        CB::input(),
        CB::obj().add_field("key", "value")
    });
    
    json expected = json::array({3, input_data, json{{"key", "value"}}});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test array object empty array  
TEST_F(LogicConstructionTest, ArrayObjectEmpty) {
    auto script = CB::empty_array();
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test complex combination: if + obj + array
TEST_F(LogicConstructionTest, IfObjArrayCombination) {
    // Complex nested structure - Builder Pattern makes intent clear
    auto script = CB::if_then_else(
        true,
        CB::obj().add_field("list", CB::array({1, 2, 3})),
        CB::empty_array()
    );
    
    json expected = json{{"list", json::array({1, 2, 3})}};
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test nested if statements
TEST_F(LogicConstructionTest, NestedIfStatements) {
    // Nested conditionals - Builder Pattern maintains readability even with nesting
    auto script = CB::if_then_else(
        true,
        CB::if_then_else(false, "inner-true", "inner-false"),
        "outer-false"
    );
    EXPECT_EQ(computo::execute(script, input_data), "inner-false");
}