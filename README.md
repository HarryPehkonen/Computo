# Computo

A safe, sandboxed, thread-safe JSON-native data transformation engine with a Lisp-like language syntax expressed in JSON.

## Overview

Computo consists of:
- **libcomputo**: A minimal, thread-safe C++ library for JSON transformations
- **computo**: A CLI tool for running Computo scripts
- **computo_repl**: An interactive REPL with debugging features

## Quick Start

### Basic Usage

```bash
# Execute a script
computo script.json

# Execute with input data
computo script.json input.json

# Interactive REPL
computo_repl
```

### Simple Example

```json
["+", 1, 2, 3]
```
Result: `6`

## Language Syntax

All Computo scripts are valid JSON. The basic syntax is:

```json
["operator", arg1, arg2, ...]
```

### Literals

- Numbers: `42`, `3.14`
- Strings: `"hello"`
- Booleans: `true`, `false`
- Null: `null`
- Arrays: `{"array": [1, 2, 3]}`
- Objects: `{"key": "value"}`

## Operators Reference

### Arithmetic (n-ary)

```json
["+", 1, 2, 3]              // 6
["-", 10, 3, 2]             // 5
["-", 5]                    // -5 (unary negation)
["*", 2, 3, 4]              // 24
["/", 20, 2, 2]             // 5
["/", 4]                    // 0.25 (reciprocal)
["%", 20, 6]                // 2
```

### Comparison (n-ary with chaining)

```json
[">", 5, 3, 1]              // true (5 > 3 && 3 > 1)
["<", 1, 3, 5]              // true
[">=", 5, 5, 3]             // true
["<=", 1, 2, 2]             // true
["==", 2, 2, 2]             // true (all equal)
["!=", 1, 2]                // true (binary only)
```

### Logical

```json
["&&", true, 1, "non-empty"] // true (all truthy)
["||", false, 0, 3]          // true (at least one truthy)
["not", false]               // true (unary only)
```

### Variables and Data Access

```json
// Access input
["$input"]                   // First input or null
["$inputs"]                  // Array of all inputs

// Variable access
["$", "/varname"]            // Access variable

// JSON Pointer access
["get", {"a": {"b": 2}}, "/a/b"]  // 2

// Variable binding
["let", [["x", 10]], ["+", ["$", "/x"], 5]]  // 15
```

### Object Construction

```json
// Static keys
["obj", ["name", "Alice"], ["age", 30]]
// Result: {"name": "Alice", "age": 30}

// Variable keys
["let", [["key", "title"]], 
  ["obj", [["$", "/key"], "Manager"]]
]
// Result: {"title": "Manager"}
```

### Control Flow

```json
["if", [">", 5, 3], "yes", "no"]  // "yes"
```

### Array Operations

```json
// Map: transform each element
["map", {"array": [1, 2, 3]}, 
  ["lambda", ["x"], ["*", ["$", "/x"], 2]]
]
// Result: {"array": [2, 4, 6]}

// Filter: keep matching elements
["filter", {"array": [1, 2, 3, 4, 5]},
  ["lambda", ["x"], [">", ["$", "/x"], 2]]
]
// Result: {"array": [3, 4, 5]}

// Reduce: aggregate to single value
["reduce", {"array": [1, 2, 3, 4]},
  ["lambda", ["args"], 
    ["+", ["get", ["$", "/args"], "/0"], 
          ["get", ["$", "/args"], "/1"]]
  ],
  0
]
// Result: 10

// Other array operations
["count", {"array": [1, 2, 3]}]     // 3
["find", {"array": [1, 2, 3, 4]},   // 4 (first > 3)
  ["lambda", ["x"], [">", ["$", "/x"], 3]]
]
["some", {"array": [1, 2, 3]},      // true
  ["lambda", ["x"], [">", ["$", "/x"], 2]]
]
["every", {"array": [1, 2, 3]},     // true
  ["lambda", ["x"], [">", ["$", "/x"], 0]]
]
```

### Functional Operations

