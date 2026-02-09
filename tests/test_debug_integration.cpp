#include "computo.hpp"
#include <gtest/gtest.h>

using json = jsom::JsonDocument;

class DebugIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { debug_ctx = std::make_unique<computo::DebugContext>(); }

    void TearDown() override { debug_ctx.reset(); }

    std::unique_ptr<computo::DebugContext> debug_ctx;
};

// Test basic debug context functionality
TEST_F(DebugIntegrationTest, BasicDebugContext) {
    EXPECT_FALSE(debug_ctx->is_debug_enabled());
    EXPECT_FALSE(debug_ctx->is_trace_enabled());
    EXPECT_FALSE(debug_ctx->is_step_mode());
    EXPECT_FALSE(debug_ctx->is_finish_mode());

    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_trace_enabled(true);

    EXPECT_TRUE(debug_ctx->is_debug_enabled());
    EXPECT_TRUE(debug_ctx->is_trace_enabled());
}

// Test operator breakpoint setting and removal
TEST_F(DebugIntegrationTest, OperatorBreakpoints) {
    debug_ctx->set_operator_breakpoint("+");
    debug_ctx->set_operator_breakpoint("map");

    auto breakpoints = debug_ctx->get_operator_breakpoints();
    EXPECT_EQ(breakpoints.size(), 2);
    EXPECT_TRUE(breakpoints.count("+") > 0);
    EXPECT_TRUE(breakpoints.count("map") > 0);

    EXPECT_TRUE(debug_ctx->should_break_on_operator("+"));
    EXPECT_TRUE(debug_ctx->should_break_on_operator("map"));
    EXPECT_FALSE(debug_ctx->should_break_on_operator("-"));

    debug_ctx->remove_operator_breakpoint("+");
    breakpoints = debug_ctx->get_operator_breakpoints();
    EXPECT_EQ(breakpoints.size(), 1);
    EXPECT_FALSE(debug_ctx->should_break_on_operator("+"));
    EXPECT_TRUE(debug_ctx->should_break_on_operator("map"));
}

// Test variable breakpoint setting and removal
TEST_F(DebugIntegrationTest, VariableBreakpoints) {
    debug_ctx->set_variable_breakpoint("/users");
    debug_ctx->set_variable_breakpoint("/config");

    auto breakpoints = debug_ctx->get_variable_breakpoints();
    EXPECT_EQ(breakpoints.size(), 2);
    EXPECT_TRUE(breakpoints.count("/users") > 0);
    EXPECT_TRUE(breakpoints.count("/config") > 0);

    EXPECT_TRUE(debug_ctx->should_break_on_variable("/users"));
    EXPECT_TRUE(debug_ctx->should_break_on_variable("/config"));
    EXPECT_FALSE(debug_ctx->should_break_on_variable("/data"));

    debug_ctx->remove_variable_breakpoint("/users");
    breakpoints = debug_ctx->get_variable_breakpoints();
    EXPECT_EQ(breakpoints.size(), 1);
    EXPECT_FALSE(debug_ctx->should_break_on_variable("/users"));
    EXPECT_TRUE(debug_ctx->should_break_on_variable("/config"));
}

// Test breakpoint clearing
TEST_F(DebugIntegrationTest, ClearBreakpoints) {
    debug_ctx->set_operator_breakpoint("+");
    debug_ctx->set_variable_breakpoint("/users");

    EXPECT_EQ(debug_ctx->get_operator_breakpoints().size(), 1);
    EXPECT_EQ(debug_ctx->get_variable_breakpoints().size(), 1);

    debug_ctx->clear_all_breakpoints();

    EXPECT_EQ(debug_ctx->get_operator_breakpoints().size(), 0);
    EXPECT_EQ(debug_ctx->get_variable_breakpoints().size(), 0);
}

// Test finish mode (ignores breakpoints)
TEST_F(DebugIntegrationTest, FinishMode) {
    debug_ctx->set_operator_breakpoint("+");
    debug_ctx->set_variable_breakpoint("/users");

    EXPECT_TRUE(debug_ctx->should_break_on_operator("+"));
    EXPECT_TRUE(debug_ctx->should_break_on_variable("/users"));

    debug_ctx->set_finish_mode(true);
    EXPECT_FALSE(debug_ctx->should_break_on_operator("+"));
    EXPECT_FALSE(debug_ctx->should_break_on_variable("/users"));
    EXPECT_FALSE(debug_ctx->should_break());

    debug_ctx->set_finish_mode(false);
    EXPECT_TRUE(debug_ctx->should_break_on_operator("+"));
    EXPECT_TRUE(debug_ctx->should_break_on_variable("/users"));
}

