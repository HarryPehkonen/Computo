# Linting Process Guide

This document outlines our systematic approach to implementing clang-tidy in C++ projects, based on lessons learned from the Asteroids project.

## Overview

Our approach focuses on achieving **100% clean builds** for production code while maintaining strict quality standards for new development. We use surgical NOLINT comments to silence acceptable existing issues without compromising future code quality.

## Initial Setup

### 1. Configure .clang-tidy

Create a `.clang-tidy` file in your project root:

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
...
```

**Key insight**: The `IgnoreMacros: true` setting eliminates false positive complexity warnings from test frameworks like Google Test, while preserving meaningful complexity analysis of your actual code.

### 2. CMake Integration

Add these settings to your root CMakeLists.txt to enable clang-tidy:

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

**Important notes:**
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` generates `compile_commands.json` which clang-tidy requires
- clang-tidy automatically reads configuration from `.clang-tidy` in the project root
- This runs clang-tidy on every source file during compilation

## Systematic Fix Process

### Phase 1: Identify All Issues

1. **Clean build** to get baseline clang-tidy output:
   ```bash
   cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -S .
   cmake --build build-debug
   ```
   
   **Command explanation:**
   - `-B build-debug`: Create build directory named "build-debug"
   - `-DCMAKE_BUILD_TYPE=Debug`: Use Debug build (better warning visibility)
   - `-S .`: Explicitly specify source directory as current directory (recommended practice)
   - `--build build-debug`: Build the project and run clang-tidy automatically

2. **Categorize warnings** by type and location:
   - Production code warnings (fix these)
   - Test framework macro expansions (configure to ignore) 
   - Legitimate test code issues (surgical NOLINT)
   - False positives (surgical NOLINT)

### Phase 1.5: Use Automatic Fixes (Optional)

clang-tidy can automatically fix many issues. Use this cautiously and review all changes:

```bash
# Run clang-tidy with automatic fixes (BACKUP YOUR CODE FIRST!)
clang-tidy --fix --fix-errors src/*.cpp -- -I./include

# Or for specific files:
clang-tidy --fix include/Constants.hpp -- -I./include

# Dry run to see what would be fixed:
clang-tidy --fix-errors --dry-run src/GameState.cpp -- -I./include
```

**Automatically fixable issues include:**
- Uppercase literal suffixes (`2.0f` → `2.0F`)
- Missing braces around statements  
- Some const-correctness issues
- Include sorting and formatting
- Trailing return type conversions

**⚠️ Important:**
- Always backup code before using `--fix`
- Review all automatic changes carefully
- Some fixes may change semantics (rare but possible)
- Manual review is still required for complex issues

### Phase 2: Fix Production Code

Address all warnings in main source code (`src/`, `include/`) systematically:

#### Common Fixes Applied:
- **Magic numbers**: Add named constants to Constants.hpp
- **Uppercase literal suffixes**: Change `f` to `F` (e.g., `2.0f` → `2.0F`)
- **Braces around statements**: Add braces to single-line if/for statements
- **Math operator precedence**: Add explicit parentheses `(a * b) + (c * d)`
- **Parameter naming**: Use descriptive names (not `a`, `b`)
- **Const correctness**: Add `const` where appropriate
- **Trailing return types**: Modern C++ function syntax
- **Static methods**: Convert methods that don't use instance data

#### Example Transformations:
```cpp
// Before
float angle = (static_cast<float>(i) * 2.0f * M_PI) / count;
if (!asteroid)
    continue;

// After  
inline constexpr float TWO_PI = 2.0F * static_cast<float>(M_PI);
float angle = (static_cast<float>(i) * TWO_PI) / count;
if (!asteroid) {
    continue;
}
```

### Phase 3: Handle Test Code Surgically

For test files, use **surgical NOLINT comments** to silence acceptable issues while preserving quality checks for new code.

#### NOLINT Strategies:

1. **Line-specific** (most common):
   ```cpp
   ship->setPosition(sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2)); // NOLINT(bugprone-integer-division,bugprone-narrowing-conversions)
   ```

2. **Next-line specific**:
   ```cpp
   // NOLINTNEXTLINE(readability-magic-numbers)
   largeAsteroid->setPosition(sf::Vector2f(-10, window.getSize().y / 2));
   ```

3. **Function-specific** (for complex test functions):
   ```cpp
   TEST_F(MyTest, ComplexValidation) { // NOLINT(readability-function-cognitive-complexity)
   ```

