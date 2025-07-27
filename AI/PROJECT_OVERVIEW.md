# Project Overview

A safe, sandboxed, thread-safe JSON-native data transformation engine with a Lisp-like language syntax expressed in JSON.

## Overview

Computo consists of:
- **libcomputo**: A minimal, thread-safe C++ library for JSON transformations
- **computo**: A CLI tool for running Computo scripts
- **computo_repl**: An interactive REPL with debugging features

## Implementation Plan

This document provides a high-level, phased implementation plan for building the Computo JSON transformation engine. We are adopting a **feature-driven, vertical slice** approach, where each component is implemented, tested, and documented iteratively.

**Documentation References**: 
- `ARCHITECTURE_GUIDE.md` - Core patterns and file structure
- `DEVELOPMENT_WORKFLOW.md` - Comprehensive guide to building and testing
- `README.md` - User-facing overview
- `docs/` directory - Detailed developer documentation for specific components

## Development Phases

### Phase 1: Project Setup and Infrastructure (COMPLETED)

This phase established the foundational elements of the project.

- [x] **Repository Setup**: Initialized Git, created directory structure, and configured `.gitignore`.
- [x] **Code Quality Infrastructure**: Configured `.clang-format` and `.clang-tidy`.
- [x] **Build System (CMake)**: Created a unified CMake build that produces all production and REPL targets (`libcomputo`, `libcomputorepl`, `computo`, `computo_repl`, `test_computo`, `test_computo_repl`).
- [x] **Dependencies**: Added nlohmann/json, Google Test, and optional readline.
- [x] **Initial Compilation Test**: Verified basic compilation and test execution.

### Phase 2: Core Architecture (COMPLETED)

This phase implemented the fundamental components of the Computo engine.

- [x] **Exception Hierarchy**: Implemented `ComputoException`, `InvalidOperatorException`, and `InvalidArgumentException`.
- [x] **ExecutionContext Design**: Implemented the immutable `ExecutionContext` for thread-safe state management.
- [x] **Operator Registry**: Implemented the thread-safe `OperatorRegistry` with `std::call_once` initialization.
- [x] **Core Evaluation Engine with TCO**: Implemented the `evaluate()` function with a trampoline pattern for Tail Call Optimization (TCO).
- [x] **TCO Verification and Deep Recursion Testing**: Created and passed tests to ensure TCO prevents stack overflows for `if` and `let` expressions.
- [x] **Public API Implementation**: Implemented `execute(script, input)` and `execute(script, inputs)`.

### Phase 3: Essential Operators (COMPLETED)

This phase focused on implementing the most critical operators, one vertical slice at a time.

- [x] **Arithmetic Operators**: Implemented `+`, `-`, `*`, `/`, `%` with n-ary semantics, comprehensive tests, and dedicated documentation.
- [x] **Comparison Operators**: Implemented `>` `<`, `>=` `<=` `==` `!=` with n-ary chaining semantics and comprehensive tests.
- [x] **Logical Operators**: Implemented `&&` `||` `not` with proper truthiness evaluation.
- [x] **Basic Data Access**: Implemented `$input`, `$inputs`, `$`, and `let` with full JSON Pointer support.
- [x] **Control Flow**: Implemented `if` with TCO support.
- [x] **Shared Utilities**: Implemented `is_truthy()` and `evaluate_lambda()` with proper TCO handling.

### Phase 4: Advanced Operators (COMPLETED)

This phase completed the remaining operator set with comprehensive implementations and optimizations.

- [x] **Object Operations**: Implemented `obj`, `keys`, `values`, `objFromPairs`, `pick`, `omit`, `merge` with comprehensive tests.
- [x] **Array Operations**: Implemented `map`, `filter`, `reduce`, `count`, `find`, `some`, `every` with lambda support and TCO handling.
- [x] **Functional Programming**: Implemented `car`, `cdr`, `cons`, `append` with proper array handling.
- [x] **String and Utility Operations**: Implemented `join`, `strConcat`, `sort` (with advanced multi-field sorting and DSU optimization), `reverse`, `unique`, `uniqueSorted` (with sliding window algorithm), `zip`, `approx`.
- [x] **Performance Optimizations**: Added `extract_array_data()` helper function, DSU pattern for sort performance, and enhanced error messages.
- [x] **Build System Optimization**: Separated code quality checks from compilation for fast development builds.

### Phase 5: CLI Implementation (COMPLETED)

This phase built unified user-facing command-line tools.

