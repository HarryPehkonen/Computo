# Computo Memory Leak Detection and Testing

This document describes the comprehensive memory leak detection and testing system implemented for the Computo project.

## Overview

The memory testing system includes:

1. **Memory safety test suite** (`tests/test_memory_safety.cpp`)
2. **CMake integration** for memory debugging flags 
3. **Automated test runner** (`scripts/run_memory_tests.sh`)
4. **Built-in memory monitoring** utilities
5. **Integration with sanitizers** (AddressSanitizer, MemorySanitizer, etc.)
6. **Valgrind integration** for detailed leak detection

## Quick Start

### Basic Memory Testing

```bash
# Build and run memory safety tests
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make test_memory_safety
./test_memory_safety
```

### Comprehensive Memory Testing

```bash
# Run all memory tests with different tools
./scripts/run_memory_tests.sh [build_directory]
```

### With AddressSanitizer

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make test_memory_safety
./test_memory_safety
```

## Memory Testing Components

### 1. Test Suite (`tests/test_memory_safety.cpp`)

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

### 2. CMake Integration

#### Memory Debugging Flags

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

#### Flags Applied

When enabled, these flags are automatically applied:
- Compiler flags: `-fsanitize=address -fno-omit-frame-pointer -g`
- Linker flags: `-fsanitize=address`
- Applied to all targets: library, executables, and tests

### 3. Test Runner Script (`scripts/run_memory_tests.sh`)

The comprehensive test runner provides:

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

#### Test Types Run

1. **Custom Memory Monitoring**: Built-in memory usage tracking
2. **AddressSanitizer**: Memory error detection (buffer overflows, use-after-free)
3. **UndefinedBehaviorSanitizer**: Undefined behavior detection
4. **Valgrind**: Detailed memory leak analysis (if available)
5. **Stress Testing**: High-load scenarios with optimized builds
6. **Memory Pattern Analysis**: Runtime memory usage monitoring

## Integration with CI/CD

### GitHub Actions Example

```yaml
- name: Memory Testing
  run: |
    ./scripts/run_memory_tests.sh build
  env:
    CLEANUP_BUILD: true
    TIMEOUT_SECONDS: 600
```

### Expected Exit Codes

- `0`: All tests passed
- `1`: Some tests failed
- `2`: Script error (missing dependencies, etc.)

## Memory Testing Best Practices

### 1. Regular Testing

Run memory tests:
- Before major releases
- After significant changes to core evaluation logic
- When adding new operators or features
- In CI/CD pipelines

### 2. Understanding Results

#### Memory Growth Thresholds

- **Acceptable**: < 10MB growth during test execution
- **Warning**: 10-50MB growth (investigate)
- **Error**: > 50MB growth (likely leak)

#### AddressSanitizer Output

- **Heap buffer overflow**: Array bounds violations
- **Use after free**: Accessing freed memory
- **Memory leak**: Unfreed allocations at exit

#### Valgrind Output

- **Definitely lost**: Clear memory leaks
- **Indirectly lost**: Leaked data referenced by leaked blocks
- **Possibly lost**: Ambiguous pointers
- **Still reachable**: Memory not freed but still accessible

### 3. Common Issues and Solutions

#### False Positives

Some libraries may show "still reachable" memory in Valgrind:
- JSON library static data
- Google Test framework data
- Standard library allocations

These are usually suppressed in `valgrind.supp`.

#### Real Leaks

Look for:
- Allocations in Computo core code (`computo::*`)
- Growing memory usage over test iterations
- Memory not freed in exception paths

## Performance Considerations

### Test Execution Times

- **Basic memory tests**: < 1 minute
- **AddressSanitizer**: 2-5x slower, ~5 minutes
- **Valgrind**: 10-20x slower, ~15 minutes
- **Full test suite**: 20-30 minutes

### Resource Usage

- **Memory overhead**: ASan uses ~3x memory
- **CPU overhead**: Varies by tool (2-20x slower)
- **Disk space**: Debug builds are larger

## Troubleshooting

### Common Issues

#### 1. AddressSanitizer Crashes

```bash
# Check for stack overflow
export ASAN_OPTIONS="detect_stack_use_after_return=false"
```

#### 2. Valgrind Not Found

```bash
# Install on Ubuntu/Debian
sudo apt-get install valgrind

# Install on macOS
brew install valgrind
```

#### 3. False Memory Leak Reports

- Check if it's in third-party code
- Add suppressions to `valgrind.supp`
- Verify with multiple tools

#### 4. Tests Timeout

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

## Extending the Test Suite

### Adding New Memory Tests

1. **Create test in test_memory_safety.cpp**:

```cpp
TEST_F(MemorySafetyTest, MyNewMemoryTest) {
    memory_monitor_.update_peak_memory();
    
    // Your memory-intensive operation here
    for (size_t i = 0; i < 1000; ++i) {
        // Test code
    }
    
    memory_monitor_.update_peak_memory();
    // TearDown() will check for leaks
}
```

2. **Test scenarios to include**:
   - Large data structure operations
   - Exception handling paths
   - Recursive operations
   - Long-running computations
   - Resource cleanup

3. **Memory monitoring**:
   - Call `memory_monitor_.update_peak_memory()` before and after intensive operations
   - Use reasonable iteration counts (balance thoroughness vs speed)
   - Test both success and error paths

### Adding New Sanitizers

1. **Update CMakeLists.txt**:

```cmake
option(ENABLE_NEWSAN "Enable NewSanitizer" OFF)

if(ENABLE_NEWSAN)
    list(APPEND MEMORY_DEBUG_FLAGS -fsanitize=new)
    list(APPEND MEMORY_DEBUG_LIBS -fsanitize=new)
    message(STATUS "NewSanitizer enabled")
endif()
```

2. **Update run_memory_tests.sh**:

```bash
run_newsan_tests() {
    log_info "Running NewSanitizer tests..."
    local newsan_build_dir
    newsan_build_dir=$(build_with_sanitizer "NewSanitizer" "newsan" "-DENABLE_NEWSAN=ON")
    # ... test execution
}
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

This comprehensive memory testing system helps ensure Computo remains memory-safe and leak-free across all use cases.
