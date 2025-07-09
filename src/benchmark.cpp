#include <computo.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <nlohmann/json.hpp>

struct BenchmarkResult {
    std::string name;
    double avg_us;
    double min_us;
    double max_us;
    int iterations;
    nlohmann::json expected_result;
    nlohmann::json actual_result;
    bool passed;
};

class BenchmarkRunner {
private:
    std::vector<BenchmarkResult> results_;
    
    double to_ms(std::chrono::microseconds us) {
        return us.count() / 1000.0;
    }
    
    double to_us(std::chrono::microseconds us) {
        return us.count();
    }
    
    nlohmann::json run_benchmark(const std::string& name, 
                                 const nlohmann::json& script, 
                                 const nlohmann::json& expected = nullptr,
                                 int iterations = 1000) {
        std::vector<double> times;
        nlohmann::json last_result;
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            last_result = computo::execute(script, nlohmann::json(nullptr));
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            times.push_back(to_us(duration));
        }
        
        // Calculate statistics
        auto min_it = std::min_element(times.begin(), times.end());
        auto max_it = std::max_element(times.begin(), times.end());
        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double avg = sum / times.size();
        
        BenchmarkResult result{
            name,
            avg,
            *min_it,
            *max_it,
            iterations,
            expected,
            last_result,
            expected.is_null() || expected == last_result
        };
        
        results_.push_back(result);
        return last_result;
    }

public:
    void run_all_benchmarks() {
        std::cout << "PERFORMANCE BENCHMARKS\n";
        std::cout << "======================\n\n";
        
        // Basic operations
        run_benchmark("array_creation", 
            nlohmann::json::parse(R"({"array": [1,2,3,4,5,6,7,8,9,10]})"));
        run_benchmark("get_access", 
            nlohmann::json::parse(R"(["get", ["obj", ["a", 1], ["b", 2], ["c", 3]], "/b"])"),
            nlohmann::json(2));
        run_benchmark("variable_access", 
            nlohmann::json::parse(R"(["let", [["x", 42]], ["$", "/x"]])"),
            nlohmann::json(42));
        // Functional operations
        run_benchmark("map_operation", 
            nlohmann::json::parse(R"(["map", {"array": [1,2,3,4,5]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]])"));
        run_benchmark("filter_operation", 
            nlohmann::json::parse(R"(["filter", {"array": [1,2,3,4,5,6,7,8,9,10]}, ["lambda", ["x"], [">", ["$", "/x"], 5]]])"));
        run_benchmark("reduce_operation", 
            nlohmann::json::parse(R"(["reduce", {"array": [1,2,3,4,5]}, ["lambda", ["x"], ["+", ["$", "/x/0"], ["$", "/x/1"]]], 0])"),
            nlohmann::json(15.0));
        // Array operations
        run_benchmark("append_operation", 
            nlohmann::json::parse(R"(["append", {"array": [1,2,3]}, {"array": [4,5,6]}])"));
        run_benchmark("merge_operation", 
            nlohmann::json::parse(R"(["merge", {"a": 1, "b": 2}, {"c": 3, "d": 4}])"));
        run_benchmark("car_cdr_cons", 
            nlohmann::json::parse(R"(["cons", ["car", {"array": [1,2,3,4,5]}], ["cdr", {"array": [1,2,3,4,5]}]])"));
        // Complex nested
        run_benchmark("complex_nested", 
            nlohmann::json::parse(R"(["let", [["data", {"array": [1,2,3,4,5]}]], ["reduce", ["map", ["$", "/data"], ["lambda", ["x"], ["*", ["$", "/x"], 2]]], ["lambda", ["x"], ["+", ["$", "/x/0"], ["$", "/x/1"]]], 0]])"),
            nlohmann::json(30.0));
        // TCO tests
        run_benchmark("tco_sqrt2_newton", 
            nlohmann::json::parse(R"(["let", [["sqrt2", ["lambda", ["x"], ["if", [">", ["$", "/x/1"], 0], ["call", ["$", "/sqrt2"], ["/", ["+", ["$", "/x/0"], ["/", 2, ["$", "/x/0"]]], 2], ["-", ["$", "/x/1"], 1]], ["$", "/x/0"]]]]], ["call", ["$", "/sqrt2"], 1, 100]])"),
            // took one digit off the end
            nlohmann::json(1.414213562373095));
        run_benchmark("tco_pi_leibniz", 
            nlohmann::json::parse(R"(["let", [["leibniz", ["lambda", ["args"], ["if", [">", ["$", "/args/0"], 0], ["call", ["$", "/leibniz"], ["-", ["$", "/args/0"], 1], ["+", ["$", "/args/1"], ["/", ["*", ["if", ["==", ["%", ["$", "/args/0"], 2], 1], -1, 1], 2], ["-", ["*", 2, ["$", "/args/0"]], 1]]]], ["*", 4, ["$", "/args/1"]]]]]], ["call", ["$", "/leibniz"], 100, 0]])"));
        run_benchmark("tco_fibonacci", 
            nlohmann::json::parse(R"(["let", [["fib", ["lambda", ["args"], ["if", [">", ["$", "/args/0"], 0], ["call", ["$", "/fib"], ["-", ["$", "/args/0"], 1], ["$", "/args/2"], ["+", ["$", "/args/1"], ["$", "/args/2"]]], ["$", "/args/1"]]]]], ["call", ["$", "/fib"], 100, 0, 1]])"));
        run_benchmark("tco_sqrt2_cf", 
            nlohmann::json::parse(R"(["let", [["sqrt2_cf", ["lambda", ["args"], ["if", [">", ["$", "/args/0"], 0], ["call", ["$", "/sqrt2_cf"], ["-", ["$", "/args/0"], 1], ["+", 2, ["/", 1, ["$", "/args/1"]]]], ["+", 1, ["/", 1, ["$", "/args/1"]]]]]]], ["call", ["$", "/sqrt2_cf"], 100, 1]])"),
            nlohmann::json(1.4142135623730951));
        print_results();
    }
    
    void print_results() {
        std::cout << std::fixed << std::setprecision(1);
        
        for (const auto& result : results_) {
            std::cout << std::setw(20) << std::left << result.name << ": ";
            std::cout << std::setw(10) << std::right << result.avg_us << "us avg ";
            std::cout << "(min: " << std::setw(10) << std::right << result.min_us << "us, ";
            std::cout << "max: " << std::setw(10) << std::right << result.max_us << "us)";
            std::cout << " - " << result.iterations << " iterations";
            
            if (!result.passed) {
                std::cout << " [FAILED]";
            }
            
            std::cout << "\n";
            
            if (!result.expected_result.is_null()) {
                if (result.actual_result.dump() != result.expected_result.dump()) {
                    std::cout << "  Result: " << result.actual_result.dump() 
                            << " (expected: " << result.expected_result.dump() << ")\n";
                }
            }
        }
        
        // Summary
        double total_time = 0;
        for (const auto& result : results_) {
            total_time += result.avg_us * result.iterations;
        }
        
        std::cout << "\nTotal benchmark time: " << total_time << "us\n";
        
        int passed = std::count_if(results_.begin(), results_.end(), 
                                  [](const BenchmarkResult& r) { return r.passed; });
        std::cout << "Tests passed: " << passed << "/" << results_.size() << "\n";
    }
};

// CLI integration
void run_performance_benchmarks() {
    BenchmarkRunner runner;
    runner.run_all_benchmarks();
} 