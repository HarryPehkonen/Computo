# Computo Performance Benchmarking Guide

This document describes the comprehensive performance benchmarking system for the Computo project, including how to run benchmarks, interpret results, and optimize performance.

## Overview

The Computo performance benchmarking system provides:

- **Comprehensive Benchmarks**: Tests for all major operator categories
- **Scalability Analysis**: Performance testing with varying data sizes
- **Memory Usage Tracking**: Monitor memory consumption during execution
- **Debug Overhead Analysis**: Compare performance with/without debugging
- **Trend Analysis**: Track performance changes over time
- **Regression Detection**: Automatically identify performance regressions

## Running Benchmarks

### Quick Start

```bash
# Build the project with performance benchmarks
cmake -B build
make -C build test_performance

# Run all performance benchmarks
./build/test_performance

# Or use the convenience target
make -C build benchmark
```

### Advanced Usage

```bash
# Run with specific iterations (higher = more accurate but slower)
./build/test_performance --gtest_filter="*Benchmark*"

# Run only specific benchmark categories
./build/test_performance --gtest_filter="*ArithmeticOperationsBenchmark*"
./build/test_performance --gtest_filter="*ArrayOperationsBenchmark*"
./build/test_performance --gtest_filter="*MemoryIntensiveBenchmark*"

# Run with memory debugging enabled (to catch leaks during benchmarking)
cmake -B build -DENABLE_ASAN=ON
make -C build test_performance
./build/test_performance
```

## Benchmark Categories

### 1. Arithmetic Operations
- Basic arithmetic (+, -, *, /, %)
- Large number operations
- Nested arithmetic expressions
- Many-operand operations (100+ operands)

### 2. Array Operations
- Map, filter, reduce operations
- Array scaling tests (100, 1K, 10K, 100K elements)
- Chained operations (map → filter → reduce)
- Functional programming primitives (car, cdr, cons, append)

### 3. String Operations
- String operations (join, strConcat)
- Bulk string processing
- String array operations

### 4. Object Operations
- Key/value extraction
- Object transformation (pick, omit)
- Large object processing (50, 500, 2K+ properties)

### 5. Logical Operations
- Boolean logic (and, or, not)
- Complex nested logical expressions
- Many-operand logical operations

### 6. Comparison Operations
- Equality and relational comparisons
- Array filtering with comparisons
- Complex comparison expressions

### 7. Control Flow
- Conditional statements (if/then/else)
- Variable scoping and binding
- Nested control structures

### 8. Memory Intensive Operations
- Large data structure processing
- Memory allocation patterns
- Garbage collection performance

### 9. Lambda and Function Calls
- Lambda function performance
- Nested function calls
- Variable capture and scoping

### 10. Debug Overhead Analysis
- Performance with/without debugging
- Breakpoint overhead
- Trace collection impact

### 11. Real-world Scenarios
- Data processing pipelines
- JSON transformation workflows
- Complex analytical queries

## Output and Results

### Console Output
The benchmark system provides detailed console output including:

```
=== Performance Benchmark Results ===
Test Name                Operation      Data Size  Avg (ms)    Min (ms)    Max (ms)    P95 (ms)    Ops/sec     Peak KB    
--------------------------------------------------------------------------------------------------------
Arithmetic_Small         addition       1          0.010       0.008       0.025       0.015       100000      1024       
Array_Map               map            1000       2.450       2.100       3.200       2.800       408         2048       
String_Array            map_join       5000       12.300      11.800      15.600      14.200      81          4096       
```

### File Exports

#### CSV Export (`performance_results.csv`)
Detailed results exported for spreadsheet analysis:
- Test name, operation, data size
- All timing metrics (avg, min, max, percentiles)
- Memory usage statistics
- Operations per second

#### JSON Export (`performance_results.json`)
Machine-readable results for automated analysis:
```json
{
  "timestamp": 1634567890123,
  "results": [
    {
      "test_name": "Arithmetic_Small",
      "operation": "addition",
      "data_size": 1,
      "avg_time_ms": 0.010,
      "min_time_ms": 0.008,
      "max_time_ms": 0.025,
      "p50_time_ms": 0.010,
      "p95_time_ms": 0.015,
      "p99_time_ms": 0.020,
      "ops_per_second": 100000,
      "memory_peak_kb": 1024,
      "memory_after_kb": 1020
    }
  ]
}
```

## Performance Analysis Tools

### Python Analysis Script

Use the provided Python script for advanced analysis:

```bash
# Generate summary report
python3 scripts/performance_analyzer.py performance_results.json

# Compare with baseline for regression detection
python3 scripts/performance_analyzer.py performance_results.json --baseline baseline_results.json

# Export detailed CSV analysis
python3 scripts/performance_analyzer.py performance_results.json --csv detailed_analysis.csv

# Generate performance charts
python3 scripts/performance_analyzer.py performance_results.json --charts --charts-dir ./charts/

# Full analysis with all outputs
python3 scripts/performance_analyzer.py performance_results.json \
    --baseline baseline_results.json \
    --output full_report.txt \
    --csv detailed_results.csv \
    --charts
```

### Analysis Features

#### Regression Detection
- Automatically compares current results with baseline
- Identifies operations that are >20% slower
- Provides regression percentage for each affected operation

#### Scaling Analysis
- Analyzes how operations scale with data size
- Estimates algorithmic complexity (O(n), O(n²), etc.)
- Identifies operations with poor scaling characteristics

#### Optimization Recommendations
- Highlights operations in the 95th percentile for execution time
- Identifies high memory usage operations
- Suggests optimization opportunities based on scaling analysis

