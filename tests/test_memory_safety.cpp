#include "computo.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#if defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

// Memory usage monitoring utilities
class MemoryMonitor {
private:
    size_t initial_rss_kb_{0};
    size_t peak_rss_kb_{0};

public:
    MemoryMonitor() { capture_initial_memory(); }

    void capture_initial_memory() {
#if defined(__linux__)
        initial_rss_kb_ = get_memory_usage_kb();
        peak_rss_kb_ = initial_rss_kb_;
#endif
    }

    void update_peak_memory() {
#if defined(__linux__)
        size_t current = get_memory_usage_kb();
        if (current > peak_rss_kb_) {
            peak_rss_kb_ = current;
        }
#endif
    }

    [[nodiscard]] auto get_memory_usage_kb() const -> size_t {
#if defined(__linux__)
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::string kb_value = line.substr(6);
                // Remove leading/trailing whitespace and "kB"
                size_t start = kb_value.find_first_not_of(" \t");
                size_t end = kb_value.find(" kB");
                if (start != std::string::npos && end != std::string::npos) {
                    return std::stoull(kb_value.substr(start, end - start));
                }
            }
        }
#endif
        return 0;
    }

    [[nodiscard]] auto get_memory_increase_kb() const -> size_t {
        size_t current = get_memory_usage_kb();
        return current > initial_rss_kb_ ? current - initial_rss_kb_ : 0;
    }

    [[nodiscard]] auto get_peak_memory_increase_kb() const -> size_t {
        return peak_rss_kb_ > initial_rss_kb_ ? peak_rss_kb_ - initial_rss_kb_ : 0;
    }
};

// Test fixture for memory safety tests
class MemorySafetyTest : public ::testing::Test {
protected:
    MemoryMonitor memory_monitor_;
    computo::DebugContext debug_ctx_;

    void SetUp() override {
        memory_monitor_.capture_initial_memory();
        debug_ctx_.reset();
    }

    void TearDown() override {
        // Check for significant memory leaks
        memory_monitor_.update_peak_memory();
        size_t memory_increase = memory_monitor_.get_memory_increase_kb();

        // Allow for some reasonable memory growth during tests
        // This threshold may need adjustment based on actual usage
        constexpr size_t MAX_REASONABLE_GROWTH_KB = 10240; // 10MB

        if (memory_increase > MAX_REASONABLE_GROWTH_KB) {
            ADD_FAILURE() << "Potential memory leak detected: " << memory_increase
                          << " KB memory increase after test completion";
        }
    }

    // Helper to create large JSON arrays
    [[nodiscard]] auto create_large_array(size_t size) -> json {
        json array = json::array();
        for (size_t i = 0; i < size; ++i) {
            array.push_back(static_cast<int>(i));
        }
        return array;
    }

    // Helper to create large JSON objects
    [[nodiscard]] auto create_large_object(size_t size) -> json {
        json obj = json::object();
        for (size_t i = 0; i < size; ++i) {
            obj["key" + std::to_string(i)] = static_cast<int>(i);
        }
        return obj;
    }

    // Helper to create nested JSON structure
    [[nodiscard]] auto create_nested_structure(size_t depth, size_t breadth) -> json {
        if (depth == 0) {
            return json(42);
        }

        json obj = json::object();
        for (size_t i = 0; i < breadth; ++i) {
            obj["child" + std::to_string(i)] = create_nested_structure(depth - 1, breadth);
        }
        return obj;
    }
};

// Test 1: Large Array Operations - Map
TEST_F(MemorySafetyTest, LargeArrayMapOperation) {
    const size_t ARRAY_SIZE = 1000; // Smaller size for faster testing
    json large_array = create_large_array(ARRAY_SIZE);

    // Test map operation on large array - add 1 to each element
    // Input array format: {"array": [0, 1, 2, ...]}
    json script = json::parse(R"(["map", ["$input"], ["lambda", ["x"], ["+", ["$", "/x"], 1]]])");

    memory_monitor_.update_peak_memory();
    json result = computo::execute(script, {large_array});
    memory_monitor_.update_peak_memory();

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result["array"].is_array());
    EXPECT_EQ(result["array"].size(), ARRAY_SIZE);

    // Verify results are correct (each element should be original + 1)
    for (size_t i = 0; i < std::min(ARRAY_SIZE, size_t(10)); ++i) {
        EXPECT_EQ(result["array"][i], static_cast<int>(i) + 1);
    }
}

