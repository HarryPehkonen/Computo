# Computo

A safe, sandboxed JSON transformation engine with Lisp-like syntax expressed in JSON. Works with **Permuto** for advanced templating, creating a powerful two-layer system for sophisticated JSON-to-JSON transformations.

## Architecture Overview

- **Computo**: Handles complex programmatic logic (conditionals, loops, calculations)
- **Permuto**: Handles simple declarative substitutions and templating using `${path}` syntax
- **Code is Data**: All scripts are valid JSON with unambiguous syntax
- **Immutable**: Pure functions that don't modify input data
- **Sandboxed**: No I/O operations or system access

## Quick Start

### Building
```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest --verbose
```

### CLI Usage
```bash
# Basic transformation
./build/cli/computo script.json input.json

# With Permuto string interpolation
./build/computo --interpolation script.json input.json

# Available CLI options (from Permuto integration)
./build/computo --missing-key=error --max-depth=32 script.json input.json
```

### Library Usage
```cpp
#include <computo/computo.hpp>

nlohmann::json script = R"(["*", ["get", ["$input"], "/value"], 2])"_json;
nlohmann::json input = R"({"value": 21})"_json;
nlohmann::json result = computo::execute(script, input);
// Result: 42
```

## Core Syntax

### Operator Calls vs Literal Data
```json
// Operator call: [operator, arg1, arg2, ...]
["*", 6, 7]

// Literal array: {"array": [item1, item2, ...]}
{"array": [6, 7]}

// Objects are standard JSON
{"key": "value", "number": 42}
```

### Essential Operators

#### Data Access & Scoping
```json
// Variable binding
["let", [["x", 10], ["y", 20]], ["*", ["$", "/x"], ["$", "/y"]]]

// Variable access
["$", "/variable_name"]

// Input data access
["$input"]

// JSON Pointer access
["get", {"key": {"nested": 42}}, "/key/nested"]
```

#### Logic & Control Flow
```json
// Conditional execution
["if", [">", ["$", "/score"], 80], "Pass", "Fail"]

// Comparison operators
[">", 10, 5]     // true
["==", "a", "b"] // false
["!=", 1, 2]     // true
```

#### Data Construction
```json
// Object creation
["obj", ["name", "Alice"], ["age", 30], ["active", true]]

// Array creation
{"array": [1, 2, 3, ["*", 2, 4]]}
```

#### Array Operations with Lambdas
```json
// Transform array elements
["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
// Result: [2, 4, 6]

// Filter array elements
["filter", {"array": [1, 2, 3, 4, 5]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]
// Result: [3, 4, 5]

// Reduce array to single value
["reduce", {"array": [1, 2, 3, 4]}, ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0]
// Result: 10
```

#### Permuto Integration
```json
// Apply Permuto templates
["permuto.apply", 
  {"greeting": "Hello ${/user/name}!", "id": "${/user/id}"}, 
  {"user": {"name": "Alice", "id": 123}}
]
// Result: {"greeting": "Hello Alice!", "id": 123}
```

## Complete Operator Reference

### Mathematical
- `["+", num1, num2]` - Addition
- `["-", num1, num2]` - Subtraction  
- `["*", num1, num2]` - Multiplication
- `["/", num1, num2]` - Division

### Logical & Comparison
- `["if", condition, then_value, else_value]` - Conditional
- `[">", a, b]`, `["<", a, b]`, `[">=", a, b]`, `["<=", a, b]` - Comparisons
- `["==", a, b]`, `["!=", a, b]` - Equality checks
- `["approx", a, b]` - Approximate equality for floats

### Data Access
- `["$input"]` - Access entire input data
- `["$", "/path"]` - Access variable by path
- `["get", object, "/json/pointer"]` - Extract data using JSON Pointer
- `["let", bindings, body]` - Create variable scope

### Data Construction  
- `["obj", ["key1", val1], ["key2", val2], ...]` - Create object
- `{"array": [item1, item2, ...]}` - Create array (literal syntax)
- `["merge", obj1, obj2, ...]` - Merge objects
- `["count", array]` - Get array length

### Array Operations
- `["map", array, lambda]` - Transform each element
- `["filter", array, lambda]` - Keep elements matching condition
- `["reduce", array, lambda, initial]` - Reduce to single value
- `["find", array, lambda]` - Find first matching element
- `["some", array, lambda]` - Test if any element matches
- `["every", array, lambda]` - Test if all elements match
- `["flatMap", array, lambda]` - Map and flatten results

### Lambda Syntax
```json
["lambda", ["param1", "param2"], body_expression]
```

### Templates
- `["permuto.apply", template, context]` - Process Permuto templates

## Permuto Integration

