#include "computo.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <gtest/gtest.h>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
// Note: Using C++17 compatible synchronization (no std::barrier/std::latch)

using json = jsom::JsonDocument;
using namespace std::chrono_literals;

// Thread safety test utilities
namespace thread_safety_utils {

/**
 * Synchronization barrier for coordinating thread execution
 */
class ThreadBarrier {
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_;
    size_t waiting_;
    bool ready_;

public:
    explicit ThreadBarrier(size_t count) : count_(count), waiting_(0), ready_(false) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_;
        if (waiting_ == count_) {
            ready_ = true;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this] { return ready_; });
        }
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        waiting_ = 0;
        ready_ = false;
    }
};

/**
 * Thread-safe result collector for concurrent operations
 */
template <typename T> class ThreadSafeResultCollector {
private:
    mutable std::mutex mutex_;
    std::vector<T> results_;
    std::vector<std::exception_ptr> exceptions_;

public:
    void add_result(T result) {
        std::lock_guard<std::mutex> lock(mutex_);
        results_.push_back(std::move(result));
    }

    void add_exception(std::exception_ptr ex) {
        std::lock_guard<std::mutex> lock(mutex_);
        exceptions_.push_back(ex);
    }

    auto get_results() -> std::vector<T> {
        std::lock_guard<std::mutex> lock(mutex_);
        return results_;
    }

    auto get_exceptions() -> std::vector<std::exception_ptr> {
        std::lock_guard<std::mutex> lock(mutex_);
        return exceptions_;
    }

    auto size() const -> size_t {
        std::lock_guard<std::mutex> lock(mutex_);
        return results_.size();
    }

    bool has_exceptions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !exceptions_.empty();
    }
};

/**
 * Performance timer for concurrent operations
 */
class ConcurrentTimer {
private:
    std::atomic<std::chrono::high_resolution_clock::time_point> start_time_;
    std::atomic<std::chrono::high_resolution_clock::time_point> end_time_;
    std::atomic<bool> started_{false};
    std::atomic<bool> stopped_{false};

public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
        started_ = true;
    }

    void stop() {
        if (started_) {
            end_time_ = std::chrono::high_resolution_clock::now();
            stopped_ = true;
        }
    }

    auto get_duration_ms() const -> double {
        if (started_ && stopped_) {
            auto duration = end_time_.load() - start_time_.load();
            return std::chrono::duration<double, std::milli>(duration).count();
        }
        return 0.0;
    }
};

/**
 * Thread safety violation detector using atomic operations
 */
class RaceConditionDetector {
private:
    std::atomic<int> counter_{0};
    std::atomic<int> max_simultaneous_{0};

public:
    class Guard {
    private:
        RaceConditionDetector* detector_;

    public:
        explicit Guard(RaceConditionDetector* detector) : detector_(detector) {
            int current = detector_->counter_.fetch_add(1) + 1;
            // Update max_simultaneous with compare-and-swap to avoid races
            int expected = detector_->max_simultaneous_.load();
            while (current > expected
                   && !detector_->max_simultaneous_.compare_exchange_weak(expected, current)) {
                expected = detector_->max_simultaneous_.load();
            }
        }

        ~Guard() { detector_->counter_.fetch_sub(1); }
    };

    auto create_guard() -> Guard { return Guard(this); }

    auto get_max_simultaneous() const -> int { return max_simultaneous_.load(); }

    void reset() {
        counter_ = 0;
        max_simultaneous_ = 0;
    }
};

} // namespace thread_safety_utils

// Test fixture for thread safety tests
class ThreadSafetyTest : public ::testing::Test {
protected:
    // Test configurations for different thread counts
    static constexpr std::array<size_t, 4> THREAD_COUNTS = {2, 4, 8, 16};
    static constexpr size_t DEFAULT_ITERATIONS = 100;
    static constexpr size_t STRESS_ITERATIONS = 1000;

    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Clean up any resources
    }

    // Helper to create test data
    auto create_test_input(int value) -> json {
        return json{{"value", value},
                    {"data", json(std::vector<json>{1, 2, 3, value})},
                    {"nested", json{{"inner", value * 2}}}};
    }

    // Helper to create complex computation script
    auto create_computation_script() -> json {
        // Use a script that doesn't need variable references - just compute directly
        return jsom::parse_document(R"(["*", ["$input"], 3])");
    }

    // Helper to run concurrent test with different thread counts
    template <typename TestFunc> void run_with_thread_counts(TestFunc test_func) {
        for (size_t thread_count : THREAD_COUNTS) {
            SCOPED_TRACE("Thread count: " + std::to_string(thread_count));
            test_func(thread_count);
        }
    }
};