// Test execution tracing
TEST_F(DebugIntegrationTest, ExecutionTracing) {
    debug_ctx->set_trace_enabled(true);
    debug_ctx->set_debug_enabled(true);

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    try {
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        EXPECT_EQ(result, 3);
    } catch (const computo::DebugBreakException&) {
        // This is expected if we hit a breakpoint
    }

    // Check that trace was recorded
    auto trace = debug_ctx->get_execution_trace();
    EXPECT_FALSE(trace.empty());

    // Should have recorded the + operator
    bool found_addition = false;
    for (const auto& step : trace) {
        if (step.operation == "+") {
            found_addition = true;
            EXPECT_EQ(step.location, "/");
            break;
        }
    }
    EXPECT_TRUE(found_addition);
}

// Test operator breakpoint triggering
TEST_F(DebugIntegrationTest, OperatorBreakpointTriggering) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_operator_breakpoint("+");

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    // Should throw DebugBreakException when hitting the + operator
    EXPECT_THROW(
        { computo::execute(script, {json(nullptr)}, debug_ctx.get()); },
        computo::DebugBreakException);

    // Try with an operator that doesn't have a breakpoint
    debug_ctx->clear_all_breakpoints();
    debug_ctx->set_operator_breakpoint("-");

    // Should execute normally (no - operator in the script)
    EXPECT_NO_THROW({
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        EXPECT_EQ(result, 3);
    });
}

// Test step mode
TEST_F(DebugIntegrationTest, StepMode) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_step_mode(true);

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    // Should trigger a step mode breakpoint
    EXPECT_THROW(
        { computo::execute(script, {json(nullptr)}, debug_ctx.get()); },
        computo::DebugBreakException);
}

// Test complex script with nested operations
TEST_F(DebugIntegrationTest, ComplexScriptDebugging) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_trace_enabled(true);
    debug_ctx->set_operator_breakpoint("map");

    auto script = jsom::parse_document(R"([
        "let", [["data", {"array": [1, 2, 3]}]],
        ["map", ["$", "/data"], ["lambda", ["x"], ["+", ["$", "/x"], 1]]]
    ])");

    // Should hit breakpoint on map operator
    EXPECT_THROW(
        { computo::execute(script, {json(nullptr)}, debug_ctx.get()); },
        computo::DebugBreakException);

    // Check trace contains multiple operations
    auto trace = debug_ctx->get_execution_trace();
    EXPECT_GT(trace.size(), 1);

    // Should have recorded let operation before hitting map breakpoint
    bool found_let = false;
    for (const auto& step : trace) {
        if (step.operation == "let") {
            found_let = true;
            break;
        }
    }
    EXPECT_TRUE(found_let);
}

// Test debug context reset
TEST_F(DebugIntegrationTest, DebugContextReset) {
    debug_ctx->set_step_mode(true);
    debug_ctx->set_finish_mode(true);
    debug_ctx->set_trace_enabled(true);

    // Add some trace data
    std::map<std::string, json> vars;
    debug_ctx->record_step("test_op", "/test", vars, json("test"));

    EXPECT_TRUE(debug_ctx->is_step_mode());
    EXPECT_TRUE(debug_ctx->is_finish_mode());
    EXPECT_FALSE(debug_ctx->get_execution_trace().empty());

    debug_ctx->reset();

    EXPECT_FALSE(debug_ctx->is_step_mode());
    EXPECT_FALSE(debug_ctx->is_finish_mode());
    EXPECT_TRUE(debug_ctx->get_execution_trace().empty());
}

// Test debug exception information
TEST_F(DebugIntegrationTest, DebugExceptionInfo) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_operator_breakpoint("*");

    auto script = jsom::parse_document(R"(["*", 2, 3])");

    try {
        computo::execute(script, {json(nullptr)}, debug_ctx.get());
        FAIL() << "Expected DebugBreakException to be thrown";
    } catch (const computo::DebugBreakException& e) {
        EXPECT_EQ(e.get_location(), "/");
        EXPECT_TRUE(std::string(e.get_reason()).find("operator breakpoint: *")
                    != std::string::npos);
        EXPECT_TRUE(std::string(e.what()).find("Debug breakpoint") != std::string::npos);
    }
}

