# Development Workflow

This guide provides comprehensive instructions for building, testing, optimizing, and maintaining code quality in the Computo project.

## Prerequisites

- **C++17 Compiler**: A modern C++ compiler (GCC, Clang, MSVC) that supports C++17.
- **CMake**: Version 3.15 or higher.
- **nlohmann/json**: The JSON library for C++. The build system will attempt to find it if installed.
- **Google Test**: The testing framework. The build system will attempt to find it if installed.
- **readline** (Optional): For an enhanced REPL experience with history and line editing. The build will proceed without it, but the REPL will have basic input capabilities.
- **Clang-Tidy** (Optional): For static code analysis. If found, it will be run automatically during compilation.

## Build System Overview

The project uses an optimized build system that separates fast development builds from code quality checks. This allows for rapid iteration during development while maintaining strict quality standards.

### Fast Development Builds (Default)

The unified build process creates all production and debugging targets in a single run. There is no need for special flags like `-DREPL=ON`.

```bash
# Fast development build without any linting/formatting (recommended for daily work)
cmake -B build
cmake --build build -j$(nproc)
```

This completes in 15-30 seconds instead of minutes, making it ideal for development iteration.

### Build Artifacts

The unified build process generates the following targets:

#### Libraries
- `libcomputo.a`: The core, production-ready static library. It is lightweight and contains no debugging or REPL-specific code, making it ideal for linking into production applications.
- `libcomputorepl.a`: The enhanced static library for development. It includes all the necessary hooks and code (compiled with the `-DREPL` flag) to support the interactive debugger and REPL.

#### Executables
- `computo`: The production command-line interface (CLI). It links against `libcomputo.a` and is optimized for fast, non-interactive script execution.
- `computo_repl`: The interactive Read-Eval-Print Loop (REPL) for development and debugging. It links against `libcomputorepl.a` and provides features like breakpoints, variable inspection, and command history.

#### Test Suites
- `test_computo`: The test suite for the production library. It links against `libcomputo.a` and verifies the correctness and performance of the core engine.
- `test_computo_repl`: The test suite for the REPL/debug library. It links against `libcomputorepl.a` and validates the debugging features and interactive functionality.

## Testing

The project uses CTest, CMake's testing tool, to run the test suites.

To run all tests after building the project:

```bash
cd build
ctest --output-on-failure
```

This command will automatically discover and run both `computo_tests` and `computo_repl_tests`.

- `computo_tests`: Validates the core functionality of the production library.
- `computo_repl_tests`: Ensures that the debugging and REPL-specific features work as expected.

For more detailed output:

```bash
ctest --verbose
```

## Code Quality System

### Separated Quality Checks

Code quality is handled through separate targets to avoid slowing down development builds:

```bash
cd build

# Format all source code
make format

# Check formatting without modifying files (CI-friendly)
make format-check

# Run static analysis with clang-tidy
make lint

# Run all code quality checks
make quality
```

### Performance Comparison

| Build Type | Time | Use Case |
|------------|------|----------|
| **Development (optimized)** | ~15-30 seconds | Daily development |
| **With quality checks** | ~30-60 seconds | Pre-commit checks |
| **Old system (with inline linting)** | 2+ minutes (often timeout) | Never practical |

### Optional: Enable Quality Checks During Build

For thorough checking (slower but comprehensive):

```bash
# Enable clang-tidy during compilation (slow but thorough)
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# Enable clang-format during compilation
cmake -B build -DENABLE_CLANG_FORMAT=ON
cmake --build build
```

## Recommended Workflows

### Daily Development Workflow
```bash
# 1. Fast development builds
cmake -B build
cmake --build build -j$(nproc)

# 2. Run tests
./build/test_computo

# 3. Before committing, check code quality
cd build
make format          # Fix formatting
make lint            # Check for issues
```

### CI/CD Workflow
```bash
# 1. Build with quality checks enabled
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# 2. Run separate quality targets
cd build
make format-check    # Fail if formatting is wrong
make lint            # Fail if linting issues exist
```

## Code Quality Standards

### Clang-Tidy Configuration

Our systematic approach achieves **100% clean builds** for production code while maintaining strict quality standards.

#### .clang-tidy Configuration
```yaml
---
Checks: '-*,bugprone-*,performance-*,readability-*,modernize-*'

CheckOptions:
  # Function size limits
  - key: readability-function-size.LineLimit
    value: 50
  # Complexity thresholds  
  - key: readability-cyclomatic-complexity.Threshold
    value: 15
  # IMPORTANT: Ignore complexity from macro expansions (fixes Google Test warnings)
  - key: readability-function-cognitive-complexity.IgnoreMacros
    value: true
```

