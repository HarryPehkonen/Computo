#include <gtest/gtest.h>
#include <computo.hpp>
using json = nlohmann::json;

class EnhancedDataAccessTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

// Test enhanced $input operator
TEST_F(EnhancedDataAccessTest, InputWithPath) {
    json input = json{{"a", json{{"b", 42}}}};
    json script = json::array({"$input", "/a/b"});
    EXPECT_EQ(exec(script, input), 42);
}

TEST_F(EnhancedDataAccessTest, InputWithoutPath) {
    json input = json{{"x", 1}};
    json script = json::array({"$input"});
    EXPECT_EQ(exec(script, input), input);
}

TEST_F(EnhancedDataAccessTest, InputInvalidPathThrows) {
    json input = json{{"x", 1}};
    json script = json::array({"$input", "/y"});
    EXPECT_THROW(exec(script, input), computo::InvalidArgumentException);
}

// Test enhanced $inputs operator
TEST_F(EnhancedDataAccessTest, InputsWithPath) {
    std::vector<json> inputs = {json{{"a", 1}}, json{{"b", 2}}};
    json script = json::array({"$inputs", "/0/a"});
    EXPECT_EQ(computo::execute(script, inputs), 1);
}

TEST_F(EnhancedDataAccessTest, InputsWithoutPath) {
    std::vector<json> inputs = {json{{"a", 1}}, json{{"b", 2}}};
    json script = json::array({"$inputs"});
    json expected = json::array({json{{"a", 1}}, json{{"b", 2}}});
    EXPECT_EQ(computo::execute(script, inputs), expected);
}

TEST_F(EnhancedDataAccessTest, InputsInvalidPathThrows) {
    std::vector<json> inputs = {json{{"a", 1}}};
    json script = json::array({"$inputs", "/1"});
    EXPECT_THROW(computo::execute(script, inputs), computo::InvalidArgumentException);
}

// Test enhanced $ operator  
TEST_F(EnhancedDataAccessTest, VariableWithPath) {
    json script = json::array({
        "let",
        json::array({json::array({"data", json{{"x", json{{"y", 42}}}}})}),
        json::array({"$", "/data/x/y"})
    });
    EXPECT_EQ(exec(script), 42);
}

TEST_F(EnhancedDataAccessTest, VariableWithoutPath) {
    json script = json::array({
        "let",
        json::array({json::array({"x", 10})}),
        json::array({"$"})
    });
    json expected = json{{"x", 10}};
    EXPECT_EQ(exec(script), expected);
}