```json
["car", {"array": [1, 2, 3]}]       // 1 (first element)
["cdr", {"array": [1, 2, 3]}]       // {"array": [2, 3]}
["cons", 0, {"array": [1, 2]}]      // {"array": [0, 1, 2]}
["append", {"array": [1, 2]},       // {"array": [1, 2, 3, 4]}
           {"array": [3, 4]}]
["zip", {"array": ["a", "b"]},      // {"array": [["a", 1], ["b", 2]]}
        {"array": [1, 2]}]
```

### Lambda Expressions

```json
["lambda", ["x"], ["*", ["$", "/x"], 2]]
```

Lambda expressions are used with array operations and can be called directly:

```json
["call", ["lambda", ["x"], ["*", ["$", "/x"], 2]], 5]  // 10
```

### Utilities

```json
["strConcat", "Hello", " ", "World"]  // "Hello World"
["merge", {"a": 1}, {"b": 2}]         // {"a": 1, "b": 2}
["approx", 0.1, 0.10001, 0.001]       // true (within tolerance)
```

## Advanced Examples

### Data Transformation Pipeline

```json
["let", [
  ["data", {"array": [
    {"name": "Alice", "score": 85},
    {"name": "Bob", "score": 92},
    {"name": "Charlie", "score": 78}
  ]}]
], [
  "map",
  ["filter", ["$", "/data"],
    ["lambda", ["s"], [">", ["get", ["$", "/s"], "/score"], 80]]
  ],
  ["lambda", ["s"], ["get", ["$", "/s"], "/name"]]
]]
```
Result: `{"array": ["Alice", "Bob"]}`

### Building Objects from Arrays

```json
["reduce", 
  ["zip", {"array": ["a", "b", "c"]}, {"array": [1, 2, 3]}],
  ["lambda", ["x"], 
    ["merge", 
      ["get", ["$", "/x"], "/0"],
      ["obj", [
        ["get", ["get", ["$", "/x"], "/1"], "/0"],
        ["get", ["get", ["$", "/x"], "/1"], "/1"]
      ]]
    ]
  ],
  {}
]
```
Result: `{"a": 1, "b": 2, "c": 3}`

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
    auto transform = nlohmann::json::array({
        "map", "$input",
        nlohmann::json::array({"lambda", nlohmann::json::array({"x"}),
            nlohmann::json::array({"*", nlohmann::json::array({"$", "/x"}), 2})
        })
    });
    auto input = nlohmann::json::object({{"array", {1, 2, 3}}});
    result = computo::execute(transform, input);
    std::cout << result << std::endl;  // {"array": [2, 4, 6]}
    
    return 0;
}
```

### Thread Safety

The library is fully thread-safe. Multiple threads can execute scripts concurrently:

```cpp
#include <computo.hpp>
#include <thread>
#include <vector>

void worker(int id) {
    auto script = nlohmann::json::array({"+", id, id});
    auto result = computo::execute(script);
    // Each thread gets correct result
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
```

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

## REPL Features

The REPL provides an interactive environment with additional features:

### Commands

- `help` - Show available commands
- `quit` or `exit` - Exit the REPL
- `_1`, `_2`, etc. - Reference previous results

### Example Session

```
$ computo_repl
Computo REPL v1.0.0
Type 'help' for commands, 'quit' to exit

computo> ["+", 1, 2, 3]
6

computo> ["*", ["$input"], 2]
12

computo> ["let", [["x", 10]], ["+", ["$", "/x"], 5]]
15

computo> quit
Goodbye!
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
# Create build directory
mkdir build && cd build

# Configure (production build)
cmake ..

# Configure (with REPL)
cmake -DREPL=ON ..

# Build
make

# Run tests
./test_computo
```

## Performance Considerations

- All operators are implemented as pure functions
- No global mutable state ensures thread safety
- Tail call optimization prevents stack overflow
- Efficient JSON manipulation using nlohmann/json

## License

Please see the LICENSE file

## Contributing

Please send pull a request.