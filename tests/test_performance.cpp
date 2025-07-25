#include <algorithm>
#include <chrono>
#include <computo.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#ifdef __linux__
#include <fstream>
#include <sys/resource.h>
#include <unistd.h>
#endif

using json = nlohmann::json;
using namespace std::chrono;

// --- Performance Measurement Infrastructure ---

struct BenchmarkResult {
    std::string test_name;
    std::string operation;
    std::size_t data_size;
    double avg_time_ms;
    double min_time_ms;
    double max_time_ms;
    double p50_time_ms;
    double p95_time_ms;
    double p99_time_ms;
    std::size_t operations_per_second;
    std::size_t memory_peak_kb;
    std::size_t memory_after_kb;
};

class PerformanceTimer {
public:
    void start() {
        start_time_ = high_resolution_clock::now();
        start_memory_kb_ = get_memory_usage_kb();
    }

    void stop() {
        end_time_ = high_resolution_clock::now();
        end_memory_kb_ = get_memory_usage_kb();
    }

    double get_duration_ms() const {
        return duration_cast<nanoseconds>(end_time_ - start_time_).count() / 1e6;
    }

    std::size_t get_memory_delta_kb() const {
        return end_memory_kb_ > start_memory_kb_ ? end_memory_kb_ - start_memory_kb_ : 0;
    }

    std::size_t get_peak_memory_kb() const { return std::max(start_memory_kb_, end_memory_kb_); }

private:
    high_resolution_clock::time_point start_time_;
    high_resolution_clock::time_point end_time_;
    std::size_t start_memory_kb_ = 0;
    std::size_t end_memory_kb_ = 0;

    std::size_t get_memory_usage_kb() const {
#ifdef __linux__
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::string kb_value = line.substr(6);
                // Remove "kB" and whitespace
                std::size_t start = kb_value.find_first_not_of(" \t");
                std::size_t end = kb_value.find_first_of(" \t", start);
                if (start != std::string::npos && end != std::string::npos) {
                    return std::stoull(kb_value.substr(start, end - start));
                }
            }
        }
#endif
        return 0; // Fallback for non-Linux systems
    }
};

class BenchmarkSuite {
public:
    static constexpr std::size_t DEFAULT_ITERATIONS = 100;
    static constexpr std::size_t WARMUP_ITERATIONS = 10;

    BenchmarkResult run_benchmark(const std::string& test_name, const std::string& operation,
                                  std::function<void()> benchmark_func, std::size_t data_size = 1,
                                  std::size_t iterations = DEFAULT_ITERATIONS) {
        // Warmup
        for (std::size_t i = 0; i < WARMUP_ITERATIONS; ++i) {
            benchmark_func();
        }

        std::vector<double> times;
        std::vector<std::size_t> memory_usage;
        times.reserve(iterations);
        memory_usage.reserve(iterations);

        PerformanceTimer timer;

        for (std::size_t i = 0; i < iterations; ++i) {
            timer.start();
            benchmark_func();
            timer.stop();

            times.push_back(timer.get_duration_ms());
            memory_usage.push_back(timer.get_peak_memory_kb());
        }

        // Calculate statistics
        std::sort(times.begin(), times.end());

        BenchmarkResult result;
        result.test_name = test_name;
        result.operation = operation;
        result.data_size = data_size;
        result.avg_time_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        result.min_time_ms = times.front();
        result.max_time_ms = times.back();
        result.p50_time_ms = times[times.size() / 2];
        result.p95_time_ms = times[static_cast<std::size_t>(times.size() * 0.95)];
        result.p99_time_ms = times[static_cast<std::size_t>(times.size() * 0.99)];
        result.operations_per_second
            = result.avg_time_ms > 0 ? static_cast<std::size_t>(1000.0 / result.avg_time_ms) : 0;
        result.memory_peak_kb = *std::max_element(memory_usage.begin(), memory_usage.end());
        result.memory_after_kb = memory_usage.back();

        results_.push_back(result);
        return result;
    }

