#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class LogicalOperatorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test && operator with all true values
TEST_F(LogicalOperatorsTest, AndOperatorAllTrue) {
    // Builder Pattern makes complex logical expressions readable
    auto script = CB::op("&&")
        .arg(true)
        .arg(1)
        .arg("hello")
        .arg(CB::array({1}));
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test && operator with false value (short-circuit)
TEST_F(LogicalOperatorsTest, AndOperatorWithFalse) {
    auto script = CB::op("&&")
        .arg(true)
        .arg(false)
        .arg("hello");  // Should short-circuit, this won't be evaluated
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with zero (falsy)
TEST_F(LogicalOperatorsTest, AndOperatorWithZero) {
    auto script = CB::op("&&")
        .arg(true)
        .arg(0)        // Falsy value
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty string (falsy)
TEST_F(LogicalOperatorsTest, AndOperatorWithEmptyString) {
    auto script = CB::op("&&")
        .arg(true)
        .arg("")       // Falsy value
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with null (falsy)
TEST_F(LogicalOperatorsTest, AndOperatorWithNull) {
    auto script = CB::op("&&")
        .arg(true)
        .arg(json(nullptr))  // Falsy value
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty array (falsy)
TEST_F(LogicalOperatorsTest, AndOperatorWithEmptyArray) {
    auto script = CB::op("&&")
        .arg(true)
        .arg(CB::empty_array())  // Falsy value
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with empty object (falsy)
TEST_F(LogicalOperatorsTest, AndOperatorWithEmptyObject) {
    auto script = CB::op("&&")
        .arg(true)
        .arg(json::object())  // Falsy value
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test && operator with single argument
TEST_F(LogicalOperatorsTest, AndOperatorSingleArg) {
    auto script1 = CB::op("&&").arg(true);
    EXPECT_EQ(computo::execute(script1, input_data), true);
    
    auto script2 = CB::op("&&").arg(false);
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// Test && operator with expressions
TEST_F(LogicalOperatorsTest, AndOperatorWithExpressions) {
    // Builder Pattern makes complex logical combinations clear!
    auto script = CB::op("&&")
        .arg(CB::greater_than(5, 3))
        .arg(CB::less_than(2, 4))
        .arg(CB::equal("hello", "hello"));
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test && operator short-circuit with expensive expressions
TEST_F(LogicalOperatorsTest, AndOperatorShortCircuit) {
    // False short-circuits, so division by zero is never evaluated
    auto script = CB::op("&&")
        .arg(false)
        .arg(CB::divide(1, 0));  // This would throw if evaluated
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test || operator with all false values
TEST_F(LogicalOperatorsTest, OrOperatorAllFalse) {
    auto script = CB::op("||")
        .arg(false)
        .arg(0)
        .arg("")
        .arg(json(nullptr))
        .arg(CB::empty_array())
        .arg(json::object());
        
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test || operator with true value (short-circuit)
TEST_F(LogicalOperatorsTest, OrOperatorWithTrue) {
    auto script = CB::op("||")
        .arg(false)
        .arg(true)     // Short-circuits here
        .arg("hello");
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test || operator with truthy value
TEST_F(LogicalOperatorsTest, OrOperatorWithTruthy) {
    auto script = CB::op("||")
        .arg(false)
        .arg(0)
        .arg("hello");  // First truthy value
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test || operator with single argument
TEST_F(LogicalOperatorsTest, OrOperatorSingleArg) {
    auto script1 = CB::op("||").arg(true);
    EXPECT_EQ(computo::execute(script1, input_data), true);
    
    auto script2 = CB::op("||").arg(false);
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// Test || operator with expressions
TEST_F(LogicalOperatorsTest, OrOperatorWithExpressions) {
    // Even though first condition is false, second is true
    auto script = CB::op("||")
        .arg(CB::greater_than(3, 5))  // false
        .arg(CB::less_than(2, 4))     // true - short-circuits here
        .arg(CB::equal("hello", "goodbye"));
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test || operator short-circuit with expensive expressions
TEST_F(LogicalOperatorsTest, OrOperatorShortCircuit) {
    // True short-circuits, so division by zero is never evaluated
    auto script = CB::op("||")
        .arg(true)
        .arg(CB::divide(1, 0));  // This would throw if evaluated
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test not operator
TEST_F(LogicalOperatorsTest, NotOperator) {
    auto script1 = CB::op("not").arg(true);
    EXPECT_EQ(computo::execute(script1, input_data), false);
    
    auto script2 = CB::op("not").arg(false);
    EXPECT_EQ(computo::execute(script2, input_data), true);
    
    auto script3 = CB::op("not").arg(0);
    EXPECT_EQ(computo::execute(script3, input_data), true);
    
    auto script4 = CB::op("not").arg("hello");
    EXPECT_EQ(computo::execute(script4, input_data), false);
}

// Test complex logical combinations
TEST_F(LogicalOperatorsTest, ComplexLogicalCombinations) {
    // (true && (false || true)) && !(false)
    auto script = CB::op("&&")
        .arg(CB::op("&&")
            .arg(true)
            .arg(CB::op("||")
                .arg(false)
                .arg(true)
            )
        )
        .arg(CB::op("not").arg(false));
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test logical operators in conditional context
TEST_F(LogicalOperatorsTest, LogicalInConditional) {
    // if (true && false) then "yes" else "no"
    auto script = CB::if_then_else(
        CB::op("&&").arg(true).arg(false),
        "yes",
        "no"
    );
    
    EXPECT_EQ(computo::execute(script, input_data), "no");
}

// Test logical operators with data access
TEST_F(LogicalOperatorsTest, LogicalWithDataAccess) {
    json test_input = json{
        {"user", json{{"active", true}, {"verified", false}, {"age", 25}}}
    };
    
    // Check if user is active AND (verified OR age > 18)
    auto script = CB::op("&&")
        .arg(CB::get(CB::input(), "/user/active"))
        .arg(CB::op("||")
            .arg(CB::get(CB::input(), "/user/verified"))
            .arg(CB::greater_than(CB::get(CB::input(), "/user/age"), 18))
        );
        
    EXPECT_EQ(computo::execute(script, test_input), true);
}

// Test logical operators with array operations
TEST_F(LogicalOperatorsTest, LogicalWithArrays) {
    // Check if any number in array is greater than 10 AND all are positive
    auto script = CB::op("&&")
        .arg(CB::some(
            CB::array({5, 15, 8, 3}),
            CB::lambda("x", CB::greater_than(CB::var("x"), 10))
        ))
        .arg(CB::every(
            CB::array({5, 15, 8, 3}),
            CB::lambda("x", CB::greater_than(CB::var("x"), 0))
        ));
        
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test nested logical expressions
TEST_F(LogicalOperatorsTest, NestedLogicalExpressions) {
    // Complex business logic: 
    // (status == "active" || status == "pending") && 
    // (priority > 5 || category == "urgent")
    json test_input = json{
        {"status", "pending"},
        {"priority", 3},
        {"category", "urgent"}
    };
    
    auto script = CB::op("&&")
        .arg(CB::op("||")
            .arg(CB::equal(CB::get(CB::input(), "/status"), "active"))
            .arg(CB::equal(CB::get(CB::input(), "/status"), "pending"))
        )
        .arg(CB::op("||")
            .arg(CB::greater_than(CB::get(CB::input(), "/priority"), 5))
            .arg(CB::equal(CB::get(CB::input(), "/category"), "urgent"))
        );
        
    EXPECT_EQ(computo::execute(script, test_input), true);
}