#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class JsonPatchTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json input_data;
};

// Test multiple inputs functionality - $inputs system variable
TEST_F(JsonPatchTest, InputsSystemVariable) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    };
    
    auto script = CB::inputs();  // ["$inputs"]
    json result = computo::execute(script, inputs);
    
    json expected = json::array({
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    });
    
    EXPECT_EQ(result, expected);
}

// Test $inputs with empty inputs
TEST_F(JsonPatchTest, InputsSystemVariableEmpty) {
    std::vector<json> inputs = {};
    
    auto script = CB::inputs();
    json result = computo::execute(script, inputs);
    
    json expected = json::array();
    EXPECT_EQ(result, expected);
}

// Test $inputs with single input
TEST_F(JsonPatchTest, InputsSystemVariableSingle) {
    std::vector<json> inputs = {json{{"test", "value"}}};
    
    auto script = CB::inputs();
    json result = computo::execute(script, inputs);
    
    json expected = json::array({json{{"test", "value"}}});
    EXPECT_EQ(result, expected);
}

// Test backward compatibility: $input with multiple inputs
TEST_F(JsonPatchTest, InputBackwardCompatibilityMultiple) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}}
    };
    
    auto script = CB::input();
    json result = computo::execute(script, inputs);
    
    // Should return the first input
    json expected = json{{"id", 1}, {"name", "first"}};
    EXPECT_EQ(result, expected);
}

// Test backward compatibility: $input with empty inputs
TEST_F(JsonPatchTest, InputBackwardCompatibilityEmpty) {
    std::vector<json> inputs = {};
    
    auto script = CB::input();
    json result = computo::execute(script, inputs);
    
    // Should return null when no inputs
    EXPECT_EQ(result, json(nullptr));
}

// Test accessing specific inputs with get and $inputs
TEST_F(JsonPatchTest, InputsWithGetOperator) {
    std::vector<json> inputs = {
        json{{"id", 1}, {"name", "first"}},
        json{{"id", 2}, {"name", "second"}},
        json{{"id", 3}, {"name", "third"}}
    };
    
    // Get the second input using Builder Pattern
    auto script = CB::get(CB::inputs(), "/1");
    
    json result = computo::execute(script, inputs);
    json expected = json{{"id", 2}, {"name", "second"}};
    EXPECT_EQ(result, expected);
}

// Test diff operator basic functionality
TEST_F(JsonPatchTest, DiffOperatorBasic) {
    json original = json{{"id", 123}, {"status", "active"}};
    json modified = json{{"id", 123}, {"status", "archived"}};
    
    // Builder Pattern makes diff operation clear
    auto script = CB::op("diff").arg(original).arg(modified);
    
    json result = computo::execute(script, input_data);
    
    // Should contain a replace operation for status
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0]["op"], "replace");
    EXPECT_EQ(result[0]["path"], "/status");
    EXPECT_EQ(result[0]["value"], "archived");
}

// Test diff operator with identical documents
TEST_F(JsonPatchTest, DiffOperatorIdentical) {
    json document = json{{"id", 123}, {"name", "test"}};
    
    auto script = CB::op("diff").arg(document).arg(document);
    
    json result = computo::execute(script, input_data);
    
    // Should return empty array for identical documents
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0);
}

// Test diff operator with complex changes
TEST_F(JsonPatchTest, DiffOperatorComplex) {
    json original = json{
        {"id", 123},
        {"user", json{{"name", "John"}, {"age", 30}}},
        {"tags", json::array({"tag1", "tag2"})}
    };
    
    json modified = json{
        {"id", 123},
        {"user", json{{"name", "Jane"}, {"age", 31}, {"email", "jane@example.com"}}},
        {"tags", json::array({"tag1", "tag3", "tag4"})}
    };
    
    auto script = CB::op("diff").arg(original).arg(modified);
    
    json result = computo::execute(script, input_data);
    
    // Should contain multiple operations
    EXPECT_TRUE(result.is_array());
    EXPECT_GT(result.size(), 0);
    
    // Test the basic functionality of diff
    bool found_user_change = false;
    bool found_tags_change = false;
    
    for (const auto& op : result) {
        if (op.contains("path")) {
            std::string path = op["path"];
            if (path.find("/user/") != std::string::npos) {
                found_user_change = true;
            }
            if (path.find("/tags") != std::string::npos) {
                found_tags_change = true;
            }
        }
    }
    
    EXPECT_TRUE(found_user_change || found_tags_change);
}

