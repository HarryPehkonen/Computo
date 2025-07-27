# Testing Strategy

This document describes the comprehensive testing strategy for the Computo project, covering memory safety, thread safety, and performance validation.

## Overview

The testing strategy includes two main components:

1. **Memory Safety Testing**: Comprehensive memory leak detection and testing system
2. **Thread Safety Testing**: Multi-threaded execution safety and performance validation

Both test suites ensure Computo can be safely used in production environments with high reliability and performance requirements.

## Memory Safety Testing

### Components

The memory testing system includes:

1. **Memory safety test suite** (`tests/test_memory_safety.cpp`)
2. **CMake integration** for memory debugging flags 
3. **Automated test runner** (`scripts/run_memory_tests.sh`)
4. **Built-in memory monitoring** utilities
5. **Integration with sanitizers** (AddressSanitizer, MemorySanitizer, etc.)
6. **Valgrind integration** for detailed leak detection

### Quick Start

#### Basic Memory Testing
```bash
# Build and run memory safety tests
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make test_memory_safety
./test_memory_safety
```

#### Comprehensive Memory Testing
```bash
# Run all memory tests with different tools
./scripts/run_memory_tests.sh [build_directory]
```

#### With AddressSanitizer
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make test_memory_safety
./test_memory_safety
```

### Memory Test Suite

The memory safety test suite includes:

- **Basic Operations Memory Stress**: Tests repeated arithmetic operations for leaks
- **Input Processing Memory Test**: Tests large input handling and JSON pointer access
- **Multiple Input Processing**: Tests handling of multiple input files
- **Variable Memory Management**: Tests let expressions and variable scoping
- **Exception Handling Cleanup**: Tests memory cleanup during exceptions
- **Debug Context Memory Management**: Tests debug tracing memory management
- **Large Object Operations**: Tests object creation and property access
- **Nested Structure Operations**: Tests deeply nested JSON structures
- **Memory Stress Test**: Mixed operations under load
- **ExecutionContext Scope Memory**: Tests execution context lifecycle

#### Built-in Memory Monitoring

Each test includes a `MemoryMonitor` class that:
- Tracks RSS (Resident Set Size) memory usage on Linux
- Monitors peak memory consumption during tests
- Detects significant memory leaks (>10MB threshold)
- Fails tests if excessive memory growth is detected

### Memory Debugging Options

The build system supports several memory debugging options:

```bash
# AddressSanitizer - detects memory errors
cmake -DENABLE_ASAN=ON ..

# MemorySanitizer - detects uninitialized memory access  
cmake -DENABLE_MSAN=ON ..

# ThreadSanitizer - detects race conditions
cmake -DENABLE_TSAN=ON ..

# UndefinedBehaviorSanitizer - detects undefined behavior
cmake -DENABLE_UBSAN=ON ..
```

When enabled, these flags are automatically applied:
- Compiler flags: `-fsanitize=address -fno-omit-frame-pointer -g`
- Linker flags: `-fsanitize=address`
- Applied to all targets: library, executables, and tests

### Comprehensive Test Runner

The test runner (`scripts/run_memory_tests.sh`) provides:

#### Features
- **Multiple sanitizer testing**: Runs tests with ASan, UBSan, etc.
- **Valgrind integration**: Detailed leak detection (if available)
- **Custom memory monitoring**: Built-in memory usage tracking
- **Stress testing**: High-load memory scenarios
- **Pattern analysis**: Runtime memory usage monitoring
- **Timeout protection**: Prevents hanging tests (300s default)
- **Report generation**: Comprehensive test results
- **CI/CD integration**: Works in continuous integration environments

#### Usage
```bash
# Basic usage
./scripts/run_memory_tests.sh

# Custom build directory
./scripts/run_memory_tests.sh build-debug

# With cleanup
CLEANUP_BUILD=true ./scripts/run_memory_tests.sh

