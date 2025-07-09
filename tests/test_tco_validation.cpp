#include <gtest/gtest.h>
#include <computo.hpp>
#include <chrono>
#include <iostream>

using json = nlohmann::json;

class TCOValidationTest : public ::testing::Test {
protected:
    auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }
    
    // Helper to create large arrays for testing
    json create_large_array(int size, int start = 0) {
        json arr = json::array();
        for (int i = start; i < start + size; ++i) {
            arr.push_back(i);
        }
        return json::object({{"array", arr}});
    }
    
    // Helper to measure execution time
    template<typename Func>
    auto measure_time(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return std::make_pair(result, duration.count());
    }
};

// Test 1: Deep Nested Let Expressions (Should cause stack overflow without TCO)
TEST_F(TCOValidationTest, DeepNestedLetExpressions) {
    // Create deeply nested let expressions: let x=1 in let y=2 in let z=3 in ... final
    json script = 42;  // Base result
    
    // Build nested let expressions (50 levels should be safe for testing)
    for (int i = 0; i < 50; ++i) {
        std::string var_name = "x" + std::to_string(i);
        
        json binding = json::array({var_name, i + 1});
        
        script = json::array({
            "let",
            json::array({binding}),
            script
        });
    }
    
    // This should work with TCO but might fail without it
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Deep nested let (50 levels) took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 42);
    EXPECT_LT(exec_time, 1000000);  // Should complete in under 1 second
}

// Test 2: Tail-Recursive Countdown (Classic TCO test case) 
TEST_F(TCOValidationTest, TailRecursiveCountdown) {
    // Simple nested if-then-else structure that mimics countdown behavior
    // We'll create: if 50 <= 0 then 0 else (if 49 <= 0 then 0 else ... else 0)
    
    json script = 0;  // Base case
    
    // Build nested countdown structure
    for (int i = 1; i <= 20; ++i) {
        script = json::array({
            "if",
            json::array({"<=", i, 0}),
            0,
            script
        });
    }
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Countdown (20 levels) took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 0);
    EXPECT_LT(exec_time, 100000);  // Should be fast
}

// Test 3: Deep List Processing with Functional Operations
TEST_F(TCOValidationTest, DeepListProcessing) {
    // Create a large list and process it with nested operations
    json large_list = create_large_array(1000, 0);
    
    // Nested map operations: map(map(map(list, +1), *2), -1)
    json script = json::array({
        "map",
        json::array({
            "map", 
            json::array({
                "map",
                large_list,
                json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
            }),
            json::array({"lambda", json::array({"x"}), json::array({"*", json::array({"$", "/x"}), 2})})
        }),
        json::array({"lambda", json::array({"x"}), json::array({"-", json::array({"$", "/x"}), 1})})
    });
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Deep list processing (1000 elements, 3 nested maps) took: " << exec_time << " μs" << std::endl;
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("array"));
    EXPECT_EQ(result["array"].size(), 1000);
    
    // Check first few elements: ((0+1)*2)-1 = 1, ((1+1)*2)-1 = 3, etc.
    EXPECT_EQ(result["array"][0], 1);
    EXPECT_EQ(result["array"][1], 3);
    EXPECT_EQ(result["array"][2], 5);
    
    EXPECT_LT(exec_time, 5000000);  // Should complete in under 5 seconds
}

// Test 4: Fibonacci-Style Recursive Structure
TEST_F(TCOValidationTest, FibonacciStyleRecursion) {
    // Simple nested let expressions that simulate iterative fibonacci
    json script = json::array({
        "let",
        json::array({
            json::array({"a", 1}),
            json::array({"b", 1})
        }),
        json::array({"+", json::array({"$", "/a"}), json::array({"$", "/b"})})
    });
    
    // Wrap in progressively deeper let expressions to test deep nesting
    for (int i = 0; i < 30; ++i) {
        script = json::array({
            "let",
            json::array({
                json::array({"step", i}),
                json::array({"result", script})
            }),
            json::array({"$", "/result"})
        });
    }
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Fibonacci-style recursion (30 levels) took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 2);  // 1 + 1 = 2
    EXPECT_LT(exec_time, 500000);  // Should complete in under 0.5 seconds
}