// Test 2: Large Array Operations - Filter
TEST_F(MemorySafetyTest, LargeArrayFilterOperation) {
    const size_t ARRAY_SIZE = 1000; // Smaller size for faster testing
    json large_array = create_large_array(ARRAY_SIZE);

    // Filter even numbers - use == for equality comparison and correct array syntax
    // Input array format: {"array": [0, 1, 2, ...]}
    json script = json::parse(
        R"(["filter", ["$input"], ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]])");

    memory_monitor_.update_peak_memory();
    json result = computo::execute(script, {large_array});
    memory_monitor_.update_peak_memory();

    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result["array"].is_array());
    EXPECT_EQ(result["array"].size(), ARRAY_SIZE / 2); // Half should be even

    // Verify all results are even
    for (const auto& item : result["array"]) {
        EXPECT_EQ(item.get<int>() % 2, 0);
    }
}

// Test 3: Simple Memory Stress with Basic Operations
TEST_F(MemorySafetyTest, BasicOperationsMemoryStress) {
    const size_t ITERATIONS = 1000;

    memory_monitor_.update_peak_memory();

    // Test repeated arithmetic operations that should not leak memory
    for (size_t i = 0; i < ITERATIONS; ++i) {
        json script = json::parse(R"(["+", 1, 2, 3, 4, 5])");
        json result = computo::execute(script, {json(nullptr)});
        EXPECT_EQ(result, 15);

        if (i % 100 == 0) {
            memory_monitor_.update_peak_memory();
        }
    }

    memory_monitor_.update_peak_memory();
}

// Test 4: Input Processing Memory Test
TEST_F(MemorySafetyTest, InputProcessingMemoryTest) {
    const size_t ARRAY_SIZE = 1000;
    json large_input_array = create_large_array(ARRAY_SIZE);

    memory_monitor_.update_peak_memory();

    // Test simple input access operations
    json script1 = json::parse(R"(["$input"])");
    json result1 = computo::execute(script1, {large_input_array});
    EXPECT_EQ(result1, large_input_array);

    // Test input with path access using JSON Pointer syntax
    json large_object = json::object();
    large_object["data"] = large_input_array;
    large_object["count"] = static_cast<int>(ARRAY_SIZE);

    json script2 = json::parse(R"(["let", {"obj": ["$input"]}, ["$", "/obj/count"]])");
    json result2 = computo::execute(script2, {large_object});
    EXPECT_EQ(result2, static_cast<int>(ARRAY_SIZE));

    memory_monitor_.update_peak_memory();
}

// Test 5: Multiple Input Processing
TEST_F(MemorySafetyTest, MultipleInputProcessing) {
    const size_t NUM_INPUTS = 100;
    std::vector<json> inputs;
    inputs.reserve(NUM_INPUTS);

    // Create multiple simple inputs
    for (size_t i = 0; i < NUM_INPUTS; ++i) {
        inputs.push_back(static_cast<int>(i));
    }

    // Process all inputs using $inputs operator
    json script = json::parse(R"(["$inputs"])");

    memory_monitor_.update_peak_memory();
    json result = computo::execute(script, inputs);
    memory_monitor_.update_peak_memory();

    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), NUM_INPUTS);

    // Verify results
    for (size_t i = 0; i < NUM_INPUTS; ++i) {
        EXPECT_EQ(result[i], static_cast<int>(i));
    }
}

// Test 6: Variable Memory Management
TEST_F(MemorySafetyTest, VariableMemoryManagement) {
    const size_t LOOP_COUNT = 100;

    memory_monitor_.update_peak_memory();

    // Test variable creation and access with let operator
    for (size_t i = 0; i < LOOP_COUNT; ++i) {
        json script = json::parse(R"(["let", [["x", 42], ["y", "hello"]], ["+", ["$", "/x"], 1]])");
        json result = computo::execute(script, {json(nullptr)});
        EXPECT_EQ(result, 43);

        if (i % 10 == 0) {
            memory_monitor_.update_peak_memory();
        }
    }

    memory_monitor_.update_peak_memory();
}