# Custom timeout (in seconds)
TIMEOUT_SECONDS=600 ./scripts/run_memory_tests.sh
```

### Memory Testing Best Practices

#### Regular Testing Schedule
Run memory tests:
- Before major releases
- After significant changes to core evaluation logic
- When adding new operators or features
- In CI/CD pipelines

#### Understanding Results

**Memory Growth Thresholds**:
- **Acceptable**: < 10MB growth during test execution
- **Warning**: 10-50MB growth (investigate)
- **Error**: > 50MB growth (likely leak)

**AddressSanitizer Output**:
- **Heap buffer overflow**: Array bounds violations
- **Use after free**: Accessing freed memory
- **Memory leak**: Unfreed allocations at exit

**Valgrind Output**:
- **Definitely lost**: Clear memory leaks
- **Indirectly lost**: Leaked data referenced by leaked blocks
- **Possibly lost**: Ambiguous pointers
- **Still reachable**: Memory not freed but still accessible

## Thread Safety Testing

### Test Suite Components

The thread safety test suite (`tests/test_thread_safety.cpp`) includes custom thread synchronization and testing utilities:

- **ThreadBarrier**: C++17-compatible barrier for synchronizing thread execution
- **ThreadSafeResultCollector**: Thread-safe collection of test results and exceptions
- **ConcurrentTimer**: Performance timing for concurrent operations
- **RaceConditionDetector**: Detection of potential race conditions using atomic operations

### Core Thread Safety Tests

#### 1. Concurrent Script Execution with Different Inputs
- Tests multiple threads executing the same script with different inputs
- Verifies isolation between concurrent executions
- Tests thread counts: 2, 4, 8, 16

#### 2. Same Script Multiple Threads Stress Test
- Stress tests the same script across multiple threads
- Each thread runs 100 iterations of the same computation
- Validates result consistency across all threads

#### 3. ExecutionContext Thread-Local Safety
- Verifies that ExecutionContext instances don't interfere between threads
- Tests variable isolation and thread-local storage
- Each thread gets unique computation results

### Component-Specific Tests

#### 4. DebugContext Thread Safety
- Tests DebugContext with separate instances per thread
- Verifies debug tracing works correctly in multi-threaded environments
- Each thread maintains its own debug state and execution trace

#### 5. Operator Thread Safety
Tests all operator categories concurrently:
- Arithmetic: +, -, *, /, %
- Comparison: >, <, >=, <=, ==, !=
- Logical: and, or, not
- Data Access: $input, variable
- Control Flow: if

### Memory and Performance Tests

#### 6. Memory Allocation Safety
- Tests memory allocation/deallocation under high concurrency
- Creates and processes large data structures in each thread
- Verifies no memory corruption or leaks

#### 7. Variable Scope Isolation
- Tests that variable scopes don't leak between threads
- Multiple iterations per thread to test cleanup
- Ensures proper RAII and scope management

#### 8. High Concurrency Stress Test
- Uses 32 threads with 100 operations each (3200 total operations)
- Random mix of different operation types
- Performance benchmarking (target: >10k ops/second)

### Reliability Tests

#### 9. Exception Handling Thread Safety
- Tests exception handling in multi-threaded environments
- Verifies consistent exception behavior across threads
- Tests division by zero, invalid operators, and invalid arguments

#### 10. Race Condition Detection
- Tests the OperatorRegistry singleton for race conditions
- 50 threads with 100 iterations each accessing the registry
- Validates thread-safe singleton pattern

#### 11. Performance Under Thread Load
- Measures performance scaling from 1 to 32 threads
- Tracks operations per second and average latency
- Validates no significant performance degradation

## Running Tests

### Thread Safety Tests

```bash
# Run all thread safety tests
./test_thread_safety

# Run specific test category
./test_thread_safety --gtest_filter="ThreadSafetyTest.ConcurrentScriptExecution*"

# Run with brief output
./test_thread_safety --gtest_brief=1

# Run through CTest
ctest -R thread_safety_tests

