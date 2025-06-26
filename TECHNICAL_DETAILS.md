# Computo - Technical Details & Implementation Plan

## 1. Introduction

This document outlines the technical architecture and implementation strategy for the Computo library, as defined in `REQUIREMENTS.md`. It is intended to guide the C++ development process.

## 2. Project Structure

The project will be organized using a standard C++/CMake layout:

```
computo/
├── CMakeLists.txt
├── README.md
├── cli/
│   └── main.cpp
├── include/
│   └── computo/
│       └── computo.hpp
├── src/
│   └── computo.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_main.cpp
    └── test_operators.cpp
```

## 3. Core Architecture: The Recursive Interpreter

The heart of Computo is a recursive `evaluate` function that traverses the JSON script.

*   **Signature:** `nlohmann::json evaluate(const nlohmann::json& expr, ExecutionContext& ctx);`
*   **`ExecutionContext`:** A struct passed by reference through the evaluation stack to maintain state.
    ```cpp
    struct ExecutionContext {
        const nlohmann::json& input; // Reference to the top-level input
        std::map<std::string, nlohmann::json> variables; // For 'let' bindings
    };
    ```
*   **Logic:**
    1.  **Base Case:** If `expr` is not an array or is an empty array, it is a literal value (or a variable to be resolved). Return it directly.
    2.  **Recursive Step:** If `expr` is a non-empty array and `expr[0]` is a string, treat it as an operator call.
        a. Look up the operator string (`expr[0]`) in the operator dispatch table.
        b. If found, call the associated C++ function, passing the arguments (`expr` with the first element removed) and the `ExecutionContext`.
        c. If not found, throw `InvalidOperatorException`.

## 4. Operator Dispatch Mechanism

An efficient dispatch mechanism will map operator names to their C++ implementations.

*   **Mechanism:** A `static std::map` will be used.
*   **Type Alias:** A `std::function` wrapper will be used for type safety and flexibility.
    ```cpp
    using OperatorFunc = std::function<nlohmann::json(
        const nlohmann::json& args,
        ExecutionContext& ctx
    )>;

    static std::map<std::string, OperatorFunc> operators;
    ```
*   **Initialization:** This map will be populated in a one-time initialization function. Each C++ lambda will implement the logic for one operator, including argument validation.

## 5. Error Handling Strategy

Structured exceptions will provide clear diagnostics.

*   **Hierarchy:**
    ```
    std::exception
    └── ComputoException
        ├── InvalidScriptException
        ├── InvalidOperatorException
        └── InvalidArgumentException
    ```
*   **Usage:**
    *   The main `evaluate` loop will throw `InvalidOperatorException`.
    *   Individual operator implementations will throw `InvalidArgumentException` if they receive the wrong number or type of arguments.
    *   Parsing or validation steps can throw `InvalidScriptException`.

*   **Debugging Aid:**
Any exception must provide the line and possibly the column number in the script where the exception occurred.  If the feasibility of that requirement is low, some other information or method must be provided to aid debugging.

## 6. Phased Implementation Plan

Development will proceed in logical, testable phases. **A test suite MUST be developed in parallel for each phase.**

*   **Phase 1: The Skeleton:**
    *   Set up the project structure and CMake files.
    *   Implement the `evaluate` function and operator dispatcher.
    *   Implement handling for literal values (numbers, strings, booleans, nulls).
    *   Implement one simple operator: `+`.
    *   Write a unit test for `+`.

*   **Phase 2: State and Data Access:**
    *   Implement the `ExecutionContext` struct.
    *   Implement the `let` operator, which modifies the context's `variables` map for its body's sub-evaluation.
    *   Implement the `$` operator for variable lookup.
    *   Implement the `get` operator for JSON Pointer access into objects.
    *   Write tests for `let` and `get`.

*   **Phase 3: Logic and Construction:**
    *   Implement the `if` operator.
    *   Implement the `obj` and `arr` data constructors.
    *   Write tests for conditional logic and object/array creation.

*   **Phase 4: The Permuto Bridge:**
    *   Add the `#include <permuto/permuto.hpp>` dependency.
    *   Implement the `permuto.apply` operator. Its C++ function will recursively call `evaluate` on its arguments before passing the results to `permuto::apply`.
    *   Write a comprehensive test that uses a simple Permuto template from within a Computo script.

*   **Phase 5: Iteration:**
    *   Implement the `map` operator. This is the most complex operator. The lambda `["lambda", ["var"], <expr>]` will be handled by performing a sub-evaluation of `<expr>` for each item, with `var` added to a *temporary, local* `ExecutionContext`.
    *   Write tests for `map`.

*   **Phase 6: Finalization:**
    *   Implement all remaining operators (`filter`, `merge`, etc.).
    *   Implement the `cli/main.cpp` for the command-line tool.
    *   Perform integration testing using the CLI tool and sample script files.
    *   Review all documentation.
