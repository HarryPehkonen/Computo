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
// Access input data
["$input"]                   // Entire input object
["$input", "/path/to/data"]  // Navigate within input using JSON Pointer
["$input", "/missing", "default"]  // With default value for missing paths

// Access multiple inputs
["$inputs"]                  // Array of all inputs
["$inputs", "/0/path"]       // Navigate to specific input by index
["$inputs", "/1/missing", "fallback"]  // With default for missing data

// Variable access
["$", "/varname"]            // Access variable
["$", "/varname/path"]       // Navigate within variable using JSON Pointer  
["$", "/missing", "default"] // With default value for missing variables
["$"]                        // All variables in current scope

// Variable binding
["let", [["x", 10]], ["+", ["$", "/x"], 5]]  // 15
```

### Object Operations

```json
// Object construction
["obj", ["name", "Alice"], ["age", 30]]
// Result: {"name": "Alice", "age": 30}

// Variable keys
["let", [["key", "title"]], 
  ["obj", [["$", "/key"], "Manager"]]
]
// Result: {"title": "Manager"}

// Extract keys and values
["keys", {"a": 1, "b": 2}]     // {"array": ["a", "b"]}
["values", {"a": 1, "b": 2}]   // {"array": [1, 2]}

// Create object from key-value pairs
["objFromPairs", {"array": [["a", 1], ["b", 2]]}]  // {"a": 1, "b": 2}

// Select specific keys
["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}]
// Result: {"a": 1, "c": 3}

// Remove specific keys
["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}]
// Result: {"a": 1, "c": 3}
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
    ["+", ["$", "/args/0"], 
          ["$", "/args/1"]]
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

// Array manipulation
["sort", {"array": [3, 1, 2]}]      // {"array": [1, 2, 3]}
["sort", {"array": [3, 1, 2]}, "desc"] // {"array": [3, 2, 1]}
["reverse", {"array": [1, 2, 3]}]   // {"array": [3, 2, 1]}

// Enhanced unique operator (requires pre-sorted data)
["unique", {"array": [1, 1, 2, 3, 3]}]  // {"array": [1, 2, 3]} - "firsts" mode
["unique", {"array": [1, 1, 2, 3, 3]}, "lasts"]   // {"array": [1, 2, 3]} - last occurrences
["unique", {"array": [1, 1, 2, 3, 3]}, "singles"] // {"array": [2]} - items appearing once
["unique", {"array": [1, 1, 2, 3, 3]}, "multiples"] // {"array": [1, 3]} - items with duplicates

// Object uniqueness by field (pre-sorted by field)
["unique", pre_sorted_array, "/field"]           // Unique by field, keep first
["unique", pre_sorted_array, "/field", "lasts"]  // Unique by field, keep last
["unique", pre_sorted_array, "/field", "singles"] // Only objects with unique field values

// Object array sorting with JSON Pointer field access
["sort", {"array": [
  {"name": "charlie", "age": 30},
  {"name": "alice", "age": 25},
  {"name": "bob", "age": 35}
]}, "/name"]  // Sort by name ascending

// Multi-field sorting
["sort", {"array": [
  {"dept": "eng", "level": 3, "salary": 90000},
  {"dept": "hr", "level": 2, "salary": 70000},
  {"dept": "eng", "level": 2, "salary": 80000}
]}, "/dept", ["/level", "desc"], "/salary"]  // dept asc, level desc, salary asc

// Sort with direction control
["sort", {"array": [...]}, ["/field", "desc"]]  // Field descending
```

**Sort Type Ordering**: When sorting mixed-type arrays, values are ordered by type first: `null < numbers < strings < booleans < arrays < objects`, then by value within each type.

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

### String Operations

```json
// String manipulation
["split", "hello world", " "]         // {"array": ["hello", "world"]}
["join", {"array": ["hello", "world"]}, " "]  // "hello world"
["trim", "  hello  "]                 // "hello"
["upper", "hello"]                    // "HELLO"
["lower", "HELLO"]                    // "hello"
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
    ["lambda", ["s"], [">", ["$", "/s/score"], 80]]
  ],
  ["lambda", ["s"], ["$", "/s/name"]]
]]
```
Result: `{"array": ["Alice", "Bob"]}`

### Building Objects from Arrays

```json
// Using the new objFromPairs operator (much simpler!)
["objFromPairs", 
  ["zip", {"array": ["a", "b", "c"]}, {"array": [1, 2, 3]}]
]
```
Result: `{"a": 1, "b": 2, "c": 3}`

### Data Cleaning Pipeline

```json
["let", [
  ["raw_data", ["$input", "/users", {"array": []}]],
  ["clean_names", ["map", ["$", "/raw_data"],
    ["lambda", ["user"], 
      ["trim", ["lower", ["$", "/user/name", "unknown"]]]
    ]
  ]]
], [
  "objFromPairs",
  ["zip", 
    ["$", "/clean_names"],
    ["unique", ["sort", ["$", "/clean_names"]]]
  ]
]]
```

### Robust Error Handling

```json
// Graceful handling of missing data with defaults
["obj",
  ["name", ["$input", "/user/name", "Anonymous"]],
  ["email", ["$input", "/user/email", "no-email@example.com"]],
  ["age", ["$input", "/user/age", 0]],
  ["roles", ["$input", "/user/roles", {"array": ["guest"]}]]
]
```

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
# Configure and build everything
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# This creates:
# - libcomputo.a (production library)
# - libcomputorepl.a (REPL library with debug features)
# - computo (production CLI)
# - computo_repl (REPL CLI with debugging)
# - test_computo (production tests)
# - test_computo_repl (REPL tests)

# Run tests directly
./build/test_computo        # Production tests
./build/test_computo_repl   # REPL and debugging tests

# Or use ctest for integrated test management
ctest                       # Run all tests
ctest --verbose             # Show detailed test output
ctest -R computo_tests      # Run only production tests
ctest -R computo_repl_tests # Run only REPL tests
ctest --output-on-failure   # Show output only for failed tests
ctest -j4                   # Run tests in parallel (up to 4 concurrent)
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