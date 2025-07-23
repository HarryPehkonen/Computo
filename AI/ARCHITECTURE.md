# Computo Architecture

This document outlines the core architectural principles and patterns for the Computo JSON transformation engine.

## Core Principles

1.  **Clean Library/CLI Separation**: The core library (`libcomputo`) is a pure, thread-safe, and minimal transformation engine. All debugging, REPL, and interactive features are built into a separate library (`libcomputorepl`) and CLI tool (`computo_repl`), ensuring the production components have zero debugging overhead.

2.  **JSON-Native Syntax**: All scripts are valid JSON. Operations are represented as arrays `["operator", arg1, arg2, ...]`, which eliminates parsing ambiguity and allows scripts to be manipulated as data.

3.  **N-ary Operator Consistency**: With few exceptions (like `not` and `!=`), all operators are n-ary, meaning they can accept a variable number of arguments. This provides a consistent and predictable user experience.

4.  **Thread Safety by Design**: The engine is designed to be thread-safe from the ground up. It uses pure functions, immutable data structures for inputs, and copy-on-write semantics for execution contexts. The operator registry is initialized safely using `std::call_once`.

5.  **Tail Call Optimization (TCO)**: To support deep recursion without stack overflow, the evaluation engine uses a trampoline-based approach. Control flow operators like `if` and `let` are implemented to return continuations rather than direct results, which the trampoline then executes.

## File Structure

```
/home/harri/Dropbox/code/Computo7/
├───CMakeLists.txt
├───include/
│   └───computo.hpp         # Single public header for the library API
├───src/
│   ├───computo.cpp         # Core evaluation engine and operator registry
│   ├───cli.cpp             # Main function for the production CLI
│   ├───repl.cpp            # Main function for the REPL/debug CLI
│   └───operators/
│       ├───shared.cpp      # Shared utilities for operators
│       ├───shared.hpp      # Header for shared utilities
│       └───...             # .cpp files for each operator category
└───tests/
    └───test_basic.cpp      # Google Test suite
```

## Build System

The project uses CMake to manage a unified build process that creates all targets from a single configuration. It produces two distinct libraries:

-   `libcomputo.a`: The core, production-ready library with no debugging features.
-   `libcomputorepl.a`: An enhanced version of the library with all the necessary hooks and features for debugging and the REPL, compiled with the `REPL` definition.

These libraries are then linked into their respective command-line tools and test suites.
