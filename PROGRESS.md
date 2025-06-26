# PROGRESS.md - Computo Implementation Log

This document tracks the implementation progress, decisions, and lessons learned during the development of the Computo library.

## Phase 1: The Skeleton (COMPLETED)

**Date Range**: June 26, 2025

### What Works

1. **CMake Build System**
   - Successfully configured with nlohmann/json dependency
   - Permuto library integration via find_library/find_path (not pkg-config)
   - All warnings enabled and treated as errors (-Wall -Wextra -Wpedantic -Werror)
   - Clean build with no warnings

2. **Core Architecture**
   - Recursive `evaluate()` function works as designed
   - Operator dispatch mechanism using `std::map<std::string, OperatorFunc>` is efficient
   - ExecutionContext pattern works well for maintaining state
   - Static operator initialization prevents re-initialization overhead

3. **Exception Hierarchy**
   - Three-level hierarchy (std::exception -> ComputoException -> specific exceptions) works perfectly
   - Exception messages are clear and descriptive
   - Inheritance allows flexible catch handling

4. **Literal Value Handling**
   - All JSON types handled correctly (numbers, strings, booleans, null, arrays, objects)
   - Arrays with non-string first element correctly treated as literals
   - Empty arrays handled properly

5. **Addition Operator**
   - Integer addition works correctly
   - Float addition works correctly  
   - Mixed integer/float addition with proper type promotion
   - Nested expressions work (e.g., `["+", ["+", 1, 2], 3]`)
   - Comprehensive argument validation

6. **$input Operator**
   - Provides access to input context as expected
   - Simple but essential functionality

7. **CLI Tool**
   - File-based input/output works correctly
   - Proper error handling and reporting
   - JSON pretty-printing with 2-space indentation

8. **Test Suite**
   - 13 comprehensive test cases covering all functionality
   - Split error tests provide precise failure diagnosis
   - Exception hierarchy verification ensures API contract compliance
   - 100% test pass rate

### What Didn't Work (Issues Resolved)

1. **Permuto Library Discovery**
   - **Issue**: Initial CMake configuration used pkg-config, but Permuto installed via "make install" doesn't provide .pc files
   - **Solution**: Switched to find_library/find_path approach targeting /usr/local/lib and /usr/local/include
   - **Lesson**: When integrating third-party libraries, check their installation method and available discovery mechanisms

2. **CLI Executable Location**
   - **Issue**: Initial confusion about executable location (expected in build/ subdirectory)
   - **Solution**: Executable is directly in build directory, not in subdirectory
   - **Lesson**: CMake places executables in CMAKE_CURRENT_BINARY_DIR by default

### Key Design Decisions

1. **Operator Function Signature**
   - Chose `std::function<nlohmann::json(const nlohmann::json& args, ExecutionContext& ctx)>`
   - Arguments passed as array to operator functions (not individual parameters)
   - This provides flexibility and consistency

2. **Type Promotion in Addition**
   - If both operands are integers, result is integer
   - If either operand is float, result is float
   - This matches typical programming language behavior

3. **Error Handling Philosophy**
   - Fail fast with descriptive messages
   - Separate exception types for different error categories
   - Validate arguments before processing

4. **Test Organization**
   - One test class (ComputoTest) with setUp method
   - Individual test methods for each specific scenario
   - Exception hierarchy tests using try/catch blocks rather than EXPECT_THROW for inheritance verification

### Performance Observations

- Static operator map initialization works efficiently
- No noticeable performance overhead in recursive evaluation
- JSON operations are fast with nlohmann/json library
- Test suite runs in ~0.01 seconds

### Surprises/Gotchas

1. **Permuto Integration**
   - Permuto is included in computo.cpp but not actually used yet (reserved for Phase 4)
   - The #include doesn't cause issues even though we're not calling permuto functions

2. **JSON Array Handling**
   - nlohmann/json makes array manipulation very clean
   - The `expr[0].is_string()` check elegantly distinguishes operators from literal arrays

3. **Exception Inheritance Testing**
   - Using try/catch blocks instead of EXPECT_THROW for hierarchy testing provides better verification
   - SUCCEED() and FAIL() macros work well for this pattern

### Next Steps (Phase 2)

The foundation is solid for implementing:
- ExecutionContext variable management
- `let` operator for variable binding
- `$` operator for variable access  
- `get` operator for JSON Pointer access

### Build Commands Reference

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Test
cd build && ctest --verbose

# CLI Usage
./build/computo script.json input.json
```

### Dependencies Status

- **nlohmann/json**: Working perfectly via find_package
- **Permuto**: Successfully linked via find_library, ready for Phase 4
- **Google Test**: Working well for comprehensive test suite