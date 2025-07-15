#include <computo.hpp>
#include <gtest/gtest.h>

using json = nlohmann::json;

class InfrastructureTest : public ::testing::Test { };

TEST_F(InfrastructureTest, ExceptionHierarchyCatch) {
    try {
        throw computo::InvalidOperatorException("foo");
    } catch (const computo::ComputoException& e) {
        SUCCEED();
        return;
    }
    FAIL() << "Should have caught ComputoException";
}

TEST_F(InfrastructureTest, ExecutionContextSingleInput) {
    json input = json { { "k", 1 } };
    computo::ExecutionContext ctx(input);
    EXPECT_EQ(ctx.input(), input);
    EXPECT_EQ(ctx.inputs().size(), 1);
}

TEST_F(InfrastructureTest, ExecuteLiteral) {
    json input = json { { "dummy", 0 } };
    json script = 42; // NOLINT(readability-magic-numbers)
    EXPECT_EQ(computo::execute(script, input), 42); // NOLINT(readability-magic-numbers)
}

TEST_F(InfrastructureTest, ExecuteInputOperator) {
    json input = json { { "value", 7 } }; // NOLINT(readability-magic-numbers)
    json script = json::array({ "$input" });
    EXPECT_EQ(computo::execute(script, input), input);
}

TEST_F(InfrastructureTest, UnknownOperatorThrows) {
    json script = json::array({ "unknown" });
    EXPECT_THROW(computo::execute(script, json {}), computo::InvalidOperatorException); // NOLINT(clang-diagnostic-unused-result)
}