Computo includes the full Permuto templating engine. When using `--interpolation`, Permuto processes `${path}` placeholders:

```json
// Computo script using Permuto
["permuto.apply", 
  {
    "user_id": "${/user/id}",
    "greeting": "Welcome ${/user/name}! You have ${/stats/messages} messages.",
    "settings": "${/user/preferences}"
  },
  {
    "user": {"id": 123, "name": "Alice"},
    "stats": {"messages": 5},
    "user": {"preferences": {"theme": "dark"}}
  }
]
```

### Permuto Path Syntax (JSON Pointer - RFC 6901)
- `/user/name` - Object property access
- `/items/0` - Array element access  
- `/user/settings/theme` - Nested access
- `/special~0key` - Escape `~` as `~0`
- `/key~1with~1slashes` - Escape `/` as `~1`

### Permuto CLI Options Available in Computo
- `--interpolation` / `--no-interpolation` - String interpolation control
- `--missing-key=MODE` - Missing key behavior (`ignore` or `error`)
- `--start=MARKER` - Custom start marker (default: `${`)
- `--end=MARKER` - Custom end marker (default: `}`)
- `--max-depth=N` - Maximum recursion depth

## Real-World Examples

### API Response Transformation
```json
// Transform API response structure
["obj",
  ["users", ["map", 
    ["get", ["$input"], "/data/users"],
    ["lambda", ["user"], 
      ["obj",
        ["id", ["get", ["$", "/user"], "/user_id"]],
        ["name", ["get", ["$", "/user"], "/full_name"]],
        ["active", ["==", ["get", ["$", "/user"], "/status"], "active"]]
      ]
    ]
  ]],
  ["total", ["count", ["get", ["$input"], "/data/users"]]],
  ["timestamp", ["get", ["$input"], "/metadata/generated_at"]]
]
```

### Configuration Generation
```json
// Generate environment-specific config
["let", [["env", ["get", ["$input"], "/environment"]]],
  ["permuto.apply",
    {
      "database": {
        "host": "${/config/db_host}",
        "port": "${/config/db_port}",
        "ssl": "${/security/use_ssl}"
      },
      "api_url": "https://${/environment}.api.company.com"
    },
    ["obj",
      ["environment", ["$", "/env"]],
      ["config", ["get", ["$input"], ["$", "/env"]]],
      ["security", ["get", ["$input"], "/security_settings"]]
    ]
  ]
]
```

### Data Aggregation
```json
// Calculate statistics from array data
["let", [
    ["numbers", ["map", ["get", ["$input"], "/records"], ["lambda", ["r"], ["get", ["$", "/r"], "/value"]]]],
    ["total", ["reduce", ["$", "/numbers"], ["lambda", ["acc", "x"], ["+", ["$", "/acc"], ["$", "/x"]]], 0]]
  ],
  ["obj",
    ["count", ["count", ["$", "/numbers"]]],
    ["sum", ["$", "/total"]],
    ["average", ["/", ["$", "/total"], ["count", ["$", "/numbers"]]]],
    ["max", ["reduce", ["$", "/numbers"], ["lambda", ["acc", "x"], ["if", [">", ["$", "/x"], ["$", "/acc"]], ["$", "/x"], ["$", "/acc"]]], 0]]
  ]
]
```

## Error Handling

Computo provides structured exceptions with debugging information:

- `ComputoException` - Base exception class
- `InvalidScriptException` - Malformed JSON scripts
- `InvalidOperatorException` - Unknown operators
- `InvalidArgumentException` - Wrong argument types/counts

## Dependencies

- **nlohmann/json** - Core JSON library
- **Permuto** - Template processing library  
- **Google Test** - Testing framework (for development)

## Project Structure

```
computo/
├── CMakeLists.txt           # Build configuration
├── CLAUDE.md                # AI development guidance
├── TECHNICAL_DETAILS.md     # Implementation details
├── PermutoREADME.md        # Permuto documentation
├── book/                   # Learning guide
│   └── 00_index.md
├── cli/                    # Command-line tool
├── include/computo/        # Public headers
├── src/                    # Library implementation
└── tests/                  # Test suite (103 tests)
```

## Development Guidance

- **CLAUDE.md**: Contains project architecture and development instructions
- **TECHNICAL_DETAILS.md**: Implementation phases and technical architecture
- **book/**: Comprehensive learning guide with examples
- **Tests**: 103 tests covering all operators and edge cases

## Safety & Security

- **Sandboxed execution**: No file system or network access
- **Memory safe**: Built-in recursion limits and cycle detection
- **Input validation**: All operator arguments are validated
- **Type preservation**: Maintains JSON data types throughout transformations