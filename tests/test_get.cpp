#include <gtest/gtest.h>
#include <computo.hpp>
using json = nlohmann::json;

class GetOperatorTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
};

TEST_F(GetOperatorTest, BasicPointer) {
    json obj = json{{"a", json{{"b", 42}}}};
    json script = json::array({"get", obj, "/a/b"});
    EXPECT_EQ(exec(script), 42);
}

TEST_F(GetOperatorTest, InvalidPointerThrows) {
    json obj = json{{"x", 1}};
    json script = json::array({"get", obj, "/y"});
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
} 