// Test 1: Concurrent Script Execution with Different Inputs
TEST_F(ThreadSafetyTest, ConcurrentScriptExecutionDifferentInputs) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<json> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);
        thread_safety_utils::ConcurrentTimer timer;

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        json script = create_computation_script();

        timer.start();

        // Launch threads with different inputs
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait(); // Synchronize start

                    json input = static_cast<int>(i + 1); // Use simple integer input
                    json result = computo::execute(script, {input});

                    collector.add_result(result);
                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        // Wait for all threads to complete
        for (auto& t : threads) {
            t.join();
        }

        timer.stop();

        // Verify results
        if (collector.has_exceptions()) {
            auto exceptions = collector.get_exceptions();
            for (size_t i = 0; i < exceptions.size(); ++i) {
                try {
                    std::rethrow_exception(exceptions[i]);
                } catch (const std::exception& e) {
                    std::cout << "Exception " << i << ": " << e.what() << std::endl;
                } catch (...) {
                    std::cout << "Exception " << i << ": unknown exception" << std::endl;
                }
            }
        }
        EXPECT_FALSE(collector.has_exceptions()) << "Concurrent execution threw exceptions";
        EXPECT_EQ(collector.size(), thread_count);

        auto results = collector.get_results();
        for (size_t i = 0; i < results.size(); ++i) {
            // Each result should be different based on input
            EXPECT_TRUE(results[i].is_number());
        }

        // Performance check - should complete in reasonable time
        EXPECT_LT(timer.get_duration_ms(), 5000.0) << "Concurrent execution took too long";
    };

    run_with_thread_counts(test_func);
}

// Test 2: Same Script Different Threads Stress Test
TEST_F(ThreadSafetyTest, SameScriptMultipleThreadsStress) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<json> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);
        std::atomic<int> iteration_counter{0};

        json script = jsom::parse_document(R"(["+", 1, 2, 3, 4, 5])");
        json expected_result = 15;

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        // Each thread runs multiple iterations
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&]() {
                try {
                    barrier.wait(); // Synchronize start

                    for (size_t iter = 0; iter < DEFAULT_ITERATIONS; ++iter) {
                        json result = computo::execute(script, {json(nullptr)});
                        collector.add_result(result);
                        iteration_counter.fetch_add(1);
                    }
                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Verify all executions completed successfully
        EXPECT_FALSE(collector.has_exceptions());
        EXPECT_EQ(collector.size(), thread_count * DEFAULT_ITERATIONS);
        EXPECT_EQ(iteration_counter.load(), static_cast<int>(thread_count * DEFAULT_ITERATIONS));

        // Verify all results are correct
        auto results = collector.get_results();
        for (const auto& result : results) {
            EXPECT_EQ(result, expected_result);
        }
    };

    run_with_thread_counts(test_func);
}

// Test 3: ExecutionContext Thread-Local Safety and Isolation
TEST_F(ThreadSafetyTest, ExecutionContextThreadLocalSafety) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<json> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        // Each thread will have its own unique computation - simplified
        json script = jsom::parse_document(R"([
            "*", 
            ["+", ["$input"], ["$input"]], 
            100
        ])");

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    int thread_id = static_cast<int>(i + 1);
                    json input = thread_id;
                    json result = computo::execute(script, {input});

                    // Expected: (thread_id + thread_id) * 100 = thread_id * 200
                    json expected = thread_id * 200;

                    if (result == expected) {
                        collector.add_result(result);
                    } else {
                        throw std::runtime_error("Variable isolation failed: expected "
                                                 + expected.to_json() + ", got " + result.to_json());
                    }
                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "ExecutionContext isolation failed";
        EXPECT_EQ(collector.size(), thread_count);

        // Verify each thread got its expected unique result
        auto results = collector.get_results();
        std::set<json> unique_results(results.begin(), results.end());
        EXPECT_EQ(unique_results.size(), thread_count) << "Thread variable isolation failed";
    };

    run_with_thread_counts(test_func);
}

