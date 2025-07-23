# Data Access Operators

This document details the data access and variable binding operators in Computo.

## `$input` (Access Input Data)

Accesses the primary input data provided to the `execute` function.

-   **Syntax**: `["$input"]`
-   **Arguments**: Takes no arguments.
-   **Returns**: The entire input JSON data.

**Examples:**
```json
// If input is: {"user": "Alice", "id": 123}
["$input"]
// Result: {"user": "Alice", "id": 123}

// If input is: 42
["$input"]
// Result: 42
```

## `$inputs` (Access All Inputs)

Accesses all input data provided to the `execute` function as a JSON array.

-   **Syntax**: `["$inputs"]`
-   **Arguments**: Takes no arguments.
-   **Returns**: A JSON array containing all input data provided.

**Examples:**
```json
// If inputs are: {"user": "Alice"}, {"product": "Book"}
["$inputs"]
// Result: [{"user": "Alice"}, {"product": "Book"}]
```

## `$` (Variable Access)

Accesses the value of a variable defined within a `let` expression.

-   **Syntax**: `["$", <variable_name>]`
-   **Arguments**: Exactly one argument, which must be a string representing the variable's name.
-   **Returns**: The value bound to the specified variable name.
-   **Errors**: Throws `InvalidArgumentException` if the variable is not found.

**Examples:**
```json
["let", [["x", 42]], ["$", "x"]]
// Result: 42

["let", [["name", "Bob"]], ["$", "name"]]
// Result: "Bob"
```

## `let` (Variable Binding)

Binds values to variable names within a specific scope and then evaluates a body expression.

-   **Syntax**: `["let", <bindings_array>, <body_expression>]`
-   **Arguments**:
    1.  `<bindings_array>`: An array of arrays, where each inner array is a `[<variable_name_string>, <value_expression>]` pair. The `value_expression` is evaluated to determine the variable's value.
    2.  `<body_expression>`: The expression to be evaluated with the new variables in scope. This is a tail-call position.
-   **Returns**: The result of evaluating the `body_expression`.
-   **Scope**: Variables defined with `let` are only accessible within the `body_expression` and any nested `let` expressions. Inner `let` expressions can shadow variables from outer scopes.

**Examples:**
```json
["let", [["x", 10]], ["+", ["$", "x"], 5]]
// Result: 15

["let", 
  [["a", 1], 
   ["b", ["+", ["$", "a"], 1]] // 'a' is in scope for 'b'
  ],
  ["+", ["$", "a"], ["$", "b"]]
]
// Result: 3 (1 + 2)

["let", 
  [["x", 10]], 
  ["let", 
    [["x", 20]], // 'x' from outer scope is shadowed
    ["$", "x"]
  ]
]
// Result: 20
```
