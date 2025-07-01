#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class NaryOperatorsTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// === Addition (n-ary) Tests ===

TEST_F(NaryOperatorsTest, AdditionNaryBasic) {
    // Test 3 arguments - n-ary operations require raw JSON syntax for now
    json script = json::array({"+", 1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), 6);
    
    // Test 4 arguments
    json script2 = json::array({"+", 10, 20, 30, 40});
    EXPECT_EQ(computo::execute(script2, input_data), 100);
    
    // Test 5 arguments with mixed integers
    json script3 = json::array({"+", 1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script3, input_data), 15);
}

TEST_F(NaryOperatorsTest, AdditionNaryFloats) {
    // Test with floating point numbers
    json script = json::array({"+", 1.5, 2.5, 3.0});
    EXPECT_EQ(computo::execute(script, input_data), 7.0);
    
    // Test mixed integers and floats
    json script2 = json::array({"+", 1, 2.5, 3});
    EXPECT_EQ(computo::execute(script2, input_data), 6.5);
}

TEST_F(NaryOperatorsTest, AdditionNarySingleArgument) {
    // Single argument should return that argument
    json script = json::array({"+", 42});
    EXPECT_EQ(computo::execute(script, input_data), 42);
    
    json script2 = json::array({"+", 3.14});
    EXPECT_EQ(computo::execute(script2, input_data), 3.14);
}

