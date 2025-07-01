#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <computo/builder.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class BuilderFixTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test fixed zipWith argument order
TEST_F(BuilderFixTest, ZipWithArgumentOrder) {
    auto script = CB::zip_with(
        CB::array({1, 2, 3}).build(),
        CB::array({10, 20, 30}).build(),
        CB::lambda({"a", "b"}, CB::add(CB::var("a"), CB::var("b")).build()).build()
    ).build();
    
    json expected = json::array({11, 22, 33});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test fixed logical operators (&& instead of 'and')
TEST_F(BuilderFixTest, LogicalOperatorsFixed) {
    auto script = CB::and_(true, CB::greater_than(5, 3).build()).build();
    EXPECT_EQ(computo::execute(script, input_data), true);
    
    auto script2 = CB::or_(false, CB::less_than(2, 4).build()).build();
    EXPECT_EQ(computo::execute(script2, input_data), true);
}

// Test fixed approx operator name
TEST_F(BuilderFixTest, ApproxOperatorFixed) {
    auto script = CB::approx_equal(3.14159, 3.14160, 0.001).build();
    EXPECT_EQ(computo::execute(script, input_data), true);
}

// Test n-ary operators
TEST_F(BuilderFixTest, NaryOperators) {
    // N-ary addition
    auto script1 = CB::add({1, 2, 3, 4}).build();
    EXPECT_EQ(computo::execute(script1, input_data), 10);
    
    // N-ary multiplication  
    auto script2 = CB::multiply({2, 3, 4}).build();
    EXPECT_EQ(computo::execute(script2, input_data), 24);
    
    // N-ary equality
    auto script3 = CB::equal({5, 5, 5}).build();
    EXPECT_EQ(computo::execute(script3, input_data), true);
    
    // N-ary chained comparison
    auto script4 = CB::less_than({1, 2, 3, 4}).build();
    EXPECT_EQ(computo::execute(script4, input_data), true);
}

// Test string concatenation with automatic type conversion
TEST_F(BuilderFixTest, StringConcatenation) {
    // Test basic string concatenation
    auto script1 = CB::str_concat({"Hello", " ", "World"}).build();
    EXPECT_EQ(computo::execute(script1, input_data), "Hello World");
    
    // Test automatic type conversion
    auto script2 = CB::str_concat({"Value: ", 42, ", Active: ", true}).build();
    EXPECT_EQ(computo::execute(script2, input_data), "Value: 42, Active: true");
    
    // Test with objects and arrays (JSON conversion)
    auto script3 = CB::str_concat({
        "Data: ",
        CB::obj().add_field("id", 123).build(),
        " Array: ",
        CB::array({1, 2, 3}).build()
    }).build();
    std::string result = computo::execute(script3, input_data);
    EXPECT_TRUE(result.find("Data: {\"id\":123}") != std::string::npos);
    EXPECT_TRUE(result.find("Array: [1,2,3]") != std::string::npos);
    
    // Test with null values (should be skipped)
    auto script4 = CB::str_concat({"Before", json(nullptr), "After"}).build();
    EXPECT_EQ(computo::execute(script4, input_data), "BeforeAfter");
}

// Test fixed JSON patch operators (commented out due to permuto linking)
/* 
TEST_F(BuilderFixTest, JsonPatchOperators) {
    json document = json{{"id", 123}, {"status", "active"}};
    json modified = json{{"id", 123}, {"status", "archived"}};
    
    auto diff_script = CB::json_patch_diff(document, modified).build();
    json patch = computo::execute(diff_script, input_data);
    
    EXPECT_TRUE(patch.is_array());
    EXPECT_GT(patch.size(), 0);
    
    // Test applying the patch
    auto patch_script = CB::json_patch_apply(document, CB::array(patch).build()).build();
    json result = computo::execute(patch_script, input_data);
    EXPECT_EQ(result, modified);
}
*/

// Test partition with predicate
TEST_F(BuilderFixTest, PartitionWithPredicate) {
    auto script = CB::partition(
        CB::array({1, 2, 3, 4, 5}).build(),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3).build()).build()
    ).build();
    
    json expected = json::array({
        json::array({4, 5}),    // truthy items  
        json::array({1, 2, 3})  // falsy items
    });
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test multi-parameter lambda construction
TEST_F(BuilderFixTest, MultiParameterLambda) {
    auto script = CB::reduce(
        CB::array({1, 2, 3, 4}).build(),
        CB::lambda({"acc", "item"}, CB::add(CB::var("acc"), CB::var("item")).build()).build(),
        0
    ).build();
    
    EXPECT_EQ(computo::execute(script, input_data), 10);
}

// Test flatMap with correct operator name
TEST_F(BuilderFixTest, FlatMapOperator) {
    auto script = CB::flatmap(
        CB::array({1, 2, 3}).build(),
        CB::lambda("x", CB::array({
            CB::var("x").build(),
            CB::multiply(CB::var("x"), 2).build()
        }).build()).build()
    ).build();
    
    json expected = json::array({1, 2, 2, 4, 3, 6});
    EXPECT_EQ(computo::execute(script, input_data), expected);
} 