# Run with verbose output
ctest -R thread_safety_tests -V
```

### ThreadSanitizer Integration

ThreadSanitizer can be enabled for enhanced race condition detection:

```bash
cmake -DENABLE_TSAN=ON .
make test_thread_safety
./test_thread_safety
```

## Performance Metrics

### Typical Thread Safety Performance
- Single thread: ~66,000 operations/second
- Multi-threaded (2-32 threads): ~140,000+ operations/second
- Average latency: 0.015-0.132ms depending on thread count
- Memory usage: Stable with no detected leaks

### Memory Testing Performance
- **Basic memory tests**: < 1 minute
- **AddressSanitizer**: 2-5x slower, ~5 minutes
- **Valgrind**: 10-20x slower, ~15 minutes
- **Full test suite**: 20-30 minutes

## Usage Recommendations

### For Multi-threaded Applications

1. **Each thread should create its own ExecutionContext**:
   ```cpp
   // Good - each thread has its own context
   void worker_thread(const json& input) {
       computo::ExecutionContext ctx(input);
       auto result = computo::execute(script, {input});
   }
   ```

2. **DebugContext should be thread-local**:
   ```cpp
   // Good - separate debug context per thread
   thread_local computo::DebugContext debug_ctx;
   ```

3. **OperatorRegistry is safe to access concurrently**:
   ```cpp
   // Safe - singleton is thread-safe
   auto& registry = computo::OperatorRegistry::get_instance();
   ```

### Performance Considerations
- Computo scales well with multiple threads
- Optimal performance typically achieved with 4-8 threads
- Memory overhead is minimal per thread
- Exception handling has negligible performance impact

## CI/CD Integration

### GitHub Actions Example
```yaml
- name: Memory Testing
  run: |
    ./scripts/run_memory_tests.sh build
  env:
    CLEANUP_BUILD: true
    TIMEOUT_SECONDS: 600

- name: Thread Safety Testing
  run: |
    cmake -DENABLE_TSAN=ON .
    make test_thread_safety
    ./test_thread_safety
```

### Expected Exit Codes
- `0`: All tests passed
- `1`: Some tests failed
- `2`: Script error (missing dependencies, etc.)

## Key Findings

Thread safety validation confirms:
-   Concurrent execution safety
-   ExecutionContext isolation
-   Operator thread safety
-   Memory allocation safety
-   Variable scope isolation
-   Exception handling consistency
-   Performance scaling

Memory safety validation confirms:
-   No memory leaks in core operations
-   Proper cleanup on exception paths
-   Efficient memory usage patterns
-   Safe handling of large data structures

## Troubleshooting

### Common Issues

#### AddressSanitizer Crashes
```bash
# Check for stack overflow
export ASAN_OPTIONS="detect_stack_use_after_return=false"
```

#### Valgrind Not Found
```bash
# Install on Ubuntu/Debian
sudo apt-get install valgrind

# Install on macOS
brew install valgrind
```

#### Tests Timeout
- Increase `TIMEOUT_SECONDS` environment variable
- Run subset of tests for debugging
- Check for infinite loops

### Debug Commands
```bash
# Run specific test with verbose output
./test_memory_safety --gtest_filter=*BasicOperations* --gtest_verbose

# Run with AddressSanitizer environment variables
export ASAN_OPTIONS="verbosity=1:detect_leaks=1"
./test_memory_safety

# Run with Valgrind manually
valgrind --leak-check=full --show-leak-kinds=all ./test_memory_safety
```

## Architecture Notes

### Memory Management Strategy
Computo uses RAII and smart pointers:
- **No raw new/delete**: Checked by `scripts/check_no_raw_memory.sh`
- **std::unique_ptr**: For owned resources
- **std::shared_ptr**: For shared resources (ExecutionContext)
- **Stack allocation**: Preferred for temporary objects

### Critical Memory Areas
1. **ExecutionContext**: Variable storage and scope management
2. **DebugContext**: Execution trace storage
3. **OperatorRegistry**: Operator function storage
4. **TailCall**: Tail call optimization structures
5. **JSON handling**: Large JSON object/array processing

### Memory Safety Features
- **RAII**: Automatic cleanup on scope exit
- **Smart pointers**: Automatic memory management
- **Exception safety**: Cleanup on exception paths
- **Tail call optimization**: Prevents stack overflow
- **Bounded recursion**: Limits in recursive operations

This comprehensive testing strategy ensures Computo remains reliable, thread-safe, and memory-efficient across all use cases.
