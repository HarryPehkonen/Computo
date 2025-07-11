# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System and Development Commands

### Dual Build Architecture
Computo uses conditional compilation to provide two distinct builds:

**Production Build (Zero Debug Overhead)**:
```bash
cmake -B build-prod -DCMAKE_BUILD_TYPE=Release
cmake --build build-prod
./build-prod/computo script.json input.json
```

**REPL Build (Full Debugging Features)**:
```bash
cmake -B build-repl -DREPL=ON -DCMAKE_BUILD_TYPE=Release  
cmake --build build-repl
./build-repl/computo_repl input.json
```

### Testing
```bash
# Build and run all tests
cmake --build build-prod
./build-prod/test_computo

# Run specific test by filtering
./build-prod/test_computo --gtest_filter="*arithmetic*"

# Debug build with assertions enabled
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
./build-debug/test_computo
```

### Performance Benchmarking
```bash
./build-prod/computo --perf
./build-repl/computo_repl --perf
```

## Architecture Overview

### Core Design Principles

**Library/CLI Separation**: The core `libcomputo` library (`src/computo.cpp` + `src/operators/*.cpp` + `include/computo.hpp`) contains zero debugging infrastructure. All debug features live in CLI tools (`src/cli.cpp` for production, `src/repl.cpp` for development).

**Conditional Compilation**: Uses CMake `REPL` option and `#ifdef REPL` blocks to ensure production builds contain literally zero debug code paths.

**Thread Safety**: Pure functional design with no global mutable state. `ExecutionContext` uses shared_ptr for inputs and copy-on-write for variable scopes.

### Key Components

**Single Include Library**: `include/computo.hpp` provides complete API with two main functions:
- `computo::execute(script, input)` - Single input
- `computo::execute(script, inputs)` - Multiple inputs

**Operator Registry**: Thread-safe lazy initialization using `std::call_once`. Operators organized by category in `src/operators/`:
- `arithmetic.cpp` - `+`, `-`, `*`, `/`, `%` (all n-ary)
- `comparison.cpp` - `>`, `<`, `>=`, `<=`, `==`, `!=` (n-ary with chaining)
- `logical.cpp` - `&&`, `||` (n-ary), `not` (unary only)
- `array.cpp` - `map`, `filter`, `reduce`, `count`, `find`, `some`, `every`
- `functional.cpp` - `car`, `cdr`, `cons`, `append`
- `data.cpp` - `$`, `let`, `obj`, `if` (Note: `$input` and `$inputs` are handled in evaluator)
- `utilities.cpp` - `strConcat`, `merge`, `approx`

**ExecutionContext Design**: Immutable input data (shared_ptr) + mutable variable scope. Supports nested scoping for `let` expressions with path tracking for error reporting.

### Language Syntax Rules

**JSON-Native**: All scripts are valid JSON using array syntax `["operator", arg1, arg2, ...]`

**N-ary Consistency**: All operators accept multiple arguments except `not` (unary only). Comparison operators use chaining semantics: `[">", a, b, c]` means `a > b && b > c`.

**Data Disambiguation**: 
- Operators: `["map", array, lambda]`
- Literal arrays: `{"array": [1, 2, 3]}`
- Objects: Standard JSON objects

**Enhanced Data Access System**: 
- `["$input"]` - Access entire input object
- `["$input", "/path"]` - Navigate within input using JSON Pointer
- `["$inputs"]` - Access all inputs as array
- `["$inputs", "/0/path"]` - Access specific input by index with navigation
- `["$", "/varname"]` - Access variable
- `["$", "/varname/path"]` - Navigate within variable using JSON Pointer
- `["$"]` - Access all variables in current scope
- Variable binding via `["let", [["var", value]], body]`

## Debug Infrastructure (REPL Build Only)

### DebugExecutionWrapper Pattern
The REPL uses a wrapper class that instruments core library calls without modifying the library itself. This enables:
- Variable inspection (`vars` command shows current scope)
- Future extensibility for `trace`, `profile`, `step`, `watch` commands
- Zero impact on production builds

### REPL Commands
- `vars` - Show variables in current scope from `let` expressions
- `help` - Show available commands  
- `debug` - Toggle debug mode
- `trace` - Toggle trace mode
- `history` - Show command history
- `_1`, `_2`, etc. - Reference previous results

## Error Handling

**Exception Hierarchy**:
- `ComputoException` (base)
- `InvalidOperatorException` 
- `InvalidArgumentException`

**Path Context**: Exceptions include execution path information (e.g., "at /let/body/map") for debugging complex nested expressions.

## Critical Implementation Details

**Tail Call Optimization**: The evaluation engine supports TCO to prevent stack overflow in recursive algorithms.

**Thread-Local Debug State**: Debug features use thread-local storage to maintain thread safety.

**Operator Initialization**: Uses `std::call_once` for thread-safe lazy initialization of the operator registry.

**JSON Pointer Integration**: All data access operators use standard JSON Pointer syntax for consistency.

## Primary Use Cases

**AI API Schema Translation**: Transform between different AI vendor API formats using complex conditional logic and object restructuring.

**Functional Programming**: Lisp-style list processing with `car`, `cdr`, `cons`, `append` and lambda expressions.

**Data Pipeline Processing**: Array transformations using `map`, `filter`, `reduce` with variable key object construction.

## Development Guidelines

**Production Impact**: Never add debug code to the core library (`src/computo.cpp` or `src/operators/*.cpp`). All debug features must be REPL-only.

**Operator Consistency**: New operators should follow n-ary patterns and use consistent argument evaluation.

**Thread Safety**: Maintain pure functional design - no global mutable state, use ExecutionContext for all state management.

**Documentation**: Update both language examples in README.md and architectural decisions in ./AI/LESSONS.md for significant changes.
