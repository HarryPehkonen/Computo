# Thread Safety Testing for Computo

This document describes the comprehensive thread safety testing suite implemented for the Computo project.

## Overview

The thread safety test suite (`tests/test_thread_safety.cpp`) provides comprehensive testing of Computo's thread safety capabilities, ensuring that the library can be safely used in multi-threaded applications.

## Test Suite Components

### 1. Thread Safety Utilities

The test suite includes custom thread synchronization and testing utilities:

- **ThreadBarrier**: C++17-compatible barrier for synchronizing thread execution
- **ThreadSafeResultCollector**: Thread-safe collection of test results and exceptions
- **ConcurrentTimer**: Performance timing for concurrent operations
- **RaceConditionDetector**: Detection of potential race conditions using atomic operations

### 2. Test Categories

#### Core Thread Safety Tests

1. **Concurrent Script Execution with Different Inputs**
   - Tests multiple threads executing the same script with different inputs
   - Verifies isolation between concurrent executions
   - Tests thread counts: 2, 4, 8, 16

2. **Same Script Multiple Threads Stress Test**
   - Stress tests the same script across multiple threads
   - Each thread runs 100 iterations of the same computation
   - Validates result consistency across all threads

3. **ExecutionContext Thread-Local Safety**
   - Verifies that ExecutionContext instances don't interfere between threads
   - Tests variable isolation and thread-local storage
   - Each thread gets unique computation results

#### Component-Specific Tests

4. **DebugContext Thread Safety**
   - Tests DebugContext with separate instances per thread
   - Verifies debug tracing works correctly in multi-threaded environments
   - Each thread maintains its own debug state and execution trace

5. **Operator Thread Safety**
   - Tests all operator categories concurrently:
     - Arithmetic: +, -, *, /, %
     - Comparison: >, <, >=, <=, ==, !=
     - Logical: and, or, not
     - Data Access: $input, variable
     - Control Flow: if
   - Ensures all operators are thread-safe

#### Memory and Performance Tests

6. **Memory Allocation Safety**
   - Tests memory allocation/deallocation under high concurrency
   - Creates and processes large data structures in each thread
   - Verifies no memory corruption or leaks

7. **Variable Scope Isolation**
   - Tests that variable scopes don't leak between threads
   - Multiple iterations per thread to test cleanup
   - Ensures proper RAII and scope management

8. **High Concurrency Stress Test**
   - Uses 32 threads with 100 operations each (3200 total operations)
   - Random mix of different operation types
   - Performance benchmarking (target: >10k ops/second)

#### Reliability Tests

9. **Exception Handling Thread Safety**
   - Tests exception handling in multi-threaded environments
   - Verifies consistent exception behavior across threads
   - Tests division by zero, invalid operators, and invalid arguments

10. **Race Condition Detection**
    - Tests the OperatorRegistry singleton for race conditions
    - 50 threads with 100 iterations each accessing the registry
    - Validates thread-safe singleton pattern

11. **Performance Under Thread Load**
    - Measures performance scaling from 1 to 32 threads
    - Tracks operations per second and average latency
    - Validates no significant performance degradation

## Build Integration

### CMake Configuration

The thread safety tests are integrated into the CMake build system:

```cmake
# Thread Safety Tests (separate target for focused thread safety testing)
add_executable(test_thread_safety tests/test_thread_safety.cpp)
target_link_libraries(test_thread_safety PRIVATE computo GTest::gtest_main)
target_include_directories(test_thread_safety PRIVATE include tests src)

# Link with pthread on Unix platforms for thread support
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(test_thread_safety PRIVATE Threads::Threads)
endif()

# Add thread safety tests to CTest
add_test(NAME thread_safety_tests COMMAND test_thread_safety)
```

### ThreadSanitizer Integration

ThreadSanitizer can be enabled for enhanced race condition detection:

```bash
cmake -DENABLE_TSAN=ON .
make test_thread_safety
./test_thread_safety
```

## Running the Tests

### Individual Tests
```bash
# Run all thread safety tests
./test_thread_safety

# Run specific test category
./test_thread_safety --gtest_filter="ThreadSafetyTest.ConcurrentScriptExecution*"

# Run with brief output
./test_thread_safety --gtest_brief=1
```

### Through CTest
```bash
# Run through CTest
ctest -R thread_safety_tests

# Run with verbose output
ctest -R thread_safety_tests -V
```

## Test Results and Performance

### Typical Performance Metrics

- Single thread: ~66,000 operations/second
- Multi-threaded (2-32 threads): ~140,000+ operations/second
- Average latency: 0.015-0.132ms depending on thread count
- Memory usage: Stable with no detected leaks

### Thread Safety Validation

All tests pass, confirming:
- ✅ Concurrent execution safety
- ✅ ExecutionContext isolation
- ✅ Operator thread safety
- ✅ Memory allocation safety
- ✅ Variable scope isolation
- ✅ Exception handling consistency
- ✅ Performance scaling

## Key Findings

1. **OperatorRegistry**: Thread-safe singleton pattern works correctly
2. **ExecutionContext**: Proper isolation between threads
3. **Memory Management**: No leaks or corruption detected
4. **Performance**: Good scaling up to 32 threads
5. **Exception Handling**: Consistent behavior across threads

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

## Future Enhancements

Potential areas for improvement:
- Additional stress testing with larger data sets
- Testing with thread pools and async operations
- Performance benchmarking on different hardware
- Integration with popular threading libraries
