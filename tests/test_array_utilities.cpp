#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class ArrayUtilitiesTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test find operator - finds first matching element
TEST_F(ArrayUtilitiesTest, FindOperatorBasic) {
    // Builder Pattern makes complex predicate logic readable!
    auto script = CB::find(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3))
    );
    
    json expected = 4; // First element > 3
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test find operator - returns null when no match
TEST_F(ArrayUtilitiesTest, FindOperatorNoMatch) {
    auto script = CB::find(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 10))
    );
    
    json expected = nullptr; // No element > 10
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test find operator on empty array
TEST_F(ArrayUtilitiesTest, FindOperatorEmpty) {
    auto script = CB::find(
        CB::empty_array(),
        CB::lambda("x", CB::var("x"))
    );
    
    json expected = nullptr;
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test some operator - returns true when any match
TEST_F(ArrayUtilitiesTest, SomeOperatorTrue) {
    auto script = CB::some(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test some operator - returns false when no match
TEST_F(ArrayUtilitiesTest, SomeOperatorFalse) {
    auto script = CB::some(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 10))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test some operator on empty array
TEST_F(ArrayUtilitiesTest, SomeOperatorEmpty) {
    auto script = CB::some(
        CB::empty_array(),
        CB::lambda("x", CB::var("x"))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test every operator - returns true when all match
TEST_F(ArrayUtilitiesTest, EveryOperatorTrue) {
    auto script = CB::every(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 0))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test every operator - returns false when not all match
TEST_F(ArrayUtilitiesTest, EveryOperatorFalse) {
    auto script = CB::every(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), false);
}

// Test every operator on empty array (vacuous truth)
TEST_F(ArrayUtilitiesTest, EveryOperatorEmpty) {
    auto script = CB::every(
        CB::empty_array(),
        CB::lambda("x", CB::greater_than(CB::var("x"), 10))
    );
    
    EXPECT_EQ(computo::execute(script, input_data), true); // Vacuous truth
}

// Test partition operator (splits into [truthy, falsy] based on predicate)
TEST_F(ArrayUtilitiesTest, PartitionOperatorBasic) {
    auto script = CB::partition(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3))
    );
    
    json expected = json::array({
        json::array({4, 5}),    // truthy items (> 3)
        json::array({1, 2, 3})  // falsy items (<= 3)
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test partition operator with all truthy
TEST_F(ArrayUtilitiesTest, PartitionOperatorAllTruthy) {
    auto script = CB::partition(
        CB::array({1, 2, 3}),
        CB::lambda("x", true)  // Always true
    );
    
    json expected = json::array({
        json::array({1, 2, 3}),  // all truthy
        json::array()            // none falsy
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test count operator
TEST_F(ArrayUtilitiesTest, CountOperator) {
    auto script = CB::count(CB::array({1, 2, 3, 4, 5}));
    EXPECT_EQ(computo::execute(script, input_data), 5);
    
    auto empty_script = CB::count(CB::empty_array());
    EXPECT_EQ(computo::execute(empty_script, input_data), 0);
}

// Test flatmap operator (commented out - not implemented in core engine yet)
TEST_F(ArrayUtilitiesTest, FlatmapOperator) {
    // Note: flatmap operator not implemented in core engine yet
    // When implemented, it should duplicate each element into a sub-array, then flatten
    // auto script = CB::flatmap(
    //     CB::array({1, 2, 3}),
    //     CB::lambda("x", CB::array({CB::var("x"), CB::var("x")}))
    // );
    // 
    // json expected = json::array({1, 1, 2, 2, 3, 3});
    // EXPECT_EQ(computo::execute(script, input_data), expected);
    
    // For now, just test that the Builder creates the correct JSON structure
    auto script = CB::flatmap(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::array({CB::var("x"), CB::var("x")}))
    );
    
    json built = script.build();
    EXPECT_TRUE(built.is_array());
    EXPECT_EQ(built[0], "flatmap");
}

// Test car operator (first element)
TEST_F(ArrayUtilitiesTest, CarOperator) {
    auto script = CB::car(CB::array({1, 2, 3, 4, 5}));
    EXPECT_EQ(computo::execute(script, input_data), 1);
    
    auto single_script = CB::car(CB::array({42}));
    EXPECT_EQ(computo::execute(single_script, input_data), 42);
}

// Test cdr operator (rest of elements)
TEST_F(ArrayUtilitiesTest, CdrOperator) {
    auto script = CB::cdr(CB::array({1, 2, 3, 4, 5}));
    json expected = json::array({2, 3, 4, 5});
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    auto single_script = CB::cdr(CB::array({42}));
    json expected_empty = json::array();
    EXPECT_EQ(computo::execute(single_script, input_data), expected_empty);
}

// Test cons operator (prepend element)
TEST_F(ArrayUtilitiesTest, ConsOperator) {
    auto script = CB::cons(0, CB::array({1, 2, 3}));
    json expected = json::array({0, 1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), expected);
    
    auto empty_script = CB::cons(42, CB::empty_array());
    json expected_single = json::array({42});
    EXPECT_EQ(computo::execute(empty_script, input_data), expected_single);
}

// Test append operator (concatenate two arrays)
TEST_F(ArrayUtilitiesTest, AppendOperator) {
    auto script = CB::append(CB::array({1, 2}), CB::array({3, 4}));
    json expected = json::array({1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test chunk operator (similar to partition but different name)
TEST_F(ArrayUtilitiesTest, ChunkOperator) {
    auto script = CB::chunk(CB::array({1, 2, 3, 4, 5, 6, 7, 8}), 3);
    
    json expected = json::array({
        json::array({1, 2, 3}),
        json::array({4, 5, 6}),
        json::array({7, 8})
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test complex functional composition
TEST_F(ArrayUtilitiesTest, ComplexFunctionalComposition) {
    // Find the first number greater than 5, if any exist
    auto script = CB::find(
        CB::filter(
            CB::array({1, 3, 5, 6, 7, 8, 9}),
            CB::lambda("x", CB::greater_than(CB::var("x"), 5))  // x > 5
        ),
        CB::lambda("x", CB::var("x"))  // Just return the element itself
    );
    
    // Should find 6 (first number > 5 after filtering)
    json expected = 6;
    EXPECT_EQ(computo::execute(script, input_data), expected);
}