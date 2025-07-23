# Implementation Plan

This document provides a high-level, phased implementation plan for completing the Computo engine. It is designed to ensure that foundational components are built and tested before more complex features are added.

## Phase 1: Core Infrastructure (Completed)

This phase focused on setting up the project structure, build system, and core architectural components.

-   [x] **Project Setup**: Git repository, directory structure, and code quality tools (`.clang-format`, `.clang-tidy`).
-   [x] **Build System**: A unified CMake build that creates all production and REPL targets (`libcomputo`, `libcomputorepl`, `computo`, `computo_repl`, and test suites).
-   [x] **Core Components**: `ComputoException` hierarchy, `ExecutionContext` design, and the thread-safe `OperatorRegistry`.
-   [x] **Evaluation Engine**: The initial recursive interpreter with the TCO trampoline pattern.

## Phase 2: Essential Operators & TCO

This phase focuses on implementing the most critical operators and ensuring the TCO mechanism is robust.

-   [ ] **Shared Utilities**: Implement shared helper functions in `src/operators/shared.cpp`, such as `is_truthy` and `evaluate_lambda`.
-   [ ] **Arithmetic Operators**: Implement `+`, `-`, `*`, `/`, `%` with n-ary semantics.
-   [ ] **Comparison & Logical Operators**: Implement all comparison and logical operators with n-ary and chaining semantics where applicable.
-   [ ] **Control Flow & Data Access**: Implement `if`, `let`, `$input`, `$inputs`, and `$`.
-   [ ] **TCO Verification**: Create comprehensive tests that would cause a stack overflow without TCO, using deeply nested `if` and `let` expressions to validate the trampoline.

**Gate**: All essential operators must be fully implemented and tested, and TCO must be verified before proceeding.

## Phase 3: Advanced Operators

With the core of the language in place, this phase completes the operator set.

-   [ ] **Object Operators**: Implement `obj` (with variable key support), `keys`, `values`, and `merge`.
-   [ ] **Array Operators**: Implement the full suite of array operations: `map`, `filter`, `reduce`, `count`, `find`, `some`, and `every`.
-   [ ] **Functional & String Operators**: Implement the Lisp-style functional operators (`car`, `cdr`, `cons`, `append`) and all string manipulation operators.

**Gate**: All 30 operators must be implemented and have complete test coverage.

## Phase 4: CLI and REPL Implementation

This phase focuses on building the user-facing tools.

-   [ ] **Production CLI (`computo`)**: Implement the minimal, non-interactive CLI for executing scripts. Ensure it handles file I/O and options correctly.
-   [ ] **REPL Scaffolding (`computo_repl`)**: Implement the basic REPL loop with readline support for command history and line editing.
-   [ ] **REPL Features**: Implement the full suite of REPL commands (`help`, `vars`, `run`, etc.).
-   [ ] **CLI Integration Tests**: Create tests specifically for the CLI tools to verify argument parsing and file handling.

**Gate**: Both CLI tools must be functional and pass all integration tests.

## Phase 5: Debugging Infrastructure

This phase builds the advanced debugging capabilities on top of the REPL.

-   [ ] **Debug Hooks**: Ensure the pre-evaluation hooks in the core library (enabled by the `REPL` compile definition) are working correctly.
-   [ ] **Breakpoint System**: Implement the `break`, `nobreak`, and `breaks` commands.
-   [ ] **Interactive Debugger**: Implement the interactive session controls: `step`, `continue`, `finish`, and `where`.
-   [ ] **Error Enhancement**: Implement the operator suggestion system using Levenshtein distance to provide helpful "Did you mean...?" messages.

**Gate**: The interactive debugger must be fully functional.

## Phase 6: Finalization and Documentation

-   [ ] **Comprehensive Testing**: Achieve >95% code coverage. Add integration tests that cover complex, real-world scenarios. Perform thread-safety stress tests and memory leak analysis.
-   [ ] **Performance Benchmarking**: Implement and run performance tests to ensure the engine meets the specified performance targets.
-   [ ] **Documentation**: Write the final `README.md`, ensuring all features, operators, and build procedures are clearly documented for end-users.
-   [ ] **Code Polish**: Perform a final review of the entire codebase to ensure consistency, clarity, and adherence to the project's quality standards.
