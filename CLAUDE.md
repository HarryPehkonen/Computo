# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Computo is a safe, sandboxed, JSON-native data transformation engine that provides a Lisp-like language syntax expressed in JSON. It works on top of the **Permuto** library, creating a two-layer system for sophisticated JSON-to-JSON transformations:

- **Computo**: Handles complex programmatic logic (conditionals, loops, calculations)
- **Permuto**: Handles simple declarative substitutions and templating

## Architecture

### Core Design
- **Code is Data**: All Computo scripts are valid JSON where:
  - **Operator calls**: `[operator, arg1, arg2, ...]`
  - **Literal arrays**: `{"array": [item1, item2, ...]}`
  - **Objects**: Standard JSON objects
- **Immutability**: Operations are pure functions that don't modify input data
- **Sandboxed Execution**: No I/O operations or system access allowed
- **Recursive Interpreter**: Central `evaluate` function traverses JSON script recursively
- **Unambiguous Syntax**: Zero ambiguity between operator calls and literal data

### Key Components
- **ExecutionContext**: Maintains state during evaluation (input reference, variable bindings)
- **Operator Dispatch**: `std::map<std::string, OperatorFunc>` for O(1) operator lookup
- **Exception Hierarchy**: Structured error handling with `ComputoException` base class

## Implementation Phases (from TECHNICAL_DETAILS.md)

1. **Phase 1**: Project skeleton, basic evaluate function, simple `+` operator
2. **Phase 2**: ExecutionContext, `let`, `$`, and `get` operators  
3. **Phase 3**: Logic (`if`) and data construction (`obj`, array objects)
4. **Phase 4**: Permuto integration (`permuto.apply` operator)
5. **Phase 5**: Iteration operators (`map`, `filter` with lambda support) + Array Syntax Revolution
6. **Phase 6**: Remaining operators, CLI tool, integration testing

## Required Operators

### Data Access & Scoping
- `["get", <object_expr>, <json_pointer_string>]`: JSON Pointer access
- `["let", [["var1", <expr1>], ["var2", <expr2>]], <body_expr>]`: Variable binding
- `["$", "/var_name"]`: Variable access

### Logic & Control Flow  
- `["if", <condition>, <then>, <else>]`: Conditional execution

### Data Construction
- `["obj", ["key1", <val1>], ["key2", <val2>], ...]`: Object creation
- `{"array": [<item1>, <item2>, ...]}`: Array creation (object syntax)

### Data Manipulation
- `["map", <array>, ["lambda", ["item_var"], <transform>]]`: Array transformation
- `["filter", <array>, ["lambda", ["item_var"], <condition>]]`: Array filtering
- `["merge", <obj1>, <obj2>, ...]`: Object merging
- `["count", <array>]`: Array length

### Mathematical
- `["+", <num1>, <num2>]`, `["-", <num1>, <num2>]`, `["*", <num1>, <num2>]`, `["/", <num1>, <num2>]`

### Permuto Integration
- `["permuto.apply", <template>, <context>]`: Bridge to Permuto templating

## Development Commands

Since this is a C++ project with CMake (per TECHNICAL_DETAILS.md):

```bash
# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build library and CLI
cmake --build build

# Run tests (Google Test framework)
cd build && ctest

# Run CLI tool (when implemented)
./build/cli/computo script.json input.json
```

## Adding New Operators

When implementing a new Computo operator, follow this checklist to ensure complete integration:

### 1. Core Implementation
- Add operator logic to `src/computo.cpp` in the operator registry
- Update the operator dispatch map with the new operator name
- Implement the operator function following existing patterns

### 2. Builder Pattern Integration
- Add a dedicated method to `include/computo/builder.hpp` in the ComputoBuilder class
- Follow naming conventions: `snake_case` for multi-word operators
- Place the method in the appropriate section (arithmetic, logic, array operations, etc.)
- Example:
  ```cpp
  static ComputoBuilder new_operator(const nlohmann::json& arg1, const nlohmann::json& arg2) {
      return ComputoBuilder(nlohmann::json::array({"new_operator", arg1, arg2}));
  }
  ```

### 3. Test Coverage
- Add tests to the appropriate test file in `tests/` directory:
  - Basic operators → `test_basic_operators.cpp`
  - Data access → `test_data_access.cpp`
  - Logic/conditionals → `test_logic_construction.cpp`
  - Array operations → `test_array_utilities.cpp` or `test_iteration_lambdas.cpp`
  - JSON operations → `test_json_patch.cpp`
  - Advanced features → `test_advanced_features.cpp`
- Create both positive and negative test cases
- Use the Builder Pattern in all new tests for consistency
- Test edge cases (empty arrays, null values, type mismatches)

### 4. Integration Testing
- Add integration tests that combine the new operator with existing ones
- Test in realistic scenarios, not just isolated unit tests
- Verify error handling and exception types

### 5. Build System
- No changes needed to CMakeLists.txt unless adding new test files
- Verify the build passes: `cmake --build build`
- Run full test suite: `cd build && ctest`

### 6. Documentation Notes
- **Do NOT** update README.md (it's auto-generated and changes will be lost)
- Document any special behavior or edge cases in code comments
- Add examples in test files to serve as documentation

### Builder Pattern Guidelines
- Always provide a descriptive method name that matches the operator
- Use `const nlohmann::json&` for parameters to allow flexibility
- Return `ComputoBuilder` for method chaining
- Group related operators together in the header file
- Consider both single-argument and multi-argument versions where appropriate

## Dependencies

- **nlohmann/json**: Core JSON library
- **Permuto**: Template processing library (separate project)
- **Google Test**: Testing framework

## API Requirements

### Library API
- Primary function: `nlohmann::json computo::execute(const nlohmann::json& script, const nlohmann::json& input)`
- Single include file: `#include <computo/computo.hpp>`
- Exception hierarchy with debugging information (line numbers)

### CLI Tool
- Usage: `computo <script_file.json> <input_file.json>`
- Must accept Permuto options and pass them through

## Documentation Structure

### README.md (AI-Focused)
The `README.md` file in this repository is primarily designed for AI assistants and contains:
- Comprehensive operator reference with examples
- Technical implementation details
- Complete syntax documentation
- Integration patterns and workflows
- Performance characteristics

This documentation is optimized for AI comprehension and code generation assistance.

### Human Learning Resources
There is a separate dedicated book project (in a different directory, not part of this codebase) specifically designed for human learning. That book provides:
- Step-by-step tutorials
- Conceptual explanations
- Learning exercises
- Practical use cases
- User-friendly examples

When working with humans who want to learn Computo, direct them to the dedicated book rather than the technical README.md.

## Progress Tracking

Document all progress, decisions, and surprises in `PROGRESS.md` in append-only mode to help future iterations understand what worked and what didn't.