    void print_results() const {
        std::cout << "\n=== Performance Benchmark Results ===\n";
        std::cout << std::left << std::setw(25) << "Test Name" << std::setw(15) << "Operation"
                  << std::setw(10) << "Data Size" << std::setw(12) << "Avg (ms)" << std::setw(12)
                  << "Min (ms)" << std::setw(12) << "Max (ms)" << std::setw(12) << "P95 (ms)"
                  << std::setw(12) << "Ops/sec" << std::setw(12) << "Peak KB" << "\n";
        std::cout << std::string(140, '-') << "\n";

        for (const auto& result : results_) {
            std::cout << std::left << std::setw(25) << result.test_name << std::setw(15)
                      << result.operation << std::setw(10) << result.data_size << std::setw(12)
                      << std::fixed << std::setprecision(3) << result.avg_time_ms << std::setw(12)
                      << std::fixed << std::setprecision(3) << result.min_time_ms << std::setw(12)
                      << std::fixed << std::setprecision(3) << result.max_time_ms << std::setw(12)
                      << std::fixed << std::setprecision(3) << result.p95_time_ms << std::setw(12)
                      << result.operations_per_second << std::setw(12) << result.memory_peak_kb
                      << "\n";
        }
    }

    void export_csv(const std::string& filename) const {
        std::ofstream file(filename);
        file << "test_name,operation,data_size,avg_time_ms,min_time_ms,max_time_ms,"
             << "p50_time_ms,p95_time_ms,p99_time_ms,ops_per_second,memory_peak_kb,memory_after_"
                "kb\n";

        for (const auto& result : results_) {
            file << result.test_name << "," << result.operation << "," << result.data_size << ","
                 << result.avg_time_ms << "," << result.min_time_ms << "," << result.max_time_ms
                 << "," << result.p50_time_ms << "," << result.p95_time_ms << ","
                 << result.p99_time_ms << "," << result.operations_per_second << ","
                 << result.memory_peak_kb << "," << result.memory_after_kb << "\n";
        }
    }

    void export_json(const std::string& filename) const {
        json results_json = json::array();
        for (const auto& result : results_) {
            json result_json = {{"test_name", result.test_name},
                                {"operation", result.operation},
                                {"data_size", result.data_size},
                                {"avg_time_ms", result.avg_time_ms},
                                {"min_time_ms", result.min_time_ms},
                                {"max_time_ms", result.max_time_ms},
                                {"p50_time_ms", result.p50_time_ms},
                                {"p95_time_ms", result.p95_time_ms},
                                {"p99_time_ms", result.p99_time_ms},
                                {"ops_per_second", result.operations_per_second},
                                {"memory_peak_kb", result.memory_peak_kb},
                                {"memory_after_kb", result.memory_after_kb}};
            results_json.push_back(result_json);
        }

        json output = {{"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                       {"results", results_json}};

        std::ofstream file(filename);
        file << output.dump(2);
    }

    const std::vector<BenchmarkResult>& get_results() const { return results_; }

private:
    std::vector<BenchmarkResult> results_;
};

// --- Test Fixture ---

class PerformanceBenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override { suite_ = std::make_unique<BenchmarkSuite>(); }

    void TearDown() override {
        if (::testing::Test::HasFailure()) {
            return; // Don't print results if test failed
        }
        suite_->print_results();

        // Export results
        suite_->export_csv("performance_results.csv");
        suite_->export_json("performance_results.json");
    }

    auto execute_script(const std::string& script_json, const json& input = json(nullptr)) -> json {
        auto script = json::parse(script_json);
        return computo::execute(script, {input});
    }

    auto create_large_array(std::size_t size) -> json {
        json array = json::array();
        for (std::size_t i = 0; i < size; ++i) {
            array.push_back(static_cast<int>(i));
        }
        return array;
    }

    auto create_large_object(std::size_t size) -> json {
        json obj = json::object();
        for (std::size_t i = 0; i < size; ++i) {
            obj["key" + std::to_string(i)] = static_cast<int>(i);
        }
        return obj;
    }

    auto create_nested_structure(std::size_t depth, std::size_t breadth = 3) -> json {
        if (depth == 0) {
            return json("leaf_value");
        }

        json obj = json::object();
        for (std::size_t i = 0; i < breadth; ++i) {
            obj["child" + std::to_string(i)] = create_nested_structure(depth - 1, breadth);
        }
        return obj;
    }

    std::unique_ptr<BenchmarkSuite> suite_;
};

// --- Arithmetic Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, ArithmeticOperationsBenchmark) {
    // Basic arithmetic with small numbers
    suite_->run_benchmark("Arithmetic_Small", "addition",
                          [this]() { execute_script(R"(["+", 1, 2, 3, 4, 5])"); });