**Key insight**: The `IgnoreMacros: true` setting eliminates false positive complexity warnings from test frameworks like Google Test, while preserving meaningful complexity analysis of your actual code.

#### CMake Integration
```cmake
# Enable compile commands database (required for clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure clang-tidy integration
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    # The checks will be read from the .clang-tidy file automatically
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
else()
    message(WARNING "clang-tidy not found.")
endif()
```

### Systematic Fix Process

#### Phase 1: Identify All Issues
1. **Clean build** to get baseline clang-tidy output:
   ```bash
   cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -S .
   cmake --build build-debug
   ```

2. **Categorize warnings** by type and location:
   - Production code warnings (fix these)
   - Test framework macro expansions (configure to ignore) 
   - Legitimate test code issues (surgical NOLINT)
   - False positives (surgical NOLINT)

#### Phase 2: Fix Production Code
Address all warnings in main source code (`src/`, `include/`) systematically:

**Common fixes applied:**
- **Magic numbers**: Add named constants
- **Uppercase literal suffixes**: Change `f` to `F` (e.g., `2.0f` → `2.0F`)
- **Braces around statements**: Add braces to single-line if/for statements
- **Math operator precedence**: Add explicit parentheses `(a * b) + (c * d)`
- **Parameter naming**: Use descriptive names (not `a`, `b`)
- **Const correctness**: Add `const` where appropriate

#### Phase 3: Handle Test Code Surgically
For test files, use **surgical NOLINT comments** to silence acceptable issues while preserving quality checks for new code:

```cpp
// Line-specific (most common)
ship->setPosition(sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2)); // NOLINT(bugprone-integer-division,bugprone-narrowing-conversions)

// Next-line specific
// NOLINTNEXTLINE(readability-magic-numbers)
largeAsteroid->setPosition(sf::Vector2f(-10, window.getSize().y / 2));

// Function-specific (for complex test functions)
TEST_F(MyTest, ComplexValidation) { // NOLINT(readability-function-cognitive-complexity)
```

### Automatic Fixes (Optional)
clang-tidy can automatically fix many issues. Use cautiously and review all changes:

```bash
# Run clang-tidy with automatic fixes (BACKUP YOUR CODE FIRST!)
clang-tidy --fix --fix-errors src/*.cpp -- -I./include

# Dry run to see what would be fixed:
clang-tidy --fix-errors --dry-run src/GameState.cpp -- -I./include
```

**Automatically fixable issues include:**
- Uppercase literal suffixes (`2.0f` → `2.0F`)
- Missing braces around statements  
- Some const-correctness issues
- Include sorting and formatting

## Available CMake Options

### Build Type Options
- `CMAKE_BUILD_TYPE=Debug|Release` - Standard CMake build types
- `ENABLE_CLANG_TIDY=ON/OFF` - Enable clang-tidy during compilation
- `ENABLE_CLANG_FORMAT=ON/OFF` - Enable clang-format during compilation

### Custom Targets
- `format` - Apply clang-format to all source files
- `format-check` - Verify formatting (CI-friendly, exits with error)
- `lint` - Run clang-tidy static analysis
- `quality` - Combined target running both format-check and lint

## Troubleshooting

### Missing Tools
If clang-format or clang-tidy are not found:
```bash
# Ubuntu/Debian
sudo apt install clang-format clang-tidy

# macOS
brew install clang-format
```

### Parallel Build Issues
If parallel builds cause issues:
```bash
# Use fewer cores
cmake --build build -j2

# Or single-threaded
cmake --build build
```

### Clean Build for Debugging
```bash
# Remove build directory and start fresh
rm -rf build
cmake -B build
cmake --build build
```

## Benefits of This System

1. **  Fast Development Builds**: No more waiting for linting during compilation
2. **  Targeted Quality Checks**: Run linting/formatting only when needed
3. **  Flexible CI Integration**: Choose appropriate checks for different contexts
4. **  Parallel Builds**: Leverage all CPU cores with `make -j$(nproc)`
5. **  Clear Status Messages**: Know exactly which tools are available
6. **  100% Clean Builds**: No warning noise during development
7. **  Strict Quality Enforcement**: New code must meet standards
8. **  Selective Enforcement**: Pragmatic handling of existing/test code

## Maintenance Guidelines

### Adding New Code
New code will automatically be checked. Any warnings must be addressed before merging.

### Code Review Guidelines
- NOLINT comments require justification in PR descriptions
- Prefer fixing issues over adding NOLINT when reasonable
- Group related NOLINT suppressions with explanatory comments

### Updating Standards
When updating .clang-tidy configuration:
1. Test on a feature branch first
2. Address any new warnings systematically
3. Update NOLINT comments if check names change

This optimized build system successfully scales to support all operators, CLI tooling, and comprehensive quality checks while maintaining rapid development iteration.