TEST_F(EnhancedDataAccessTest, VariableInvalidPathThrows) {
    json script = json::array({
        "let",
        json::array({json::array({"x", 10})}),
        json::array({"$", "/y"})
    });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

// Test enhanced $input operator with default values
TEST_F(EnhancedDataAccessTest, InputWithDefault) {
    json input = json{{"a", json{{"b", 42}}}};
    json script = json::array({"$input", "/a/b", "default"});
    EXPECT_EQ(exec(script, input), 42);
}

TEST_F(EnhancedDataAccessTest, InputWithDefaultOnMissingPath) {
    json input = json{{"a", json{{"b", 42}}}};
    json script = json::array({"$input", "/missing", "default_value"});
    EXPECT_EQ(exec(script, input), "default_value");
}

TEST_F(EnhancedDataAccessTest, InputWithDefaultOnInvalidPath) {
    json input = json{{"a", json{{"b", 42}}}};
    json script = json::array({"$input", "/a/b/c", "fallback"});
    EXPECT_EQ(exec(script, input), "fallback");
}

TEST_F(EnhancedDataAccessTest, InputWithComplexDefault) {
    json input = json{{"data", json{{"users", json::array({1, 2, 3})}}}};
    json default_value = json{{"error", "not_found"}};
    json script = json::array({"$input", "/data/admins", default_value});
    EXPECT_EQ(exec(script, input), default_value);
}

// Test enhanced $inputs operator with default values
TEST_F(EnhancedDataAccessTest, InputsWithDefault) {
    std::vector<json> inputs = {json{{"a", 1}}, json{{"b", 2}}};
    json script = json::array({"$inputs", "/0/a", "default"});
    EXPECT_EQ(computo::execute(script, inputs), 1);
}

TEST_F(EnhancedDataAccessTest, InputsWithDefaultOnMissingIndex) {
    std::vector<json> inputs = {json{{"a", 1}}, json{{"b", 2}}};
    json script = json::array({"$inputs", "/2", "no_third_input"});
    EXPECT_EQ(computo::execute(script, inputs), "no_third_input");
}

TEST_F(EnhancedDataAccessTest, InputsWithDefaultOnMissingPath) {
    std::vector<json> inputs = {json{{"a", 1}}, json{{"b", 2}}};
    json script = json::array({"$inputs", "/0/missing", "default_field"});
    EXPECT_EQ(computo::execute(script, inputs), "default_field");
}

TEST_F(EnhancedDataAccessTest, InputsWithComplexDefault) {
    std::vector<json> inputs = {json{{"x", 1}}};
    json default_value = json::array({1, 2, 3});
    json script = json::array({"$inputs", "/1/data", default_value});
    EXPECT_EQ(computo::execute(script, inputs), default_value);
}

// Test enhanced $ operator with default values
TEST_F(EnhancedDataAccessTest, VariableWithDefault) {
    json script = json::array({
        "let",
        json::array({json::array({"data", json{{"x", json{{"y", 42}}}}})}),
        json::array({"$", "/data/x/y", "default"})
    });
    EXPECT_EQ(exec(script), 42);
}

TEST_F(EnhancedDataAccessTest, VariableWithDefaultOnMissingVariable) {
    json script = json::array({
        "let",
        json::array({json::array({"x", 10})}),
        json::array({"$", "/missing_var", "default_value"})
    });
    EXPECT_EQ(exec(script), "default_value");
}

TEST_F(EnhancedDataAccessTest, VariableWithDefaultOnMissingPath) {
    json script = json::array({
        "let",
        json::array({json::array({"data", json{{"x", 1}}})}),
        json::array({"$", "/data/y", "fallback"})
    });
    EXPECT_EQ(exec(script), "fallback");
}

TEST_F(EnhancedDataAccessTest, VariableWithDefaultOnArrayIndexOutOfBounds) {
    json script = json::array({
        "let",
        json::array({json::array({"arr", json::array({1, 2, 3})})}),
        json::array({"$", "/arr/5", "out_of_bounds"})
    });
    EXPECT_EQ(exec(script), "out_of_bounds");
}

TEST_F(EnhancedDataAccessTest, VariableWithComplexDefault) {
    json script = json::array({
        "let",
        json::array({json::array({"config", json{{"enabled", true}}})}),
        json::array({"$", "/config/settings", json{{"default", "config"}}})
    });
    json expected = json{{"default", "config"}};
    EXPECT_EQ(exec(script), expected);
}

// Test nested default scenarios
TEST_F(EnhancedDataAccessTest, NestedDefaultsWithInputAndVariable) {
    json input = json{{"user", json{{"name", "Alice"}}}};
    json script = json::array({
        "let",
        json::array({json::array({"fallback", "unknown"})}),
        json::array({"$input", "/user/email", json::array({"$", "/fallback"})})
    });
    EXPECT_EQ(exec(script, input), "unknown");
}

TEST_F(EnhancedDataAccessTest, NestedDefaultsWithMultipleInputs) {
    std::vector<json> inputs = {json{{"primary", 1}}, json{{"secondary", 2}}};
    json script = json::array({
        "$inputs", "/0/backup", 
        json::array({"$inputs", "/1/backup", "no_backup"})
    });
    EXPECT_EQ(computo::execute(script, inputs), "no_backup");
}

// Test expression evaluation in defaults
TEST_F(EnhancedDataAccessTest, DefaultWithExpressionEvaluation) {
    json input = json{{"x", 5}};
    json script = json::array({
        "$input", "/missing", 
        json::array({"+", json::array({"$input", "/x"}), 10})
    });
    EXPECT_EQ(exec(script, input), 15);
} 