    suite_->run_benchmark("Arithmetic_Small", "subtraction",
                          [this]() { execute_script(R"(["-", 100, 1, 2, 3, 4, 5])"); });

    suite_->run_benchmark("Arithmetic_Small", "multiplication",
                          [this]() { execute_script(R"(["*", 2, 3, 4, 5])"); });

    suite_->run_benchmark("Arithmetic_Small", "division",
                          [this]() { execute_script(R"(["/", 1000, 2, 5, 10])"); });

    // Arithmetic with large numbers
    suite_->run_benchmark("Arithmetic_Large", "addition",
                          [this]() { execute_script(R"(["+", 1000000, 2000000, 3000000])"); });

    // Nested arithmetic operations
    suite_->run_benchmark("Arithmetic_Nested", "complex", [this]() {
        execute_script(R"(["+", ["*", 2, 3], ["-", 10, 5], ["/", 20, 4]])");
    });

    // Many operands
    std::string many_operands = R"(["+")";
    for (int i = 1; i <= 100; ++i) {
        many_operands += ", " + std::to_string(i);
    }
    many_operands += "]";

    suite_->run_benchmark(
        "Arithmetic_Many", "addition_100",
        [this, many_operands]() { execute_script(many_operands); }, 100);
}

// --- Array Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, ArrayOperationsBenchmark) {
    const std::vector<std::size_t> sizes = {10, 100, 1000};

    for (std::size_t size : sizes) {
        auto large_array = create_large_array(size);

        // Map operation
        suite_->run_benchmark(
            "Array_Map", "map",
            [this, large_array]() {
                execute_script(R"(["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]])",
                               large_array);
            },
            size);

        // Filter operation
        suite_->run_benchmark(
            "Array_Filter", "filter",
            [this, large_array]() {
                execute_script(R"(["filter", ["$input"], [["x"], [">", ["$", "/x"], 500]]])",
                               large_array);
            },
            size);

        // Reduce operation
        suite_->run_benchmark(
            "Array_Reduce", "reduce",
            [this, large_array]() {
                execute_script(
                    R"(["reduce", ["$input"], [["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0])",
                    large_array);
            },
            size);

        // Count operation
        suite_->run_benchmark(
            "Array_Count", "count",
            [this, large_array]() { execute_script(R"(["count", ["$input"]])", large_array); },
            size);

        // Find operation
        suite_->run_benchmark(
            "Array_Find", "find",
            [this, large_array]() {
                execute_script(R"(["find", ["$input"], [["x"], ["==", ["$", "/x"], 500]]])",
                               large_array);
            },
            size);

        // Chained operations
        suite_->run_benchmark(
            "Array_Chained", "map_filter_reduce",
            [this, large_array]() {
                execute_script(R"(["reduce", 
                ["filter", 
                    ["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]], 
                    [["x"], [">", ["$", "/x"], 100]]
                ], 
                [["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 
                0
            ])",
                               large_array);
            },
            size);
    }
}