// Test patch operator basic functionality
TEST_F(JsonPatchTest, PatchOperatorBasic) {
    json document = json{{"id", 123}, {"status", "active"}};
    json patch = CB::array({
        json{{"op", "replace"}, {"path", "/status"}, {"value", "archived"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"status", "archived"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with add operation
TEST_F(JsonPatchTest, PatchOperatorAdd) {
    json document = json{{"id", 123}};
    json patch = CB::array({
        json{{"op", "add"}, {"path", "/name"}, {"value", "John"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"name", "John"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with remove operation
TEST_F(JsonPatchTest, PatchOperatorRemove) {
    json document = json{{"id", 123}, {"name", "John"}, {"temp", "delete_me"}};
    json patch = CB::array({
        json{{"op", "remove"}, {"path", "/temp"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"name", "John"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with move operation
TEST_F(JsonPatchTest, PatchOperatorMove) {
    json document = json{{"id", 123}, {"old_field", "value"}, {"other", "data"}};
    json patch = CB::array({
        json{{"op", "move"}, {"from", "/old_field"}, {"path", "/new_field"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"new_field", "value"}, {"other", "data"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with copy operation
TEST_F(JsonPatchTest, PatchOperatorCopy) {
    json document = json{{"id", 123}, {"name", "John"}};
    json patch = CB::array({
        json{{"op", "copy"}, {"from", "/name"}, {"path", "/display_name"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    json result = computo::execute(script, input_data);
    json expected = json{{"id", 123}, {"name", "John"}, {"display_name", "John"}};
    
    EXPECT_EQ(result, expected);
}

// Test patch operator with test operation (successful)
TEST_F(JsonPatchTest, PatchOperatorTestSuccess) {
    json document = json{{"id", 123}, {"status", "active"}};
    json patch = CB::array({
        json{{"op", "test"}, {"path", "/status"}, {"value", "active"}}
    });
    
    auto script = CB::op("patch").arg(document).arg(patch);
    
    // Should succeed and return the document unchanged
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result, document);
}

// Test round-trip: diff + patch should restore original
TEST_F(JsonPatchTest, DiffPatchRoundTrip) {
    json original = json{{"id", 123}, {"name", "John"}, {"age", 30}};
    json modified = json{{"id", 123}, {"name", "Jane"}, {"age", 31}, {"email", "jane@example.com"}};
    
    // Generate diff
    auto diff_script = CB::op("diff").arg(original).arg(modified);
    json patch = computo::execute(diff_script, input_data);
    
    // Apply patch to original
    auto patch_script = CB::op("patch").arg(original).arg(CB::array(patch));
    json result = computo::execute(patch_script, input_data);
    
    // Should match the modified document
    EXPECT_EQ(result, modified);
}

// Test complex pipeline with diff/patch
TEST_F(JsonPatchTest, ComplexDiffPatchPipeline) {
    // Create a pipeline that modifies data, diffs it, then applies patches
    auto script = CB::let(
        {
            {"original", CB::obj().add_field("count", 1).add_field("status", "new")},
            {"modified", CB::obj().add_field("count", 5).add_field("status", "updated").add_field("timestamp", "2024-01-01")}
        },
        CB::let(
            {{"patch", CB::op("diff").arg(CB::var("original")).arg(CB::var("modified"))}},
            CB::op("patch").arg(CB::var("original")).arg(CB::var("patch"))
        )
    );
    
    json result = computo::execute(script, input_data);
    json expected = json{{"count", 5}, {"status", "updated"}, {"timestamp", "2024-01-01"}};
    
    EXPECT_EQ(result, expected);
}