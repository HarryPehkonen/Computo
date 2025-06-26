# Computo - Requirements Specification

## 1. Introduction

Computo is a safe, sandboxed, JSON-native data transformation engine. It provides a Lisp-like language syntax, expressed in JSON, to perform complex logic, mapping, and calculations.

It is designed to work *on top of* the **Permuto** library. Computo handles complex programmatic logic, while delegating simple, declarative substitutions and whole-structure templating to Permuto. This creates a powerful, two-layer system for sophisticated JSON-to-JSON transformations.

## 2. Core Concepts

*   **Code is Data:** All Computo scripts are valid JSON. An operation is represented as a JSON array where the first element is a string representing the operator, and subsequent elements are arguments. Example: `["+", 1, 2]`.
*   **Immutability:** Operations are pure and do not modify input data. They produce new data as output. Variables, once set via `let`, cannot be changed within their scope.
*   **Sandboxed Execution:** Scripts cannot perform any I/O (file access, network calls) or interact with the system outside of the provided input data.
*   **The Input Context:** The initial JSON data provided to the `execute` function is available within the script via the special `"$input"` operator.

## 3. Functional Requirements

### 3.1. Core Operators

The Computo interpreter MUST implement the following set of operators:

#### 3.1.1. Data Access & Scoping
*   `["get", <object_expr>, <json_pointer_string>]`: Retrieves a value from a JSON object using a JSON Pointer.
*   `["let", [["var1", <expr1>], ["var2", <expr2>]], <body_expr>]`: Binds the results of expressions to variable names, which are available within the `body_expr`. Variables are accessed using `["$", "/var_name"]`.

#### 3.1.2. Logic
*   `["if", <condition_expr>, <then_expr>, <else_expr>]`: If `condition_expr` evaluates to `true`, returns the result of `then_expr`, otherwise returns the result of `else_expr`.

#### 3.1.3. Data Construction
*   `["obj", ["key1", <val_expr1>], ["key2", <val_expr2>], ...]`: Creates a new JSON object.
*   `["arr", <item_expr1>, <item_expr2>, ...]`: Creates a new JSON array.

#### 3.1.4. Data Manipulation & Iteration
*   `["map", <array_expr>, ["lambda", ["item_var"], <transform_expr>]]`: Iterates over `array_expr`, applying the `transform_expr` to each item. Returns a new array of the transformed items. The current item is available via the `item_var`.
*   `["filter", <array_expr>, ["lambda", ["item_var"], <condition_expr>]]`: Iterates over `array_expr`, returning a new array containing only the items for which `condition_expr` evaluates to `true`.
*   `["merge", <obj_expr1>, <obj_expr2>, ...]`: Merges multiple objects into a new object. For conflicting keys, the rightmost object's value wins.
*   `["count", <array_expr>]`: Returns the number of elements in an array.

#### 3.1.5. Mathematical Operators
*   `["+", <num_expr1>, <num_expr2>]`
*   `["-", <num_expr1>, <num_expr2>]`
*   `["*", <num_expr1>, <num_expr2>]`
*   `["/", <num_expr1>, <num_expr2>]`

#### 3.1.6. The Permuto Bridge
*   `["permuto.apply", <template_json_expr>, <context_json_expr>]`: The primary integration point. It invokes `permuto::apply()` with the evaluated template and context, returning the result. This allows for powerful, declarative templating within a logical script.  The permuto options can be passed via computo.execute.

### 3.2. C++ Library API

*   A library named libcomputo MUST be provided.
*   A primary function `computo::execute` MUST be provided.
*   **Signature:** `nlohmann::json computo::execute(const nlohmann::json& script, const nlohmann::json& input);`
*   A single #include file MUST be sufficient (in addition to nlohmann::json) when using the library in other projects.  The inclusion of Permuto may necessitate the use of an additional #include file.

### 3.3. Command-Line Interface (CLI)

*   A CLI tool named `computo` MUST be provided.
*   **Usage:** `computo <script_file.json> <input_file.json>`
*   The tool reads the script and input files, calls `computo::execute`, and prints the resulting JSON to standard output.
*   Permuto options MUST be accepted by the CLI tool and passed to permuto.apply.

## 4. Non-Functional Requirements

*   **Safety:** The interpreter MUST be fully sandboxed. No operator shall permit side effects.
*   **Error Handling:** The library MUST use a structured exception hierarchy inheriting from `std::exception`. Key exceptions include:
    *   `ComputoException` (base class)
    *   `InvalidScriptException`
    *   `InvalidOperatorException`
    *   `InvalidArgumentException`
*   **Debugging:** The library MUST be able to provide the line number where the exception occurred.
*   **Dependencies:** The core library will depend only on `nlohmann/json` and the `Permuto` library. The test suite will additionally depend on Google Test.
*   **Progress Documentation:** Document progress, including decisions and changes, into PROGRESS.md in an append-only mode.  It should note "what works", "what didn't work", etc. so that any future re-writes can anticipate anything that is surprising in this iteration.