// Test 7: Exception Handling Memory Cleanup
TEST_F(MemorySafetyTest, ExceptionHandlingCleanup) {
    memory_monitor_.update_peak_memory();

    // Test operations that should throw exceptions - using simpler operations
    for (int i = 0; i < 10; ++i) {
        EXPECT_THROW(
            {
                // Division by zero should throw
                json script = json::parse(R"(["/", 10, 0])");
                computo::execute(script, {json(nullptr)});
            },
            computo::InvalidArgumentException);

        EXPECT_THROW(
            {
                // Invalid operator should throw
                json script = json::parse(R"(["invalid_operator", 1, 2])");
                computo::execute(script, {json(nullptr)});
            },
            computo::InvalidOperatorException);

        EXPECT_THROW(
            {
                // Invalid argument to modulo should throw
                json script = json::parse(R"(["%", 10, 0])");
                computo::execute(script, {json(nullptr)});
            },
            computo::InvalidArgumentException);
    }

    memory_monitor_.update_peak_memory();

    // After exceptions, memory should be properly cleaned up
    // The TearDown() method will check for memory leaks
}

// Test 8: Debug Context Memory Management
TEST_F(MemorySafetyTest, DebugContextMemoryManagement) {
    // Enable debug tracing which stores execution steps
    debug_ctx_.set_debug_enabled(true);
    debug_ctx_.set_trace_enabled(true);

    memory_monitor_.update_peak_memory();

    // Execute multiple operations with debug tracing
    for (int i = 0; i < 50; ++i) {
        json script = json::parse(R"(["+", 1, 2, 3])");
        json result = computo::execute(script, {json(nullptr)}, &debug_ctx_);
        EXPECT_EQ(result, 6);
    }

    memory_monitor_.update_peak_memory();

    // Check that trace was recorded
    const auto& trace = debug_ctx_.get_execution_trace();
    EXPECT_FALSE(trace.empty());

    // Reset debug context to clean up trace data
    debug_ctx_.reset();

    memory_monitor_.update_peak_memory();
}

// Test 9: Large Object Operations
TEST_F(MemorySafetyTest, LargeObjectOperations) {
    const size_t OBJECT_SIZE = 100;
    json large_object = create_large_object(OBJECT_SIZE);

    memory_monitor_.update_peak_memory();

    // Test simple object access using JSON Pointer syntax
    json access_script = json::parse(R"(["let", {"obj": ["$input"]}, ["$", "/obj/key1"]])");
    json access_result = computo::execute(access_script, {large_object});
    EXPECT_EQ(access_result, 1);

    memory_monitor_.update_peak_memory();
}

// Test 10: Nested Structure Operations
TEST_F(MemorySafetyTest, NestedStructureOperations) {
    // Create a simple nested structure
    json nested_obj = json::object();
    nested_obj["level1"] = json::object();
    nested_obj["level1"]["level2"] = json::object();
    nested_obj["level1"]["level2"]["value"] = 42;

    memory_monitor_.update_peak_memory();

    // Test nested property access using JSON Pointer syntax
    json script = json::parse(R"(["let", {"obj": ["$input"]}, ["$", "/obj/level1/level2/value"]])");

    json result = computo::execute(script, {nested_obj});
    EXPECT_EQ(result, 42);

    memory_monitor_.update_peak_memory();
}

// Test 11: Memory Stress Test - Mixed Operations
TEST_F(MemorySafetyTest, MemoryStressTestMixedOperations) {
    const size_t ITERATIONS = 200;

    memory_monitor_.update_peak_memory();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        // Mix of different operation types
        json arith_result
            = computo::execute(json::parse(R"(["+", 1, 2, 3, 4, 5])"), {json(nullptr)});
        EXPECT_EQ(arith_result, 15);

        json comp_result = computo::execute(json::parse(R"([">", 10, 5])"), {json(nullptr)});
        EXPECT_EQ(comp_result, true);

        json input_result = computo::execute(json::parse(R"(["$input"])"), {static_cast<int>(i)});
        EXPECT_EQ(input_result, static_cast<int>(i));

        // Update memory tracking periodically
        if (i % 50 == 0) {
            memory_monitor_.update_peak_memory();
        }
    }

    memory_monitor_.update_peak_memory();
}

// Test 12: ExecutionContext Variable Scope Memory Management
TEST_F(MemorySafetyTest, ExecutionContextScopeMemory) {
    const size_t NUM_ITERATIONS = 100;

    memory_monitor_.update_peak_memory();

    // Test creating and destroying execution contexts in a loop
    for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
        // Create contexts with let expressions (automatic cleanup)
        json script = json::parse(R"(["let", [["x", 42], ["y", "test"]], ["*", ["$", "/x"], 2]])");
        json result = computo::execute(script, {json(nullptr)});
        EXPECT_EQ(result, 84);

        // Context and variables should be automatically cleaned up
        if (i % 20 == 0) {
            memory_monitor_.update_peak_memory();
        }
    }

    memory_monitor_.update_peak_memory();
}
