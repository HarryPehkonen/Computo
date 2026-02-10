# Computo

[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://HarryPehkonen.github.io/Computo)
[![Documentation Status](https://github.com/HarryPehkonen/Computo/workflows/Documentation%20Validation%20and%20Deployment/badge.svg)](https://github.com/HarryPehkonen/Computo/actions/workflows/docs.yml)

A safe, sandboxed, thread-safe JSON-native data transformation engine with a Lisp-like language syntax expressed in JSON.

## Documentation

- **[Complete Language Reference](https://HarryPehkonen.github.io/Computo)** - Interactive documentation with all 46 operators
- **[Syntax Verification Process](docs/SYNTAX_VERIFICATION_PROCESS.md)** - How documentation stays accurate
- **[Quick Start Guide](https://HarryPehkonen.github.io/Computo#quick-start)** - Get started immediately

All documentation is automatically validated against the live Computo engine with 66+ tested examples.

## Overview

Computo consists of:
- **libcomputo**: A minimal, thread-safe C++ library for JSON transformations.
- **computo**: A unified CLI tool with script execution mode, interactive REPL, and comprehensive debugging features.

Scripts can be written in either **JSON syntax** (native format) or **sugar syntax** (human-friendly alternative with infix operators, arrow lambdas, and path access). The CLI auto-detects the format and can convert between them.

## Quick Start

### Basic Usage

```bash
# Execute a JSON script
computo --script script.json

# Execute a sugar syntax script (auto-detected)
computo --script script.computo

# Execute with input data
computo --script script.json input.json

# Convert between formats
computo --tocomputo script.json    # JSON -> sugar
computo --tojson script.computo    # sugar -> JSON

# Interactive REPL with debugging
computo --repl

# Load script into REPL for debugging
computo --repl script.json
```

### CLI Options

```bash
# Execution modes
--script <file>       Execute script from file (auto-detects JSON or sugar syntax)
--repl               Start interactive REPL

# Syntax conversion
--tocomputo <file>   Convert JSON script to sugar syntax
--tojson <file>      Convert sugar syntax to JSON

# Data options
--comments           Enable JSON comment parsing
--array=<key>        Use custom array wrapper key (default: "array")

# Output options
--format <file>      Pretty-print script with semantic formatting
--highlight <file>   Syntax-highlighted output
--color / --no-color Force color output on/off

# Debug options
--debug              Enable debugging features (REPL only)

# Information
--list-operators     Output JSON array of all available operators
--help, -h           Show help message
--version, -v        Show version information
```

#### Array Key Customization

The `--array=<key>` option allows you to customize the array wrapper key, enabling output of literal `{"array": [...]}` objects:

```bash
# Default behavior uses "array" key
computo --script script.json
# Output: {"array": [1, 2, 3]}

# Custom key allows literal "array" objects in output
computo --script script.json --array="@data" 
# Can now output: {"data": [1, 2, 3], "array": ["preserved"]}
```

**Example:**
```json
// With --array="$array", this script:
["obj", "data", {"$array": [1, 2, 3]}, "metadata", {"array": ["original"]}]

// Produces:
{
  "data": [1, 2, 3],
  "metadata": {"array": ["original"]}  
}
```

### Simple Example

**Script:**
```json
["+", 1, 2, 3]
```

**Result:**
```
6.0
```

### Multiple Inputs Example

**Script:** (add two numbers from different input files)
```json
["+", ["$inputs", "/0"], ["$inputs", "/1"]]
```

**Input files:**
- `input1.json`: `10`
- `input2.json`: `20`

**Command:**
```bash
computo --script script.json input1.json input2.json
```

**Result:**
```
30.0
```

## Language Syntax

Computo scripts can be written in two formats:

1. **JSON syntax** - The native format where all scripts are valid JSON
2. **Sugar syntax** - A human-friendly alternative that transpiles bidirectionally to/from JSON

### JSON Syntax

The basic syntax for an operation is a JSON array where the first element is a string representing the operator.

`["operator", arg1, arg2, ...]`

#### Literals

- **Numbers**: `42`, `3.14`
- **Strings**: `"hello"`
- **Booleans**: `true`, `false`
- **Null**: `null`
- **Arrays**: To represent a literal array and distinguish it from an operator expression, wrap it in an object with the key `"array"`: `{"array": [1, 2, 3]}`.
- **Objects**: Standard JSON objects: `{"key": "value"}`.

### Sugar Syntax

Sugar syntax provides a more readable way to write Computo scripts. The CLI auto-detects the format, or you can explicitly convert between them.

#### Quick Comparison

| JSON Syntax | Sugar Syntax |
|-------------|--------------|
| `["+", 1, ["*", 2, 3]]` | `1 + 2 * 3` |
| `["$", "/x"]` | `x` |
| `["$input", "/users"]` | `$input/users` |
| `["lambda", ["x"], ["+", ["$", "/x"], 1]]` | `(x) => x + 1` |
| `["let", [["x", 10]], ["$", "/x"]]` | `let x = 10 in x` |
| `["if", true, 1, 0]` | `if true then 1 else 0` |
| `{"array": [1, 2, 3]}` | `[1, 2, 3]` |

#### Sugar Syntax Features

- **Infix operators**: `a + b`, `x * 2`, `count > 0`
- **Arrow lambdas**: `(x) => x * 2`, `(a, b) => a + b`
- **Path access with slash**: `user/name` instead of `["$", "/user/name"]`
- **Input access**: `$input/users/0/name`
- **Let bindings**: `let x = 10, y = 20 in x + y`
- **Conditionals**: `if condition then yes else no`
- **Line comments**: `-- this is a comment`
- **Array literals**: `[1, 2, 3]` (no wrapper needed)

#### Conversion Examples

```bash
# Convert JSON to sugar syntax
computo --tocomputo script.json

# Convert sugar to JSON
computo --tojson script.computo

# Execute sugar syntax directly (auto-detected)
computo --script script.computo data.json
```

#### Full Example

**Sugar syntax:**
```
-- Calculate total spending for active users
let
  active = filter($input/users, (u) => count(u/orders) > 0)
  totals = map(active, (u) =>
    reduce(u/orders, (a, o) => a + o/total, 0)
  )
in
  totals
```

**Equivalent JSON:**
```json
["let", [
  ["active", ["filter", ["$input", "/users"],
    ["lambda", ["u"], [">", ["count", ["$", "/u/orders"]], 0]]
  ]],
  ["totals", ["map", ["$", "/active"],
    ["lambda", ["u"],
      ["reduce", ["$", "/u/orders"],
        ["lambda", ["a", "o"], ["+", ["$", "/a"], ["$", "/o/total"]]],
        0
      ]
    ]
  ]]
], ["$", "/totals"]]
```

#### Slash Disambiguation

The `/` character serves dual purposes:
- **Division**: requires spaces on both sides: `a / b`
- **Path access**: no spaces: `user/name`

Asymmetric spacing (e.g., `a /b` or `a/ b`) is a syntax error to prevent ambiguity.

## Operators Reference

### Arithmetic (n-ary)
- `["+", 1, 2, 3]` → `6.0`
- `["-", 10, 3, 2]` → `5.0`
- `["-", 5]` (unary negation) → `-5.0`
- `["*", 2, 3, 4]` → `24.0`
- `["/", 20, 2, 2]` → `5.0`
- `["/", 4]` (reciprocal) → `0.25`
- `["%", 20, 6]` → `2.0`

### Comparison (n-ary with chaining)
- `[">", 5, 3, 1]` → `true` (evaluates as `5 > 3 && 3 > 1`)
- `["<", 1, 3, 5]` → `true`
- `[">=", 5, 5, 3]` → `true`
- `["<=", 1, 2, 2]` → `true`
- `["==", 2, 2, 2]` → `true` (all arguments are equal)
- `["!=", 1, 2]` (binary only) → `true`

### Logical
- `["and", true, 1, "non-empty"]` → `true` (all arguments are truthy)
- `["or", false, 0, 3]` → `true` (at least one argument is truthy)
- `["not", false]` (unary only) → `true`

### Variables and Data Access
- `["$input"]`: Access the entire input object.
- `["$input", "/path/to/data"]`: Access a specific part of the input using a JSON Pointer path.
- `["$input", "/users/0/profile/email"]`: Navigate deep into nested objects (users[0].profile.email).
- `["$input", "/api/response/data/items/5/metadata/tags/1"]`: Access deeply nested array elements.
- `["$inputs"]`: Access an array of all inputs when multiple are provided.
- `["$inputs", "/0"]`: Access the first input using JSON Pointer syntax.
- `["$inputs", "/0/users/0/name"]`: Navigate deep into the first input (inputs[0].users[0].name).
- `["$", "/varname"]`: Access a variable defined with `let`.
- `["$", "/config/database/connections/primary/host"]`: Navigate deep within variable data.
- `["let", [["x", 10]], ["+", ["$", "/x"], 5]]`: Bind variables using array syntax. Result: `15.0`.
- `["let", {"x": 10, "y": 20}, ["+", ["$", "/x"], ["$", "/y"]]]`: Bind variables using object syntax. Result: `30.0`.

### Object Operations
- `["obj", "name", "Alice", "age", 30]`: Create an object with static keys. Result: `{"name": "Alice", "age": 30}`.
- `["obj", ["strConcat", "user_", ["$", "/id"]], "Alice"]`: Create object with dynamic key evaluation. 
- `["obj", ["$", "/key_name"], ["$", "/value"]]`: Both keys and values are evaluated expressions.
- `["keys", {"a": 1, "b": 2}]`: Get object keys. Result: `{"array": ["a", "b"]}`.
- `["values", {"a": 1, "b": 2}]`: Get object values. Result: `{"array": [1, 2]}`.
- `["objFromPairs", {"array": [["a", 1], ["b", 2]]}]`: Create object from key-value pairs. Result: `{"a": 1, "b": 2}`.
- `["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}]`: Select specific keys. Result: `{"a": 1, "c": 3}`.
- `["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}]`: Remove specific keys. Result: `{"a": 1, "c": 3}`.
- `["merge", {"a": 1}, {"b": 2}]`: Merge objects. Result: `{"a": 1, "b": 2}`.

### Control Flow
- `["if", [">", 5, 3], "yes", "no"]` → `"yes"`

### Lambda Functions
- `["lambda", ["x"], ["+", ["$", "/x"], 1]]`: Create a lambda function with one parameter
- `["lambda", ["a", "b"], ["*", ["$", "/a"], ["$", "/b"]]]`: Create a lambda with multiple parameters
- `["lambda", [], 42]`: Create a lambda with no parameters (returns constant value)

### Array Operations
- `["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]`
  - Result: `{"array": [2, 4, 6]}`
- `["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]`
  - Result: `{"array": [3, 4, 5]}`
- `["reduce", {"array": [1, 2, 3, 4]}, ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]], 0]`
  - Result: `10`
- `["count", {"array": [1, 2, 3]}]` → `3`
- `["find", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 3]]]` → `4`
- `["some", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]` → `true`
- `["every", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 0]]]` → `true`

### Functional Operations
- `["car", {"array": [1, 2, 3]}]` (first element) → `1`
- `["cdr", {"array": [1, 2, 3]}]` (rest of elements) → `{"array": [2, 3]}`
- `["cons", 0, {"array": [1, 2]}]` (prepend) → `{"array": [0, 1, 2]}`
- `["append", {"array": [1, 2]}, {"array": [3, 4]}]` → `{"array": [1, 2, 3, 4]}`

### String Operations
- `["strConcat", "Hello", " ", "World"]` → `"Hello World"`
- `["join", {"array": ["hello", "world"]}, " "]` → `"hello world"`

### Array Manipulation
- `["sort", {"array": [3, 1, 4, 1, 5]}]` → `{"array": [1, 1, 3, 4, 5]}`
- `["sort", {"array": [3, 1, 4]}, "desc"]` → `{"array": [4, 3, 1]}`
- `["sort", {"array": [...]}, "/name"]` (sort objects by field) → sorted array
- `["sort", {"array": [...]}, "/dept", ["/salary", "desc"]]` (multi-field sort)
- `["reverse", {"array": [1, 2, 3]}]` → `{"array": [3, 2, 1]}`
- `["unique", {"array": [1, 2, 2, 3, 3, 3]}]` → `{"array": [1, 2, 3]}`
- `["uniqueSorted", {"array": [1, 1, 2, 2, 3]}]` → `{"array": [1, 2, 3]}` (requires sorted input)
- `["uniqueSorted", {"array": [...]}, "lasts"]` → last occurrence of each unique value
- `["uniqueSorted", {"array": [...]}, "singles"]` → elements that appear exactly once
- `["uniqueSorted", {"array": [...]}, "/field", "firsts"]` → field-based uniqueness
- `["zip", {"array": [1, 2]}, {"array": ["a", "b"]}]` → `{"array": [[1, "a"], [2, "b"]]}`

### Utilities
- `["approx", 0.1, 0.10001, 0.001]` (approximate equality) → `true`

## Advanced Examples

### Working with Nested Data Structures

Given this input data:
```json
{
  "users": [
    {
      "id": 1,
      "profile": {
        "name": "Alice",
        "contact": {
          "email": "alice@example.com",
          "preferences": {
            "notifications": true,
            "theme": "dark"
          }
        }
      },
      "orders": [
        {"id": 101, "total": 25.99, "status": "shipped"},
        {"id": 102, "total": 15.50, "status": "pending"}
      ]
    },
    {
      "id": 2,
      "profile": {
        "name": "Bob",
        "contact": {
          "email": "bob@example.com",
          "preferences": {
            "notifications": false,
            "theme": "light"
          }
        }
      },
      "orders": [
        {"id": 103, "total": 89.99, "status": "delivered"}
      ]
    }
  ]
}
```

**Extract all user emails:**
```json
["map", ["$input", "/users"],
  ["lambda", ["user"], ["$", "/user/profile/contact/email"]]
]
```
Result: `{"array": ["alice@example.com", "bob@example.com"]}`

**Get users with dark theme preference:**
```json
["filter", ["$input", "/users"],
  ["lambda", ["user"], 
    ["==", ["$", "/user/profile/contact/preferences/theme"], "dark"]
  ]
]
```

**Calculate total order value for each user:**
```json
["map", ["$input", "/users"],
  ["lambda", ["user"],
    ["obj",
      "name", ["$", "/user/profile/name"],
      "total_orders", 
        ["reduce", ["$", "/user/orders"],
          ["lambda", ["acc", "order"], 
            ["+", ["$", "/acc"], ["$", "/order/total"]]
          ],
          0
        ]
    ]
  ]
]
```

### Complex Data Transformation Pipeline

**Extract and transform nested e-commerce data:**
```json
["let", [
  ["active_users", ["filter", ["$input", "/users"],
    ["lambda", ["user"], [">", ["count", ["$", "/user/orders"]], 0]]
  ]],
  ["order_summary", ["map", ["$", "/active_users"],
    ["lambda", ["user"],
      ["obj",
        "user_id", ["$", "/user/id"],
        "email", ["$", "/user/profile/contact/email"],
        "pending_orders", ["count", 
          ["filter", ["$", "/user/orders"],
            ["lambda", ["order"], ["==", ["$", "/order/status"], "pending"]]
          ]
        ],
        "total_spent", ["reduce", ["$", "/user/orders"],
          ["lambda", ["acc", "order"], ["+", ["$", "/acc"], ["$", "/order/total"]]],
          0
        ]
      ]
    ]
  ]]
], [
  "filter", ["$", "/order_summary"],
  ["lambda", ["summary"], [">", ["$", "/summary/pending_orders"], 0]]
]]
```

This extracts users with pending orders, calculates their total spending, and returns a summary with deep field access throughout.

### Advanced Sorting and Deduplication

**Sort and deduplicate an array of objects by multiple fields:**
```json
["uniqueSorted", 
  ["sort", {"array": [
    {"dept": "eng", "salary": 95000, "name": "alice"},
    {"dept": "sales", "salary": 75000, "name": "bob"},
    {"dept": "eng", "salary": 95000, "name": "charlie"},
    {"dept": "sales", "salary": 80000, "name": "diana"},
    {"dept": "eng", "salary": 90000, "name": "eve"}
  ]}, "/dept", ["/salary", "desc"], "/name"],
  "/dept"
]
```

This pipeline:
1. Sorts by department (ascending), then salary (descending), then name (ascending)
2. Removes duplicates based on the department field, keeping first occurrence
3. Automatically uses DSU optimization for performance with multiple sort fields

**Extract unique values from nested arrays:**
```json
["uniqueSorted",
  ["sort", 
    ["reduce", ["$input", "/departments"],
      ["lambda", ["acc", "dept"], 
        ["append", ["$", "/acc"], ["$", "/dept/employees"]]
      ],
      {"array": []}
    ]
  ],
  "/employee_id"
]
```

This flattens employee arrays from multiple departments, sorts them, then deduplicates by employee ID.

## First-Class Lambda Functions

Computo supports first-class lambda functions that can be stored in variables, passed as arguments, and used throughout your programs. This enables powerful functional programming patterns.

### Basic Lambda Storage and Usage

**Store a lambda in a variable:**
```json
["let", [["doubler", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]], 
 ["map", {"array": [1, 2, 3]}, ["$", "/doubler"]]]
```
Result: `{"array": [2, 4, 6]}`

**Multiple lambda functions:**
```json
["let", [
  ["add1", ["lambda", ["x"], ["+", ["$", "/x"], 1]]],
  ["mul2", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
],
["map", 
  ["map", {"array": [1, 2, 3]}, ["$", "/add1"]], 
  ["$", "/mul2"]
]]
```
Result: `{"array": [4, 6, 8]}` (first add 1, then multiply by 2)

### Lambda Functions with Reduce

**Store and reuse accumulator functions:**
```json
["let", [["summer", ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]]]],
 ["reduce", {"array": [1, 2, 3, 4]}, ["$", "/summer"], 0]]
```
Result: `10`

### Complex Lambda Pipelines

**Filter and transform using stored lambdas:**
```json
["let", [
  ["isEven", ["lambda", ["x"], ["==", ["%", ["$", "/x"], 2], 0]]],
  ["square", ["lambda", ["x"], ["*", ["$", "/x"], ["$", "/x"]]]],
  ["data", {"array": [1, 2, 3, 4, 5, 6]}]
],
["map", 
  ["filter", ["$", "/data"], ["$", "/isEven"]], 
  ["$", "/square"]
]]
```
Result: `{"array": [4, 16, 36]}` (filter evens: [2,4,6], then square them)

### Nested Lambda Scope

**Lambdas can access variables from their enclosing scope:**
```json
["let", [["multiplier", 10]],
 ["map", {"array": [1, 2, 3]}, 
  ["lambda", ["x"], ["*", ["$", "/x"], ["$", "/multiplier"]]]
 ]]
```
Result: `{"array": [10, 20, 30]}`

### Lambda Function Benefits

1. **Reusability**: Define once, use multiple times
2. **Modularity**: Break complex logic into small, testable functions  
3. **Composition**: Combine simple functions to build complex transformations
4. **Readability**: Named functions make code self-documenting
5. **Functional Programming**: Support for higher-order functions and function composition

# Language Reference

This section provides complete specifications for all Computo operators. Each operator specification includes syntax, parameter requirements, return types, and error conditions.

## Syntax Conventions

- `<expr>`: Any evaluable expression
- `<number>`: Numeric literal or expression evaluating to number
- `<string>`: String literal or expression evaluating to string  
- `<boolean>`: Boolean literal or expression evaluating to boolean
- `<array>`: Array literal `{"array": [...]}` or expression evaluating to array
- `<object>`: Object literal `{...}` or expression evaluating to object
- `<json_pointer>`: String literal containing JSON Pointer path (e.g., "/path/to/data")
- `[...]`: Required parameter
- `[..., ...]`: Multiple required parameters
- `[..., ...]?`: Optional parameter(s)

## Arithmetic Operators

### `+` - Addition
**Syntax**: `["+", <number>, <number>, ...]`  
**Parameters**: 1 or more numbers  
**Returns**: Number (sum of all arguments)  
**Special Cases**: 
- Single argument: Returns the argument unchanged
- Empty: Not allowed, throws InvalidArgumentException
**Examples**: `["+", 1, 2, 3]` → `6`

### `-` - Subtraction  
**Syntax**: `["-", <number>, <number>, ...]`  
**Parameters**: 1 or more numbers  
**Returns**: Number  
**Behavior**: 
- Single argument: Unary negation
- Multiple arguments: Left-associative subtraction (first - second - third - ...)
**Examples**: `["-", 10, 3, 2]` → `5`, `["-", 5]` → `-5`

### `*` - Multiplication
**Syntax**: `["*", <number>, <number>, ...]`  
**Parameters**: 1 or more numbers  
**Returns**: Number (product of all arguments)  
**Examples**: `["*", 2, 3, 4]` → `24`

### `/` - Division
**Syntax**: `["/", <number>, <number>, ...]`  
**Parameters**: 1 or more numbers  
**Returns**: Number  
**Behavior**:
- Single argument: Reciprocal (1/argument)  
- Multiple arguments: Left-associative division
**Error Conditions**: Division by zero throws InvalidArgumentException  
**Examples**: `["/", 20, 2, 2]` → `5`, `["/", 4]` → `0.25`

### `%` - Modulo
**Syntax**: `["%", <number>, <number>]`  
**Parameters**: Exactly 2 numbers  
**Returns**: Number (remainder of first / second)  
**Error Conditions**: Division by zero throws InvalidArgumentException  
**Examples**: `["%", 20, 6]` → `2`

## Comparison Operators

All comparison operators support n-ary chaining (e.g., `a > b > c` means `a > b && b > c`).

### `>` - Greater Than
**Syntax**: `[">", <number>, <number>, ...]`  
**Parameters**: 2 or more numbers  
**Returns**: Boolean (true if chain comparison holds)  
**Examples**: `[">", 5, 3, 1]` → `true`

### `<` - Less Than  
**Syntax**: `["<", <number>, <number>, ...]`  
**Parameters**: 2 or more numbers  
**Returns**: Boolean  
**Examples**: `["<", 1, 3, 5]` → `true`

### `>=` - Greater Than or Equal
**Syntax**: `[">=", <number>, <number>, ...]`  
**Parameters**: 2 or more numbers  
**Returns**: Boolean  
**Examples**: `[">=", 5, 5, 3]` → `true`

### `<=` - Less Than or Equal
**Syntax**: `["<=", <number>, <number>, ...]`  
**Parameters**: 2 or more numbers  
**Returns**: Boolean  
**Examples**: `["<=", 1, 2, 2]` → `true`

### `==` - Equal
**Syntax**: `["==", <expr>, <expr>, ...]`  
**Parameters**: 2 or more expressions of any type  
**Returns**: Boolean (true if all arguments are equal)  
**Examples**: `["==", 2, 2, 2]` → `true`

### `!=` - Not Equal
**Syntax**: `["!=", <expr>, <expr>]`  
**Parameters**: Exactly 2 expressions of any type  
**Returns**: Boolean (true if arguments are not equal)  
**Examples**: `["!=", 1, 2]` → `true`

## Logical Operators

### `and` - Logical AND
**Syntax**: `["and", <expr>, <expr>, ...]`  
**Parameters**: 1 or more expressions  
**Returns**: Boolean (true if all arguments are truthy)  
**Truthiness Rules**: false, 0, "", null, empty arrays/objects are falsy; everything else is truthy  
**Examples**: `["and", true, 1, "non-empty"]` → `true`

### `or` - Logical OR  
**Syntax**: `["or", <expr>, <expr>, ...]`  
**Parameters**: 1 or more expressions  
**Returns**: Boolean (true if any argument is truthy)  
**Examples**: `["or", false, 0, 3]` → `true`

### `not` - Logical NOT
**Syntax**: `["not", <expr>]`  
**Parameters**: Exactly 1 expression  
**Returns**: Boolean (negation of argument's truthiness)  
**Examples**: `["not", false]` → `true`

## Data Access Operators

### `$input` - Input Access
**Syntax**: `["$input"]` or `["$input", <json_pointer>]`  
**Parameters**: 0 or 1 JSON Pointer string  
**Returns**: JSON value from input data  
**Behavior**: 
- No arguments: Returns entire input
- With pointer: Returns data at specified path
**Error Conditions**: Invalid JSON Pointer throws InvalidArgumentException  
**Examples**: `["$input"]`, `["$input", "/users/0/name"]`

### `$inputs` - Multiple Inputs Access
**Syntax**: `["$inputs"]` or `["$inputs", <json_pointer>]`  
**Parameters**: 0 or 1 JSON Pointer string  
**Returns**: Array of all inputs or data from specific input index  
**Examples**: `["$inputs"]`, `["$inputs", "/0"]`, `["$inputs", "/1/users"]`

### `$` - Variable Access
**Syntax**: `["$", <json_pointer>]`  
**Parameters**: Exactly 1 JSON Pointer string  
**Returns**: Value of variable at specified path  
**Error Conditions**: Undefined variable throws InvalidArgumentException  
**Examples**: `["$", "/varname"]`, `["$", "/config/host"]`

### `let` - Variable Binding
**Syntax**: `["let", <bindings>, <expr>]`  
**Parameters**: Bindings (object or array), expression  
**Bindings Format**:
- Object: `{"var1": <expr>, "var2": <expr>}`  
- Array: `[["var1", <expr>], ["var2", <expr>]]`
**Returns**: Result of evaluating expression with bound variables  
**Scope**: Variables are available in expression and nested scopes  
**Examples**: 
- `["let", {"x": 10}, ["+", ["$", "/x"], 5]]` → `15`
- `["let", [["x", 10]], ["+", ["$", "/x"], 5]]` → `15`

## Object Operators

### `obj` - Object Construction
**Syntax**: `["obj", <key_expr>, <value_expr>, ...]`  
**Parameters**: Even number of expressions (key-value pairs)  
**Returns**: Object with evaluated key-value pairs  
**Key Requirements**: Keys must evaluate to strings  
**Dynamic Keys**: Both keys and values are evaluated expressions  
**Examples**: 
- `["obj", "name", "Alice"]` → `{"name": "Alice"}`
- `["obj", ["strConcat", "user_", ["$", "/id"]], "Alice"]` → Dynamic key

### `keys` - Object Keys
**Syntax**: `["keys", <object>]`  
**Parameters**: Exactly 1 object  
**Returns**: `{"array": [key1, key2, ...]}`  
**Examples**: `["keys", {"a": 1, "b": 2}]` → `{"array": ["a", "b"]}`

### `values` - Object Values
**Syntax**: `["values", <object>]`  
**Parameters**: Exactly 1 object  
**Returns**: `{"array": [value1, value2, ...]}`  
**Examples**: `["values", {"a": 1, "b": 2}]` → `{"array": [1, 2]}`

### `objFromPairs` - Object from Key-Value Pairs
**Syntax**: `["objFromPairs", <array>]`  
**Parameters**: Array of [key, value] pairs  
**Returns**: Object constructed from pairs  
**Examples**: `["objFromPairs", {"array": [["a", 1], ["b", 2]]}]` → `{"a": 1, "b": 2}`

### `pick` - Select Object Keys
**Syntax**: `["pick", <object>, <array>]`  
**Parameters**: Object, array of key names  
**Returns**: New object with only specified keys  
**Examples**: `["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}]` → `{"a": 1, "c": 3}`

### `omit` - Remove Object Keys
**Syntax**: `["omit", <object>, <array>]`  
**Parameters**: Object, array of key names to remove  
**Returns**: New object without specified keys  
**Examples**: `["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}]` → `{"a": 1, "c": 3}`

### `merge` - Merge Objects
**Syntax**: `["merge", <object>, <object>, ...]`  
**Parameters**: 1 or more objects  
**Returns**: Single object with all keys merged (later objects override earlier)  
**Examples**: `["merge", {"a": 1}, {"b": 2}]` → `{"a": 1, "b": 2}`

## Control Flow Operators

### `if` - Conditional Expression
**Syntax**: `["if", <condition>, <then_expr>, <else_expr>]`  
**Parameters**: Condition, then expression, else expression  
**Returns**: Result of then expression if condition is truthy, else expression otherwise  
**Tail Call Optimization**: Supports TCO for recursive patterns  
**Examples**: `["if", [">", 5, 3], "yes", "no"]` → `"yes"`

### `lambda` - Lambda Function
**Syntax**: `["lambda", [param1, param2, ...], <body>]`  
**Parameters**: Array of parameter names (strings), body expression  
**Parameter Count**: Supports 0, 1, or multiple parameters  
**Returns**: Lambda function object  
**Scope**: Captures lexical scope, parameters accessible via `["$", "/param_name"]`  
**Examples**: 
- `["lambda", [], 42]` - No parameters
- `["lambda", ["x"], ["+", ["$", "/x"], 1]]` - One parameter  
- `["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]]` - Two parameters

## Array Operators

### `map` - Array Mapping
**Syntax**: `["map", <array>, <lambda>]`  
**Parameters**: Array, lambda function  
**Returns**: New array with lambda applied to each element  
**Examples**: `["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]` → `{"array": [2, 4, 6]}`

### `filter` - Array Filtering
**Syntax**: `["filter", <array>, <lambda>]`  
**Parameters**: Array, lambda function (must return boolean)  
**Returns**: New array with elements where lambda returns truthy  
**Examples**: `["filter", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]` → `{"array": [3, 4]}`

### `reduce` - Array Reduction
**Syntax**: `["reduce", <array>, <lambda>, <initial>]`  
**Parameters**: Array, lambda function (2 parameters: accumulator, item), initial value  
**Returns**: Single value from reducing array  
**Examples**: `["reduce", {"array": [1, 2, 3]}, ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]], 0]` → `6`

### `count` - Array Length
**Syntax**: `["count", <array>]`  
**Parameters**: Exactly 1 array  
**Returns**: Number (length of array)  
**Examples**: `["count", {"array": [1, 2, 3]}]` → `3`

### `find` - Find Array Element
**Syntax**: `["find", <array>, <lambda>]`  
**Parameters**: Array, lambda function (predicate)  
**Returns**: First element where lambda returns truthy, or null if not found  
**Examples**: `["find", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]` → `3`

### `some` - Array Some Test
**Syntax**: `["some", <array>, <lambda>]`  
**Parameters**: Array, lambda function (predicate)  
**Returns**: Boolean (true if any element satisfies predicate)  
**Examples**: `["some", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]` → `true`

### `every` - Array Every Test
**Syntax**: `["every", <array>, <lambda>]`  
**Parameters**: Array, lambda function (predicate)  
**Returns**: Boolean (true if all elements satisfy predicate)  
**Examples**: `["every", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 0]]]` → `true`

## Functional Programming Operators

### `car` - First Element
**Syntax**: `["car", <array>]`  
**Parameters**: Exactly 1 non-empty array  
**Returns**: First element of array  
**Error Conditions**: Empty array throws InvalidArgumentException  
**Examples**: `["car", {"array": [1, 2, 3]}]` → `1`

### `cdr` - Rest Elements
**Syntax**: `["cdr", <array>]`  
**Parameters**: Exactly 1 array  
**Returns**: Array containing all elements except the first  
**Examples**: `["cdr", {"array": [1, 2, 3]}]` → `{"array": [2, 3]}`

### `cons` - Prepend Element
**Syntax**: `["cons", <element>, <array>]`  
**Parameters**: Element to prepend, array  
**Returns**: New array with element prepended  
**Examples**: `["cons", 0, {"array": [1, 2]}]` → `{"array": [0, 1, 2]}`

### `append` - Concatenate Arrays
**Syntax**: `["append", <array>, <array>, ...]`  
**Parameters**: 2 or more arrays  
**Returns**: New array with all arrays concatenated  
**Examples**: `["append", {"array": [1, 2]}, {"array": [3, 4]}]` → `{"array": [1, 2, 3, 4]}`

## String Operators

### `strConcat` - String Concatenation
**Syntax**: `["strConcat", <string>, <string>, ...]`  
**Parameters**: 1 or more strings  
**Returns**: Single concatenated string  
**Examples**: `["strConcat", "Hello", " ", "World"]` → `"Hello World"`

### `join` - Array to String
**Syntax**: `["join", <array>, <separator>]`  
**Parameters**: Array of strings, separator string  
**Returns**: Single string with array elements joined by separator  
**Examples**: `["join", {"array": ["hello", "world"]}, " "]` → `"hello world"`

## Array Manipulation Operators

### `sort` - Array Sorting
**Syntax**: 
- `["sort", <array>]` - Natural ascending order
- `["sort", <array>, "asc"|"desc"]` - With direction
- `["sort", <array>, <json_pointer>]` - Sort objects by field
- `["sort", <array>, <field1>, <field2>, ...]` - Multi-field sort
**Parameters**: Array, optional sort criteria  
**Returns**: New sorted array  
**Performance**: Automatically uses DSU optimization for complex sorts  
**Examples**: 
- `["sort", {"array": [3, 1, 4]}]` → `{"array": [1, 3, 4]}`
- `["sort", {"array": [...]}, "/name"]` - Sort objects by name field

### `reverse` - Array Reversal
**Syntax**: `["reverse", <array>]`  
**Parameters**: Exactly 1 array  
**Returns**: New array with elements in reverse order  
**Examples**: `["reverse", {"array": [1, 2, 3]}]` → `{"array": [3, 2, 1]}`

### `unique` - Remove Duplicates
**Syntax**: `["unique", <array>]`  
**Parameters**: Exactly 1 array  
**Returns**: New array with duplicate elements removed (preserves first occurrence)  
**Examples**: `["unique", {"array": [1, 2, 2, 3, 3, 3]}]` → `{"array": [1, 2, 3]}`

### `uniqueSorted` - Remove Duplicates from Sorted Array
**Syntax**: 
- `["uniqueSorted", <array>]` - Basic deduplication
- `["uniqueSorted", <array>, "firsts"|"lasts"|"singles"]` - Occurrence selection
- `["uniqueSorted", <array>, <json_pointer>, <mode>]` - Field-based deduplication
**Parameters**: Sorted array, optional mode and field  
**Returns**: New array with duplicates removed  
**Performance**: Optimized for pre-sorted input  
**Examples**: `["uniqueSorted", {"array": [1, 1, 2, 2, 3]}]` → `{"array": [1, 2, 3]}`

### `zip` - Array Pairing
**Syntax**: `["zip", <array1>, <array2>, ...]`  
**Parameters**: 2 or more arrays  
**Returns**: Array of tuples pairing corresponding elements  
**Length**: Result length equals shortest input array  
**Examples**: `["zip", {"array": [1, 2]}, {"array": ["a", "b"]}]` → `{"array": [[1, "a"], [2, "b"]]}`

## Utility Operators

### `approx` - Approximate Equality
**Syntax**: `["approx", <number1>, <number2>, <tolerance>]`  
**Parameters**: Two numbers to compare, tolerance value  
**Returns**: Boolean (true if numbers are within tolerance)  
**Examples**: `["approx", 0.1, 0.10001, 0.001]` → `true`

# Error Handling

All operators throw structured exceptions:

- **InvalidArgumentException**: Wrong argument types, counts, or values
- **InvalidOperatorException**: Unknown operator names  
- **ComputoException**: Base class for all Computo errors

Each exception includes the evaluation path where the error occurred for precise debugging.

## C++ Library Usage

### Including the Library
```cpp
#include <computo.hpp>
```

### Basic Execution
```cpp
#include <computo.hpp>
#include <iostream>

int main() {
    // Simple arithmetic
    auto script = nlohmann::json::array({"+", 1, 2, 3});
    auto result = computo::execute(script);
    std::cout << result << std::endl;  // 6

    // With input data
    auto transform = nlohmann::json::parse(R"([
        "map",
        ["$input"],
        ["lambda", ["x"], ["*", ["$", "/x"], 2]]
    ])");
    auto input = nlohmann::json::array({1, 2, 3});
    result = computo::execute(transform, input);
    std::cout << result << std::endl;  // [2, 4, 6]

    return 0;
}
```

### Thread Safety
The library is fully thread-safe. Multiple threads can execute scripts concurrently without external locking.

### Error Handling
```cpp
try {
    auto script = nlohmann::json::array({"+", "not a number", 1});
    auto result = computo::execute(script);
} catch (const computo::InvalidArgumentException& e) {
    std::cerr << "Argument error: " << e.what() << std::endl;
} catch (const computo::InvalidOperatorException& e) {
    std::cerr << "Unknown operator: " << e.what() << std::endl;
} catch (const computo::ComputoException& e) {
    std::cerr << "Computo error: " << e.what() << std::endl;
}
```

### Debugging API

The library provides comprehensive debugging capabilities for programmatic use:

```cpp
#include <computo.hpp>

int main() {
    // Create a debug context
    computo::DebugContext debug_ctx;
    
    // Set breakpoints
    debug_ctx.set_operator_breakpoint("map");
    debug_ctx.set_variable_breakpoint("/users");
    
    // Enable debugging and tracing
    debug_ctx.set_debug_enabled(true);
    debug_ctx.set_trace_enabled(true);
    
    auto script = nlohmann::json::parse(R"([
        "let", [["users", {"array": [1, 2, 3]}]],
        ["map", ["$", "/users"], ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
    ])");
    
    try {
        // Execute with debugging
        auto result = computo::execute(script, {}, &debug_ctx);
        std::cout << "Result: " << result << std::endl;
    } catch (const computo::DebugBreakException& e) {
        // Handle breakpoint hit
        std::cout << "Breakpoint hit: " << e.get_reason() 
                  << " at " << e.get_location() << std::endl;
        
        // Examine execution trace
        auto trace = debug_ctx.get_execution_trace();
        for (const auto& step : trace) {
            std::cout << "Step: " << step.operation 
                      << " at " << step.location << std::endl;
        }
        
        // Continue execution
        debug_ctx.set_step_mode(false);
        auto result = computo::execute(script, {}, &debug_ctx);
    }
    
    return 0;
}
```

## REPL Features

The REPL provides a comprehensive interactive debugging environment for developing, testing, and debugging JSON transformation scripts.

### Getting Started

```bash
# Start REPL
computo --repl

# Load a script for debugging
computo --repl script.json

# Enable JSON comment support
computo --repl --comments script.jsonc
```

### Basic Commands

- **`help`**: Show all available commands with descriptions
- **`quit`** or **`exit`**: Exit the REPL
- **`vars`**: Display all variables in current scope (from `let` expressions)
- **`clear`**: Clear the screen
- **`history`**: Show command history

### Script Execution

- **`run <filename>`**: Load and execute a script file
  - Supports `.json` files and `.jsonc` files with comments (when `--comments` flag used)
  - Example: `run transform.json`, `run complex_pipeline.jsonc`
- **Direct JSON execution**: Type any valid JSON expression to execute it immediately
  - Example: `["+", 1, 2, 3]` → `6`

### Debugging System

#### Breakpoint Management

- **`break <operator>`**: Set a breakpoint on an operator
  - Example: `break +`, `break map`, `break filter`
  - Execution will pause whenever the specified operator is encountered
- **`break <variable>`**: Set a breakpoint on a variable access
  - Example: `break /users`, `break /config/database`
  - Uses JSON Pointer syntax for nested variables
- **`nobreak <operator_or_variable>`**: Remove a specific breakpoint
  - Example: `nobreak +`, `nobreak /users`
- **`breaks`**: List all active breakpoints

#### Debug Mode Control

When execution hits a breakpoint, you enter debug mode with these commands:

- **`step`** or **`s`**: Execute one operation and pause
- **`continue`** or **`c`**: Continue execution until next breakpoint
- **`finish`** or **`f`**: Complete current execution, ignoring remaining breakpoints
- **`where`** or **`w`**: Show current execution location and context

#### Debug Features

- **`debug on`**: Enable debugging mode (breakpoints will trigger)
- **`debug off`**: Disable debugging mode (breakpoints ignored)
- **`trace on`**: Enable execution tracing (records all operations)
- **`trace off`**: Disable execution tracing

### Debugging Workflow Examples

#### Basic Debugging Session

```bash
computo --repl
> break map
> [["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]]
Debug breakpoint: operator breakpoint: map at /
> step
> continue
```

#### Variable Inspection

```bash
computo --repl
> break /users
> ["let", [["users", {"array": [{"name": "Alice"}, {"name": "Bob"}]}]], ["map", ["$", "/users"], ["lambda", ["user"], ["$", "/user/name"]]]]
Debug breakpoint: variable breakpoint: /users at /let
> vars
users: [{"name": "Alice"}, {"name": "Bob"}]
> continue
```

#### Script Debugging

```bash
computo --repl
> run complex_transform.json
> break +
> step
> where
Current location: /let/map/lambda/+
> vars
acc: 10
item: 5
> continue
```

### JSON Comment Support

When using the `--comments` flag, you can include comments in JSON files:

```json
{
  // This is a comment
  "operation": "+",
  "args": [1, 2, 3], // Another comment
  /* Multi-line
     comment */
  "nested": {
    "value": 42 // End-line comment
  }
}
```

## Building from Source

### Requirements
- C++17 compiler
- CMake 3.15+
- nlohmann/json
- Google Test (for tests)
- readline (for REPL, optional)

### Build Instructions
```bash
# Fast development build (recommended)
cmake -B build
cmake --build build -j$(nproc)

# This creates:
# - libcomputo.a (unified library with debug features)
# - computo (unified CLI with script execution and REPL modes)
# - test_computo (comprehensive tests)

# Run tests
ctest --test-dir build

# Code quality checks (separate from build for speed)
cd build
make format      # Format all code
make lint        # Run static analysis
make quality     # Run all quality checks
```

### Documentation Development
```bash
# Validate all documentation
cmake --build build --target docs

# Test all examples against engine  
cmake --build build --target docs-validate

# Generate reference docs
cmake --build build --target docs-generate

# Check operator coverage
cmake --build build --target docs-coverage
```

### Unicode Support

Computo provides comprehensive Unicode support for all string operations through the nlohmann/json library. String operations correctly handle:

- **International text**: Full support for CJK characters, emoji, accented text, and mixed scripts
- **Proper encoding**: All operations preserve UTF-8 encoding integrity

### Dependency Installation

All dependencies are expected to be installed in the system.  They will be found by CMake.

# Build Computo normally
cmake -B build
cmake --build build
```
**Note:** The build system is optimized for fast development. Code formatting and linting are separate targets to avoid slow compilation times. See `BUILD_OPTIMIZATION.md` for details.
