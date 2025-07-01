#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class IterationLambdasTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test map operator basic functionality
TEST_F(IterationLambdasTest, MapOperatorBasic) {
    // WOW! Compare this Builder Pattern syntax:
    auto script = CB::map(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::add(CB::var("x"), 1))
    );
    
    // vs the original horrific syntax:
    // json::array({"map", json{{"array", json::array({1, 2, 3})}}, 
    //   json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})})
    
    json expected = json::array({2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with string transformation
TEST_F(IterationLambdasTest, MapOperatorStringTransform) {
    // Identity transformation - Builder Pattern makes the intent crystal clear
    auto script = CB::map(
        CB::array({"hello", "world"}),
        CB::lambda("s", CB::var("s"))
    );
    
    json expected = json::array({"hello", "world"});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with complex lambda
TEST_F(IterationLambdasTest, MapOperatorComplexLambda) {
    json array_data = json{{"array", json::array({
        json{{"id", 1}, {"value", 10}},
        json{{"id", 2}, {"value", 20}},
        json{{"id", 3}, {"value", 30}}
    })}};
    
    // Complex lambda transformation - Builder Pattern makes this READABLE!
    auto script = CB::map(
        array_data,
        CB::lambda("item", 
            CB::obj()
                .add_field("id", CB::get(CB::var("item"), "/id"))
                .add_field("doubled", CB::add(
                    CB::get(CB::var("item"), "/value"),
                    CB::get(CB::var("item"), "/value")
                ))
        )
    );
    
    json expected = json::array({
        json{{"id", 1}, {"doubled", 20}},
        json{{"id", 2}, {"doubled", 40}},
        json{{"id", 3}, {"doubled", 60}}
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test map operator with empty array
TEST_F(IterationLambdasTest, MapOperatorEmpty) {
    auto script = CB::map(
        CB::empty_array(),
        CB::lambda("x", CB::add(CB::var("x"), 1))
    );
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator basic functionality
TEST_F(IterationLambdasTest, FilterOperatorBasic) {
    // Filter using truthiness - Builder Pattern makes the logic clear
    auto script = CB::filter(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::var("x"))  // All numbers are truthy
    );
    
    json expected = json::array({1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with boolean condition
TEST_F(IterationLambdasTest, FilterOperatorBoolean) {
    auto script = CB::filter(
        CB::array({true, false, true, false}),
        CB::lambda("x", CB::var("x"))  // Filter for truthy values
    );
    
    json expected = json::array({true, true});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with string condition
TEST_F(IterationLambdasTest, FilterOperatorString) {
    // Filter out empty strings - Builder Pattern makes intent obvious
    auto script = CB::filter(
        CB::array({"hello", "", "world", ""}),
        CB::lambda("s", CB::var("s"))  // Non-empty strings are truthy
    );
    
    json expected = json::array({"hello", "world"});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with object condition
TEST_F(IterationLambdasTest, FilterOperatorObject) {
    json array_data = json{{"array", json::array({
        json{{"active", true}, {"name", "Alice"}},
        json{{"active", false}, {"name", "Bob"}},
        json{{"active", true}, {"name", "Charlie"}}
    })}};
    
    // Filter by object property - Builder Pattern makes this elegant
    auto script = CB::filter(
        array_data,
        CB::lambda("user", CB::get(CB::var("user"), "/active"))
    );
    
    json expected = json::array({
        json{{"active", true}, {"name", "Alice"}},
        json{{"active", true}, {"name", "Charlie"}}
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test filter operator with empty array
TEST_F(IterationLambdasTest, FilterOperatorEmpty) {
    auto script = CB::filter(
        CB::empty_array(),
        CB::lambda("x", CB::var("x"))
    );
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test nested map and filter
TEST_F(IterationLambdasTest, NestedMapFilter) {
    // Chained operations - Builder Pattern makes the pipeline clear!
    auto script = CB::map(
        CB::filter(
            CB::array({1, 2, 3, 4, 5}),
            CB::lambda("x", CB::var("x"))  // All are truthy
        ),
        CB::lambda("x", CB::add(CB::var("x"), 10))
    );
    
    json expected = json::array({11, 12, 13, 14, 15});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test complex functional pipeline
TEST_F(IterationLambdasTest, ComplexFunctionalPipeline) {
    // Real-world functional programming pattern - Builder Pattern shines here!
    auto script = CB::map(
        CB::filter(
            CB::array({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}),
            CB::lambda("x", CB::greater_than(CB::var("x"), 5))  // x > 5
        ),
        CB::lambda("x", CB::multiply(CB::var("x"), 2))  // double the values
    );
    
    // Filter numbers > 5, then double them: [6,7,8,9,10] -> [12,14,16,18,20]
    json expected = json::array({12, 14, 16, 18, 20});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}