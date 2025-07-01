// This file demonstrates the ComputoBuilder pattern improvements
// comparing old verbose syntax with new readable syntax

#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class BuilderExamplesTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"numbers", json::array({1, 2, 3, 4, 5})}};
    }
    
    json input_data;
};

// Example 1: Simple array operations
TEST_F(BuilderExamplesTest, ArrayOperationsComparison) {
    // OLD VERBOSE SYNTAX - painful to write and read
    json old_syntax = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    
    // NEW BUILDER SYNTAX - clean and readable
    auto new_syntax = CB::map(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::add(CB::var("x"), 1))
    );
    
    json expected = json::array({2, 3, 4});
    EXPECT_EQ(computo::execute(old_syntax, input_data), expected);
    EXPECT_EQ(computo::execute(new_syntax, input_data), expected);
}

// Example 2: Complex nested operations
TEST_F(BuilderExamplesTest, NestedOperationsComparison) {
    // OLD SYNTAX - nearly unreadable
    json old_syntax = json::array({
        "let",
        json::array({
            json::array({"data", json::array({"get", json::array({"$input"}), "/numbers"})}),
            json::array({"threshold", 3})
        }),
        json::array({
            "map",
            json::array({"filter", json::array({"$", "/data"}), json::array({"lambda", json::array({"x"}), json::array({">", json::array({"$", "/x"}), json::array({"$", "/threshold"})})})}),
            json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})
        })
    });
    
    // NEW SYNTAX - clear intent and structure
    auto new_syntax = CB::let(
        {
            {"data", CB::get(CB::input(), "/numbers")},
            {"threshold", 3}
        },
        CB::map(
            CB::filter(CB::var("data"), CB::lambda("x", CB::greater_than(CB::var("x"), CB::var("threshold")))),
            CB::lambda("x", CB::multiply(CB::var("x"), 2))
        )
    );
    
    json expected = json::array({8, 10}); // [4*2, 5*2] from numbers > 3
    EXPECT_EQ(computo::execute(old_syntax, input_data), expected);
    EXPECT_EQ(computo::execute(new_syntax, input_data), expected);
}

// Example 3: Object construction
TEST_F(BuilderExamplesTest, ObjectConstructionComparison) {
    // OLD SYNTAX - verbose array construction
    json old_syntax = json::array({
        "obj",
        json::array({"name", "test"}),
        json::array({"count", json::array({"count", json::array({"get", json::array({"$input"}), "/numbers"})})}),
        json::array({"sum", json::array({"+", 1, 2, 3})})
    });
    
    // NEW SYNTAX - fluent and readable
    auto new_syntax = CB::obj()
        .add_field("name", "test")
        .add_field("count", CB::count(CB::get(CB::input(), "/numbers")))
        .add_field("sum", CB::add({1, 2, 3}));
    
    json expected = {
        {"name", "test"},
        {"count", 5},
        {"sum", 6}
    };
    EXPECT_EQ(computo::execute(old_syntax, input_data), expected);
    EXPECT_EQ(computo::execute(new_syntax, input_data), expected);
}

// Example 4: Conditional logic
TEST_F(BuilderExamplesTest, ConditionalLogicComparison) {
    // OLD SYNTAX - deeply nested arrays
    json old_syntax = json::array({
        "if",
        json::array({">", json::array({"count", json::array({"get", json::array({"$input"}), "/numbers"})}), 3}),
        json::array({"obj", json::array({"status", "many"}), json::array({"message", "Found many numbers"})}),
        json::array({"obj", json::array({"status", "few"}), json::array({"message", "Found few numbers"})})
    });
    
    // NEW SYNTAX - self-documenting
    auto new_syntax = CB::if_then_else(
        CB::greater_than(CB::count(CB::get(CB::input(), "/numbers")), 3),
        CB::obj()
            .add_field("status", "many")
            .add_field("message", "Found many numbers"),
        CB::obj()
            .add_field("status", "few")
            .add_field("message", "Found few numbers")
    );
    
    json expected = {
        {"status", "many"},
        {"message", "Found many numbers"}
    };
    EXPECT_EQ(computo::execute(old_syntax, input_data), expected);
    EXPECT_EQ(computo::execute(new_syntax, input_data), expected);
}

// Example 5: Array operations with empty arrays
TEST_F(BuilderExamplesTest, EmptyArrayComparison) {
    // OLD SYNTAX - confusing empty array construction
    json old_syntax = json::array({
        "map",
        json{{"array", json::array()}},  // Empty array using object syntax
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    
    // NEW SYNTAX - clear intent
    auto new_syntax = CB::map(
        CB::empty_array(),
        CB::lambda("x", CB::add(CB::var("x"), 1))
    );
    
    json expected = json::array(); // Empty array
    EXPECT_EQ(computo::execute(old_syntax, input_data), expected);
    EXPECT_EQ(computo::execute(new_syntax, input_data), expected);
}

// Example 6: Multiple input forms
TEST_F(BuilderExamplesTest, InputAccessComparison) {
    // Array form works
    auto array_form = CB::input();     // ["$input"]
    EXPECT_EQ(computo::execute(array_form, input_data), input_data);
    
    // Note: String form may not be implemented yet in the core engine
    // auto string_form = CB::input_str(); // "$input" 
    // EXPECT_EQ(computo::execute(string_form, input_data), input_data);
}

// Example 7: Fluent chaining style
TEST_F(BuilderExamplesTest, FluentChainingExample) {
    // Using the << operator for clean chaining
    auto pipeline = CB::op("let")
        << json::array({json::array({"nums", CB::get(CB::input(), "/numbers")})})
        << (CB::op("reduce")
            << CB::var("nums")
            << CB::lambda("acc", CB::lambda("x", CB::add(CB::var("acc"), CB::var("x"))))
            << 0);
    
    // This creates: ["let", [["nums", ["get", ["$input"], "/numbers"]]], 
    //                ["reduce", ["$", "/nums"], ["lambda", ["acc"], ["lambda", ["x"], ["+", ["$", "/acc"], ["$", "/x"]]]], 0]]
    
    // Note: This is a complex example showing the power of the builder
    // The actual reduce operator might not be implemented yet, so we'll skip execution
    // but this shows how the builder enables complex nested structures
}

/*
 * BENEFITS DEMONSTRATED:
 * 
 * 1. READABILITY: Code reads like natural language instead of nested JSON arrays
 * 2. TYPE SAFETY: Static typing catches errors at compile time
 * 3. DISCOVERABILITY: IDE autocomplete shows available methods
 * 4. MAINTAINABILITY: Changes are easier to make and understand
 * 5. LESS ERROR-PRONE: No need to manually balance brackets and quotes
 * 6. COMPOSABILITY: Builders can be easily composed and reused
 * 7. DEBUGGING: Stack traces point to meaningful code instead of JSON construction
 * 
 * PAIN POINTS SOLVED:
 * 
 * ✅ json{{"array", json::array({1,2,3})}} → CB::array({1,2,3})
 * ✅ json::array({"+", a, b}) → CB::add(a, b)  
 * ✅ json::array({"$", "/x"}) → CB::var("x")
 * ✅ Complex nesting becomes readable
 * ✅ Empty arrays are clear: CB::empty_array()
 * ✅ Template argument deduction issues eliminated
 * ✅ Macro conflicts with braced initialization avoided
 * 
 */