// --- String Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, StringOperationsBenchmark) {
    // Single string operations
    suite_->run_benchmark("String_Single", "join", [this]() {
        execute_script(
            R"(["join", {"array": ["hello", "world", "this", "is", "a", "test"]}, " "])");
    });

    suite_->run_benchmark("String_Single", "strConcat", [this]() {
        execute_script(R"(["strConcat", "hello", " ", "world", " ", "test", " ", "string"])");
    });

    // Array of strings
    const std::vector<std::size_t> sizes = {10, 100, 500};

    for (std::size_t size : sizes) {
        json string_array = json::array();
        for (std::size_t i = 0; i < size; ++i) {
            string_array.push_back("test_string_" + std::to_string(i) + "_with_some_content");
        }

        suite_->run_benchmark(
            "String_Array", "join_large",
            [this, string_array]() {
                execute_script(R"(["join", ["$input"], "||"])", string_array);
            },
            size);

        suite_->run_benchmark(
            "String_Array", "map_strConcat",
            [this, string_array]() {
                execute_script(
                    R"(["map", ["$input"], [["s"], ["strConcat", "prefix_", ["$", "/s"]]]])",
                    string_array);
            },
            size);
    }
}

// --- Object Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, ObjectOperationsBenchmark) {
    const std::vector<std::size_t> sizes = {50, 500, 2000};

    for (std::size_t size : sizes) {
        auto large_object = create_large_object(size);

        // Keys operation
        suite_->run_benchmark(
            "Object_Keys", "keys",
            [this, large_object]() { execute_script(R"(["keys", ["$input"]])", large_object); },
            size);

        // Values operation
        suite_->run_benchmark(
            "Object_Values", "values",
            [this, large_object]() { execute_script(R"(["values", ["$input"]])", large_object); },
            size);

        // Pick operation (select subset of keys)
        std::vector<std::string> pick_keys;
        for (std::size_t i = 0; i < std::min(size, static_cast<std::size_t>(10)); ++i) {
            pick_keys.push_back("key" + std::to_string(i));
        }

        std::string pick_script = "[\"pick\", [\"$input\"], {\"array\": [";
        for (std::size_t i = 0; i < pick_keys.size(); ++i) {
            if (i > 0)
                pick_script += ", ";
            pick_script += "\"" + pick_keys[i] + "\"";
        }
        pick_script += "]}]";

        suite_->run_benchmark(
            "Object_Pick", "pick",
            [this, large_object, pick_script]() { execute_script(pick_script, large_object); },
            size);
    }
}

// --- Logical Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, LogicalOperationsBenchmark) {
    suite_->run_benchmark("Logical_Simple", "and",
                          [this]() { execute_script(R"(["and", true, true, true, false])"); });

    suite_->run_benchmark("Logical_Simple", "or",
                          [this]() { execute_script(R"(["or", false, false, true, false])"); });

    suite_->run_benchmark("Logical_Simple", "not",
                          [this]() { execute_script(R"(["not", false])"); });

    // Complex logical expressions
    suite_->run_benchmark("Logical_Complex", "nested_and_or", [this]() {
        execute_script(
            R"(["and", ["or", true, false], ["or", ["not", false], true], ["and", true, true]])");
    });

    // Many operands
    std::string many_true = R"(["and")";
    for (int i = 0; i < 100; ++i) {
        many_true += ", true";
    }
    many_true += "]";

    suite_->run_benchmark(
        "Logical_Many", "and_100", [this, many_true]() { execute_script(many_true); }, 100);
}

// --- Comparison Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, ComparisonOperationsBenchmark) {
    suite_->run_benchmark("Comparison_Simple", "equal",
                          [this]() { execute_script(R"(["==", 42, 42])"); });

    suite_->run_benchmark("Comparison_Simple", "greater_than",
                          [this]() { execute_script(R"([">", 10, 5])"); });

    suite_->run_benchmark("Comparison_Simple", "less_than",
                          [this]() { execute_script(R"(["<", 5, 10])"); });

    // Array comparison benchmarks
    const std::vector<std::size_t> sizes = {100, 1000};

    for (std::size_t size : sizes) {
        auto large_array = create_large_array(size);

        suite_->run_benchmark(
            "Comparison_Array", "filter_greater",
            [this, large_array]() {
                execute_script(R"(["filter", ["$input"], [["x"], [">", ["$", "/x"], 500]]])",
                               large_array);
            },
            size);

        suite_->run_benchmark(
            "Comparison_Array", "filter_equal",
            [this, large_array]() {
                execute_script(R"(["filter", ["$input"], [["x"], ["==", ["$", "/x"], 500]]])",
                               large_array);
            },
            size);
    }
}

