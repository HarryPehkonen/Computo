## 1. Summary

This proposal addresses a redundancy and inconsistency in the Computo language's data access operators. Currently, data is accessed via three distinct operators: `get`, `$`, and `$input`.

This change proposes to **deprecate the `get` operator entirely** and enhance the `$` and `$input` operators to support optional JSON Pointer paths, creating a single, consistent pattern for all data access.

## 2. Problem Statement

The current design has three operators for what is fundamentally one operation: retrieving data from a context.

*   `["get", <object_expr>, <path_string>]`: General-purpose, but verbose.
*   `["$", "/varname"]`: Special-purpose for `let` variables, but cannot access data *within* a variable.
*   `["$input"]`: Special-purpose for the root input, but cannot access data *within* the input.

This leads to several issues:
*   **Increased Cognitive Load:** Users must learn and remember three separate operators for a single conceptual task.
*   **Inconsistent Syntax:** The pattern for accessing data is not uniform.
*   **Verbose Scripts:** Common operations require nesting multiple operators, for example: `["get", ["$", "/my_var"], "/some/path"]`.

## 3. Proposed Solution

We will consolidate all data access into two "data source" operators: `$input` for the root input context, and `$` for the `let` variable context.

### 3.1. Operator Deprecation

*   The `get` operator will be **removed** from the Computo standard library.

### 3.2. Operator Enhancement

The `$input` and `$` operators will be enhanced to accept an optional second argument: a JSON Pointer string.

#### 3.2.1. The `$input` Operator

*   **`["$input"]`**
    *   **Arguments:** 0
    *   **Behavior:** Returns the entire root input JSON object, unchanged from the current implementation.

*   **`["$input", <path_expr>]`**
    *   **Arguments:** 1
    *   **Behavior:** The `<path_expr>` must evaluate to a string representing a valid JSON Pointer (e.g., `"/user/name"`). The operator applies this pointer to the root input object and returns the resulting value. Throws `InvalidArgumentException` if the path is invalid or not found.
    *   **Replaces:** `["get", ["$input"], <path_expr>]`

#### 3.2.2. The `$` (Variable) Operator

*   **`["$"]`**
    *   **Arguments:** 0
    *   **Behavior:** Returns a JSON object representing the entire current variable scope. (Primarily for introspection or debugging).

*   **`["$", <path_expr>]`**
    *   **Arguments:** 1
    *   **Behavior:** The `<path_expr>` must evaluate to a string representing a JSON Pointer where the first segment is the variable name (e.g., `"/my_var/address/city"`).
    *   **Replaces:** `["get", ["$", "/my_var"], "/address/city"]` and the old `["$", "/my_var"]`.
    *   **Implementation Note:** The implementation will parse the pointer. The first segment identifies the variable to retrieve. The rest of the pointer is then applied to the retrieved variable's value.

## 4. Path Parsing for the `$` Operator

A decision is required for the precise syntax of the path for the `$` operator.

### Option A: Split Arguments

*   **Syntax:** `["$", <var_name_string>, <json_pointer_string>]`
*   **Example:** `["$", "my_var", "/address/city"]`
*   **Pros:**
    *   Very explicit and simple to parse in the C++ implementation.
    *   No ambiguity.
*   **Cons:**
    *   More verbose for the user.
    *   Syntactically inconsistent with the single-argument path for `$input`.

### Option B: Unified JSON Pointer (Recommended)

*   **Syntax:** `["$", <unified_json_pointer_string>]`
*   **Example:** `["$", "/my_var/address/city"]`
*   **Pros:**
    *   Maximally consistent with the `$input` operator. The user only needs to learn one pattern: `[<source>, <path>]`.
    *   More powerful and concise for the user. `["$", "/my_var"]` and `["$", "/my_var/path"]` work with the same logic.
*   **Cons:**
    *   Requires slightly more complex parsing logic in the C++ implementation to split the variable name from the rest of the path.

**Recommendation:** **Option B is strongly recommended.** The improved user experience and API consistency far outweigh the minor, one-time implementation cost of the parser.

## 5. Impact on Existing Tests and Scripts

This is a **breaking change**. All existing tests and scripts must be updated.

*   All instances of `["get", ...]` must be refactored.
*   All instances of `["$", "/varname"]` remain valid under Option B.
*   All literal arrays in scripts must now use the `{"array": [...]}` syntax to avoid being misinterpreted as operator calls, as `get` was often used to retrieve literal arrays.

### Example Refactoring

**Old Script:**
```json
["let", [["user_list", {"array": [{"name": "Alice"}]}]],
  ["get", ["get", ["$", "/user_list"], "/0"], "/name"]
]
```

**New Script (with Option B):**
```json
["let", [["user_list", {"array": [{"name": "Alice"}]}]],
  ["$", "/user_list/0/name"]
]
```

## 6. Conclusion

Adopting this proposal will significantly simplify the Computo language, reduce its API surface, and improve its internal consistency. It represents a major step forward in the usability and elegance of the language design.
```
