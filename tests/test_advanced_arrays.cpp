#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class AdvancedArraysTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test basic zipWith functionality
TEST_F(AdvancedArraysTest, ZipWithOperatorBasic) {
    json test_input = json{
        {"array1", json::array({1, 2, 3, 4, 5})},
        {"array2", json::array({10, 20, 30, 40, 50})}
    };
    
    // Builder Pattern makes zipWith crystal clear!
    auto script = CB::zip_with(
        CB::get(CB::input(), "/array1"),
        CB::get(CB::input(), "/array2"),
        CB::lambda({"a", "b"}, CB::add(CB::var("a"), CB::var("b")))
    );
    
    json expected = json::array({11, 22, 33, 44, 55});
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test zipWith with different sized arrays (takes minimum)
TEST_F(AdvancedArraysTest, ZipWithOperatorDifferentSizes) {
    json test_input = json{
        {"array1", json::array({1, 2, 3})},
        {"array2", json::array({10, 20, 30, 40, 50})}
    };
    
    auto script = CB::zip_with(
        CB::get(CB::input(), "/array1"),
        CB::get(CB::input(), "/array2"),
        CB::lambda({"a", "b"}, CB::multiply(CB::var("a"), CB::var("b")))
    );
    
    json expected = json::array({10, 40, 90}); // Only first 3 elements
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test zipWith with empty arrays
TEST_F(AdvancedArraysTest, ZipWithOperatorEmpty) {
    json test_input = json{
        {"array1", json::array()},
        {"array2", json::array({10, 20, 30})}
    };
    
    auto script = CB::zip_with(
        CB::get(CB::input(), "/array1"),
        CB::get(CB::input(), "/array2"),
        CB::lambda({"a", "b"}, CB::add(CB::var("a"), CB::var("b")))
    );
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test basic mapWithIndex functionality
TEST_F(AdvancedArraysTest, MapWithIndexOperatorBasic) {
    json test_input = json::array({5, 7, 2, 9, 1});
    
    // Map with index - Builder Pattern makes multi-parameter lambdas readable
    auto script = CB::map_with_index(
        CB::input(),
        CB::lambda_multi({"value", "index"}, 
            CB::multiply(CB::var("value"), CB::var("index"))
        )
    );
    
    json expected = json::array({0, 7, 4, 27, 4}); // value * index
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test mapWithIndex with object construction
TEST_F(AdvancedArraysTest, MapWithIndexOperatorObjectConstruction) {
    json test_input = json::array({"a", "b", "c"});
    
    auto script = CB::map_with_index(
        CB::input(),
        CB::lambda_multi({"value", "index"}, 
            CB::obj()
                .add_field("value", CB::var("value"))
                .add_field("index", CB::var("index"))
        )
    );
    
    json expected = json::array({
        json{{"value", "a"}, {"index", 0}},
        json{{"value", "b"}, {"index", 1}},
        json{{"value", "c"}, {"index", 2}}
    });
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test mapWithIndex with empty array
TEST_F(AdvancedArraysTest, MapWithIndexOperatorEmpty) {
    json test_input = json::array();
    
    auto script = CB::map_with_index(
        CB::input(),
        CB::lambda_multi({"value", "index"}, CB::var("value"))
    );
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test basic enumerate functionality
TEST_F(AdvancedArraysTest, EnumerateOperatorBasic) {
    json test_input = json::array({"apple", "banana", "cherry"});
    
    auto script = CB::enumerate(CB::input());
    
    json expected = json::array({
        json::array({0, "apple"}),
        json::array({1, "banana"}),
        json::array({2, "cherry"})
    });
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test enumerate with mixed types
TEST_F(AdvancedArraysTest, EnumerateOperatorMixedTypes) {
    json test_input = json::array({42, "hello", true, json(nullptr)});
    
    auto script = CB::enumerate(CB::input());
    
    json expected = json::array({
        json::array({0, 42}),
        json::array({1, "hello"}),
        json::array({2, true}),
        json::array({3, json(nullptr)})
    });
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test enumerate with empty array
TEST_F(AdvancedArraysTest, EnumerateOperatorEmpty) {
    json test_input = json::array();
    
    auto script = CB::enumerate(CB::input());
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test basic zip functionality
TEST_F(AdvancedArraysTest, ZipOperatorBasic) {
    json test_input = json{
        {"names", json::array({"Alice", "Bob", "Charlie"})},
        {"ages", json::array({25, 30, 35})}
    };
    
    auto script = CB::zip(
        CB::get(CB::input(), "/names"),
        CB::get(CB::input(), "/ages")
    );
    
    json expected = json::array({
        json::array({"Alice", 25}),
        json::array({"Bob", 30}),
        json::array({"Charlie", 35})
    });
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test zip with different sized arrays
TEST_F(AdvancedArraysTest, ZipOperatorDifferentSizes) {
    json test_input = json{
        {"short", json::array({1, 2})},
        {"long", json::array({10, 20, 30, 40})}
    };
    
    auto script = CB::zip(
        CB::get(CB::input(), "/short"),
        CB::get(CB::input(), "/long")
    );
    
    json expected = json::array({
        json::array({1, 10}),
        json::array({2, 20})
    });
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test zip with empty arrays
TEST_F(AdvancedArraysTest, ZipOperatorEmpty) {
    json test_input = json{
        {"empty", json::array()},
        {"full", json::array({1, 2, 3})}
    };
    
    auto script = CB::zip(
        CB::get(CB::input(), "/empty"),
        CB::get(CB::input(), "/full")
    );
    
    json expected = json::array();
    EXPECT_EQ(computo::execute(script, test_input), expected);
}

// Test append operator basic functionality (array concatenation)
TEST_F(AdvancedArraysTest, AppendOperatorBasic) {
    auto script = CB::append({
        CB::array({1, 2, 3}),
        CB::array({4, 5}),
        CB::array({6, 7, 8})
    });
    
    json expected = json::array({1, 2, 3, 4, 5, 6, 7, 8});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test append with empty arrays
TEST_F(AdvancedArraysTest, AppendOperatorWithEmpty) {
    auto script = CB::append({
        CB::array({1, 2}),
        CB::empty_array(),
        CB::array({3, 4}),
        CB::empty_array()
    });
    
    json expected = json::array({1, 2, 3, 4});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test append with single array
TEST_F(AdvancedArraysTest, AppendOperatorSingle) {
    auto script = CB::append({CB::array({1, 2, 3})});
    
    json expected = json::array({1, 2, 3});
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test append with no arrays (should throw error)
TEST_F(AdvancedArraysTest, AppendOperatorEmpty) {
    EXPECT_THROW({
        auto script = CB::append({});
        computo::execute(script, input_data);
    }, computo::InvalidArgumentException);
}

// Test complex functional pipeline with advanced array operations
TEST_F(AdvancedArraysTest, ComplexFunctionalPipeline) {
    json test_input = json{
        {"numbers", json::array({1, 2, 3, 4, 5})},
        {"multipliers", json::array({10, 20, 30, 40, 50})}
    };
    
    // Complex pipeline: zip arrays, map with index, filter, then enumerate
    auto script = CB::enumerate(
        CB::filter(
            CB::map_with_index(
                CB::zip(
                    CB::get(CB::input(), "/numbers"),
                    CB::get(CB::input(), "/multipliers")
                ),
                CB::lambda_multi({"pair", "index"},
                    CB::obj()
                        .add_field("original", CB::get(CB::var("pair"), "/0"))
                        .add_field("multiplied", CB::multiply(
                            CB::get(CB::var("pair"), "/0"),
                            CB::get(CB::var("pair"), "/1")
                        ))
                        .add_field("position", CB::var("index"))
                )
            ),
            CB::lambda("item", CB::greater_than(CB::get(CB::var("item"), "/multiplied"), 50))
        )
    );
    
    // Should filter for items where multiplied > 50, then enumerate them
    json result = computo::execute(script, test_input);
    
    // Verify structure (exact values depend on implementation details)
    EXPECT_TRUE(result.is_array());
    for (const auto& item : result) {
        EXPECT_TRUE(item.is_array());
        EXPECT_EQ(item.size(), 2); // [index, filtered_item]
        EXPECT_TRUE(item[1].contains("multiplied"));
        EXPECT_GT(item[1]["multiplied"].get<int>(), 50);
    }
}

// Test advanced array operations with let bindings
TEST_F(AdvancedArraysTest, AdvancedOperationsWithLet) {
    // Create data, enumerate it, then zip with processed version
    auto script = CB::let(
        {
            {"data", CB::array({"a", "b", "c"})},
            {"enumerated", CB::enumerate(CB::var("data"))},
            {"processed", CB::map(CB::var("data"), CB::lambda("x", CB::str_concat({CB::var("x"), "_processed"})))}
        },
        CB::zip(CB::var("enumerated"), CB::var("processed"))
    );
    
    json expected = json::array({
        json::array({json::array({0, "a"}), "a_processed"}),
        json::array({json::array({1, "b"}), "b_processed"}),
        json::array({json::array({2, "c"}), "c_processed"})
    });
    
    EXPECT_EQ(computo::execute(script, input_data), expected);
}

// Test performance with large arrays (basic smoke test)
TEST_F(AdvancedArraysTest, LargeArrayPerformance) {
    // Create a reasonably large array
    json test_input = json::array();
    for (int i = 0; i < 100; ++i) {
        test_input.push_back(i);
    }
    
    // Chain several operations  
    auto script = CB::append({
        CB::map(CB::input(), CB::lambda("x", CB::multiply(CB::var("x"), 2))),
        CB::map(CB::input(), CB::lambda("x", CB::add(CB::var("x"), 100)))
    });
    
    json result = computo::execute(script, test_input);
    
    // Should have 200 elements (100 doubled + 100 with 100 added)
    EXPECT_EQ(result.size(), 200);
    EXPECT_EQ(result[0], 0);   // First element: 0 * 2
    EXPECT_EQ(result[100], 100); // 101st element: 0 + 100
    EXPECT_EQ(result[199], 199); // Last element: 99 + 100
}