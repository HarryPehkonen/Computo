# Core Project Requirements

This document specifies the core principles, features, and success criteria for the Computo engine.

## 1. Core Principles

-   **Clean Separation of Concerns**: The core library (`libcomputo`) must remain a pure, thread-safe transformation engine. All debugging, REPL, and interactive features must be isolated in a separate development library (`libcomputorepl`) and tool (`computo_repl`).
-   **Functional Consistency**: All operators, where applicable, should be n-ary (accept a variable number of arguments) to provide a consistent and predictable user experience. Mixed binary/n-ary behavior for the same conceptual operation is to be avoided.
-   **JSON-Native Design**: All scripts must be valid JSON. The language syntax is built directly on JSON's data structures (arrays for operations, objects for data), eliminating the need for a custom parser.
-   **Thread Safety by Design**: The library must be safe for concurrent execution without requiring external locks. This is achieved through pure functions, immutable input data, and safe, one-time initialization of shared resources.

## 2. Library Requirements (libcomputo)

### API

-   **Single Header**: The entire public API must be available through a single header file, `computo.hpp`.
-   **Core Functions**: The API must provide two primary execution functions:
    -   `nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);`
    -   `nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);`
-   **Exception Hierarchy**: A clear exception hierarchy must be provided for error handling:
    -   `ComputoException` (base class)
    -   `InvalidOperatorException`
    -   `InvalidArgumentException`

### Required Operators (30 Total)

#### Arithmetic (5)
`+`, `-`, `*`, `/`, `%`

#### Comparison (6)
`>`, `<`, `>=`, `<=`, `==`, `!=`

#### Logical (3)
`&&`, `||`, `not`

#### Data Access (4)
`$input`, `$inputs`, `$`, `let`

#### Data Construction (1)
`obj` (with variable key support)

#### Control Flow (1)
`if`

#### Array Operations (7)
`map`, `filter`, `reduce`, `count`, `find`, `some`, `every`

#### Functional Programming (4)
`car`, `cdr`, `cons`, `append`

#### Utilities (3)
`strConcat`, `merge`, `approx`

## 3. CLI Requirements (computo & computo_repl)

### Production CLI (`computo`)

-   **Usage**: `computo [options] script.json [context1.json ...]`
-   **Functionality**: A minimal, fast, non-interactive tool for executing scripts. It should support pretty-printing output and parsing JSON with comments.

### Development REPL (`computo_repl`)

-   **Functionality**: A feature-rich, interactive REPL for development, debugging, and exploration. It must support:
    -   Loading context files.
    -   Command history and line editing (via readline).
    -   Inspecting variables (`vars`).
    -   Loading and running script files (`run`).
    -   Setting and managing breakpoints (`break`, `nobreak`, `breaks`).
    -   Interactive debugging (`step`, `continue`, `finish`, `where`).

## 4. Success Criteria

The project is considered successful when:

1.  All 30 specified operators are implemented and behave correctly according to their n-ary semantics.
2.  The library is fully thread-safe and passes all concurrent execution tests.
3.  The build system produces all specified targets correctly from a single, unified build command.
4.  The CLI tools provide the full required feature set for both production execution and interactive development.
5.  Tail Call Optimization (TCO) is correctly implemented, preventing stack overflows on deep recursion.
6.  The project has zero warnings from `clang-tidy` and is formatted according to `.clang-format`.
7.  The test suite is comprehensive, with high code coverage and tests for all major features and error conditions.
