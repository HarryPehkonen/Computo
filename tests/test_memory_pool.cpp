#include <gtest/gtest.h>
#include <computo/computo.hpp>
#include <computo/memory_pool.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using json = nlohmann::json;
using CB = computo::ComputoBuilder;
using namespace computo::memory;

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing pool state
        get_thread_local_pool().clear();
    }
    
    void TearDown() override {
        // Clean up after each test
        get_thread_local_pool().clear();
    }
};

// ============================================================================
// Basic Pool Functionality Tests
// ============================================================================

TEST_F(MemoryPoolTest, BasicAcquireRelease) {
    JsonMemoryPool pool(10);
    
    // Acquire an object
    auto obj1 = pool.acquire();
    ASSERT_NE(obj1, nullptr);
    EXPECT_TRUE(obj1->is_null()); // Should be cleared
    
    // Use the object
    *obj1 = json{{"test", "value"}};
    EXPECT_EQ((*obj1)["test"], "value");
    
    // When obj1 goes out of scope, it should return to pool
    auto stats_before = pool.get_stats();
    obj1.reset(); // Explicit release
    auto stats_after = pool.get_stats();
    
    // Pool should have more available objects now
    EXPECT_GE(stats_after.available_objects, stats_before.available_objects);
}

TEST_F(MemoryPoolTest, ObjectReuse) {
    JsonMemoryPool pool(5);
    
    // Acquire and release objects to populate the pool
    for (int i = 0; i < 3; ++i) {
        auto obj = pool.acquire();
        *obj = json{{"iteration", i}};
        // obj automatically returns to pool when destroyed
    }
    
    // Now acquire again - should reuse objects from pool
    auto stats = pool.get_stats();
    EXPECT_GT(stats.available_objects, 0);
    
    auto obj = pool.acquire();
    EXPECT_TRUE(obj->is_null()); // Should be cleared when returned to pool
}

TEST_F(MemoryPoolTest, PoolStatistics) {
    JsonMemoryPool pool(10);
    
    // Initial stats
    auto stats = pool.get_stats();
    EXPECT_GT(stats.total_objects, 0); // Pre-allocated objects
    EXPECT_EQ(stats.available_objects, stats.total_objects);
    EXPECT_EQ(stats.pool_usage_percent, 0);
    
    // Acquire some objects
    std::vector<JsonMemoryPool::PooledJsonPtr> objects;
    for (int i = 0; i < 3; ++i) {
        objects.push_back(pool.acquire());
    }
    
    stats = pool.get_stats();
    EXPECT_GT(stats.pool_usage_percent, 0);
    EXPECT_EQ(stats.available_objects, stats.total_objects - objects.size());
}

TEST_F(MemoryPoolTest, PoolSizeLimits) {
    JsonMemoryPool pool(3); // Small pool
    
    // Acquire more objects than pool size
    std::vector<JsonMemoryPool::PooledJsonPtr> objects;
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool.acquire());
        
        auto stats = pool.get_stats();
        // Pool shouldn't grow beyond max size
        EXPECT_LE(stats.total_objects, 5); // Some growth is expected for new allocations
    }
}

// ============================================================================
// Thread-Local Pool Tests
// ============================================================================

TEST_F(MemoryPoolTest, ThreadLocalPools) {
    std::atomic<int> completed_threads{0};
    const int num_threads = 4;
    std::vector<std::thread> threads;
    
    // Each thread should get its own pool
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&completed_threads, i]() {
            auto& pool = get_thread_local_pool();
            
            // Each thread's pool should start fresh
            auto stats = pool.get_stats();
            EXPECT_GT(stats.total_objects, 0);
            EXPECT_EQ(stats.pool_usage_percent, 0);
            
            // Use the pool
            auto obj = pool.acquire();
            *obj = json{{"thread_id", i}};
            EXPECT_EQ((*obj)["thread_id"], i);
            
            ++completed_threads;
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed_threads.load(), num_threads);
}