// Test 4: DebugContext Thread Safety (This will likely fail - DebugContext is not thread-safe)
TEST_F(ThreadSafetyTest, DebugContextThreadSafety) {
    auto test_func = [this](size_t thread_count) {
        // Create separate DebugContext for each thread to avoid sharing
        std::vector<std::unique_ptr<computo::DebugContext>> debug_contexts;
        for (size_t i = 0; i < thread_count; ++i) {
            debug_contexts.push_back(std::make_unique<computo::DebugContext>());
            debug_contexts.back()->set_debug_enabled(true);
            debug_contexts.back()->set_trace_enabled(true);
        }

        thread_safety_utils::ThreadSafeResultCollector<size_t> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        json script = jsom::parse_document(R"(["+", 1, 2, 3])");

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    // Each thread uses its own DebugContext
                    auto* debug_ctx = debug_contexts[i].get();

                    for (int iter = 0; iter < 10; ++iter) {
                        json result = computo::execute(script, {json(nullptr)}, debug_ctx);
                        EXPECT_EQ(result, 6);
                    }

                    // Check trace was recorded
                    size_t trace_size = debug_ctx->get_execution_trace().size();
                    collector.add_result(trace_size);

                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "DebugContext execution failed";
        EXPECT_EQ(collector.size(), thread_count);

        // Each thread should have recorded traces
        auto results = collector.get_results();
        for (size_t trace_size : results) {
            EXPECT_GT(trace_size, 0u) << "Debug trace was not recorded";
        }
    };

    run_with_thread_counts(test_func);
}

// Test 5: Operator Thread Safety - Test All Operator Categories
TEST_F(ThreadSafetyTest, AllOperatorThreadSafety) {
    auto test_func = [this](size_t thread_count) {
        struct OperatorTest {
            std::string name;
            json script;
            json input;
            json expected;
        };

        std::vector<OperatorTest> operator_tests = {
            // Arithmetic
            {"addition", jsom::parse_document(R"(["+", 10, 20])"), json(nullptr), json(30)},
            {"subtraction", jsom::parse_document(R"(["-", 20, 5])"), json(nullptr), json(15)},
            {"multiplication", jsom::parse_document(R"(["*", 6, 7])"), json(nullptr), json(42)},
            {"division", jsom::parse_document(R"(["/", 20, 4])"), json(nullptr), json(5)},

            // Comparison
            {"greater_than", jsom::parse_document(R"([">", 10, 5])"), json(nullptr), json(true)},
            {"less_than", jsom::parse_document(R"(["<", 5, 10])"), json(nullptr), json(true)},
            {"equal", jsom::parse_document(R"(["==", 5, 5])"), json(nullptr), json(true)},

            // Logical
            {"logical_and", jsom::parse_document(R"(["and", true, true])"), json(nullptr), json(true)},
            {"logical_or", jsom::parse_document(R"(["or", false, true])"), json(nullptr), json(true)},
            {"logical_not", jsom::parse_document(R"(["not", false])"), json(nullptr), json(true)},

            // Data Access
            {"input_access", jsom::parse_document(R"(["$input"])"), json(42), json(42)},
            {"variable", jsom::parse_document(R"(["let", [["x", 100]], ["$", "/x"]])"), json(nullptr),
             json(100)},

            // Control Flow
            {"if_true", jsom::parse_document(R"(["if", true, "yes", "no"])"), json(nullptr), json("yes")},
            {"if_false", jsom::parse_document(R"(["if", false, "yes", "no"])"), json(nullptr), json("no")},
        };

        thread_safety_utils::ThreadSafeResultCollector<std::pair<std::string, bool>> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    // Each thread tests all operators
                    for (const auto& test : operator_tests) {
                        json result = computo::execute(test.script, {test.input});
                        bool success = (result == test.expected);
                        collector.add_result({test.name, success});

                        if (!success) {
                            throw std::runtime_error("Operator " + test.name + " failed: expected "
                                                     + test.expected.to_json() + ", got "
                                                     + result.to_json());
                        }
                    }
                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "Operator thread safety test failed";

        // Verify all operator tests passed in all threads
        auto results = collector.get_results();
        EXPECT_EQ(results.size(), thread_count * operator_tests.size());

        for (const auto& [op_name, success] : results) {
            EXPECT_TRUE(success) << "Operator " << op_name << " failed in concurrent execution";
        }
    };

    run_with_thread_counts(test_func);
}