// --- Control Flow Benchmarks ---

TEST_F(PerformanceBenchmarkTest, ControlFlowBenchmark) {
    suite_->run_benchmark("ControlFlow_Simple", "if_true",
                          [this]() { execute_script(R"(["if", true, 42, 0])"); });

    suite_->run_benchmark("ControlFlow_Simple", "if_false",
                          [this]() { execute_script(R"(["if", false, 42, 0])"); });

    // Nested if statements
    suite_->run_benchmark("ControlFlow_Nested", "nested_if", [this]() {
        execute_script(R"(["if", true, ["if", true, ["if", true, 42, 0], 0], 0])");
    });

    // Let with variables
    suite_->run_benchmark("ControlFlow_Variables", "let_simple", [this]() {
        execute_script(R"(["let", {"x": 10, "y": 20}, ["+", ["$", "/x"], ["$", "/y"]]])");
    });

    // Complex variable scoping
    suite_->run_benchmark("ControlFlow_Variables", "let_complex", [this]() {
        execute_script(
            R"(["let", {"x": 10}, ["let", {"y": 20}, ["let", {"z": 30}, ["+", ["$", "/x"], ["$", "/y"], ["$", "/z"]]]]])");
    });
}

// --- Nested Operations Benchmarks ---

TEST_F(PerformanceBenchmarkTest, NestedOperationsBenchmark) {
    // Deep nesting
    std::string deep_nested = "42";
    for (int i = 0; i < 50; ++i) {
        deep_nested = "[\"if\", true, " + deep_nested + ", 0]";
    }

    suite_->run_benchmark(
        "Nested_Deep", "if_50_levels", [this, deep_nested]() { execute_script(deep_nested); }, 50);

    // Complex data transformation
    const std::vector<std::size_t> sizes = {10, 100};

    for (std::size_t size : sizes) {
        auto nested_data = create_nested_structure(3, size / 10);

        suite_->run_benchmark(
            "Nested_DataTransform", "deep_traverse",
            [this, nested_data]() { execute_script(R"(["keys", ["$input"]])", nested_data); },
            size);
    }
}

// --- Memory Intensive Benchmarks ---

TEST_F(PerformanceBenchmarkTest, MemoryIntensiveBenchmark) {
    const std::vector<std::size_t> sizes = {10, 100, 500};

    for (std::size_t size : sizes) {
        auto large_data = create_large_array(size);

        suite_->run_benchmark(
            "Memory_Intensive", "large_copy",
            [this, large_data]() {
                execute_script(R"(["map", ["$input"], [["x"], ["$", "/x"]]])", large_data);
            },
            size);

        suite_->run_benchmark(
            "Memory_Intensive", "large_transform",
            [this, large_data]() {
                execute_script(R"(["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]])",
                               large_data);
            },
            size);
    }
}

// --- Functional Programming Benchmarks ---

TEST_F(PerformanceBenchmarkTest, FunctionalProgrammingBenchmark) {
    auto test_array = create_large_array(1000);

    suite_->run_benchmark("Functional_Ops", "car", [this, test_array]() {
        execute_script(R"(["car", ["$input"]])", test_array);
    });

    suite_->run_benchmark("Functional_Ops", "cdr", [this, test_array]() {
        execute_script(R"(["cdr", ["$input"]])", test_array);
    });

    suite_->run_benchmark("Functional_Ops", "cons",
                          [this]() { execute_script(R"(["cons", 42, [1, 2, 3, 4, 5]])"); });

    suite_->run_benchmark("Functional_Ops", "append",
                          [this]() { execute_script(R"(["append", [1, 2, 3], [4, 5, 6]])"); });
}