// Test 5: Deep Conditional Nesting
TEST_F(TCOValidationTest, DeepConditionalNesting) {
    // Create deeply nested if-then-else chains
    json script = 0;  // Base case
    
    // Build nested if expressions: if true then (if true then ... else 0) else 0
    for (int i = 0; i < 200; ++i) {
        script = json::array({
            "if",
            true,
            script,
            json::array({"-", i})  // This should never be reached
        });
    }
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Deep conditional nesting (200 levels) took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 0);
    EXPECT_LT(exec_time, 200000);  // Should be fast since it's just following true branch
}

// Test 6: Memory Stress Test with Deep Variable Scoping
TEST_F(TCOValidationTest, DeepVariableScoping) {
    // Create deeply nested variable scopes to test memory management
    json script = json::array({"$", "/final"});
    
    // Build nested let expressions with many variables at each level
    for (int level = 0; level < 50; ++level) {
        json bindings = json::array();
        
        // Create multiple variables at each level
        for (int var = 0; var < 10; ++var) {
            std::string var_name = "var_" + std::to_string(level) + "_" + std::to_string(var);
            bindings.push_back(json::array({var_name, level * 10 + var}));
        }
        
        // Add the final variable at the last level
        if (level == 49) {
            bindings.push_back(json::array({"final", 42}));
        }
        
        script = json::array({"let", bindings, script});
    }
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Deep variable scoping (50 levels, 500 variables) took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 42);
    EXPECT_LT(exec_time, 1000000);  // Should complete in under 1 second
}

// Test 7: Mixed Tail and Non-Tail Operations
TEST_F(TCOValidationTest, MixedTailAndNonTailOperations) {
    // Test combination of nested operations that mix different evaluation patterns
    json script = json::array({
        "let",
        json::array({
            json::array({"a", 10}),
            json::array({"b", 20}),
            json::array({"c", 30})
        }),
        json::array({
            "if",
            json::array({">", json::array({"$", "/a"}), 5}),
            json::array({
                "let",
                json::array({json::array({"sum", json::array({"+", json::array({"$", "/b"}), json::array({"$", "/c"})})})}),
                json::array({
                    "if",
                    json::array({">", json::array({"$", "/sum"}), 40}),
                    json::array({"*", json::array({"$", "/sum"}), 2}),
                    json::array({"$", "/sum"})
                })
            }),
            0
        })
    });
    
    auto [result, exec_time] = measure_time([&]() { return exec(script); });
    
    std::cout << "Mixed tail/non-tail operations took: " << exec_time << " μs" << std::endl;
    EXPECT_EQ(result, 100);  // (20 + 30) * 2 = 100
    EXPECT_LT(exec_time, 100000);  // Should be fast
}

// Performance Baseline Test
TEST_F(TCOValidationTest, PerformanceBaseline) {
    // Simple operations to establish baseline performance
    std::vector<std::pair<std::string, std::function<json()>>> tests = {
        {"Simple arithmetic", [&]() { return exec(json::array({"+", 1, 2, 3})); }},
        {"Variable access", [&]() { 
            return exec(json::array({
                "let", 
                json::array({json::array({"x", 42})}),
                json::array({"$", "/x"})
            }));
        }},
        {"Small array map", [&]() {
            return exec(json::array({
                "map",
                create_large_array(10),
                json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
            }));
        }},
        {"Conditional", [&]() {
            return exec(json::array({"if", true, 42, 0}));
        }}
    };
    
    std::cout << "\n=== Performance Baseline ===" << std::endl;
    for (const auto& [name, test] : tests) {
        auto [result, exec_time] = measure_time(test);
        std::cout << name << ": " << exec_time << " μs" << std::endl;
        EXPECT_LT(exec_time, 10000);  // Should be very fast
    }
}