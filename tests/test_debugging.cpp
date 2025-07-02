#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class DebuggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"value", 42}, {"name", "test"}};
        computo::set_debugger(nullptr);
    }
    
    void TearDown() override {
        computo::set_debugger(nullptr);
    }
    
    json input_data;
};

TEST_F(DebuggerTest, BasicConfiguration) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    EXPECT_FALSE(debugger->is_tracing_enabled());
    EXPECT_FALSE(debugger->is_profiling_enabled());
    EXPECT_FALSE(debugger->is_interactive_enabled());
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::INFO);
    
    debugger->set_log_level(computo::LogLevel::DEBUG)
             .enable_execution_trace(true)
             .enable_performance_profiling(true)
             .set_interactive_mode(true);
    
    EXPECT_TRUE(debugger->is_tracing_enabled());
    EXPECT_TRUE(debugger->is_profiling_enabled());
    EXPECT_TRUE(debugger->is_interactive_enabled());
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::DEBUG);
}

TEST_F(DebuggerTest, ExecutionIntegration) {
    auto debugger = std::make_unique<computo::Debugger>();
    debugger->enable_execution_trace(true)
             .enable_performance_profiling(true);
    
    computo::set_debugger(std::move(debugger));
    
    EXPECT_NE(computo::get_debugger(), nullptr);
    
    auto script = CB::add(10, 32);
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result, 42);
    
    auto* active_debugger = computo::get_debugger();
    ASSERT_NE(active_debugger, nullptr);
    
    std::string execution_report = active_debugger->get_execution_report();
    EXPECT_FALSE(execution_report.empty());
    
    std::string performance_report = active_debugger->get_performance_report();
    EXPECT_FALSE(performance_report.empty());
}

TEST_F(DebuggerTest, LogLevelConfiguration) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    debugger->set_log_level(computo::LogLevel::ERROR);
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::ERROR);
    
    debugger->set_log_level(computo::LogLevel::WARNING);
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::WARNING);
    
    debugger->set_log_level(computo::LogLevel::INFO);
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::INFO);
    
    debugger->set_log_level(computo::LogLevel::DEBUG);
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::DEBUG);
    
    debugger->set_log_level(computo::LogLevel::VERBOSE);
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::VERBOSE);
}

TEST_F(DebuggerTest, BreakpointConfiguration) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    EXPECT_NO_THROW(debugger->set_breakpoint_on_operator("add"));
    EXPECT_NO_THROW(debugger->set_breakpoint_on_operator("multiply"));
    EXPECT_NO_THROW(debugger->set_breakpoint_on_path("/test/path"));
    EXPECT_NO_THROW(debugger->set_breakpoint_on_operator("map"));
}

TEST_F(DebuggerTest, VariableWatching) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    debugger->add_variable_watch("test_var")
             .add_variable_watch("another_var");
    
    // Variable watching should work without needing to check tracing status
    // (The debugger enables variable tracking internally, which is separate from execution tracing)
    EXPECT_NO_THROW(debugger->add_variable_watch("third_var"));
}

TEST_F(DebuggerTest, SlowOperationThreshold) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    auto threshold = std::chrono::milliseconds(50);
    debugger->set_slow_operation_threshold(threshold);
    
    // Setting threshold should work (profiling needs to be explicitly enabled)
    EXPECT_NO_THROW(debugger->set_slow_operation_threshold(std::chrono::milliseconds(100)));
    
    // When profiling is enabled, the threshold should be used
    debugger->enable_performance_profiling(true);
    EXPECT_TRUE(debugger->is_profiling_enabled());
}

TEST_F(DebuggerTest, ComplexOperationDebugging) {
    auto debugger = std::make_unique<computo::Debugger>();
    debugger->enable_execution_trace(true)
             .enable_performance_profiling(true)
             .set_log_level(computo::LogLevel::VERBOSE);
    
    computo::set_debugger(std::move(debugger));
    
    auto script = CB::add(
        CB::multiply(2, 3),
        CB::add(4, CB::multiply(5, 6))
    );
    
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result, 40); // (2*3) + (4 + (5*6)) = 6 + 34 = 40
    
    auto* active_debugger = computo::get_debugger();
    ASSERT_NE(active_debugger, nullptr);
    
    std::string execution_report = active_debugger->get_execution_report();
    EXPECT_TRUE(execution_report.find("*") != std::string::npos);
    EXPECT_TRUE(execution_report.find("+") != std::string::npos);
}

TEST_F(DebuggerTest, ErrorHandlingWithDebugging) {
    auto debugger = std::make_unique<computo::Debugger>();
    debugger->enable_execution_trace(true)
             .set_log_level(computo::LogLevel::VERBOSE);
    
    computo::set_debugger(std::move(debugger));
    
    auto script = CB::divide(42, 0);
    
    EXPECT_THROW({
        computo::execute(script, input_data);
    }, computo::ComputoException);
    
    auto* active_debugger = computo::get_debugger();
    ASSERT_NE(active_debugger, nullptr);
    
    std::string execution_report = active_debugger->get_execution_report();
    EXPECT_FALSE(execution_report.empty());
}

TEST_F(DebuggerTest, DebuggerCleanup) {
    auto debugger = std::make_unique<computo::Debugger>();
    computo::set_debugger(std::move(debugger));
    
    EXPECT_NE(computo::get_debugger(), nullptr);
    
    computo::set_debugger(nullptr);
    
    EXPECT_EQ(computo::get_debugger(), nullptr);
    
    auto script = CB::add(21, 21);
    json result = computo::execute(script, input_data);
    EXPECT_EQ(result, 42);
}

TEST_F(DebuggerTest, FluentInterface) {
    auto debugger = std::make_unique<computo::Debugger>();
    
    auto& result_ref = debugger->set_log_level(computo::LogLevel::VERBOSE)
                                .enable_execution_trace(true)
                                .enable_performance_profiling(true)
                                .enable_variable_tracking(true)
                                .set_interactive_mode(false)
                                .set_breakpoint_on_operator("test")
                                .add_variable_watch("test_var");
    
    EXPECT_EQ(&result_ref, debugger.get());
    EXPECT_EQ(debugger->get_log_level(), computo::LogLevel::VERBOSE);
    EXPECT_TRUE(debugger->is_tracing_enabled());
    EXPECT_TRUE(debugger->is_profiling_enabled());
    EXPECT_FALSE(debugger->is_interactive_enabled());
}