// --- Lambda and Function Call Benchmarks ---

TEST_F(PerformanceBenchmarkTest, LambdaFunctionBenchmark) {
    // Simple lambda operations
    suite_->run_benchmark(
        "Lambda_Simple", "map_lambda",
        [this]() {
            execute_script(R"(["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]])",
                           {{"array", {1, 2, 3, 4, 5}}});
        },
        5);

    suite_->run_benchmark(
        "Lambda_Simple", "filter_lambda",
        [this]() {
            execute_script(R"(["filter", ["$input"], [["x"], [">", ["$", "/x"], 5]]])",
                           {{"array", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}});
        },
        10);

    suite_->run_benchmark(
        "Lambda_Simple", "reduce_lambda",
        [this]() {
            execute_script(
                R"(["reduce", ["$input"], [["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0])",
                {{"array", {1, 2, 3, 4, 5}}});
        },
        5);

    // Complex lambda with variable binding
    suite_->run_benchmark("Lambda_Complex", "nested_lambda", [this]() {
        execute_script(R"(["let", [["data", [1,2,3,4,5]]], 
            ["reduce", 
                ["map", ["$", "/data"], [["x"], ["*", ["$", "/x"], 2]]], 
                [["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 
                0
            ]
        ])");
    });
}

// --- Tail Call Optimization (TCO) Benchmarks ---

TEST_F(PerformanceBenchmarkTest, TailCallOptimizationBenchmark) {
    // NOTE: These benchmarks test the existing TCO functionality
    // Commented out due to syntax differences - would need to adjust for current Computo syntax

    /*
    // TCO Fibonacci
    suite_->run_benchmark("TCO_Fibonacci", "fibonacci_100", [this]() {
        execute_script(R"([
            "let",
            {"fib": [["args"],
                ["if", [">", ["$", "/args/0"], 0],
                    ["call", ["$", "/fib"], ["-", ["$", "/args/0"], 1], ["$", "/args/2"], ["+",
    ["$", "/args/1"], ["$", "/args/2"]]],
                    ["$", "/args/1"]
                ]
            ]},
            ["call", ["$", "/fib"], 100, 0, 1]
        ])");
    }, 100);

    // TCO Square root using Newton's method
    suite_->run_benchmark("TCO_Newton", "sqrt2_newton", [this]() {
        execute_script(R"([
            "let",
            {"sqrt2": [["x"],
                ["if", [">", ["$", "/x/1"], 0],
                    ["call", ["$", "/sqrt2"], ["/", ["+", ["$", "/x/0"], ["/", 2, ["$", "/x/0"]]],
    2], ["-", ["$", "/x/1"], 1]],
                    ["$", "/x/0"]
                ]
            ]},
            ["call", ["$", "/sqrt2"], 1, 100]
        ])");
    }, 100);
    */

    // Recursive-like operations without actual recursion
    suite_->run_benchmark(
        "TCO_Like", "deep_nesting",
        [this]() {
            std::string deep_expr = "42";
            for (int i = 0; i < 20; ++i) {
                deep_expr = "[\"if\", true, " + deep_expr + ", 0]";
            }
            execute_script(deep_expr);
        },
        20);
}

// --- Variable Access and Scoping Benchmarks ---

