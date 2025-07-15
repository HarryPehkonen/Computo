#include <chrono>
#include <computo.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

using json = nlohmann::json;

class PerformanceTest : public ::testing::Test {
protected:
    static auto exec(const json& script, const json& input = json(nullptr)) {
        return computo::execute(script, input);
    }

    template <typename Func>
    auto measure_time(Func func, const std::string& operation_name) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << operation_name << ": " << duration.count() << " μs" << '\n';
        return duration.count();
    }
};

TEST_F(PerformanceTest, ArithmeticOperations) {
    const int iterations = 10000;

    // Addition
    auto add_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "+", i, i + 1 });
            exec(script);
        }
    },
        "Addition (" + std::to_string(iterations) + " ops)");

    // Multiplication
    auto mul_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "*", i, 2 });
            exec(script);
        }
    },
        "Multiplication (" + std::to_string(iterations) + " ops)");

    EXPECT_LT(add_time, 1000000); // Should complete in under 1 second
    EXPECT_LT(mul_time, 1000000);
}

TEST_F(PerformanceTest, ArrayOperations) {
    const int iterations = 1000;
    const int array_size = 100;

    // Create large array
    json large_array = json::array();
    for (int i = 0; i < array_size; ++i) {
        large_array.push_back(i);
    }

    // Map operation
    auto map_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "map",
                json::object({ { "array", large_array } }),
                json::array({ "lambda", json::array({ "x" }), json::array({ "*", json::array({ "$", "/x" }), 2 }) }) });
            exec(script);
        }
    },
        "Map (" + std::to_string(iterations) + " ops on " + std::to_string(array_size) + " elements)");

    // Filter operation
    auto filter_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "filter",
                json::object({ { "array", large_array } }),
                json::array({ "lambda", json::array({ "x" }), json::array({ ">", json::array({ "$", "/x" }), 50 }) }) });
            exec(script);
        }
    },
        "Filter (" + std::to_string(iterations) + " ops on " + std::to_string(array_size) + " elements)");

    EXPECT_LT(map_time, 5000000); // Should complete in under 5 seconds
    EXPECT_LT(filter_time, 5000000);
}

TEST_F(PerformanceTest, ComplexExpressions) {
    const int iterations = 100;

    auto complex_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "let",
                json::array({ json::array({ "data", json::object({ { "array", json::array({ 1, 2, 3, 4, 5 }) } }) }),
                    json::array({ "factor", 10 }) }),
                json::array({ "map",
                    json::array({ "$", "/data" }),
                    json::array({ "lambda",
                        json::array({ "x" }),
                        json::array({ "let",
                            json::array({ json::array({ "doubled", json::array({ "*", json::array({ "$", "/x" }), 2 }) }) }),
                            json::array({ "let",
                                json::array({ json::array({ "result", json::array({ "*", json::array({ "$", "/doubled" }), json::array({ "$", "/factor" }) }) }) }),
                                json::array({ "$", "/result" }) }) }) }) }) });
            exec(script);
        }
    },
        "Complex nested expressions (" + std::to_string(iterations) + " ops)");

    EXPECT_LT(complex_time, 1000000);
}

TEST_F(PerformanceTest, MemoryEfficiency) {
    const int iterations = 1000;
    std::vector<json> results;
    results.reserve(iterations);

    auto memory_time = measure_time([&]() {
        for (int i = 0; i < iterations; ++i) {
            json script = json::array({ "let",
                json::array({ json::array({ "x", i }),
                    json::array({ "y", i * 2 }),
                    json::array({ "z", i * 3 }) }),
                json::array({ "+",
                    json::array({ "$", "/x" }),
                    json::array({ "$", "/y" }) }) });
            results.push_back(exec(script));
        }
    },
        "Memory-intensive operations (" + std::to_string(iterations) + " ops)");

    EXPECT_LT(memory_time, 2000000);
    EXPECT_EQ(results.size(), iterations);
}