TEST_F(NaryOperatorsTest, AdditionNaryLargeNumber) {
    // Test with many arguments
    json script = json::array({"+", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}); // 10 ones
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

TEST_F(NaryOperatorsTest, AdditionNaryWithExpressions) {
    // Test with expressions as arguments
    json script = json::array({
        "+", 
        json::array({"+", 1, 2}),  // 3
        json::array({"*", 2, 3}),  // 6
        5                          // 5
    });
    EXPECT_EQ(computo::execute(script, input_data), 14);
}

// === Multiplication (n-ary) Tests ===

TEST_F(NaryOperatorsTest, MultiplicationNaryBasic) {
    // Test 3 arguments
    json script = json::array({"*", 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), 24);
    
    // Test 4 arguments
    json script2 = json::array({"*", 1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script2, input_data), 24);
}

TEST_F(NaryOperatorsTest, MultiplicationNaryFloats) {
    // Test with floating point numbers
    json script = json::array({"*", 2.0, 3.0, 1.5});
    EXPECT_EQ(computo::execute(script, input_data), 9.0);
    
    // Test mixed integers and floats
    json script2 = json::array({"*", 2, 2.5, 2});
    EXPECT_EQ(computo::execute(script2, input_data), 10.0);
}

TEST_F(NaryOperatorsTest, MultiplicationNarySingleArgument) {
    // Single argument should return that argument
    json script = json::array({"*", 7});
    EXPECT_EQ(computo::execute(script, input_data), 7);
    
    json script2 = json::array({"*", 2.5});
    EXPECT_EQ(computo::execute(script2, input_data), 2.5);
}

TEST_F(NaryOperatorsTest, MultiplicationNaryWithZero) {
    // Test with zero (should result in zero)
    json script = json::array({"*", 5, 0, 10});
    EXPECT_EQ(computo::execute(script, input_data), 0);
}

TEST_F(NaryOperatorsTest, MultiplicationNaryWithExpressions) {
    // Test with expressions as arguments
    json script = json::array({
        "*", 
        json::array({"+", 1, 1}),  // 2
        json::array({"+", 2, 1}),  // 3
        json::array({"+", 1, 3})   // 4
    });
    EXPECT_EQ(computo::execute(script, input_data), 24);
}

// === Chained Comparison Tests ===

TEST_F(NaryOperatorsTest, ChainedLessThanBasic) {
    // Test a < b < c (true case) - chained comparisons
    json script = json::array({"<", 1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a < b < c (false case)
    json script2 = json::array({"<", 1, 3, 2});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain
    json script3 = json::array({"<", 1, 2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(NaryOperatorsTest, ChainedGreaterThanBasic) {
    // Test a > b > c (true case)
    json script = json::array({">", 5, 3, 1});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a > b > c (false case)
    json script2 = json::array({">", 5, 1, 3});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain
    json script3 = json::array({">", 10, 8, 6, 4, 2});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(NaryOperatorsTest, ChainedLessEqualBasic) {
    // Test a <= b <= c (true case with equality)
    json script = json::array({"<=", 1, 2, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a <= b <= c (true case strictly increasing)
    json script2 = json::array({"<=", 1, 2, 3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
    
    // Test a <= b <= c (false case)
    json script3 = json::array({"<=", 3, 2, 4});
    EXPECT_EQ(computo::execute(script3, input_data), false);
}

TEST_F(NaryOperatorsTest, ChainedGreaterEqualBasic) {
    // Test a >= b >= c (true case with equality)
    json script = json::array({">=", 3, 3, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test a >= b >= c (true case strictly decreasing)
    json script2 = json::array({">=", 5, 3, 1});
    EXPECT_EQ(computo::execute(script2, input_data), true);
    
    // Test a >= b >= c (false case)
    json script3 = json::array({">=", 2, 4, 1});
    EXPECT_EQ(computo::execute(script3, input_data), false);
}

TEST_F(NaryOperatorsTest, ChainedComparisonFloats) {
    // Test with floating point numbers
    json script = json::array({"<", 1.1, 2.2, 3.3});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({">", 5.5, 4.4, 3.3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

TEST_F(NaryOperatorsTest, ChainedComparisonWithExpressions) {
    // Test with expressions
    json script = json::array({
        "<",
        json::array({"+", 1, 1}),  // 2
        json::array({"*", 2, 2}),  // 4
        json::array({"+", 3, 3})   // 6
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

TEST_F(NaryOperatorsTest, ChainedComparisonMinimal) {
    // Test with just 2 arguments (should work like binary)
    json script = json::array({"<", 1, 2});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({">", 5, 3});
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// === N-ary Equality Tests ===

TEST_F(NaryOperatorsTest, EqualityNaryBasic) {
    // Test all equal (true case)
    json script = json::array({"==", 5, 5, 5});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test one different (false case)
    json script2 = json::array({"==", 5, 5, 6});
    EXPECT_EQ(computo::execute(script2, input_data), false);
    
    // Test longer chain of equal values
    json script3 = json::array({"==", 42, 42, 42, 42, 42});
    EXPECT_EQ(computo::execute(script3, input_data), true);
}

TEST_F(NaryOperatorsTest, EqualityNaryStrings) {
    // Test with strings
    json script = json::array({"==", "hello", "hello", "hello"});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({"==", "hello", "hello", "world"});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

TEST_F(NaryOperatorsTest, EqualityNaryMixedTypes) {
    // Test with same value, different types (in JSON, 5 and 5.0 are actually equal)
    json script = json::array({"==", 5, 5.0});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    // Test different types that are actually different
    json script2 = json::array({"==", 5, "5"});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

TEST_F(NaryOperatorsTest, EqualityNaryWithExpressions) {
    // Test with expressions that evaluate to same value
    json script = json::array({
        "==",
        json::array({"+", 2, 3}),  // 5
        json::array({"*", 1, 5}),  // 5
        json::array({"-", 7, 2})   // 5
    });
    EXPECT_EQ(computo::execute(script, input_data), true);
}

TEST_F(NaryOperatorsTest, EqualityNaryMinimal) {
    // Test with just 2 arguments (should work like binary)
    json script = json::array({"==", 5, 5});
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    json script2 = json::array({"==", 5, 6});
    EXPECT_EQ(computo::execute(script2, input_data), false);
}

// === Error Cases for N-ary Operators ===

TEST_F(NaryOperatorsTest, AdditionNaryNoArguments) {
    // Should still throw error for no arguments
    json script = json::array({"+"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, MultiplicationNaryNoArguments) {
    // Should still throw error for no arguments
    json script = json::array({"*"});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, AdditionNaryNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"+", 1, "hello", 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, MultiplicationNaryNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"*", 2, true, 4});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, ChainedComparisonTooFewArgs) {
    // Should throw error for less than 2 arguments
    json script = json::array({"<", 5});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    json script2 = json::array({">", 5});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, ChainedComparisonNonNumeric) {
    // Should throw error for non-numeric arguments
    json script = json::array({"<", 1, "hello", 3});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

TEST_F(NaryOperatorsTest, EqualityNaryTooFewArgs) {
    // Should throw error for less than 2 arguments
    json script = json::array({"==", 5});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
}

// === Integration Tests for N-ary Operators ===

TEST_F(NaryOperatorsTest, NaryOperatorsWithLet) {
    // Test n-ary operators within let expressions using Builder Pattern for let
    auto script = CB::let(
        {
            {"a", 1},
            {"b", 2},
            {"c", 3},
            {"d", 4}
        },
        json::array({
            "+",
            CB::var("a"),
            CB::var("b"),
            CB::var("c"),
            CB::var("d")
        })
    );
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

TEST_F(NaryOperatorsTest, NaryOperatorsInArray) {
    // Test n-ary operators in object construction
    auto script = CB::obj()
        .add_field("sum", json::array({"+", 1, 2, 3, 4}))
        .add_field("product", json::array({"*", 2, 3, 4}))
        .add_field("chain_check", json::array({"<", 1, 2, 3, 4}));
    
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result["sum"], 10);
    EXPECT_EQ(result["product"], 24);
    EXPECT_EQ(result["chain_check"], true);
}

TEST_F(NaryOperatorsTest, NaryOperatorsWithMap) {
    // Test n-ary operators with map - mixing Builder Pattern and raw JSON
    auto script = CB::map(
        CB::array({
            CB::array({1, 2, 3}),
            CB::array({2, 3, 4}),
            CB::array({5, 6, 7})
        }),
        CB::lambda("arr", 
            json::array({
                "+",
                CB::get(CB::var("arr"), "/0"),
                CB::get(CB::var("arr"), "/1"),
                CB::get(CB::var("arr"), "/2")
            })
        )
    );
    
    json expected = json::array({6, 9, 18});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

TEST_F(NaryOperatorsTest, NaryOperatorsPerformanceStressTest) {
    // Test with many arguments to ensure no significant performance degradation
    json args = json::array({"+"});
    for (int i = 1; i <= 100; ++i) {
        args.push_back(i);
    }
    
    json result = computo::execute(args, input_data);
    EXPECT_EQ(result, 5050); // Sum of 1 to 100
}

TEST_F(NaryOperatorsTest, ChainedComparisonComplexUsage) {
    // Test chained comparison in realistic scenario
    auto script = CB::let(
        {{"score", 85}},
        CB::if_then_else(
            json::array({"<", 80, CB::var("score"), 90}),  // 80 < score < 90
            "B grade",
            "Other grade"
        )
    );
    
    EXPECT_EQ(computo::execute(script, input_data), "B grade");
}

// === Edge Cases and Advanced Scenarios ===

TEST_F(NaryOperatorsTest, MixedNaryAndBinaryOperators) {
    // Test mixing n-ary and binary operators
    json script = json::array({
        "+",
        json::array({"*", 2, 3}),      // 6 (binary multiplication)
        json::array({"*", 1, 2, 3}),   // 6 (n-ary multiplication)
        json::array({"+", 1, 1})       // 2 (binary addition)
    });
    EXPECT_EQ(computo::execute(script, input_data), 14);
}

TEST_F(NaryOperatorsTest, NestedNaryOperators) {
    // Test deeply nested n-ary operators
    json script = json::array({
        "*",
        json::array({"+", 1, 1, 1}),           // 3
        json::array({"+", 2, 2}),               // 4
        json::array({"*", 1, 2, 3})             // 6
    });
    EXPECT_EQ(computo::execute(script, input_data), 72); // 3 * 4 * 6
}

TEST_F(NaryOperatorsTest, ComplexNaryBusinessLogic) {
    // Complex business logic using n-ary operators
    json test_input = json{
        {"scores", json::array({85, 92, 78, 88, 95})},
        {"thresholds", json{{"min", 80}, {"max", 90}}}
    };
    
    // Check if all scores are within acceptable range (80 <= score <= 90)
    auto script = CB::every(
        CB::get(CB::input(), "/scores"),
        CB::lambda("score",
            CB::op("&&")
                .arg(json::array({">=", CB::var("score"), CB::get(CB::input(), "/thresholds/min")}))
                .arg(json::array({"<=", CB::var("score"), CB::get(CB::input(), "/thresholds/max")}))
        )
    );
    
    // Not all scores are in range (92 and 95 are above 90)
    EXPECT_EQ(computo::execute(script, test_input), false);
}