#### Common Test Code NOLINT Categories:
- `readability-magic-numbers`: Test constants like -10, +10
- `bugprone-integer-division`: window.getSize().x / 2 for positioning
- `bugprone-narrowing-conversions`: unsigned int to float in graphics APIs
- `readability-uppercase-literal-suffix`: Existing test literals
- `readability-braces-around-statements`: Single-line test conditions
- `readability-qualified-auto`: Variables that need to be modified

## False Positives Encountered

### 1. Google Test Macro Complexity
**Issue**: Test functions flagged for high cognitive complexity due to EXPECT_* macro expansions
**Solution**: Configure `IgnoreMacros: true` in .clang-tidy
**Lesson**: Framework macro expansions should not count toward code complexity metrics

### 2. Const Auto& Suggestions for Modified Variables  
**Issue**: clang-tidy suggested `const auto&` for variables that call non-const methods
```cpp
auto& firstAsteroid = asteroids[0]; // NOLINT(readability-qualified-auto) - needs to be non-const for setPosition
firstAsteroid->setPosition(ship->getPosition());
```
**Solution**: NOLINT with explanatory comment
**Lesson**: Static analysis can't always understand full variable usage patterns

### 3. Test-Specific Code Patterns
**Issue**: Graphics/game test code often uses "magic numbers" that are actually meaningful test values
**Solution**: NOLINT for test positioning constants rather than creating named constants for every test value
**Lesson**: Different quality standards may apply to test vs production code

## Verification Process

### 1. Confirm Clean Build
```bash
cmake --build build-debug  # Should show 0 warnings
```

### 2. Test New Code Detection
Temporarily add problematic code to verify clang-tidy still catches new issues:
```cpp
float badFloat = 2.0f; // Should trigger warnings
if (condition)         // Should trigger braces warning  
    doSomething();
```

### 3. Document Decisions
Record any systematic decisions about when to use NOLINT vs fix in project documentation.

## Project-Specific Adaptations

### For Graphics/Game Projects:
- Integer division for screen positioning is common in tests (NOLINT acceptable)
- Magic numbers for test positioning (-10, +10) are often clearer than named constants
- SFML/graphics APIs often require narrowing conversions (unsigned to float)

### For Other Project Types:
- Adjust check severity based on domain requirements
- Consider stricter standards for library code vs application code
- Evaluate magic number policies based on context

## Maintenance

### Adding New Code
New code will automatically be checked. Any warnings must be addressed before merging.

### Updating Standards
When updating .clang-tidy configuration:
1. Test on a feature branch first
2. Address any new warnings systematically
3. Update NOLINT comments if check names change

### Code Review Guidelines
- NOLINT comments require justification in PR descriptions
- Prefer fixing issues over adding NOLINT when reasonable
- Group related NOLINT suppressions with explanatory comments

## Benefits Achieved

✅ **100% clean builds** - No warning noise during development
✅ **Strict quality enforcement** - New code must meet standards  
✅ **Selective enforcement** - Pragmatic handling of existing/test code
✅ **Framework compatibility** - Works with Google Test, SFML, etc.
✅ **Maintainable process** - Clear documentation for team adoption

## Tools Used
- **clang-tidy**: Static analysis and linting
- **CMake integration**: Automatic execution during builds  
- **Debug builds**: Better warning visibility than Release builds

## Nej Project Integration

The Nej project successfully implements this linting process with the following configuration:

### Actual .clang-tidy Configuration
```yaml
# Example .clang-tidy configuration
# This will be refined based on project needs.
Checks: 'clang-diagnostic-*,clang-analyzer-*,modernize-*,readability-*,performance-*,bugprone-*,cppcoreguidelines-*'
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
```

### Actual .clang-format Configuration  
```yaml
# Example .clang-format configuration
# This will be refined based on project needs.
BasedOnStyle: Google
IndentWidth: 4
UseTab: Never
BreakBeforeBraces: Attach
AllowShortIfStatementsOnASingleLine: false
ColumnLimit: 100
```

### CMake Integration in Nej
```cmake
# Enable compile commands database (required for clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure clang-tidy and clang-format for the project
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    message(STATUS "Generating compile_commands.json for clang-tidy")
endif()
```

### UTF8-CPP Library Integration
When using external libraries like UTF8-CPP, configure to avoid warnings from dependencies:
- Use `HeaderFilterRegex: '.*'` to include project headers only
- External dependency headers are automatically excluded from analysis
- FetchContent dependencies are placed in `_deps/` and ignored by default

---

*This process was developed and validated on the Asteroids C++/SFML project, achieving zero warnings across ~2000 lines of game code and comprehensive test suite. It has been successfully applied to the Nej project with UTF8-CPP integration and modern C++17 features.*