TEST_F(PerformanceBenchmarkTest, VariableScopingBenchmark) {
    // Simple variable access
    suite_->run_benchmark("Variable_Access", "single_var",
                          [this]() { execute_script(R"(["let", {"x": 42}, ["$", "/x"]])"); });

    // Nested variable scoping
    suite_->run_benchmark("Variable_Access", "nested_scoping", [this]() {
        execute_script(R"([
            "let", {"x": 1}, 
            ["let", {"y": 2}, 
                ["let", {"z": 3}, 
                    ["+", ["$", "/x"], ["$", "/y"], ["$", "/z"]]
                ]
            ]
        ])");
    });

    // Variable shadowing
    suite_->run_benchmark("Variable_Access", "variable_shadowing", [this]() {
        execute_script(R"([
            "let", {"x": 10}, 
            ["let", {"x": 20}, 
                ["let", {"x": 30}, 
                    ["$", "/x"]
                ]
            ]
        ])");
    });

    // Complex variable expressions
    suite_->run_benchmark("Variable_Access", "complex_expressions", [this]() {
        execute_script(R"([
            "let", {"a": 5, "b": 10, "c": 15}, 
            ["*", 
                ["+", ["$", "/a"], ["$", "/b"]], 
                ["-", ["$", "/c"], ["$", "/a"]]
            ]
        ])");
    });
}

// --- Real-world Scenario Benchmarks ---

TEST_F(PerformanceBenchmarkTest, RealWorldScenariosBenchmark) {
    // Data processing pipeline
    auto sales_data = json::array();
    for (int i = 0; i < 1000; ++i) {
        sales_data.push_back({{"id", i},
                              {"amount", (i % 100) + 50},
                              {"category", i % 5 == 0 ? "premium" : "standard"},
                              {"date", "2024-01-" + std::to_string((i % 28) + 1)}});
    }

    // Complex aggregation pipeline
    suite_->run_benchmark(
        "RealWorld_Pipeline", "sales_analysis",
        [this, sales_data]() {
            execute_script(R"([
            "reduce",
            ["filter", 
                ["map", ["$input"], [["item"], 
                    ["obj", 
                        "id", ["$", "/item/id"], 
                        "amount", ["$", "/item/amount"], 
                        "is_premium", ["==", ["$", "/item/category"], "premium"]
                    ]
                ]], 
                [["item"], ["$", "/item/is_premium"]]
            ],
            [["acc", "item"], ["+", ["$", "/acc"], ["$", "/item/amount"]]],
            0
        ])",
                           sales_data);
        },
        1000);

    // JSON transformation
    suite_->run_benchmark(
        "RealWorld_Transform", "json_reshape",
        [this, sales_data]() {
            execute_script(R"([
            "objFromPairs",
            ["map", ["$input"], [["item"], 
                [["strConcat", "id_", ["$", "/item/id"]], ["$", "/item/amount"]]
            ]]
        ])",
                           sales_data);
        },
        1000);
}

// --- Sort Operator Baseline Benchmarks (Pre-DSU Refactor) ---

