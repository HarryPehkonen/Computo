# Computo

A safe, sandboxed, thread-safe JSON-native data transformation engine with a Lisp-like language syntax expressed in JSON.

## Overview

Computo consists of:
- **libcomputo**: A minimal, thread-safe C++ library for JSON transformations.
- **computo**: A unified CLI tool with script execution mode and interactive REPL with comprehensive debugging features.

## Quick Start

### Basic Usage

```bash
# Execute a script
computo script.json

# Execute with input data
computo script.json input.json

# Interactive REPL with debugging
computo --repl

# Load script into REPL for debugging
computo --repl script.json
```

### Simple Example

**Script:**
```json
["+", 1, 2, 3]
```

**Result:**
```
6
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
computo script.json input1.json input2.json
```

**Result:**
```
30
```

## Language Syntax

All Computo scripts are valid JSON. The basic syntax for an operation is a JSON array where the first element is a string representing the operator.

`["operator", arg1, arg2, ...]`

### Literals

- **Numbers**: `42`, `3.14`
- **Strings**: `"hello"`
- **Booleans**: `true`, `false`
- **Null**: `null`
- **Arrays**: To represent a literal array and distinguish it from an operator expression, wrap it in an object with the key `"array"`: `{"array": [1, 2, 3]}`.
- **Objects**: Standard JSON objects: `{"key": "value"}`.

## Operators Reference

### Arithmetic (n-ary)
- `["+", 1, 2, 3]` → `6`
- `["-", 10, 3, 2]` → `5`
- `["-", 5]` (unary negation) → `-5`
- `["*", 2, 3, 4]` → `24`
- `["/", 20, 2, 2]` → `5`
- `["/", 4]` (reciprocal) → `0.25`
- `["%", 20, 6]` → `2`

### Comparison (n-ary with chaining)
- `[">", 5, 3, 1]` → `true` (evaluates as `5 > 3 && 3 > 1`)
- `["<", 1, 3, 5]` → `true`
- `[">=", 5, 5, 3]` → `true`
- `["<=", 1, 2, 2]` → `true`
- `["==", 2, 2, 2]` → `true` (all arguments are equal)
- `["!=", 1, 2]` (binary only) → `true`

### Logical
- `["&&", true, 1, "non-empty"]` → `true` (all arguments are truthy)
- `["||", false, 0, 3]` → `true` (at least one argument is truthy)
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
- `["let", [["x", 10]], ["+", ["$", "/x"], 5]]`: Bind variables for use in an expression. Result: `15`.

### Object Operations
- `["obj", ["name", "Alice"], ["age", 30]]`: Create an object. Result: `{"name": "Alice", "age": 30}`.
- `["keys", {"a": 1, "b": 2}]`: Get object keys. Result: `{"array": ["a", "b"]}`.
- `["values", {"a": 1, "b": 2}]`: Get object values. Result: `{"array": [1, 2]}`.
- `["objFromPairs", {"array": [["a", 1], ["b", 2]]}]`: Create object from key-value pairs. Result: `{"a": 1, "b": 2}`.
- `["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}]`: Select specific keys. Result: `{"a": 1, "c": 3}`.
- `["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}]`: Remove specific keys. Result: `{"a": 1, "c": 3}`.
- `["merge", {"a": 1}, {"b": 2}]`: Merge objects. Result: `{"a": 1, "b": 2}`.

### Control Flow
- `["if", [">", 5, 3], "yes", "no"]` → `"yes"`

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
- `["split", "hello world", " "]` → `{"array": ["hello", "world"]}`
- `["join", {"array": ["hello", "world"]}, " "]` → `"hello world"`
- `["trim", "  hello  "]` → `"hello"`
- `["upper", "hello"]` → `"HELLO"`
- `["lower", "HELLO"]` → `"hello"`

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
      ["name", ["$", "/user/profile/name"]],
      ["total_orders", 
        ["reduce", ["$", "/user/orders"],
          ["lambda", ["acc", "order"], 
            ["+", ["$", "/acc"], ["$", "/order/total"]]
          ],
          0
        ]
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
        ["user_id", ["$", "/user/id"]],
        ["email", ["$", "/user/profile/contact/email"]],
        ["pending_orders", ["count", 
          ["filter", ["$", "/user/orders"],
            ["lambda", ["order"], ["==", ["$", "/order/status"], "pending"]]
          ]
        ]],
        ["total_spent", ["reduce", ["$", "/user/orders"],
          ["lambda", ["acc", "order"], ["+", ["$", "/acc"], ["$", "/order/total"]]],
          0
        ]]
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

### Unicode Support

Computo provides comprehensive Unicode support for all string operations through the [uni-algo library](https://github.com/uni-algo/uni-algo). String operations correctly handle:

- **UTF-8 character splitting**: `["split", "café", ""]` properly splits into Unicode characters `["c", "a", "f", "é"]`
- **Unicode case conversion**: `["upper", "straße"]` converts to `"STRASSE"` (ß → SS), `["lower", "CAFÉ"]` converts to `"café"`
- **Unicode whitespace trimming**: `["trim", "   café   "]` handles Unicode whitespace characters beyond ASCII
- **International text**: Full support for CJK characters, emoji, accented text, and mixed scripts
- **Proper encoding**: All operations preserve UTF-8 encoding integrity

### Dependency Installation

The project uses [uni-algo](https://github.com/uni-algo/uni-algo) for Unicode support. You can install it using different methods:

#### Option 1: Conan (Recommended)
```bash
# Install dependencies with Conan
conan install . --output-folder=build --build=missing

# Build with Conan toolchain
cmake --preset conan-release
cmake --build build
```

#### Option 2: Manual Installation
```bash
# Download and build uni-algo from source
git clone https://github.com/uni-algo/uni-algo.git
cd uni-algo
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build

# Then build Computo normally
cmake -B build
cmake --build build
```

#### Option 3: System Package Manager
```bash
# Some distributions may have uni-algo available
# Check your package manager (results may vary):
sudo apt search uni-algo        # Ubuntu/Debian
sudo pacman -Ss uni-algo       # Arch Linux
brew search uni-algo           # macOS Homebrew
```

**Note:** Conan is the recommended method as it ensures consistent dependency versions across platforms and is actively maintained for uni-algo.

**Note:** The build system is optimized for fast development. Code formatting and linting are separate targets to avoid slow compilation times. See `BUILD_OPTIMIZATION.md` for details.