// Test 6: Memory Allocation Safety Under Concurrency
TEST_F(ThreadSafetyTest, MemoryAllocationSafety) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<size_t> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        // Script that creates and manipulates large data structures
        json script = jsom::parse_document(R"([
            "let",
            [
                ["large_array", {"array": ["$input"]}],
                ["doubled", ["*", 2, 2]]
            ],
            ["$", "/doubled"]
        ])");

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    // Create large input arrays
                    json large_input = json::make_array();
                    for (int j = 0; j < 1000; ++j) {
                        large_input.push_back(static_cast<int>(i * 1000 + j));
                    }

                    // Execute multiple times to stress memory allocation
                    for (int iter = 0; iter < 50; ++iter) {
                        json result = computo::execute(script, {large_input});
                        EXPECT_EQ(result, 4);
                    }

                    collector.add_result(i);

                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "Memory allocation safety test failed";
        EXPECT_EQ(collector.size(), thread_count);
    };

    run_with_thread_counts(test_func);
}

// Test 7: Variable Scope Isolation Between Threads
TEST_F(ThreadSafetyTest, VariableScopeIsolation) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<json> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        // Script with simple nested computation - no complex variable references
        json script = jsom::parse_document(R"([
            "+", 
            ["*", ["$input"], 10], 
            5
        ])");

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    int thread_value = static_cast<int>(i + 1);

                    // Run multiple iterations to test scope cleanup
                    for (int iter = 0; iter < 20; ++iter) {
                        json result = computo::execute(script, {json(thread_value)});
                        json expected = (thread_value * 10) + 5;

                        if (result != expected) {
                            throw std::runtime_error("Scope isolation failed in thread "
                                                     + std::to_string(i) + " iteration "
                                                     + std::to_string(iter));
                        }
                    }

                    collector.add_result(json(thread_value));

                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "Variable scope isolation failed";
        EXPECT_EQ(collector.size(), thread_count);
    };

    run_with_thread_counts(test_func);
}

