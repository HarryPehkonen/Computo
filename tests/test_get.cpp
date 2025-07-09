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