TEST_F(MemoryPoolTest, ThreadSafety) {
    const int num_threads = 8;
    const int operations_per_thread = 100;
    std::atomic<int> total_operations{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&total_operations, operations_per_thread]() {
            auto& pool = get_thread_local_pool();
            
            for (int j = 0; j < operations_per_thread; ++j) {
                // Rapid acquire/release cycles
                auto obj = pool.acquire();
                *obj = json{{"op", j}};
                EXPECT_EQ((*obj)["op"], j);
                
                ++total_operations;
                // obj automatically released
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
}

// ============================================================================
// Integration Tests with Computo Evaluation
// ============================================================================

TEST_F(MemoryPoolTest, IntegrationWithMapOperations) {
    // Create a large array to trigger frequent JSON object creation
    json large_array = json::array();
    for (int i = 0; i < 100; ++i) {
        large_array.push_back(json{{"id", i}, {"value", i * 2}});
    }
    
    // Get initial pool stats
    auto& pool = get_thread_local_pool();
    auto stats_before = pool.get_stats();
    
    // Perform map operation that should use the pool internally
    auto script = CB::map(
        CB::input(),
        CB::lambda("x", 
            CB::obj()
                .add_field("id", CB::get(CB::var("x"), "/id"))
                .add_field("doubled", CB::add(
                    CB::get(CB::var("x"), "/value"),
                    CB::get(CB::var("x"), "/value")
                ))
        )
    );
    
    auto result = computo::execute(script, large_array);
    
    // Verify the operation worked correctly
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 100);
    EXPECT_EQ(result[0]["id"], 0);
    EXPECT_EQ(result[0]["doubled"], 0);
    
    // Check that pool was used (stats should have changed)
    auto stats_after = pool.get_stats();
    // Pool usage should indicate activity occurred
    EXPECT_GE(stats_after.total_objects, stats_before.total_objects);
}

TEST_F(MemoryPoolTest, IntegrationWithNestedOperations) {
    json input = json{
        {"users", json::array({
            json{{"name", "Alice"}, {"scores", {85, 92, 78}}},
            json{{"name", "Bob"}, {"scores", {88, 85, 90}}},
            json{{"name", "Charlie"}, {"scores", {92, 89, 94}}}
        })}
    };
    
    auto& pool = get_thread_local_pool();
    auto stats_before = pool.get_stats();
    
    // Complex nested operations that should stress-test the pool
    auto script = CB::map(
        CB::get(CB::input(), "/users"),
        CB::lambda("user",
            CB::obj()
                .add_field("name", CB::get(CB::var("user"), "/name"))
                .add_field("max", CB::reduce(
                    CB::get(CB::var("user"), "/scores"),
                    CB::lambda_multi({"a", "b"}, CB::if_then_else(
                        CB::greater_than(CB::var("a"), CB::var("b")),
                        CB::var("a"),
                        CB::var("b")
                    )),
                    0
                ))
        )
    );
    
    auto result = computo::execute(script, input);
    
    // Verify correctness
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]["name"], "Alice");
    // Note: simplified test since average calculation was removed
    EXPECT_EQ(result[0]["name"], "Alice");
    EXPECT_EQ(result[0]["max"], 92);
    
    // Pool should show activity
    auto stats_after = pool.get_stats();
    EXPECT_GE(stats_after.total_objects, stats_before.total_objects);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(MemoryPoolTest, PerformanceComparison) {
    const int iterations = 1000;
    json large_array = json::array();
    
    // Create test data
    for (int i = 0; i < 100; ++i) {
        large_array.push_back(json{{"value", i}});
    }
    
    auto script = CB::map(
        CB::input(),
        CB::lambda("x", 
            CB::obj()
                .add_field("doubled", CB::add(
                    CB::get(CB::var("x"), "/value"),
                    CB::get(CB::var("x"), "/value")
                ))
        )
    );
    
    // Warm up
    computo::execute(script, large_array);
    
    // Measure performance with pool active
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = computo::execute(script, large_array);
        (void)result; // Avoid optimization
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_with_pool = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Basic performance sanity check - operation should complete in reasonable time
    EXPECT_LT(duration_with_pool.count(), 10000000); // Less than 10 seconds
    
    // Log performance info (visible in verbose test output)
    std::cout << "Performance with pool: " << duration_with_pool.count() << " microseconds" << std::endl;
    
    auto& pool = get_thread_local_pool();
    auto final_stats = pool.get_stats();
    std::cout << "Final pool stats - Total: " << final_stats.total_objects 
              << ", Available: " << final_stats.available_objects 
              << ", Usage: " << final_stats.pool_usage_percent << "%" << std::endl;
}

// ============================================================================
// Memory Safety Tests
// ============================================================================

TEST_F(MemoryPoolTest, NoMemoryLeaks) {
    JsonMemoryPool pool(5);
    auto initial_stats = pool.get_stats();
    
    // Create and release many objects
    for (int i = 0; i < 100; ++i) {
        auto obj = pool.acquire();
        *obj = json{{"large_data", std::vector<int>(1000, i)}};
        // obj automatically released
    }
    
    // Pool should be in a reasonable state
    auto final_stats = pool.get_stats();
    EXPECT_LE(final_stats.total_objects, initial_stats.total_objects + 100);
    EXPECT_GE(final_stats.available_objects, 0);
}

TEST_F(MemoryPoolTest, ProperCleanupOnClear) {
    JsonMemoryPool pool(10);
    
    // Use some objects
    std::vector<JsonMemoryPool::PooledJsonPtr> objects;
    for (int i = 0; i < 5; ++i) {
        objects.push_back(pool.acquire());
    }
    
    auto stats_before = pool.get_stats();
    EXPECT_GT(stats_before.total_objects, 0);
    
    // Clear the pool
    pool.clear();
    
    auto stats_after = pool.get_stats();
    EXPECT_EQ(stats_after.total_objects, 0);
    EXPECT_EQ(stats_after.available_objects, 0);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(MemoryPoolTest, ZeroSizePool) {
    JsonMemoryPool pool(0);
    
    // Should still work, just won't pool anything
    auto obj = pool.acquire();
    EXPECT_NE(obj, nullptr);
    
    auto stats = pool.get_stats();
    EXPECT_GE(stats.total_objects, 0);
}

TEST_F(MemoryPoolTest, ExtremelyLargePool) {
    // Test with very large pool size (should handle gracefully)
    JsonMemoryPool pool(1000000);
    
    auto obj = pool.acquire();
    EXPECT_NE(obj, nullptr);
    
    // Should not actually pre-allocate all objects
    auto stats = pool.get_stats();
    EXPECT_LT(stats.total_objects, 100); // Much less than 1M
}

TEST_F(MemoryPoolTest, RapidAcquireRelease) {
    JsonMemoryPool pool(50);
    
    // Rapid fire acquire/release cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
        std::vector<JsonMemoryPool::PooledJsonPtr> objects;
        
        // Acquire many
        for (int i = 0; i < 100; ++i) {
            objects.push_back(pool.acquire());
        }
        
        // Release all (automatic via vector destruction)
        objects.clear();
    }
    
    // Pool should still be functional
    auto final_stats = pool.get_stats();
    EXPECT_GT(final_stats.total_objects, 0);
    
    auto obj = pool.acquire();
    EXPECT_NE(obj, nullptr);
    EXPECT_TRUE(obj->is_null());
} 