#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class DataAccessTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test let operator basic functionality
TEST_F(DataAccessTest, LetOperatorBasic) {
    // Builder Pattern: Much cleaner than nested json::array construction!
    auto script = CB::let({{"x", 5}}, CB::var("x"));
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test let operator with multiple variables
TEST_F(DataAccessTest, LetOperatorMultipleVars) {
    // Builder Pattern makes complex variable binding readable
    auto script = CB::let(
        {{"x", 5}, {"y", 10}}, 
        CB::add(CB::var("x"), CB::var("y"))
    );
    EXPECT_EQ(computo::execute(script, input_data), 15);
}

// Test let operator with expression evaluation in bindings
TEST_F(DataAccessTest, LetOperatorExpressionBinding) {
    // Expression evaluation in bindings - Builder Pattern shines here!
    auto script = CB::let(
        {{"x", CB::add(2, 3)}}, 
        CB::var("x")
    );
    EXPECT_EQ(computo::execute(script, input_data), 5);
}

// Test let operator variable shadowing
TEST_F(DataAccessTest, LetOperatorShadowing) {
    // Nested let with shadowing - Builder Pattern makes structure clear
    auto script = CB::let(
        {{"x", 5}}, 
        CB::let(
            {{"x", 10}}, 
            CB::var("x")
        )
    );
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

// Test $ operator variable lookup
TEST_F(DataAccessTest, DollarOperatorLookup) {
    // Create context with a variable manually (for isolated testing)
    computo::ExecutionContext ctx(input_data);
    ctx.variables["test_var"] = 42;
    
    auto script = CB::var("test_var");
    EXPECT_EQ(computo::evaluate(script, ctx), 42);
}

// Test get operator with simple object access
TEST_F(DataAccessTest, GetOperatorSimple) {
    // JSON Pointer access - Builder Pattern makes intent crystal clear
    json test_obj = json{{"name", "John"}, {"age", 30}};
    auto script = CB::get(test_obj, "/name");
    EXPECT_EQ(computo::execute(script, input_data), "John");
}

// Test get operator with nested object access
TEST_F(DataAccessTest, GetOperatorNested) {
    // Nested object access with JSON Pointer
    json nested_obj = json{{"user", json{{"profile", json{{"name", "Alice"}}}}}};
    auto script = CB::get(nested_obj, "/user/profile/name");
    EXPECT_EQ(computo::execute(script, input_data), "Alice");
}

// Test get operator with array access
TEST_F(DataAccessTest, GetOperatorArray) {
    // Array access via JSON Pointer - much cleaner than manual JSON construction
    json array_obj = json{{"array", json::array({10, 20, 30})}};
    auto script = CB::get(array_obj, "/1");
    EXPECT_EQ(computo::execute(script, input_data), 20);
}

// Test get operator with $input
TEST_F(DataAccessTest, GetOperatorWithInput) {
    json test_input = json{{"user", json{{"id", 123}}}};
    
    // Accessing input data with JSON Pointer - Builder Pattern makes this very readable
    auto script = CB::get(CB::input(), "/user/id");
    EXPECT_EQ(computo::execute(script, test_input), 123);
}

// Test complex combination: let + $ + get
TEST_F(DataAccessTest, LetDollarGetCombination) {
    json test_input = json{{"data", json{{"values", json::array({1, 2, 3})}}}};
    
    // Complex combination - Builder Pattern makes the flow clear:
    // 1. Bind input to variable "obj"
    // 2. Access that variable and get deep path from it
    auto script = CB::let(
        {{"obj", CB::input()}}, 
        CB::get(CB::var("obj"), "/data/values/2")
    );
    EXPECT_EQ(computo::execute(script, test_input), 3);
}