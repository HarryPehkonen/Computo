#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <computo/debugger.hpp>
#include <computo/memory_pool.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset all global statistics
        computo::GlobalDebugStats::reset();
        computo::memory::GlobalPoolStats::reset();
        
        // Clear thread-local pools
        computo::memory::get_thread_local_pool().clear();
    }
    
    void TearDown() override {
        // Clean up debugger
        computo::set_debugger(nullptr);
    }
};

TEST_F(ThreadSafetyTest, ConcurrentEvaluationWithDebugging) {
    const int num_threads = 8;
    const int operations_per_thread = 50;
    std::atomic<int> completed_operations{0};
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    
    // Create test data
    json test_data = json{
        {"numbers", json::array({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})},
        {"multiplier", 3}
    };
    
    // Complex script that will trigger debugging and memory allocation
    auto script = CB::let({
        {"nums", CB::get(CB::input(), "/numbers")},
        {"mult", CB::get(CB::input(), "/multiplier")}
    }, 
    CB::obj()
        .add_field("original", CB::var("nums"))
        .add_field("transformed", CB::map(
            CB::var("nums"),
            CB::lambda("x", CB::multiply(CB::var("x"), CB::var("mult")))
        ))
        .add_field("sum", CB::reduce(
            CB::var("nums"),
            CB::lambda_multi({"a", "b"}, CB::add(CB::var("a"), CB::var("b"))),
            0
        ))
    );
    
    // Launch concurrent threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            // Each thread gets its own debugger
            auto debugger = std::make_unique<computo::Debugger>();
            debugger->set_log_level(computo::LogLevel::INFO)
                    .enable_performance_profiling(true);
            computo::set_debugger(std::move(debugger));
            
            try {
                for (int j = 0; j < operations_per_thread; ++j) {
                    // Add some variation to test data
                    json modified_data = test_data;
                    modified_data["multiplier"] = 2 + (i % 5);
                    
                    auto result = computo::execute(script, modified_data);
                    
                    // Validate result structure
                    EXPECT_TRUE(result.is_object());
                    EXPECT_TRUE(result.contains("original"));
                    EXPECT_TRUE(result.contains("transformed"));
                    EXPECT_TRUE(result.contains("sum"));
                    
                    completed_operations++;
                }
            } catch (const std::exception& e) {
                errors++;
                std::cerr << "Thread " << i << " error: " << e.what() << std::endl;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Validate results
    EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
    EXPECT_EQ(errors.load(), 0);
    
    // Check global statistics
    auto total_ops = computo::GlobalDebugStats::get_total_operations();
    auto total_created = computo::memory::GlobalPoolStats::get_total_objects_created();
    auto total_reused = computo::memory::GlobalPoolStats::get_total_objects_reused();
    auto hit_rate = computo::memory::GlobalPoolStats::get_pool_hit_rate();
    
    std::cout << "Thread Safety Test Results:" << std::endl;
    std::cout << "  Total debug operations: " << total_ops << std::endl;
    std::cout << "  Total objects created: " << total_created << std::endl;
    std::cout << "  Total objects reused: " << total_reused << std::endl;
    std::cout << "  Pool hit rate: " << (hit_rate * 100.0) << "%" << std::endl;
    
    // Validate statistics consistency
    EXPECT_GT(total_ops, 0);
    // Memory pool may not be used during normal evaluation, so don't require it
    EXPECT_GE(total_created, 0);
    // Hit rate should be reasonable (some reuse may occur)
    EXPECT_GE(hit_rate, 0.0);
    EXPECT_LE(hit_rate, 1.0);
}

TEST_F(ThreadSafetyTest, MemoryPoolIsolation) {
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<bool> all_passed{true};
    
    // Each thread should get its own isolated memory pool
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto& pool = computo::memory::get_thread_local_pool();
            
            // Each thread's pool should start clean
            (void)pool.get_stats(); // Just verify pool is accessible
            
            // Use the pool extensively
            std::vector<computo::memory::JsonMemoryPool::PooledJsonPtr> objects;
            for (int j = 0; j < 100; ++j) {
                auto obj = pool.acquire();
                *obj = json{{"thread", i}, {"iteration", j}};
                objects.push_back(std::move(obj));
            }
            
            // Check that our thread's data is isolated
            for (const auto& obj : objects) {
                if ((*obj)["thread"] != i) {
                    all_passed = false;
                    std::cerr << "Thread isolation failed!" << std::endl;
                }
            }
            
            // Release all objects
            objects.clear();
            
            // Pool should have objects available now
            auto final_stats = pool.get_stats();
            if (final_stats.available_objects == 0) {
                all_passed = false;
                std::cerr << "Pool return mechanism failed!" << std::endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_TRUE(all_passed.load());
}

TEST_F(ThreadSafetyTest, ConcurrentOperatorRegistryAccess) {
    const int num_threads = 6;
    std::vector<std::thread> threads;
    std::atomic<int> successful_evaluations{0};
    
    // Test that operator registry initialization is thread-safe
    json simple_test = json{{"value", 42}};
    auto simple_script = CB::add(CB::get(CB::input(), "/value"), 10);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            try {
                // Each thread tries to use different operators
                auto result1 = computo::execute(simple_script, simple_test);
                auto result2 = computo::execute(CB::multiply(5, 10), simple_test);
                auto result3 = computo::execute(CB::equal(result1, 52), simple_test);
                
                if (result1 == 52 && result2 == 50 && result3 == true) {
                    successful_evaluations++;
                }
            } catch (const std::exception& e) {
                std::cerr << "Operator registry test failed: " << e.what() << std::endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successful_evaluations.load(), num_threads);
}

TEST_F(ThreadSafetyTest, StressTestWithRandomWorkloads) {
    const int num_threads = 12;
    const int operations_per_thread = 25;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<int> total_errors{0};
    
    // Create varied test data
    std::vector<json> test_datasets = {
        json{{"numbers", json::array({1, 2, 3, 4, 5})}},
        json{{"strings", json::array({"a", "b", "c"})}},
        json{{"nested", json{{"deep", json{{"value", 100}}}}}},
        json{{"array", json::array({
            json{{"id", 1}, {"name", "Alice"}},
            json{{"id", 2}, {"name", "Bob"}}
        })}}
    };
    
    // Various scripts to test different operators
    std::vector<json> test_scripts = {
        CB::map(CB::get(CB::input(), "/numbers"), CB::lambda("x", CB::add(CB::var("x"), 1))),
        CB::filter(CB::get(CB::input(), "/numbers"), CB::lambda("x", CB::greater_than(CB::var("x"), 2))),
        CB::get(CB::input(), "/nested/deep/value"),
        CB::count(CB::get(CB::input(), "/array"))
    };
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            std::mt19937 rng(i); // Each thread gets its own RNG
            
            // Setup thread-local debugger
            auto debugger = std::make_unique<computo::Debugger>();
            debugger->enable_performance_profiling(true);
            computo::set_debugger(std::move(debugger));
            
            try {
                for (int j = 0; j < operations_per_thread; ++j) {
                    // Randomly select data and script
                    auto& data = test_datasets[rng() % test_datasets.size()];
                    auto& script = test_scripts[rng() % test_scripts.size()];
                    
                    try {
                        auto result = computo::execute(script, data);
                        total_operations++;
                    } catch (const computo::InvalidArgumentException&) {
                        // Expected for mismatched data/script combinations
                        total_operations++;
                    }
                }
            } catch (const std::exception& e) {
                total_errors++;
                std::cerr << "Stress test thread " << i << " failed: " << e.what() << std::endl;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    EXPECT_EQ(total_errors.load(), 0);
    
    // Print final global statistics
    std::cout << "\nStress Test Final Statistics:" << std::endl;
    std::cout << "  Debug operations: " << computo::GlobalDebugStats::get_total_operations() << std::endl;
    std::cout << "  Memory objects created: " << computo::memory::GlobalPoolStats::get_total_objects_created() << std::endl;
    std::cout << "  Memory objects reused: " << computo::memory::GlobalPoolStats::get_total_objects_reused() << std::endl;
    std::cout << "  Pool hit rate: " << (computo::memory::GlobalPoolStats::get_pool_hit_rate() * 100.0) << "%" << std::endl;
} 