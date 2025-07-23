# Architectural Principles & Anti-Patterns

This document details the key architectural patterns that must be followed for a successful implementation of Computo, as well as the anti-patterns that must be strictly avoided.

## 1. Core Architectural Patterns

### Recursive Interpreter with Tail Call Optimization (TCO)

-   **Pattern**: The core evaluation engine is a recursive interpreter. To prevent stack overflow on deeply nested expressions (a key requirement for functional programming), it must be implemented with a trampoline pattern for tail call optimization.
-   **Implementation**: The main `evaluate` function enters a `while(true)` loop. Control flow operators like `if` and `let` do not make a direct recursive call; instead, they return a `TailCall` struct containing the next expression and context to be evaluated. The loop then "bounces" to the next evaluation, keeping the stack flat.
-   **Why it Works**: This is essential for supporting functional programming paradigms and ensures the engine is robust against complex, deeply nested scripts.

### Thread-Safe Operator Registry

-   **Pattern**: A central, static registry holds mappings from operator names (e.g., `"+"`) to their corresponding C++ function implementations.
-   **Implementation**: The registry is a `std::map`. To ensure thread-safe initialization in a multi-threaded environment, it is populated within a `std::call_once` block. This guarantees that the registry is initialized exactly once, on the first use, without race conditions.
-   **Why it Works**: Provides O(1) lookup for operators, is easy to extend, and is guaranteed to be thread-safe.

### Immutable Execution Context

-   **Pattern**: All state required for an expression's evaluation is contained within an `ExecutionContext` object. This object is treated as immutable.
-   **Implementation**: When a new scope is created (e.g., in a `let` expression), a *new* context is created by copying the parent context and adding the new variables. To make this efficient, input data is stored in `std::shared_ptr`s, avoiding deep copies of large JSON objects.
-   **Why it Works**: This is fundamental to thread safety. Since contexts are not shared or mutated across threads, there are no race conditions. It also makes variable scoping clean and predictable.

### Clean Library/CLI Separation

-   **Pattern**: The core transformation logic is completely contained within `libcomputo`. All user-facing features like the REPL, debugging, command history, and interactive control are in `libcomputorepl` and the `computo_repl` executable.
-   **Implementation**: The build system uses conditional compilation (`-DREPL`). The core library is compiled without this flag. The REPL library is compiled *with* it, which enables hooks and debugging code paths that are completely absent from the production version.
-   **Why it Works**: Guarantees zero performance overhead from debugging features in the production library, keeping it minimal and fast. It provides a clear separation of concerns between the engine and the tools.

## 2. Key Anti-Patterns to Avoid

### ❌ **Global Mutable State**
-   **Description**: Using global or `thread_local` variables for managing state (like debug flags or execution traces) within the core library.
-   **Why it Fails**: It breaks thread safety, introduces hidden dependencies, and makes the code difficult to reason about and test.
-   **Correct Approach**: Pass all state through the `ExecutionContext`. For debugging, use a wrapper pattern or dependency-injected hooks that are not part of the core library's function signatures.

### ❌ **Mixing Binary and N-ary Operators**
-   **Description**: Implementing some operators (like `+`) as n-ary but others (like `-`) as strictly binary, forcing users to nest calls: `["-", ["-", 10, 5], 2]`.
-   **Why it Fails**: It creates an inconsistent and unpredictable user experience. Users cannot easily guess how an operator will behave.
-   **Correct Approach**: All operators that can logically accept multiple arguments should be implemented as n-ary with consistent behavior (e.g., `["-", 10, 5, 2]` works as expected).

### ❌ **Debugging Logic in the Core Library**
-   **Description**: Placing `if (debug_enabled)` checks, logging, or timing code directly within the main evaluation path of `libcomputo`.
-   **Why it Fails**: It adds performance overhead (even if the check is false), clutters the core logic, and violates the principle of a clean separation of concerns.
-   **Correct Approach**: Use conditional compilation (`#ifdef REPL`) to add hooks to the evaluation engine. The debugging tools can then attach to these hooks, but the hooks themselves are compiled out of the production library.

### ❌ **Over-Engineering with Unnecessary Abstractions**
-   **Description**: Creating complex builder patterns, memory pools, or elaborate type systems to solve problems that do not exist in the core requirements.
-   **Why it Fails**: It adds massive complexity for little to no real-world benefit. For example, `nlohmann::json` is already highly optimized for copying, so a custom memory pool is unnecessary.
-   **Correct Approach**: Keep the design simple and direct. Use standard library features and the `nlohmann::json` library to their full potential before inventing custom solutions.
