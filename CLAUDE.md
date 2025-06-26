# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Computo is a safe, sandboxed, JSON-native data transformation engine that provides a Lisp-like language syntax expressed in JSON. It works on top of the **Permuto** library, creating a two-layer system for sophisticated JSON-to-JSON transformations:

- **Computo**: Handles complex programmatic logic (conditionals, loops, calculations)
- **Permuto**: Handles simple declarative substitutions and templating

## Architecture

### Core Design
- **Code is Data**: All Computo scripts are valid JSON arrays where `[operator, arg1, arg2, ...]`
- **Immutability**: Operations are pure functions that don't modify input data
- **Sandboxed Execution**: No I/O operations or system access allowed
- **Recursive Interpreter**: Central `evaluate` function traverses JSON script recursively

### Key Components
- **ExecutionContext**: Maintains state during evaluation (input reference, variable bindings)
- **Operator Dispatch**: `std::map<std::string, OperatorFunc>` for O(1) operator lookup
- **Exception Hierarchy**: Structured error handling with `ComputoException` base class

## Implementation Phases (from TECHNICAL_DETAILS.md)

1. **Phase 1**: Project skeleton, basic evaluate function, simple `+` operator
2. **Phase 2**: ExecutionContext, `let`, `$`, and `get` operators  
3. **Phase 3**: Logic (`if`) and data construction (`obj`, `arr`)
4. **Phase 4**: Permuto integration (`permuto.apply` operator)
5. **Phase 5**: Iteration operators (`map` with lambda support)
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
- `["arr", <item1>, <item2>, ...]`: Array creation

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

## Progress Tracking

Document all progress, decisions, and surprises in `PROGRESS.md` in append-only mode to help future iterations understand what worked and what didn't.