// Test debugging with multiple input files
TEST_F(DebugIntegrationTest, MultipleInputsDebugging) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_trace_enabled(true);
    debug_ctx->set_operator_breakpoint("$inputs");

    auto script = jsom::parse_document(R"(["$inputs", "/1"])");
    std::vector<json> inputs = {json(10), json(20), json(30)};

    // Should hit breakpoint on $inputs operator
    EXPECT_THROW(
        { computo::execute(script, inputs, debug_ctx.get()); }, computo::DebugBreakException);
}

// Test debugging with let variables
TEST_F(DebugIntegrationTest, LetVariableDebugging) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_trace_enabled(true);

    auto script = jsom::parse_document(R"([
        "let", [["x", 10], ["y", 20]],
        ["+", ["$", "/x"], ["$", "/y"]]
    ])");

    try {
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        EXPECT_EQ(result, 30);
    } catch (const computo::DebugBreakException&) {
        // Expected if we set breakpoints
    }

    // Check that variables are recorded in trace
    auto trace = debug_ctx->get_execution_trace();
    bool found_variable_access = false;
    for (const auto& step : trace) {
        if (!step.variables.empty()) {
            found_variable_access = true;
            // Should have x and y variables
            if (step.variables.count("x") > 0 && step.variables.count("y") > 0) {
                EXPECT_EQ(step.variables.at("x"), 10);
                EXPECT_EQ(step.variables.at("y"), 20);
            }
            break;
        }
    }
    EXPECT_TRUE(found_variable_access);
}

// Test current location tracking
TEST_F(DebugIntegrationTest, CurrentLocationTracking) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_trace_enabled(true);

    // Initial location should be "start"
    EXPECT_EQ(debug_ctx->get_current_location(), "start");

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    try {
        computo::execute(script, {json(nullptr)}, debug_ctx.get());
    } catch (const computo::DebugBreakException&) {
        // Expected
    }

    // After execution, current location should be updated
    EXPECT_NE(debug_ctx->get_current_location(), "start");
}

// Test no debugging when debug context is null
TEST_F(DebugIntegrationTest, NullDebugContext) {
    auto script = jsom::parse_document(R"(["+", 1, 2])");

    // Should execute normally without debugging
    EXPECT_NO_THROW({
        auto result = computo::execute(script, {json(nullptr)}, nullptr);
        EXPECT_EQ(result, 3);
    });
}

// Test debug disabled mode
TEST_F(DebugIntegrationTest, DebugDisabled) {
    debug_ctx->set_debug_enabled(false); // Explicitly disabled
    debug_ctx->set_operator_breakpoint("+");

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    // Should execute normally even with breakpoints set
    EXPECT_NO_THROW({
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        EXPECT_EQ(result, 3);
    });
}

// Test that "break +" actually breaks on ["+", 1, 2] execution
TEST_F(DebugIntegrationTest, BreakOnPlusOperator) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_operator_breakpoint("+");

    auto script = jsom::parse_document(R"(["+", 1, 2])");

    // Should throw DebugBreakException when hitting the + operator
    bool breakpoint_hit = false;
    std::string break_location;
    std::string break_reason;

    try {
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        // Should not reach here if breakpoint works
        FAIL() << "Expected DebugBreakException to be thrown";
    } catch (const computo::DebugBreakException& e) {
        breakpoint_hit = true;
        break_location = e.get_location();
        break_reason = e.get_reason();
    }

    EXPECT_TRUE(breakpoint_hit);
    EXPECT_EQ(break_location, "/");
    EXPECT_EQ(break_reason, "operator breakpoint: +");

    // Verify the breakpoint message contains expected information
    std::string full_message
        = std::string("Debug breakpoint: ") + break_reason + " at " + break_location;
    EXPECT_EQ(std::string(full_message), "Debug breakpoint: operator breakpoint: + at /");
}

// Test that breakpoints work with nested expressions
TEST_F(DebugIntegrationTest, BreakOnNestedPlusOperator) {
    debug_ctx->set_debug_enabled(true);
    debug_ctx->set_operator_breakpoint("+");

    // Nested expression: ["+", ["+", 1, 2], 3] - should break on first +
    auto script = jsom::parse_document(R"(["+", ["+", 1, 2], 3])");

    bool breakpoint_hit = false;
    std::string break_location;

    try {
        auto result = computo::execute(script, {json(nullptr)}, debug_ctx.get());
        FAIL() << "Expected DebugBreakException to be thrown";
    } catch (const computo::DebugBreakException& e) {
        breakpoint_hit = true;
        break_location = e.get_location();
    }

    EXPECT_TRUE(breakpoint_hit);
    // Should break on the nested + operator first (inner expression evaluated first)
    EXPECT_TRUE(break_location.find("/") == 0); // Should start with "/"
}