- [x] **Unified CLI (`computo`)**: Implemented single binary with `--script` and `--repl` modes using custom cross-platform argument parser.
- [x] **Script Execution Mode**: Implemented non-interactive CLI for script execution with multiple input file support.
- [x] **Interactive REPL Mode**: Implemented REPL with readline support and JSON input handling.
- [x] **API Simplification**: Unified execute API to single vector-based function, eliminating convenience overloads.
- [x] **Build System Optimization**: Created single binary target, eliminated duplicate compilation, separated quality checks for faster development builds.

### Phase 6: Debugging Infrastructure (COMPLETED)

This phase added comprehensive debugging capabilities to help developers verify, modify, and understand their scripts.

- [x] **DebugContext Class**: Implemented complete breakpoint management, execution tracking, and debug state control with optimized memory usage.
- [x] **API Integration**: Added DebugContext parameter to `computo::execute()` API with proper null-safety checks.
- [x] **REPL Command Parser**: Implemented sophisticated parser that distinguishes between JSON scripts and debug commands at the same prompt.
- [x] **Basic REPL Commands**: Implemented `help`, `vars`, `debug`, `trace`, `history`, `clear`, `quit` with full functionality.
- [x] **Breakpoint System**: Implemented operator breakpoints (`break +`, `break map`) and variable breakpoints (`break /users`) with management commands (`nobreak`, `breaks`).
- [x] **Script Execution**: Implemented `run <file>` command and direct JSON expression execution with debug integration.
- [x] **Debug Mode Commands**: Implemented `step`/`s`, `continue`/`c`, `finish`/`f`, `where`/`w` with proper debug mode transitions.
- [x] **JSON Comment Support**: Implemented `--comments` flag using nlohmann::json's built-in comment parsing.
- [x] **Debug Engine Integration**: Modified evaluation engine to check breakpoints, record execution steps, and trigger debug mode via `DebugBreakException`.
- [x] **Code Quality**: Resolved all clang-tidy warnings including performance optimizations (enum base type) and readability improvements.

### Phase 7: Comprehensive Testing and Polish (NEXT)

This final phase will ensure the project meets all quality and performance standards.

- [ ] **CLI Integration Tests**: Create comprehensive tests for CLI argument parsing, file handling, and error reporting.
- [ ] **Debug Feature Testing**: Implement tests for breakpoint functionality, step-by-step debugging, and REPL command parsing.
- [ ] **Unit Test Completion**: Ensure comprehensive test coverage for all operators and features.
- [ ] **Integration Testing**: Implement real-world integration scenarios.
- [ ] **Thread Safety Testing**: Conduct thorough concurrent execution tests.
- [ ] **Performance Testing**: Verify all performance targets are met.
- [ ] **Memory Testing**: Run memory leak detection and usage analysis.
- [ ] **Documentation**: Finalize all user and developer documentation.
- [ ] **Build and Packaging**: Test builds on multiple platforms and prepare for distribution.
- [ ] **Final Validation**: Verify all success criteria are met and anti-patterns are avoided.

## Quick Start

### Basic Usage

```bash
# Execute a script
computo script.json

# Execute with input data
computo script.json input.json

# Interactive REPL
computo_repl
```

### Simple Example

```json
["+", 1, 2, 3]
```
Result: `6`

## Language Syntax

All Computo scripts are valid JSON. The basic syntax is:

```json
["operator", arg1, arg2, ...]
```

### Literals

- Numbers: `42`, `3.14`
- Strings: `"hello"`
- Booleans: `true`, `false`
- Null: `null`
- Arrays: `{"array": [1, 2, 3]}`
- Objects: `{"key": "value"}`

## Development Guidelines

### Code Quality Standards
- **Every commit** must compile without warnings.
- **Every commit** must pass all existing tests.
- **New code** must include tests and pass `clang-tidy`.
- **Complex functions** must be under 50 lines (`clang-tidy` enforced).
- **NO EMOJIS** in any code, documentation, or error messages.

### Testing Philosophy
- Write tests **before** implementing features when possible.
- Test error conditions as thoroughly as success conditions.
- Use integration scenarios from integration documentation for future integration tests.
- Maintain high code coverage.

### Performance Mindset
- Profile before optimizing.
- Measure performance impact of changes.
- Use benchmarks from performance documentation for future performance tests.
- Maintain thread safety without locks in hot paths.

### Reference the Documentation
When implementing any feature, consult the relevant documentation:
- **Architecture decisions**: `ARCHITECTURE_GUIDE.md`
- **Development standards**: Development workflow documentation
- **Build process**: `DEVELOPMENT_WORKFLOW.md`
- **Specific operator details**: Operator documentation files