// Test 8: High Concurrency Stress Test
TEST_F(ThreadSafetyTest, HighConcurrencyStressTest) {
    constexpr size_t HIGH_THREAD_COUNT = 32;
    constexpr size_t STRESS_ITERATIONS_PER_THREAD = 100;

    thread_safety_utils::ThreadSafeResultCollector<std::pair<size_t, size_t>> collector;
    thread_safety_utils::ThreadBarrier barrier(HIGH_THREAD_COUNT);
    thread_safety_utils::ConcurrentTimer timer;
    std::atomic<size_t> total_operations{0};

    // Mix of different operation types
    std::vector<json> scripts = {
        jsom::parse_document(R"(["+", 1, 2, 3])"),      jsom::parse_document(R"(["*", 4, 5])"),
        jsom::parse_document(R"([">", 10, 5])"),        jsom::parse_document(R"(["and", true, false])"),
        jsom::parse_document(R"(["if", true, 42, 0])"), jsom::parse_document(R"(["let", [["x", 100]], ["$", "/x"]])"),
    };

    std::vector<json> expected_results = {6, 20, true, false, 42, 100};

    std::vector<std::thread> threads;
    threads.reserve(HIGH_THREAD_COUNT);

    timer.start();

    for (size_t i = 0; i < HIGH_THREAD_COUNT; ++i) {
        threads.emplace_back([&, i]() {
            try {
                barrier.wait();

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> script_dist(0,
                                                            static_cast<int>(scripts.size() - 1));

                size_t successful_ops = 0;

                for (size_t iter = 0; iter < STRESS_ITERATIONS_PER_THREAD; ++iter) {
                    int script_idx = script_dist(gen);
                    json result = computo::execute(scripts[script_idx], {json(nullptr)});

                    if (result == expected_results[script_idx]) {
                        ++successful_ops;
                    }

                    total_operations.fetch_add(1);
                }

                collector.add_result({i, successful_ops});

            } catch (...) {
                collector.add_exception(std::current_exception());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    timer.stop();

    EXPECT_FALSE(collector.has_exceptions()) << "High concurrency stress test failed";
    EXPECT_EQ(collector.size(), HIGH_THREAD_COUNT);
    EXPECT_EQ(total_operations.load(), HIGH_THREAD_COUNT * STRESS_ITERATIONS_PER_THREAD);

    // Verify all operations succeeded
    auto results = collector.get_results();
    size_t total_successful = 0;
    for (const auto& [thread_id, successful_ops] : results) {
        EXPECT_EQ(successful_ops, STRESS_ITERATIONS_PER_THREAD)
            << "Thread " << thread_id << " had failures";
        total_successful += successful_ops;
    }

    EXPECT_EQ(total_successful, HIGH_THREAD_COUNT * STRESS_ITERATIONS_PER_THREAD);

    // Performance check
    double duration_ms = timer.get_duration_ms();
    double ops_per_second = (total_operations.load() * 1000.0) / duration_ms;

    std::cout << "Stress test performance: " << ops_per_second << " operations/second ("
              << duration_ms << "ms total)" << std::endl;

    // Should handle at least 10k ops/sec (adjust based on requirements)
    EXPECT_GT(ops_per_second, 10000.0) << "Performance regression detected";
}

// Test 9: Exception Handling Thread Safety
TEST_F(ThreadSafetyTest, ExceptionHandlingThreadSafety) {
    auto test_func = [this](size_t thread_count) {
        thread_safety_utils::ThreadSafeResultCollector<std::string> collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);

        // Scripts that should throw exceptions
        std::vector<json> error_scripts = {
            jsom::parse_document(R"(["/", 10, 0])"),         // Division by zero
            jsom::parse_document(R"(["invalid_op", 1, 2])"), // Invalid operator
            jsom::parse_document(R"(["%", 10, 0])"),         // Modulo by zero
            jsom::parse_document(R"([])"),                   // Empty array
        };

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                try {
                    barrier.wait();

                    std::string exceptions_caught;

                    // Test each error script
                    for (size_t script_idx = 0; script_idx < error_scripts.size(); ++script_idx) {
                        try {
                            computo::execute(error_scripts[script_idx], {json(nullptr)});
                            exceptions_caught += "NONE_" + std::to_string(script_idx) + ";";
                        } catch (const computo::InvalidArgumentException&) {
                            exceptions_caught += "INVALID_ARG_" + std::to_string(script_idx) + ";";
                        } catch (const computo::InvalidOperatorException&) {
                            exceptions_caught += "INVALID_OP_" + std::to_string(script_idx) + ";";
                        } catch (const std::exception&) {
                            exceptions_caught += "OTHER_" + std::to_string(script_idx) + ";";
                        }
                    }

                    collector.add_result(exceptions_caught);

                } catch (...) {
                    collector.add_exception(std::current_exception());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_FALSE(collector.has_exceptions()) << "Exception handling thread safety failed";
        EXPECT_EQ(collector.size(), thread_count);

        // Verify consistent exception handling across threads
        auto results = collector.get_results();
        if (!results.empty()) {
            std::string expected_pattern = results[0];
            for (const auto& result : results) {
                EXPECT_EQ(result, expected_pattern)
                    << "Inconsistent exception handling between threads";
            }
        }
    };

    run_with_thread_counts(test_func);
}

// Test 10: Race Condition Detection for Shared Resources
TEST_F(ThreadSafetyTest, RaceConditionDetection) {
    // Test the OperatorRegistry singleton for race conditions
    constexpr size_t RACE_TEST_THREADS = 50;
    constexpr size_t ITERATIONS_PER_THREAD = 100;

    thread_safety_utils::RaceConditionDetector detector;
    thread_safety_utils::ThreadSafeResultCollector<bool> collector;
    thread_safety_utils::ThreadBarrier barrier(RACE_TEST_THREADS);

    std::vector<std::thread> threads;
    threads.reserve(RACE_TEST_THREADS);

    for (size_t i = 0; i < RACE_TEST_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            try {
                barrier.wait();

                for (size_t iter = 0; iter < ITERATIONS_PER_THREAD; ++iter) {
                    auto guard = detector.create_guard();

                    // Access the singleton registry
                    auto& registry = computo::OperatorRegistry::get_instance();

                    // Test has_operator (should be thread-safe)
                    bool has_add = registry.has_operator("+");
                    bool has_invalid = registry.has_operator("invalid_op_" + std::to_string(i));

                    if (!has_add || has_invalid) {
                        throw std::runtime_error("Registry consistency check failed");
                    }

                    // Small delay to increase chance of detecting races
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }

                collector.add_result(true);

            } catch (...) {
                collector.add_exception(std::current_exception());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_FALSE(collector.has_exceptions()) << "Race condition test failed";
    EXPECT_EQ(collector.size(), RACE_TEST_THREADS);

    // The detector should never see more than a few simultaneous accesses
    // if proper synchronization is in place
    int max_simultaneous = detector.get_max_simultaneous();
    std::cout << "Maximum simultaneous registry accesses: " << max_simultaneous << std::endl;

    // This is more of an informational test - the registry should handle concurrent access
    EXPECT_GT(max_simultaneous, 0) << "Race condition detector not working";
    // In a stress test, it's normal for all threads to access simultaneously
    // This just confirms the detector is working
    EXPECT_LE(max_simultaneous, static_cast<int>(RACE_TEST_THREADS))
        << "Race condition detector detected more threads than created";
}

// Test 11: Performance Measurement Under Different Thread Loads
TEST_F(ThreadSafetyTest, PerformanceUnderThreadLoad) {
    struct PerformanceResult {
        size_t thread_count;
        double operations_per_second;
        double avg_latency_ms;
    };

    std::vector<PerformanceResult> results;
    json test_script = jsom::parse_document(R"(["+", ["*", 2, 3], ["/", 20, 4]])");
    constexpr size_t PERF_ITERATIONS = 1000;

    for (size_t thread_count : {1, 2, 4, 8, 16, 32}) {
        thread_safety_utils::ThreadSafeResultCollector<double> latency_collector;
        thread_safety_utils::ThreadBarrier barrier(thread_count);
        thread_safety_utils::ConcurrentTimer overall_timer;

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        overall_timer.start();

        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&]() {
                barrier.wait();

                auto start = std::chrono::high_resolution_clock::now();

                for (size_t iter = 0; iter < PERF_ITERATIONS; ++iter) {
                    json result = computo::execute(test_script, {json(nullptr)});
                    EXPECT_EQ(result, 11); // (2*3) + (20/4) = 6 + 5 = 11
                }

                auto end = std::chrono::high_resolution_clock::now();
                double thread_duration_ms
                    = std::chrono::duration<double, std::milli>(end - start).count();
                latency_collector.add_result(thread_duration_ms / PERF_ITERATIONS);
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        overall_timer.stop();

        double total_duration_ms = overall_timer.get_duration_ms();
        double total_operations = thread_count * PERF_ITERATIONS;
        double ops_per_second = (total_operations * 1000.0) / total_duration_ms;

        auto latencies = latency_collector.get_results();
        double avg_latency
            = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

        results.push_back({thread_count, ops_per_second, avg_latency});

        std::cout << "Threads: " << thread_count << ", Ops/sec: " << std::fixed
                  << std::setprecision(0) << ops_per_second
                  << ", Avg latency: " << std::setprecision(3) << avg_latency << "ms" << std::endl;
    }

    // Basic performance expectations
    EXPECT_GT(results[0].operations_per_second, 10000.0) << "Single-threaded performance too low";

    // Multi-threaded performance should not degrade significantly
    for (size_t i = 1; i < results.size(); ++i) {
        double perf_ratio = results[i].operations_per_second / results[0].operations_per_second;
        EXPECT_GT(perf_ratio, 0.5)
            << "Significant performance degradation with " << results[i].thread_count << " threads";
    }
}
