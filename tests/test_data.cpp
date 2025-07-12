#include <computo.hpp>
#include <gtest/gtest.h>
using json = nlohmann::json;

class VarAccessTest : public ::testing::Test {
protected:
    auto exec(const json& s) { return computo::execute(s, json(nullptr)); }
};

TEST_F(VarAccessTest, UndefinedVariableThrows) {
    json script = json::array({ "$", "/foo" });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}

TEST_F(VarAccessTest, BadPointerThrows) {
    json script = json::array({ "$", "foo" });
    EXPECT_THROW(exec(script), computo::InvalidArgumentException);
}