## Performance Optimization Guidelines

### 1. Identifying Bottlenecks

Look for operations with:
- High average execution time (>10ms for simple operations)
- Poor scaling (time ratio > size ratio²)
- High memory usage (>1MB for small datasets)
- High P99 latency (indicating inconsistent performance)

### 2. Common Performance Issues

#### Algorithmic Complexity
```
Scaling Factor > 2.0: Worse than O(n²) scaling
Scaling Factor 1.0-2.0: Between O(n) and O(n²)
Scaling Factor < 1.0: Better than O(n) (possibly O(log n))
```

#### Memory Usage
- Operations using >10MB for <10K elements may have memory leaks
- Growing memory usage across iterations indicates poor cleanup
- High peak memory vs. post-operation memory suggests temporary allocations

#### Consistency
- High P99/P50 ratio (>3x) indicates inconsistent performance
- Large max/min time ratios suggest system interference or GC pauses

### 3. Optimization Strategies

#### For High Execution Time:
1. Profile specific operator implementations
2. Consider algorithm improvements
3. Optimize hot paths identified by benchmarks
4. Add operation-specific optimizations

#### For Poor Scaling:
1. Review algorithmic complexity
2. Consider data structure changes
3. Implement caching where appropriate
4. Add early termination conditions

#### For High Memory Usage:
1. Check for memory leaks
2. Optimize data structure usage
3. Implement memory pooling
4. Add explicit cleanup calls

## Continuous Integration

### Baseline Management
1. Run benchmarks on stable releases to establish baselines
2. Store baseline files in version control
3. Update baselines after significant optimizations
4. Use regression detection in CI/CD pipelines

### Performance Testing in CI
```yaml
# Example GitHub Actions workflow
- name: Run Performance Benchmarks
  run: |
    make benchmark
    python3 scripts/performance_analyzer.py performance_results.json \
      --baseline baselines/v1.0.0_baseline.json \
      --output performance_report.txt

- name: Check for Regressions
  run: |
    if grep -q "Performance Regressions Detected" performance_report.txt; then
      echo "Performance regression detected!"
      cat performance_report.txt
      exit 1
    fi
```

## Benchmark Customization

### Adding New Benchmarks

1. **Simple Operation Benchmark**:
```cpp
TEST_F(PerformanceBenchmarkTest, CustomOperationBenchmark) {
    suite_->run_benchmark("Custom_Operation", "my_op", [this]() {
        execute_script(R"(["my_operator", "arg1", "arg2"])");
    });
}
```

2. **Scaling Benchmark**:
```cpp
TEST_F(PerformanceBenchmarkTest, CustomScalingBenchmark) {
    const std::vector<std::size_t> sizes = {100, 1000, 10000};
    
    for (std::size_t size : sizes) {
        auto large_data = create_large_array(size);
        
        suite_->run_benchmark("Custom_Scaling", "my_op", [this, large_data]() {
            execute_script(R"(["my_operator", ["$", ""]])", large_data);
        }, size);
    }
}
```

3. **Memory-focused Benchmark**:
```cpp
suite_->run_benchmark("Memory_Test", "large_allocation", [this]() {
    // Operation that allocates significant memory
    execute_script(R"(["create_large_structure", 100000])");
}, 100000, 50); // Fewer iterations for memory-intensive operations
```

### Customizing Measurement Parameters

```cpp
// Custom iteration count
suite_->run_benchmark("Fast_Operation", "quick_op", benchmark_func, data_size, 10000);

// Custom data size for reporting
suite_->run_benchmark("Complex_Operation", "complex_op", benchmark_func, 42);
```

## Interpreting Results

### Performance Metrics

- **Average Time**: Most important for typical performance
- **P95 Time**: Important for user-facing operations (95% of operations complete within this time)
- **P99 Time**: Critical for identifying outliers and tail latency
- **Operations/Second**: Useful for throughput-critical applications
- **Memory Peak**: Important for memory-constrained environments

### When to Investigate

- **Regressions >20%**: Significant performance degradation requiring investigation
- **Scaling Factor >2.0**: Algorithm may need optimization
- **Memory Growth**: Potential memory leaks or inefficient allocation patterns
- **High Variance (P99/P50 >5x)**: Inconsistent performance needs investigation

## Troubleshooting

### Common Issues

1. **Inconsistent Results**:
   - Run benchmarks on dedicated hardware
   - Close other applications
   - Run multiple times and average results

2. **Memory Measurement Not Working**:
   - Linux: Ensure `/proc/self/status` is accessible
   - Other platforms: Memory tracking returns 0 (feature limitation)

3. **Build Issues**:
   - Ensure GTest is properly installed
   - Check that threading support is available
   - Verify nlohmann/json dependency

4. **Performance Seems Wrong**:
   - Check if debug builds are being used (use Release builds for benchmarks)
   - Verify compiler optimizations are enabled
   - Ensure CPU frequency scaling is disabled

### Best Practices

1. **Run on Consistent Hardware**: Use the same machine/environment for comparisons
2. **Use Release Builds**: Debug builds can be 10x+ slower
3. **Multiple Runs**: Average results across multiple benchmark runs
4. **System Isolation**: Close other applications during benchmarking
5. **Thermal Throttling**: Allow CPU to cool between intensive benchmark runs

## Contributing

When adding new operators or modifying existing ones:

1. Add corresponding performance benchmarks
2. Test with multiple data sizes if applicable
3. Run regression tests against previous baselines
4. Update this documentation if adding new benchmark categories

For questions or issues with the performance benchmarking system, please refer to the main project documentation or file an issue in the project repository.
