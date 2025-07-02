#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class LambdaAnalysisTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test 1: Simple inline lambda (should work)
TEST_F(LambdaAnalysisTest, SimpleInlineLambda) {
    json working_script = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    
    json expected = json::array({2, 3, 4});
    EXPECT_EQ(computo::execute(working_script, input_data), expected);
}

// Test 2: Builder Pattern equivalent 
TEST_F(LambdaAnalysisTest, BuilderPatternLambda) {
    auto builder_script = CB::map(
        CB::array({1, 2, 3}),
        CB::lambda("x", CB::add(CB::var("x"), 1))
    );
    
    // First verify the JSON structure is identical
    json manual_equivalent = json::array({
        "map",
        json{{"array", json::array({1, 2, 3})}},
        json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
    });
    
    EXPECT_EQ(builder_script.build(), manual_equivalent);
    
    // Then test execution
    json expected = json::array({2, 3, 4});
    EXPECT_EQ(computo::execute(builder_script, input_data), expected);
}

// Test 3: Lambda stored in variable (LESSONS.md says this fails)
TEST_F(LambdaAnalysisTest, StoredLambdaManual) {
    json stored_lambda_script = json::array({
        "let",
        json::array({json::array({"add_one", json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})})}),
        json::array({
            "map",
            json{{"array", json::array({1, 2, 3})}},
            json::array({"$", "/add_one"})
        })
    });
    
    json expected = json::array({2, 3, 4});
    
    // This test will show us if stored lambdas work or fail
    try {
        auto result = computo::execute(stored_lambda_script, input_data);
        EXPECT_EQ(result, expected);
        std::cout << "SUCCESS: Stored lambda (manual) works!\n";
    } catch (const std::exception& e) {
        std::cout << "ERROR: Stored lambda (manual) failed: " << e.what() << "\n";
        FAIL() << "Stored lambda failed: " << e.what();
    }
}

// Test 4: Builder Pattern stored lambda
TEST_F(LambdaAnalysisTest, StoredLambdaBuilder) {
    auto stored_builder_script = CB::let(
        {{"add_one", CB::lambda("x", CB::add(CB::var("x"), 1))}},
        CB::map(CB::array({1, 2, 3}), CB::var("add_one"))
    );
    
    json expected = json::array({2, 3, 4});
    
    try {
        auto result = computo::execute(stored_builder_script, input_data);
        EXPECT_EQ(result, expected);
        std::cout << "SUCCESS: Stored lambda (builder) works!\n";
    } catch (const std::exception& e) {
        std::cout << "ERROR: Stored lambda (builder) failed: " << e.what() << "\n";
        FAIL() << "Stored lambda (builder) failed: " << e.what();
    }
}

// Test 5: Partition with detailed error analysis
TEST_F(LambdaAnalysisTest, PartitionWithAnalysis) {
    // First test manual partition (from original working tests)
    json manual_partition = json::array({
        "partition",
        json{{"array", json::array({1, 2, 3, 4, 5})}},
        json::array({
            "lambda",
            json::array({"x"}),
            json::array({">", json::array({"$", "/x"}), 3})
        })
    });
    
    json expected = json::array({
        json::array({4, 5}),    // truthy items (> 3)
        json::array({1, 2, 3})  // falsy items (<= 3)
    });
    
    EXPECT_EQ(computo::execute(manual_partition, input_data), expected);
    
    // Now test builder version
    auto builder_partition = CB::partition(
        CB::array({1, 2, 3, 4, 5}),
        CB::lambda("x", CB::greater_than(CB::var("x"), 3))
    );
    
    // Verify JSON structure match
    EXPECT_EQ(builder_partition.build(), manual_partition);
    
    // Test execution
    try {
        auto result = computo::execute(builder_partition, input_data);
        EXPECT_EQ(result, expected);
        std::cout << "SUCCESS: Partition (builder) works!\n";
    } catch (const std::exception& e) {
        std::cout << "ERROR: Partition (builder) failed: " << e.what() << "\n";
        std::cout << "Builder JSON: " << builder_partition.build().dump(2) << "\n";
        std::cout << "Manual JSON:  " << manual_partition.dump(2) << "\n";
        FAIL() << "Partition (builder) failed: " << e.what();
    }
}