TEST_F(PerformanceBenchmarkTest, SortBaselineBenchmark) {
    // Test data generators for sort benchmarks
    auto create_object_array = [](std::size_t size, int field_depth = 1) -> json {
        json array = json::array();
        for (std::size_t i = 0; i < size; ++i) {
            json obj = {
                {"id", static_cast<int>(size - i)}, // Reverse order for sorting
                {"name", "item_" + std::to_string(i)},
                {"value", static_cast<int>((i * 37) % 100)} // Semi-random values
            };

            // Add nested structure for deep path tests
            if (field_depth > 1) {
                json nested = obj;
                for (int d = 1; d < field_depth; ++d) {
                    nested = {{"level" + std::to_string(d), nested}};
                }
                obj["nested"] = nested;
            }

            array.push_back(obj);
        }
        return array;
    };

    // Simple array sorting (should remain unchanged after refactor)
    const std::vector<std::size_t> simple_sizes = {50, 200, 1000, 5000};
    for (std::size_t size : simple_sizes) {
        auto simple_array = create_large_array(size);

        // Reverse order for worst-case sorting
        std::reverse(simple_array.begin(), simple_array.end());

        suite_->run_benchmark(
            "Sort_Baseline_Simple", "asc_" + std::to_string(size),
            [this, simple_array]() {
                execute_script(R"(["sort", ["$input"], "asc"])", simple_array);
            },
            size);

        suite_->run_benchmark(
            "Sort_Baseline_Simple", "desc_" + std::to_string(size),
            [this, simple_array]() {
                execute_script(R"(["sort", ["$input"], "desc"])", simple_array);
            },
            size);
    }

    // Object array sorting - single field (tests current DSU threshold logic)
    const std::vector<std::size_t> object_sizes = {50, 100, 200, 500, 1000, 2000};
    for (std::size_t size : object_sizes) {
        auto object_array = create_object_array(size);

        // Single field sorting
        suite_->run_benchmark(
            "Sort_Baseline_Object", "single_field_" + std::to_string(size),
            [this, object_array]() {
                execute_script(R"(["sort", ["$input"], "/id"])", object_array);
            },
            size);

        // Single field with direction
        suite_->run_benchmark(
            "Sort_Baseline_Object", "single_field_desc_" + std::to_string(size),
            [this, object_array]() {
                execute_script(R"(["sort", ["$input"], ["/id", "desc"]])", object_array);
            },
            size);
    }

    // Multi-field sorting (always uses DSU in current implementation)
    for (std::size_t size : {100, 500, 1000}) {
        auto object_array = create_object_array(size);

        suite_->run_benchmark(
            "Sort_Baseline_MultiField", "two_fields_" + std::to_string(size),
            [this, object_array]() {
                execute_script(R"(["sort", ["$input"], "/value", "/id"])", object_array);
            },
            size);

        suite_->run_benchmark(
            "Sort_Baseline_MultiField", "three_fields_" + std::to_string(size),
            [this, object_array]() {
                execute_script(R"(["sort", ["$input"], "/value", ["/id", "desc"], "/name"])",
                               object_array);
            },
            size);
    }

    // Deep JSON path sorting (tests path depth threshold)
    for (std::size_t size : {100, 500}) {
        auto deep_array = create_object_array(size, 4); // 4 levels deep

        suite_->run_benchmark(
            "Sort_Baseline_DeepPath", "shallow_path_" + std::to_string(size),
            [this, deep_array]() { execute_script(R"(["sort", ["$input"], "/id"])", deep_array); },
            size);

        suite_->run_benchmark(
            "Sort_Baseline_DeepPath", "deep_path_" + std::to_string(size),
            [this, deep_array]() {
                execute_script(R"(["sort", ["$input"], "/nested/level1/level2/level3/id"])",
                               deep_array);
            },
            size);
    }

    // Edge cases that test threshold boundaries
    auto boundary_test_95 = create_object_array(95);   // Just under 100 threshold
    auto boundary_test_105 = create_object_array(105); // Just over 100 threshold

    suite_->run_benchmark(
        "Sort_Baseline_Boundary", "size_95_single_field",
        [this, boundary_test_95]() {
            execute_script(R"(["sort", ["$input"], "/id"])", boundary_test_95);
        },
        95);

    suite_->run_benchmark(
        "Sort_Baseline_Boundary", "size_105_single_field",
        [this, boundary_test_105]() {
            execute_script(R"(["sort", ["$input"], "/id"])", boundary_test_105);
        },
        105);
}

// --- Debug Overhead Benchmarks ---

TEST_F(PerformanceBenchmarkTest, DebugOverheadBenchmark) {
    auto test_data = create_large_array(1000);

    // Without debugging
    suite_->run_benchmark(
        "Debug_Overhead", "no_debug",
        [this, test_data]() {
            execute_script(R"(["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]])", test_data);
        },
        1000);

    // With debugging enabled (but no breakpoints)
    suite_->run_benchmark(
        "Debug_Overhead", "debug_enabled",
        [this, test_data]() {
            computo::DebugContext debug_ctx;
            debug_ctx.set_debug_enabled(true);
            debug_ctx.set_trace_enabled(true);

            auto script = json::parse(R"(["map", ["$input"], [["x"], ["*", ["$", "/x"], 2]]])");
            computo::execute(script, {test_data}, &debug_ctx);
        },
        1000);
}
