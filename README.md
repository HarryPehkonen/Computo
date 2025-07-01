# Computo

A safe, sandboxed JSON transformation engine with Lisp-like syntax expressed in JSON

Features RFC 6902 JSON Patch support for document diffing and patching, plus **Permuto** integration for advanced templating.

**📖 Human-readable documentation:** [Computo & Permuto Book](https://harrypehkonen.github.io/ComputoPermutoBook/) | [Repository](https://github.com/HarryPehkonen/ComputoPermutoBook)

*This README is optimized for AI assistants. For tutorials, examples, and learning materials, see the book above.*

## Architecture Overview

- **Computo**: Handles complex programmatic logic (conditionals, loops, calculations, diff/patch operations)
- **Permuto**: Handles simple declarative substitutions and templating using `${path}` syntax
- **JSON Patch**: RFC 6902 compliant diff generation and patch application
- **Code is Data**: All scripts are valid JSON with unambiguous syntax
- **Immutable**: Pure functions that don't modify input data
- **Sandboxed**: No I/O operations or system access
- **Enhanced Error Reporting**: Precise path tracking for debugging complex scripts

## Installation & Building

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- nlohmann/json library
- Permuto library (included as submodule)

### Building from Source
```bash
# Clone the repository
git clone https://github.com/your-org/computo.git
cd computo

# Initialize submodules
git submodule init
git submodule update

# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build library and CLI
cmake --build build

# Run tests (198 tests, 100% passing)
cd build && ctest --verbose
```

### Installation
```bash
# Install system-wide
sudo cmake --install build

# Or install to custom prefix
cmake --install build --prefix=/usr/local
```

## Library Usage

### C++ API

Include the header and use the execute function:

```cpp
#include <computo/computo.hpp>

// Single input (traditional)
nlohmann::json script = R"(["*", ["get", ["$input"], "/value"], 2])"_json;
nlohmann::json input = R"({"value": 21})"_json;
nlohmann::json result = computo::execute(script, input);
// Result: 42

// Multiple inputs (new)
std::vector<nlohmann::json> inputs = {
    R"({"data": [1, 2, 3]})"_json,
    R"({"multiplier": 10})"_json
};
nlohmann::json multi_script = R"(["map", ["get", ["$inputs"], "/0/data"], 
  ["lambda", ["x"], ["*", ["$", "/x"], ["get", ["$inputs"], "/1/multiplier"]]]])"_json;
nlohmann::json result = computo::execute(multi_script, inputs);
// Result: [10, 20, 30]
```

### Exception Handling
```cpp
try {
    nlohmann::json result = computo::execute(script, input);
    std::cout << result.dump(2) << std::endl;
} catch (const computo::ComputoException& e) {
    std::cerr << "Computo error: " << e.what() << std::endl;
}
```

### CMake Integration
```cmake
find_package(computo REQUIRED)
target_link_libraries(your_target computo::computo)
```

## Core Syntax

### Operator Calls vs Literal Data
- **Operator call**: `[operator, arg1, arg2, ...]`
- **Literal array**: `{"array": [item1, item2, ...]}`
- **Objects**: Standard JSON objects

### System Variables
- `["$input"]` - Access entire input data (first input if multiple)
- `["$inputs"]` - Access all input documents as array
- `["$", "/variable_name"]` - Access variable by path

### Unambiguous Syntax
The key insight of Computo is that **code is data** - all scripts are valid JSON with zero ambiguity between operator calls and literal data.

## CLI Usage

### Basic Usage
```bash
# Single input transformation
./build/computo script.json input.json

# Multiple input processing  
./build/computo script.json input1.json input2.json input3.json

# No input (script only)
./build/computo script.json
```

### Diff Mode
```bash
# Generate patch from transformation
./build/computo --diff transform_script.json original.json
```

### Permuto Integration
```bash
# Enable string interpolation
./build/computo --interpolation script.json input.json
```

### Output Formatting
```bash
# Pretty print with indentation
./build/computo --pretty=2 script.json input.json
```

### Comment Support (CLI Only)
```bash
# Allow comments in script files
./build/computo --comments script_with_comments.json input.json
```


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
# Basic transformation (single input)
./build/computo script.json input.json

# Multiple input files
./build/computo script.json input1.json input2.json input3.json

# With flags
./build/computo --interpolation script.json input.json
```

## Arithmetic Examples

### Basic Addition

Basic addition of two numbers.
Computo supports standard arithmetic operations on numbers.


**Script:**
```json
[
  "+",
  15,
  27
]
```

**Expected Output:**
```json
42
```

### Basic Subtraction

Basic subtraction operation.
Demonstrates numeric subtraction with integer operands.


**Script:**
```json
[
  "-",
  50,
  8
]
```

**Expected Output:**
```json
42
```

### Basic Multiplication

Basic multiplication of two numbers.
This is the simplest form of Computo operation.


**Script:**
```json
[
  "*",
  6,
  7
]
```

**Expected Output:**
```json
42
```

### Basic Division

Basic division operation.
Demonstrates numeric division with precise results.


**Script:**
```json
[
  "/",
  84,
  2
]
```

**Expected Output:**
```json
42
```

### Nested Arithmetic

Nested arithmetic operations showing operator composition.
Demonstrates how operators can be nested to create complex expressions.


**Script:**
```json
[
  "+",
  [
    "*",
    3,
    4
  ],
  [
    "*",
    5,
    6
  ]
]
```

**Expected Output:**
```json
42
```

### Basic Modulo

Basic modulo operation for remainder calculation.
Demonstrates integer remainder operation using the % operator.


**Script:**
```json
[
  "%",
  17,
  5
]
```

**Expected Output:**
```json
2
```

### Modulo Even Odd Check

Modulo operation for checking even/odd numbers.
Shows practical use of modulo for determining if a number is even.


**Script:**
```json
[
  "%",
  42,
  2
]
```

**Expected Output:**
```json
0
```

## Array Operations Examples

### Array Map Double

Array transformation using map to double values.
Demonstrates applying a function to each array element.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "*",
      [
        "$",
        "/x"
      ],
      2
    ]
  ]
]
```

**Expected Output:**
```json
[
  2,
  4,
  6,
  8
]
```

### Array Map From Input

Map operation on array from input data.
Shows transforming arrays that come from input documents.


**Script:**
```json
[
  "map",
  [
    "get",
    [
      "$input"
    ],
    "/numbers"
  ],
  [
    "lambda",
    [
      "n"
    ],
    [
      "+",
      [
        "$",
        "/n"
      ],
      10
    ]
  ]
]
```

**Input:**
```json
{
  "numbers": [
    1,
    2,
    3
  ]
}
```

**Expected Output:**
```json
[
  11,
  12,
  13
]
```

### Array Filter Greater Than

Array filtering based on numeric condition.
Shows keeping only elements that satisfy a predicate.


**Script:**
```json
[
  "filter",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      3
    ]
  ]
]
```

**Expected Output:**
```json
[
  4,
  5,
  6
]
```

### Array Filter Greater Than Two

Filter array to keep numbers greater than 2.
Demonstrates filtering arrays based on numeric conditions.


**Script:**
```json
[
  "filter",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      2
    ]
  ]
]
```

**Expected Output:**
```json
[
  3,
  4,
  5,
  6,
  7,
  8
]
```

### Array Reduce Sum

Array reduction to sum all elements.
Demonstrates aggregating an array to a single value.


**Script:**
```json
[
  "reduce",
  {
    "array": [
      1,
      2,
      3,
      4,
      5
    ]
  },
  [
    "lambda",
    [
      "acc",
      "x"
    ],
    [
      "+",
      [
        "$",
        "/acc"
      ],
      [
        "$",
        "/x"
      ]
    ]
  ],
  0
]
```

**Expected Output:**
```json
15
```

### Array Reduce Product

Array reduction to multiply all elements.
Shows calculating the product using reduce with different initial value.


**Script:**
```json
[
  "reduce",
  {
    "array": [
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "acc",
      "x"
    ],
    [
      "*",
      [
        "$",
        "/acc"
      ],
      [
        "$",
        "/x"
      ]
    ]
  ],
  1
]
```

**Expected Output:**
```json
24
```

### Array Find First Match

Find first element matching condition.
Shows locating the first element that satisfies a predicate.


**Script:**
```json
[
  "find",
  {
    "array": [
      1,
      2,
      5,
      8,
      10
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      4
    ]
  ]
]
```

**Expected Output:**
```json
5
```

### Array Some Has Match

Test if some elements match condition.
Demonstrates checking if any element satisfies a condition.


**Script:**
```json
[
  "some",
  {
    "array": [
      1,
      2,
      3,
      8
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      5
    ]
  ]
]
```

**Expected Output:**
```json
true
```

### Array Some No Match

Test if some elements match condition - no matches.
Shows some operator returning false when no elements match.


**Script:**
```json
[
  "some",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      10
    ]
  ]
]
```

**Expected Output:**
```json
false
```

### Array Every All Match

Test if all elements match condition - all match.
Shows every operator returning true when all elements satisfy condition.


**Script:**
```json
[
  "every",
  {
    "array": [
      4,
      6,
      8,
      10
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      3
    ]
  ]
]
```

**Expected Output:**
```json
true
```

### Array Every Not All

Test if all elements match condition - not all match.
Demonstrates every operator returning false when some elements don't match.


**Script:**
```json
[
  "every",
  {
    "array": [
      2,
      4,
      6,
      8
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      3
    ]
  ]
]
```

**Expected Output:**
```json
false
```

### Array Flatmap Expand

Map and flatten array results.
Demonstrates mapping over an array and flattening nested results.


**Script:**
```json
[
  "flatMap",
  {
    "array": [
      1,
      2,
      3
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    {
      "array": [
        [
          "$",
          "/x"
        ],
        [
          "*",
          [
            "$",
            "/x"
          ],
          2
        ]
      ]
    }
  ]
]
```

**Expected Output:**
```json
[
  1,
  2,
  2,
  4,
  3,
  6
]
```

### Array Partition By Size

Partition array into large and small numbers.
Shows splitting an array based on a predicate into [matching, non-matching].


**Script:**
```json
[
  "partition",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      ">",
      [
        "$",
        "/x"
      ],
      3
    ]
  ]
]
```

**Expected Output:**
```json
[
  [
    4,
    5,
    6
  ],
  [
    1,
    2,
    3
  ]
]
```

### Array Count Length

Get array length using count.
Demonstrates measuring the size of arrays.


**Script:**
```json
[
  "count",
  {
    "array": [
      "apple",
      "banana",
      "cherry",
      "date"
    ]
  }
]
```

**Expected Output:**
```json
4
```

### Array Zip Combine Pairs

Zip two arrays into element pairs.
Demonstrates combining corresponding elements from two arrays into pairs.


**Script:**
```json
[
  "zip",
  {
    "array": [
      "a",
      "b",
      "c"
    ]
  },
  {
    "array": [
      1,
      2,
      3
    ]
  }
]
```

**Expected Output:**
```json
[
  [
    "a",
    1
  ],
  [
    "b",
    2
  ],
  [
    "c",
    3
  ]
]
```

### Array Zip Different Lengths

Zip arrays of different lengths.
Shows that zip stops at the shorter array's length.


**Script:**
```json
[
  "zip",
  {
    "array": [
      1,
      2,
      3,
      4,
      5
    ]
  },
  {
    "array": [
      "x",
      "y"
    ]
  }
]
```

**Expected Output:**
```json
[
  [
    1,
    "x"
  ],
  [
    2,
    "y"
  ]
]
```

### Array Zipwith Custom Combination

Zip two arrays with custom combination function.
Demonstrates combining arrays element-wise using a lambda function.


**Script:**
```json
[
  "zipWith",
  {
    "array": [
      1,
      2,
      3
    ]
  },
  {
    "array": [
      10,
      20,
      30
    ]
  },
  [
    "lambda",
    [
      "a",
      "b"
    ],
    [
      "+",
      [
        "$",
        "/a"
      ],
      [
        "$",
        "/b"
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  11,
  22,
  33
]
```

### Array Zipwith String Formatting

ZipWith for string formatting and concatenation.
Shows practical use of zipWith for combining data from parallel arrays.


**Script:**
```json
[
  "zipWith",
  {
    "array": [
      "Alice",
      "Bob",
      "Charlie"
    ]
  },
  {
    "array": [
      25,
      30,
      35
    ]
  },
  [
    "lambda",
    [
      "name",
      "age"
    ],
    [
      "str_concat",
      [
        "$",
        "/name"
      ],
      " is ",
      [
        "$",
        "/age"
      ],
      " years old"
    ]
  ]
]
```

**Expected Output:**
```json
[
  "Alice is 25 years old",
  "Bob is 30 years old",
  "Charlie is 35 years old"
]
```

### Array Mapwithindex Element Position

Map over array with element indices.
Demonstrates accessing both element value and position during transformation.


**Script:**
```json
[
  "mapWithIndex",
  {
    "array": [
      "apple",
      "banana",
      "cherry"
    ]
  },
  [
    "lambda",
    [
      "item",
      "index"
    ],
    [
      "obj",
      [
        "position",
        [
          "$",
          "/index"
        ]
      ],
      [
        "fruit",
        [
          "$",
          "/item"
        ]
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  {
    "position": 0,
    "fruit": "apple"
  },
  {
    "position": 1,
    "fruit": "banana"
  },
  {
    "position": 2,
    "fruit": "cherry"
  }
]
```

### Array Mapwithindex Conditional Processing

Map with index for conditional processing based on position.
Shows using index information to apply different logic to even/odd positions.


**Script:**
```json
[
  "mapWithIndex",
  {
    "array": [
      10,
      20,
      30,
      40
    ]
  },
  [
    "lambda",
    [
      "value",
      "index"
    ],
    [
      "if",
      [
        "==",
        [
          "%",
          [
            "$",
            "/index"
          ],
          2
        ],
        0
      ],
      [
        "*",
        [
          "$",
          "/value"
        ],
        2
      ],
      [
        "$",
        "/value"
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  20,
  20,
  60,
  40
]
```

### Array Enumerate Index Value Pairs

Convert array to [index, value] pairs.
Demonstrates creating enumerated pairs for processing with indices.


**Script:**
```json
[
  "enumerate",
  {
    "array": [
      "red",
      "green",
      "blue"
    ]
  }
]
```

**Expected Output:**
```json
[
  [
    0,
    "red"
  ],
  [
    1,
    "green"
  ],
  [
    2,
    "blue"
  ]
]
```

### Array Enumerate For Processing

Use enumerate for indexed processing.
Shows practical use of enumerate to create numbered lists or process with position awareness.


**Script:**
```json
[
  "map",
  [
    "enumerate",
    {
      "array": [
        "Task A",
        "Task B",
        "Task C"
      ]
    }
  ],
  [
    "lambda",
    [
      "pair"
    ],
    [
      "str_concat",
      [
        "get",
        [
          "$",
          "/pair"
        ],
        "/0"
      ],
      ". ",
      [
        "get",
        [
          "$",
          "/pair"
        ],
        "/1"
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  "0. Task A",
  "1. Task B",
  "2. Task C"
]
```

## Cli Usage Examples

### Cli Basic Transformation

Basic CLI usage for single input transformation.
This demonstrates the standard command-line interface pattern.
Note: This is a documentation example showing CLI usage patterns.


**Script:**
```json
[
  "obj",
  [
    "greeting",
    [
      "get",
      [
        "$input"
      ],
      "/message"
    ]
  ],
  [
    "timestamp",
    "2024-01-01"
  ]
]
```

**Input:**
```json
{
  "message": "Hello from CLI",
  "user": "Alice"
}
```

**Expected Output:**
```json
{
  "greeting": "Hello from CLI",
  "timestamp": "2024-01-01"
}
```

### Cli Multiple Inputs Merge

CLI usage with multiple input files for data merging.
Shows processing multiple input files from the command line.
Note: This is a documentation example showing multi-input CLI patterns.


**Script:**
```json
[
  "obj",
  [
    "combined_data",
    [
      "append",
      [
        "get",
        [
          "$inputs"
        ],
        "/0/items"
      ],
      [
        "get",
        [
          "$inputs"
        ],
        "/1/items"
      ]
    ]
  ],
  [
    "sources",
    [
      "count",
      [
        "$inputs"
      ]
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "items": [
    "from_file_1",
    "data_1"
  ]
}
```

*Input 1:*
```json
{
  "items": [
    "from_file_2",
    "data_2"
  ]
}
```
**Expected Output:**
```json
{
  "combined_data": [
    "from_file_1",
    "data_1",
    "from_file_2",
    "data_2"
  ],
  "sources": 2
}
```

### Cli Permuto Interpolation

CLI with Permuto interpolation for template processing.
Shows using --interpolation flag for string template processing.
Note: This is a documentation example showing Permuto integration.


**Script:**
```json
[
  "permuto.apply",
  {
    "welcome": "Welcome ${/user/name}!",
    "summary": "You have ${/user/points} points."
  },
  [
    "$input"
  ]
]
```

**Input:**
```json
{
  "user": {
    "name": "Bob",
    "points": 250
  }
}
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "welcome": "Welcome Bob!",
  "summary": "You have 250 points."
}
```

### Cli Pretty Output Formatting

CLI with pretty-printed output formatting.
Demonstrates using --pretty flag for readable JSON output.
Note: This is a documentation example showing output formatting.


**Script:**
```json
[
  "obj",
  [
    "data",
    [
      "map",
      {
        "array": [
          1,
          2,
          3
        ]
      },
      [
        "lambda",
        [
          "x"
        ],
        [
          "obj",
          [
            "value",
            [
              "$",
              "/x"
            ]
          ],
          [
            "squared",
            [
              "*",
              [
                "$",
                "/x"
              ],
              [
                "$",
                "/x"
              ]
            ]
          ]
        ]
      ]
    ]
  ],
  [
    "timestamp",
    "2024-01-01"
  ]
]
```

**Expected Output:**
```json
{
  "data": [
    {
      "value": 1,
      "squared": 1
    },
    {
      "value": 2,
      "squared": 4
    },
    {
      "value": 3,
      "squared": 9
    }
  ],
  "timestamp": "2024-01-01"
}
```

### Cli Comments In Scripts

CLI with comment support in script files.
Shows using --comments flag to allow comments in JSON scripts.
Note: This is a documentation example showing comment parsing.


**Script:**
```json
[
  "obj",
  [
    "message",
    "Hello World"
  ],
  [
    "computed",
    [
      "*",
      6,
      7
    ]
  ]
]
```

**Expected Output:**
```json
{
  "message": "Hello World",
  "computed": 42
}
```

## Comparison Examples

### Greater Than True

Greater than comparison returning true.
Demonstrates numeric comparison operations.


**Script:**
```json
[
  ">",
  10,
  5
]
```

**Expected Output:**
```json
true
```

### Greater Than False

Greater than comparison returning false.
Shows how comparison operators work with different outcomes.


**Script:**
```json
[
  ">",
  3,
  8
]
```

**Expected Output:**
```json
false
```

### Less Than True

Less than comparison returning true.
Demonstrates the less than operator.


**Script:**
```json
[
  "<",
  5,
  10
]
```

**Expected Output:**
```json
true
```

### Greater Equal True

Greater than or equal comparison with equality.
Shows boundary condition handling.


**Script:**
```json
[
  ">=",
  5,
  5
]
```

**Expected Output:**
```json
true
```

### Less Equal False

Less than or equal comparison returning false.
Demonstrates boundary condition failure.


**Script:**
```json
[
  "<=",
  10,
  5
]
```

**Expected Output:**
```json
false
```

### Equality Strings True

String equality comparison returning true.
Demonstrates exact string matching.


**Script:**
```json
[
  "==",
  "hello",
  "hello"
]
```

**Expected Output:**
```json
true
```

### Equality Strings False

String equality comparison returning false.
Shows string inequality detection.


**Script:**
```json
[
  "==",
  "hello",
  "world"
]
```

**Expected Output:**
```json
false
```

### Not Equal Numbers

Not equal comparison with numbers.
Demonstrates inequality operator with numeric values.


**Script:**
```json
[
  "!=",
  1,
  2
]
```

**Expected Output:**
```json
true
```

### Approximate Equality

Approximate equality for floating point numbers.
Uses epsilon tolerance for float comparison to handle precision issues.


**Script:**
```json
[
  "approx",
  0.1,
  0.10000001,
  0.001
]
```

**Expected Output:**
```json
true
```

## Conditional Examples

### If Condition True

Conditional execution with true condition.
Demonstrates basic if-then-else control flow executing the then branch.


**Script:**
```json
[
  "if",
  [
    ">",
    10,
    5
  ],
  "condition_was_true",
  "condition_was_false"
]
```

**Expected Output:**
```json
"condition_was_true"
```

### If Condition False

Conditional execution with false condition.
Shows else branch execution when condition is false.


**Script:**
```json
[
  "if",
  [
    "<",
    10,
    5
  ],
  "condition_was_true",
  "condition_was_false"
]
```

**Expected Output:**
```json
"condition_was_false"
```

### If Nested Conditions

Nested conditional expressions.
Demonstrates how if statements can be nested for complex decision logic.


**Script:**
```json
[
  "if",
  [
    ">",
    15,
    10
  ],
  [
    "if",
    [
      "<",
      15,
      20
    ],
    "both_conditions_true",
    "only_first_true"
  ],
  "first_condition_false"
]
```

**Expected Output:**
```json
"both_conditions_true"
```

### If With Complex Condition

Conditional with complex logical expression.
Shows combining logical operators in if conditions.


**Script:**
```json
[
  "if",
  [
    "&&",
    [
      ">",
      20,
      10
    ],
    [
      "<",
      20,
      30
    ]
  ],
  "in_range",
  "out_of_range"
]
```

**Expected Output:**
```json
"in_range"
```

## Cpp Builder Examples

### Builder Basic Vs Manual

ComputoBuilder vs manual JSON construction comparison.
Shows how the builder pattern eliminates verbose JSON syntax in C++ tests.
Note: This is C++ API documentation - builder creates the JSON shown in 'script'.


**Script:**
```json
[
  "+",
  15,
  27
]
```

**Expected Output:**
```json
42
```

### Builder Array Construction

Builder pattern for array construction.
Demonstrates how builder eliminates the painful array wrapper syntax.
Note: CB::array creates the JSON array structure shown in 'script'.


**Script:**
```json
{
  "array": [
    1,
    2,
    3,
    4,
    5
  ]
}
```

**Expected Output:**
```json
[
  1,
  2,
  3,
  4,
  5
]
```

### Builder Complex Nesting

Builder pattern for complex nested operations.
Shows how builder makes deeply nested expressions readable and maintainable.
Note: This C++ builder code produces the JSON script shown.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "+",
      [
        "$",
        "/x"
      ],
      10
    ]
  ]
]
```

**Expected Output:**
```json
[
  11,
  12,
  13,
  14
]
```

### Builder Object Construction

Fluent object construction with builder pattern.
Demonstrates chaining methods for building complex objects.
Note: Shows C++ fluent API that generates the JSON object structure.


**Script:**
```json
[
  "obj",
  [
    "name",
    "Alice"
  ],
  [
    "age",
    30
  ],
  [
    "score",
    [
      "+",
      85,
      15
    ]
  ]
]
```

**Expected Output:**
```json
{
  "name": "Alice",
  "age": 30,
  "score": 100
}
```

### Builder Variables And Let

Variable binding with builder pattern.
Shows clean syntax for let expressions and variable references.
Note: Builder provides type-safe variable handling in C++.


**Script:**
```json
[
  "let",
  [
    [
      "multiplier",
      3
    ],
    [
      "base",
      14
    ]
  ],
  [
    "+",
    [
      "*",
      [
        "$",
        "/base"
      ],
      [
        "$",
        "/multiplier"
      ]
    ],
    [
      "$",
      "/base"
    ]
  ]
]
```

**Expected Output:**
```json
56
```

### Builder Conditional Logic

Conditional expressions with builder pattern.
Demonstrates readable if-then-else construction in C++.
Note: Builder methods provide compile-time safety for conditional logic.


**Script:**
```json
[
  "if",
  [
    ">",
    [
      "get",
      [
        "$input"
      ],
      "/value"
    ],
    50
  ],
  "high",
  "low"
]
```

**Input:**
```json
{
  "value": 75
}
```

**Expected Output:**
```json
"high"
```

### Builder Data Access Patterns

Data access patterns with builder.
Shows input access, JSON pointer usage, and data extraction.
Note: Type-safe data access with compile-time path validation.


**Script:**
```json
[
  "obj",
  [
    "user",
    [
      "get",
      [
        "$input"
      ],
      "/user/name"
    ]
  ],
  [
    "processed",
    true
  ]
]
```

**Input:**
```json
{
  "user": {
    "name": "Bob",
    "email": "bob@test.com"
  }
}
```

**Expected Output:**
```json
{
  "user": "Bob",
  "processed": true
}
```

### Builder Functional Operations

Functional array operations with builder.
Demonstrates map, filter, and reduce operations with readable syntax.
Note: Builder makes functional programming patterns accessible in C++.


**Script:**
```json
[
  "map",
  [
    "filter",
    {
      "array": [
        1,
        2,
        3,
        4,
        5,
        6
      ]
    },
    [
      "lambda",
      [
        "x"
      ],
      [
        ">",
        [
          "$",
          "/x"
        ],
        3
      ]
    ]
  ],
  [
    "lambda",
    [
      "x"
    ],
    [
      "*",
      [
        "$",
        "/x"
      ],
      2
    ]
  ]
]
```

**Expected Output:**
```json
[
  8,
  10,
  12
]
```

### Builder Generic Operators

Generic operator construction with builder.
Shows fallback syntax for any operator using the generic builder pattern.
Note: Provides escape hatch for operators without dedicated builder methods.


**Script:**
```json
[
  "reduce",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "acc",
      "x"
    ],
    [
      "+",
      [
        "$",
        "/acc"
      ],
      [
        "$",
        "/x"
      ]
    ]
  ],
  0
]
```

**Expected Output:**
```json
10
```

### Builder Type Safety Benefits

Type safety and IDE benefits of builder pattern.
Demonstrates compile-time error checking and development experience improvements.
Note: Builder prevents common JSON construction errors at compile time.


**Script:**
```json
[
  "*",
  6,
  7
]
```

**Expected Output:**
```json
42
```

## Data Access Examples

### Input Access Whole

Access entire input document.
Demonstrates using $input to access the complete input data.


**Script:**
```json
[
  "$input"
]
```

**Input:**
```json
{
  "message": "Hello World",
  "number": 42
}
```

**Expected Output:**
```json
{
  "message": "Hello World",
  "number": 42
}
```

### Json Pointer Simple

Simple JSON Pointer data extraction.
Shows extracting a specific field using JSON Pointer syntax.


**Script:**
```json
[
  "get",
  [
    "$input"
  ],
  "/message"
]
```

**Input:**
```json
{
  "message": "Hello Computo",
  "status": "active"
}
```

**Expected Output:**
```json
"Hello Computo"
```

### Json Pointer Nested

Nested JSON Pointer access.
Demonstrates accessing deeply nested data structures.


**Script:**
```json
[
  "get",
  [
    "$input"
  ],
  "/user/profile/name"
]
```

**Input:**
```json
{
  "user": {
    "profile": {
      "name": "Alice",
      "age": 30
    },
    "id": 123
  }
}
```

**Expected Output:**
```json
"Alice"
```

### Json Pointer Array Index

JSON Pointer array element access.
Shows accessing specific array elements by index.


**Script:**
```json
[
  "get",
  [
    "$input"
  ],
  "/items/1"
]
```

**Input:**
```json
{
  "items": [
    "first",
    "second",
    "third"
  ]
}
```

**Expected Output:**
```json
"second"
```

### Variable Binding Simple

Simple variable binding with let.
Demonstrates creating local variables for reuse in expressions.


**Script:**
```json
[
  "let",
  [
    [
      "x",
      25
    ],
    [
      "y",
      17
    ]
  ],
  [
    "+",
    [
      "$",
      "/x"
    ],
    [
      "$",
      "/y"
    ]
  ]
]
```

**Expected Output:**
```json
42
```

### Variable Binding From Input

Variable binding using input data.
Shows extracting input values into variables for cleaner expressions.


**Script:**
```json
[
  "let",
  [
    [
      "user_name",
      [
        "get",
        [
          "$input"
        ],
        "/user/name"
      ]
    ],
    [
      "user_age",
      [
        "get",
        [
          "$input"
        ],
        "/user/age"
      ]
    ]
  ],
  [
    "obj",
    [
      "greeting",
      [
        "$",
        "/user_name"
      ]
    ],
    [
      "next_age",
      [
        "+",
        [
          "$",
          "/user_age"
        ],
        1
      ]
    ]
  ]
]
```

**Input:**
```json
{
  "user": {
    "name": "Bob",
    "age": 25
  }
}
```

**Expected Output:**
```json
{
  "greeting": "Bob",
  "next_age": 26
}
```

### Variable Binding Nested

Nested variable bindings.
Demonstrates creating multiple variable scopes with let expressions.


**Script:**
```json
[
  "let",
  [
    [
      "outer",
      10
    ]
  ],
  [
    "let",
    [
      [
        "inner",
        32
      ]
    ],
    [
      "+",
      [
        "$",
        "/outer"
      ],
      [
        "$",
        "/inner"
      ]
    ]
  ]
]
```

**Expected Output:**
```json
42
```

## Data Construction Examples

### Object Construction Simple

Simple object construction using obj operator.
Shows creating JSON objects with literal values.


**Script:**
```json
[
  "obj",
  [
    "name",
    "Alice"
  ],
  [
    "age",
    30
  ],
  [
    "active",
    true
  ]
]
```

**Expected Output:**
```json
{
  "name": "Alice",
  "age": 30,
  "active": true
}
```

### Object Construction Computed

Object construction with computed values.
Demonstrates creating objects with calculated fields.


**Script:**
```json
[
  "obj",
  [
    "name",
    "Bob"
  ],
  [
    "birth_year",
    [
      "-",
      2024,
      25
    ]
  ],
  [
    "score",
    [
      "*",
      6,
      7
    ]
  ]
]
```

**Expected Output:**
```json
{
  "name": "Bob",
  "birth_year": 1999,
  "score": 42
}
```

### Array Construction Literal

Array construction using literal syntax.
Shows creating arrays with the {"array": [...]} syntax.


**Script:**
```json
{
  "array": [
    1,
    2,
    3,
    "hello",
    true
  ]
}
```

**Expected Output:**
```json
[
  1,
  2,
  3,
  "hello",
  true
]
```

### Array Construction Mixed

Array construction with mixed literal and computed values.
Demonstrates arrays containing both static and calculated elements.


**Script:**
```json
{
  "array": [
    1,
    [
      "*",
      2,
      3
    ],
    [
      "+",
      4,
      3
    ],
    "mixed"
  ]
}
```

**Expected Output:**
```json
[
  1,
  6,
  7,
  "mixed"
]
```

### Nested Object Construction

Nested object and array construction.
Shows creating complex nested data structures.


**Script:**
```json
[
  "obj",
  [
    "user",
    [
      "obj",
      [
        "name",
        "Charlie"
      ],
      [
        "hobbies",
        {
          "array": [
            "reading",
            "coding"
          ]
        }
      ]
    ]
  ],
  [
    "metadata",
    [
      "obj",
      [
        "created",
        "2024-01-01"
      ],
      [
        "version",
        1
      ]
    ]
  ]
]
```

**Expected Output:**
```json
{
  "user": {
    "name": "Charlie",
    "hobbies": [
      "reading",
      "coding"
    ]
  },
  "metadata": {
    "created": "2024-01-01",
    "version": 1
  }
}
```

### Object From Input Data

Dynamic object construction from input data.
Demonstrates building objects using values extracted from input.


**Script:**
```json
[
  "obj",
  [
    "full_name",
    [
      "get",
      [
        "$input"
      ],
      "/first_name"
    ]
  ],
  [
    "age_category",
    [
      "if",
      [
        ">",
        [
          "get",
          [
            "$input"
          ],
          "/age"
        ],
        18
      ],
      "adult",
      "minor"
    ]
  ]
]
```

**Input:**
```json
{
  "first_name": "Diana",
  "age": 25
}
```

**Expected Output:**
```json
{
  "full_name": "Diana",
  "age_category": "adult"
}
```

## Debugging Examples

### Debug Basic Tracing

Basic debugging with execution tracing enabled.
Shows how to enable execution tracing to see operation flow.
Note: This demonstrates CLI debugging flags - actual trace output goes to stderr.


**Script:**
```json
[
  "obj",
  [
    "result",
    [
      "+",
      [
        "*",
        3,
        4
      ],
      [
        "*",
        5,
        6
      ]
    ]
  ],
  [
    "computed",
    true
  ]
]
```

**Expected Output:**
```json
{
  "result": 42,
  "computed": true
}
```

### Debug Performance Profiling

Performance profiling to identify slow operations.
Demonstrates using profiling to measure execution times and identify bottlenecks.
Note: Performance reports are sent to stderr, JSON result to stdout.


**Script:**
```json
[
  "reduce",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10
    ]
  },
  [
    "lambda",
    [
      "acc",
      "x"
    ],
    [
      "+",
      [
        "$",
        "/acc"
      ],
      [
        "*",
        [
          "$",
          "/x"
        ],
        [
          "$",
          "/x"
        ]
      ]
    ]
  ],
  0
]
```

**Expected Output:**
```json
385
```

### Debug Slow Operation Detection

Detect and report operations slower than threshold.
Shows using slow operation detection to find performance issues.
Note: Operations slower than 5ms will be flagged in the debug output.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      2,
      3,
      4,
      5
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "reduce",
      {
        "array": [
          1,
          2,
          3,
          4,
          5
        ]
      },
      [
        "lambda",
        [
          "a",
          "b"
        ],
        [
          "+",
          [
            "$",
            "/a"
          ],
          [
            "*",
            [
              "$",
              "/b"
            ],
            [
              "$",
              "/x"
            ]
          ]
        ]
      ],
      0
    ]
  ]
]
```

**Expected Output:**
```json
[
  15,
  30,
  45,
  60,
  75
]
```

### Debug Operator Breakpoints

Set breakpoints on specific operators for debugging.
Demonstrates halting execution when specific operators are encountered.
Note: In non-interactive mode, breakpoints log to stderr without stopping.


**Script:**
```json
[
  "let",
  [
    [
      "data",
      {
        "array": [
          1,
          2,
          3,
          4,
          5
        ]
      }
    ]
  ],
  [
    "map",
    [
      "$",
      "/data"
    ],
    [
      "lambda",
      [
        "x"
      ],
      [
        "*",
        [
          "$",
          "/x"
        ],
        2
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  2,
  4,
  6,
  8,
  10
]
```

### Debug Variable Watching

Watch variable changes during execution.
Shows tracking specific variables and their value changes throughout execution.
Note: Variable watch output appears in the execution trace on stderr.


**Script:**
```json
[
  "let",
  [
    [
      "multiplier",
      3
    ],
    [
      "base",
      14
    ]
  ],
  [
    "+",
    [
      "*",
      [
        "$",
        "/base"
      ],
      [
        "$",
        "/multiplier"
      ]
    ],
    [
      "$",
      "/base"
    ]
  ]
]
```

**Expected Output:**
```json
56
```

### Debug Comprehensive Analysis

Comprehensive debugging with all features enabled.
Demonstrates using multiple debugging features together for complete analysis.
Note: Combines tracing, profiling, breakpoints, and variable watching.


**Script:**
```json
[
  "let",
  [
    [
      "numbers",
      {
        "array": [
          10,
          20,
          30
        ]
      }
    ]
  ],
  [
    "reduce",
    [
      "$",
      "/numbers"
    ],
    [
      "lambda",
      [
        "sum",
        "num"
      ],
      [
        "+",
        [
          "$",
          "/sum"
        ],
        [
          "*",
          [
            "$",
            "/num"
          ],
          2
        ]
      ]
    ],
    0
  ]
]
```

**Expected Output:**
```json
120
```

### Debug Error Diagnosis

Enhanced error reporting with debugging enabled.
Shows how debugging provides detailed error context and execution history.
Note: This example would normally cause an error, shown here for documentation.


**Script:**
```json
[
  "obj",
  [
    "safe_result",
    [
      "+",
      20,
      22
    ]
  ],
  [
    "info",
    "This part works fine"
  ]
]
```

**Expected Output:**
```json
{
  "safe_result": 42,
  "info": "This part works fine"
}
```

### Debug Log Levels

Different debug log levels for varying detail amounts.
Demonstrates controlling the verbosity of debug output.
Note: Higher levels (verbose) provide more detailed execution information.


**Script:**
```json
[
  "map",
  {
    "array": [
      "hello",
      "world",
      "debug"
    ]
  },
  [
    "lambda",
    [
      "str"
    ],
    [
      "str_concat",
      [
        "$",
        "/str"
      ],
      "!"
    ]
  ]
]
```

**Expected Output:**
```json
[
  "hello!",
  "world!",
  "debug!"
]
```

### Debug Complex Operations

Debugging complex nested operations and data transformations.
Shows debugging sophisticated scripts with multiple operators and deep nesting.
Note: Trace output shows the complete execution flow through nested operations.


**Script:**
```json
[
  "let",
  [
    [
      "users",
      {
        "array": [
          {
            "name": "Alice",
            "score": 85
          },
          {
            "name": "Bob",
            "score": 92
          },
          {
            "name": "Charlie",
            "score": 78
          }
        ]
      }
    ]
  ],
  [
    "obj",
    [
      "top_scorer",
      [
        "car",
        [
          "filter",
          [
            "$",
            "/users"
          ],
          [
            "lambda",
            [
              "user"
            ],
            [
              ">",
              [
                "get",
                [
                  "$",
                  "/user"
                ],
                "/score"
              ],
              90
            ]
          ]
        ]
      ]
    ],
    [
      "average_score",
      [
        "/",
        [
          "reduce",
          [
            "map",
            [
              "$",
              "/users"
            ],
            [
              "lambda",
              [
                "user"
              ],
              [
                "get",
                [
                  "$",
                  "/user"
                ],
                "/score"
              ]
            ]
          ],
          [
            "lambda",
            [
              "sum",
              "score"
            ],
            [
              "+",
              [
                "$",
                "/sum"
              ],
              [
                "$",
                "/score"
              ]
            ]
          ],
          0
        ],
        [
          "count",
          [
            "$",
            "/users"
          ]
        ]
      ]
    ]
  ]
]
```

**Expected Output:**
```json
{
  "top_scorer": {
    "name": "Bob",
    "score": 92
  },
  "average_score": 85
}
```

### Debug Interactive Session

Interactive debugging for step-through execution.
Demonstrates interactive debugging mode for manual script inspection.
Note: Interactive mode allows step-by-step execution with variable inspection.


**Script:**
```json
[
  "let",
  [
    [
      "step1",
      15
    ],
    [
      "step2",
      27
    ]
  ],
  [
    "+",
    [
      "$",
      "/step1"
    ],
    [
      "$",
      "/step2"
    ]
  ]
]
```

**Expected Output:**
```json
42
```

## Functional Lists Examples

### List Car First Element

Get first element using car.
Demonstrates the car operation from functional programming.


**Script:**
```json
[
  "car",
  {
    "array": [
      "first",
      "second",
      "third"
    ]
  }
]
```

**Expected Output:**
```json
"first"
```

### List Car Single Element

Car operation on single-element array.
Shows car behavior with minimal arrays.


**Script:**
```json
[
  "car",
  {
    "array": [
      42
    ]
  }
]
```

**Expected Output:**
```json
42
```

### List Cdr Rest Elements

Get all but first element using cdr.
Shows the cdr operation for accessing the tail of a list.


**Script:**
```json
[
  "cdr",
  {
    "array": [
      1,
      2,
      3,
      4,
      5
    ]
  }
]
```

**Expected Output:**
```json
[
  2,
  3,
  4,
  5
]
```

### List Cdr Single Element

Cdr operation on single-element array.
Shows cdr returning empty array for single-element lists.


**Script:**
```json
[
  "cdr",
  {
    "array": [
      "only"
    ]
  }
]
```

**Expected Output:**
```json
[]
```

### List Cons Prepend

Prepend element using cons.
Demonstrates list construction by adding element to front.


**Script:**
```json
[
  "cons",
  "new_first",
  {
    "array": [
      "second",
      "third"
    ]
  }
]
```

**Expected Output:**
```json
[
  "new_first",
  "second",
  "third"
]
```

### List Cons Empty Array

Cons operation with empty array.
Shows creating single-element array using cons.


**Script:**
```json
[
  "cons",
  "only_element",
  {
    "array": []
  }
]
```

**Expected Output:**
```json
[
  "only_element"
]
```

### List Append Two Arrays

Concatenate two arrays using append.
Shows basic array concatenation functionality.


**Script:**
```json
[
  "append",
  {
    "array": [
      1,
      2
    ]
  },
  {
    "array": [
      3,
      4
    ]
  }
]
```

**Expected Output:**
```json
[
  1,
  2,
  3,
  4
]
```

### List Append Multiple Arrays

Concatenate multiple arrays using append.
Demonstrates joining several arrays together.


**Script:**
```json
[
  "append",
  {
    "array": [
      "a"
    ]
  },
  {
    "array": [
      "b",
      "c"
    ]
  },
  {
    "array": [
      "d",
      "e",
      "f"
    ]
  }
]
```

**Expected Output:**
```json
[
  "a",
  "b",
  "c",
  "d",
  "e",
  "f"
]
```

### List Chunk Even Split

Split array into even chunks.
Shows breaking an array into equal-sized smaller arrays.


**Script:**
```json
[
  "chunk",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6
    ]
  },
  2
]
```

**Expected Output:**
```json
[
  [
    1,
    2
  ],
  [
    3,
    4
  ],
  [
    5,
    6
  ]
]
```

### List Chunk Uneven Split

Split array into chunks with remainder.
Demonstrates chunking when array size isn't divisible by chunk size.


**Script:**
```json
[
  "chunk",
  {
    "array": [
      1,
      2,
      3,
      4,
      5,
      6,
      7
    ]
  },
  3
]
```

**Expected Output:**
```json
[
  [
    1,
    2,
    3
  ],
  [
    4,
    5,
    6
  ],
  [
    7
  ]
]
```

### List Car Cdr Composition

Compose car and cdr to get second element.
Shows how functional operations can be composed for complex access.


**Script:**
```json
[
  "car",
  [
    "cdr",
    {
      "array": [
        "first",
        "second",
        "third",
        "fourth"
      ]
    }
  ]
]
```

**Expected Output:**
```json
"second"
```

### List Functional Pipeline

Complex functional composition pipeline.
Demonstrates chaining multiple functional operations together.


**Script:**
```json
[
  "cons",
  "new_head",
  [
    "cdr",
    [
      "cdr",
      {
        "array": [
          "remove1",
          "remove2",
          "keep1",
          "keep2"
        ]
      }
    ]
  ]
]
```

**Expected Output:**
```json
[
  "new_head",
  "keep1",
  "keep2"
]
```

## Json Patch Examples

### Json Patch Diff Replace

Generate JSON Patch diff with replace operation.
Demonstrates creating RFC 6902 compliant patches for field changes.


**Script:**
```json
[
  "diff",
  {
    "name": "Alice",
    "status": "active"
  },
  {
    "name": "Alice",
    "status": "inactive"
  }
]
```

**Expected Output:**
```json
[
  {
    "op": "replace",
    "path": "/status",
    "value": "inactive"
  }
]
```

### Json Patch Diff Add Field

Generate JSON Patch diff with add operation.
Shows patch generation when new fields are added.


**Script:**
```json
[
  "diff",
  {
    "name": "Bob"
  },
  {
    "name": "Bob",
    "age": 30
  }
]
```

**Expected Output:**
```json
[
  {
    "op": "add",
    "path": "/age",
    "value": 30
  }
]
```

### Json Patch Diff Remove Field

Generate JSON Patch diff with remove operation.
Demonstrates patch creation when fields are deleted.


**Script:**
```json
[
  "diff",
  {
    "name": "Charlie",
    "temp": "remove_me"
  },
  {
    "name": "Charlie"
  }
]
```

**Expected Output:**
```json
[
  {
    "op": "remove",
    "path": "/temp"
  }
]
```

### Json Patch Apply Replace

Apply JSON Patch replace operation.
Shows applying RFC 6902 patches to modify documents.


**Script:**
```json
[
  "patch",
  {
    "name": "David",
    "status": "pending"
  },
  {
    "array": [
      {
        "op": "replace",
        "path": "/status",
        "value": "completed"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "name": "David",
  "status": "completed"
}
```

### Json Patch Apply Add

Apply JSON Patch add operation.
Demonstrates adding new fields to documents via patches.


**Script:**
```json
[
  "patch",
  {
    "name": "Eve"
  },
  {
    "array": [
      {
        "op": "add",
        "path": "/role",
        "value": "admin"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "name": "Eve",
  "role": "admin"
}
```

### Json Patch Apply Remove

Apply JSON Patch remove operation.
Shows removing fields from documents using patches.


**Script:**
```json
[
  "patch",
  {
    "name": "Frank",
    "temp_field": "delete_this",
    "role": "user"
  },
  {
    "array": [
      {
        "op": "remove",
        "path": "/temp_field"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "name": "Frank",
  "role": "user"
}
```

### Json Patch Apply Move

Apply JSON Patch move operation.
Demonstrates moving fields within documents.


**Script:**
```json
[
  "patch",
  {
    "user_name": "Grace",
    "profile": {}
  },
  {
    "array": [
      {
        "op": "move",
        "from": "/user_name",
        "path": "/profile/name"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "profile": {
    "name": "Grace"
  }
}
```

### Json Patch Apply Copy

Apply JSON Patch copy operation.
Shows copying values to new locations in documents.


**Script:**
```json
[
  "patch",
  {
    "id": 12345,
    "data": {
      "important": "value"
    }
  },
  {
    "array": [
      {
        "op": "copy",
        "from": "/data/important",
        "path": "/backup_value"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "id": 12345,
  "data": {
    "important": "value"
  },
  "backup_value": "value"
}
```

### Json Patch Apply Test Success

Apply JSON Patch with successful test operation.
Demonstrates conditional patching using test operations.


**Script:**
```json
[
  "patch",
  {
    "version": 1,
    "data": "current"
  },
  {
    "array": [
      {
        "op": "test",
        "path": "/version",
        "value": 1
      },
      {
        "op": "replace",
        "path": "/data",
        "value": "updated"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "version": 1,
  "data": "updated"
}
```

### Json Patch Multiple Operations

Apply multiple JSON Patch operations in sequence.
Shows complex document transformations with multiple patch operations.


**Script:**
```json
[
  "patch",
  {
    "name": "Henry",
    "status": "draft",
    "temp": "remove"
  },
  {
    "array": [
      {
        "op": "replace",
        "path": "/status",
        "value": "published"
      },
      {
        "op": "remove",
        "path": "/temp"
      },
      {
        "op": "add",
        "path": "/published_date",
        "value": "2024-01-01"
      }
    ]
  }
]
```

**Expected Output:**
```json
{
  "name": "Henry",
  "status": "published",
  "published_date": "2024-01-01"
}
```

### Diff Generation Example

Generate JSON Patch between transformed and original documents.
Demonstrates diff generation for document transformation.


**Script:**
```json
[
  "diff",
  {
    "id": 123,
    "status": "pending",
    "created": "2023-12-01"
  },
  [
    "obj",
    [
      "id",
      123
    ],
    [
      "status",
      "completed"
    ],
    [
      "created",
      "2023-12-01"
    ],
    [
      "completed_date",
      "2024-01-01"
    ]
  ]
]
```

**Expected Output:**
```json
[
  {
    "op": "replace",
    "path": "/status",
    "value": "completed"
  },
  {
    "op": "add",
    "path": "/completed_date",
    "value": "2024-01-01"
  }
]
```

## Lambda Functions Examples

### Lambda Simple Transform

Simple lambda function for array transformation.
Demonstrates basic lambda syntax with single parameter.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "+",
      [
        "$",
        "/x"
      ],
      100
    ]
  ]
]
```

**Expected Output:**
```json
[
  101,
  102,
  103,
  104
]
```

### Lambda Conditional Logic

Lambda with conditional logic.
Shows lambda functions containing if-then-else expressions.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      5,
      10,
      15
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "if",
      [
        ">",
        [
          "$",
          "/x"
        ],
        5
      ],
      [
        "*",
        [
          "$",
          "/x"
        ],
        2
      ],
      [
        "$",
        "/x"
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  1,
  5,
  20,
  30
]
```

### Lambda Multiple Parameters

Lambda with multiple parameters in reduce.
Demonstrates lambda functions with accumulator and item parameters for string concatenation.


**Script:**
```json
[
  "reduce",
  {
    "array": [
      "Hello",
      "World",
      "From",
      "Computo"
    ]
  },
  [
    "lambda",
    [
      "acc",
      "word"
    ],
    [
      "str_concat",
      [
        "$",
        "/acc"
      ],
      [
        "$",
        "/word"
      ]
    ]
  ],
  ""
]
```

**Expected Output:**
```json
"HelloWorldFromComputo"
```

### Lambda Nested Operations

Lambda with nested arithmetic operations.
Shows complex expressions within lambda bodies.


**Script:**
```json
[
  "map",
  {
    "array": [
      1,
      2,
      3,
      4
    ]
  },
  [
    "lambda",
    [
      "x"
    ],
    [
      "+",
      [
        "*",
        [
          "$",
          "/x"
        ],
        [
          "$",
          "/x"
        ]
      ],
      [
        "*",
        2,
        [
          "$",
          "/x"
        ]
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  3,
  8,
  15,
  24
]
```

### Lambda Variables For Reuse

Reusable lambda functions stored in variables.
Demonstrates storing lambda functions in let variables for reuse.


**Script:**
```json
[
  "let",
  [
    [
      "doubler",
      [
        "lambda",
        [
          "x"
        ],
        [
          "*",
          [
            "$",
            "/x"
          ],
          2
        ]
      ]
    ]
  ],
  [
    "obj",
    [
      "first_list",
      [
        "map",
        {
          "array": [
            1,
            2,
            3
          ]
        },
        [
          "$",
          "/doubler"
        ]
      ]
    ],
    [
      "second_list",
      [
        "map",
        {
          "array": [
            10,
            20,
            30
          ]
        },
        [
          "$",
          "/doubler"
        ]
      ]
    ]
  ]
]
```

**Expected Output:**
```json
{
  "first_list": [
    2,
    4,
    6
  ],
  "second_list": [
    20,
    40,
    60
  ]
}
```

### Lambda Multiple Reusable Functions

Multiple lambda variables for different operations.
Shows storing and reusing multiple lambda functions.


**Script:**
```json
[
  "let",
  [
    [
      "add_ten",
      [
        "lambda",
        [
          "x"
        ],
        [
          "+",
          [
            "$",
            "/x"
          ],
          10
        ]
      ]
    ],
    [
      "gt_three",
      [
        "lambda",
        [
          "x"
        ],
        [
          ">",
          [
            "$",
            "/x"
          ],
          3
        ]
      ]
    ]
  ],
  [
    "map",
    [
      "filter",
      {
        "array": [
          1,
          2,
          3,
          4,
          5,
          6
        ]
      },
      [
        "$",
        "/gt_three"
      ]
    ],
    [
      "$",
      "/add_ten"
    ]
  ]
]
```

**Expected Output:**
```json
[
  14,
  15,
  16
]
```

### Lambda Closure Behavior

Lambda function accessing outer scope variables.
Demonstrates variable capture in lambda expressions.


**Script:**
```json
[
  "let",
  [
    [
      "multiplier",
      3
    ]
  ],
  [
    "map",
    {
      "array": [
        1,
        2,
        3,
        4
      ]
    },
    [
      "lambda",
      [
        "x"
      ],
      [
        "*",
        [
          "$",
          "/x"
        ],
        [
          "$",
          "/multiplier"
        ]
      ]
    ]
  ]
]
```

**Expected Output:**
```json
[
  3,
  6,
  9,
  12
]
```

## Logical Examples

### Logical And All True

Logical AND with all true conditions.
Demonstrates short-circuit evaluation - all expressions must be true.


**Script:**
```json
[
  "&&",
  true,
  [
    ">",
    10,
    5
  ],
  [
    "==",
    2,
    2
  ]
]
```

**Expected Output:**
```json
true
```

### Logical And Short Circuit

Logical AND with short-circuit evaluation.
Shows how AND stops at first false condition, preventing division by zero.


**Script:**
```json
[
  "&&",
  false,
  [
    "/",
    1,
    0
  ]
]
```

**Expected Output:**
```json
false
```

### Logical Or First True

Logical OR with short-circuit evaluation.
Demonstrates that OR returns true when first condition is true.


**Script:**
```json
[
  "||",
  true,
  [
    "/",
    1,
    0
  ]
]
```

**Expected Output:**
```json
true
```

### Logical Or Second True

Logical OR with second condition true.
Shows OR continuing evaluation until it finds a true condition.


**Script:**
```json
[
  "||",
  false,
  [
    "==",
    3,
    3
  ],
  [
    "!=",
    1,
    1
  ]
]
```

**Expected Output:**
```json
true
```

### Logical Or All False

Logical OR with all false conditions.
Shows OR behavior when all conditions are false.


**Script:**
```json
[
  "||",
  false,
  [
    ">",
    2,
    5
  ],
  [
    "==",
    "a",
    "b"
  ]
]
```

**Expected Output:**
```json
false
```

### Logical Not True

Logical NOT operator with true input.
Demonstrates boolean negation returning false for true input.


**Script:**
```json
[
  "not",
  true
]
```

**Expected Output:**
```json
false
```

### Logical Not False

Logical NOT operator with false input.
Shows boolean negation returning true for false input.


**Script:**
```json
[
  "not",
  false
]
```

**Expected Output:**
```json
true
```

### Logical Not Complex Condition

Logical NOT with complex boolean expression.
Demonstrates negating the result of a comparison operation.


**Script:**
```json
[
  "not",
  [
    ">",
    5,
    10
  ]
]
```

**Expected Output:**
```json
true
```

### Logical Not With String

Logical NOT with non-empty string.
Shows that non-empty strings are truthy, so NOT returns false.


**Script:**
```json
[
  "not",
  "hello"
]
```

**Expected Output:**
```json
false
```

### Logical Not With Zero

Logical NOT with zero value.
Demonstrates that zero is falsy, so NOT returns true.


**Script:**
```json
[
  "not",
  0
]
```

**Expected Output:**
```json
true
```

### Logical Not With Array

Logical NOT with non-empty array.
Shows that non-empty arrays are truthy, so NOT returns false.


**Script:**
```json
[
  "not",
  {
    "array": [
      1,
      2
    ]
  }
]
```

**Expected Output:**
```json
false
```

## Multiple Inputs Examples

### Multiple Inputs Access All

Access all input documents using $inputs.
Demonstrates the new system variable for multiple input handling.


**Script:**
```json
[
  "$inputs"
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "type": "user",
  "name": "Alice"
}
```

*Input 1:*
```json
{
  "type": "config",
  "theme": "dark"
}
```

*Input 2:*
```json
{
  "type": "metadata",
  "version": "1.0"
}
```
**Expected Output:**
```json
[
  {
    "type": "user",
    "name": "Alice"
  },
  {
    "type": "config",
    "theme": "dark"
  },
  {
    "type": "metadata",
    "version": "1.0"
  }
]
```

### Multiple Inputs By Index

Access specific inputs by index.
Shows extracting individual documents from multiple inputs.


**Script:**
```json
[
  "obj",
  [
    "first_name",
    [
      "get",
      [
        "$inputs"
      ],
      "/0/name"
    ]
  ],
  [
    "second_name",
    [
      "get",
      [
        "$inputs"
      ],
      "/1/name"
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "name": "Alice",
  "role": "admin"
}
```

*Input 1:*
```json
{
  "name": "Bob",
  "role": "user"
}
```
**Expected Output:**
```json
{
  "first_name": "Alice",
  "second_name": "Bob"
}
```

### Multi Input Merge

Merge data from multiple inputs using $inputs.
Shows how to access different input files and combine them.


**Script:**
```json
[
  "obj",
  [
    "user",
    [
      "get",
      [
        "$inputs"
      ],
      "/0"
    ]
  ],
  [
    "config",
    [
      "get",
      [
        "$inputs"
      ],
      "/1"
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "name": "Alice",
  "id": 123
}
```

*Input 1:*
```json
{
  "theme": "dark",
  "lang": "en"
}
```
**Expected Output:**
```json
{
  "user": {
    "name": "Alice",
    "id": 123
  },
  "config": {
    "theme": "dark",
    "lang": "en"
  }
}
```

### Input Count

Count the number of input documents.
Demonstrates using count operator with the $inputs array.


**Script:**
```json
[
  "count",
  [
    "$inputs"
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "data": "first"
}
```

*Input 1:*
```json
{
  "data": "second"
}
```

*Input 2:*
```json
{
  "data": "third"
}
```
**Expected Output:**
```json
3
```

### Backward Compatibility

Backward compatibility - $input equals first input.
Shows that existing scripts still work with multiple inputs.


**Script:**
```json
[
  "obj",
  [
    "old_way",
    [
      "$input"
    ]
  ],
  [
    "new_way",
    [
      "get",
      [
        "$inputs"
      ],
      "/0"
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "message": "Hello World"
}
```
**Expected Output:**
```json
{
  "old_way": {
    "message": "Hello World"
  },
  "new_way": {
    "message": "Hello World"
  }
}
```

## Permuto Examples

### Permuto Simple Interpolation

Simple Permuto template with string interpolation.
Demonstrates basic variable substitution in strings.


**Script:**
```json
[
  "permuto.apply",
  {
    "greeting": "Hello ${/name}!",
    "role": "User: ${/role}"
  },
  {
    "name": "Alice",
    "role": "Administrator"
  }
]
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "greeting": "Hello Alice!",
  "role": "User: Administrator"
}
```

### Permuto With Input Data

Permuto template using input data as context.
Shows using input document as the template context.


**Script:**
```json
[
  "permuto.apply",
  {
    "summary": "Profile: ${/profile/name} (${/profile/email})",
    "status": "Active: ${/active}"
  },
  [
    "$input"
  ]
]
```

**Input:**
```json
{
  "profile": {
    "name": "Bob",
    "email": "bob@example.com"
  },
  "active": true
}
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "summary": "Profile: Bob (bob@example.com)",
  "status": "Active: true"
}
```

### Permuto Nested Object Access

Permuto templates with deep object access.
Demonstrates accessing nested data in template interpolation.


**Script:**
```json
[
  "permuto.apply",
  {
    "message": "User ${/user/profile/firstName} ${/user/profile/lastName} has ${/user/stats/points} points"
  },
  {
    "user": {
      "profile": {
        "firstName": "Charlie",
        "lastName": "Brown"
      },
      "stats": {
        "points": 1500
      }
    }
  }
]
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "message": "User Charlie Brown has 1500 points"
}
```

### Permuto Array Access

Permuto templates accessing array elements.
Shows using array indices in template paths.


**Script:**
```json
[
  "permuto.apply",
  {
    "first_item": "First: ${/items/0}",
    "last_item": "Last: ${/items/2}"
  },
  {
    "items": [
      "apple",
      "banana",
      "cherry"
    ]
  }
]
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "first_item": "First: apple",
  "last_item": "Last: cherry"
}
```

### Permuto Computed Context

Permuto with computed template context.
Demonstrates building template context using Computo operations.


**Script:**
```json
[
  "permuto.apply",
  {
    "report": "Total: ${/sum}, Average: ${/avg}"
  },
  [
    "obj",
    [
      "sum",
      [
        "reduce",
        {
          "array": [
            10,
            20,
            30
          ]
        },
        [
          "lambda",
          [
            "a",
            "b"
          ],
          [
            "+",
            [
              "$",
              "/a"
            ],
            [
              "$",
              "/b"
            ]
          ]
        ],
        0
      ]
    ],
    [
      "avg",
      [
        "/",
        60,
        3
      ]
    ]
  ]
]
```

**Flags:** `--interpolation`

**Expected Output:**
```json
{
  "report": "Total: 60, Average: 20.000000"
}
```

## Real World Examples

### Api Response Transform

Transform API response structure.
Demonstrates typical API data transformation patterns in real applications.


**Script:**
```json
[
  "obj",
  [
    "users",
    [
      "map",
      [
        "get",
        [
          "$input"
        ],
        "/data"
      ],
      [
        "lambda",
        [
          "user"
        ],
        [
          "obj",
          [
            "id",
            [
              "get",
              [
                "$",
                "/user"
              ],
              "/userId"
            ]
          ],
          [
            "name",
            [
              "get",
              [
                "$",
                "/user"
              ],
              "/fullName"
            ]
          ],
          [
            "active",
            [
              "==",
              [
                "get",
                [
                  "$",
                  "/user"
                ],
                "/status"
              ],
              "active"
            ]
          ]
        ]
      ]
    ]
  ],
  [
    "metadata",
    [
      "obj",
      [
        "total",
        [
          "count",
          [
            "get",
            [
              "$input"
            ],
            "/data"
          ]
        ]
      ],
      [
        "processed_at",
        "2024-01-01"
      ]
    ]
  ]
]
```

**Input:**
```json
{
  "data": [
    {
      "userId": 1,
      "fullName": "Alice Smith",
      "status": "active"
    },
    {
      "userId": 2,
      "fullName": "Bob Jones",
      "status": "inactive"
    }
  ]
}
```

**Expected Output:**
```json
{
  "users": [
    {
      "id": 1,
      "name": "Alice Smith",
      "active": true
    },
    {
      "id": 2,
      "name": "Bob Jones",
      "active": false
    }
  ],
  "metadata": {
    "total": 2,
    "processed_at": "2024-01-01"
  }
}
```

### Configuration Merge Multiple Sources

Merge configuration from multiple input sources.
Shows practical configuration management with precedence rules.


**Script:**
```json
[
  "let",
  [
    [
      "defaults",
      [
        "get",
        [
          "$inputs"
        ],
        "/0"
      ]
    ],
    [
      "environment",
      [
        "get",
        [
          "$inputs"
        ],
        "/1"
      ]
    ],
    [
      "user_prefs",
      [
        "get",
        [
          "$inputs"
        ],
        "/2"
      ]
    ]
  ],
  [
    "obj",
    [
      "database",
      [
        "obj",
        [
          "host",
          [
            "get",
            [
              "$",
              "/user_prefs"
            ],
            "/database/host"
          ]
        ],
        [
          "port",
          [
            "get",
            [
              "$",
              "/defaults"
            ],
            "/database/port"
          ]
        ],
        [
          "ssl",
          [
            "get",
            [
              "$",
              "/environment"
            ],
            "/database/ssl"
          ]
        ]
      ]
    ],
    [
      "logging",
      [
        "get",
        [
          "$",
          "/environment"
        ],
        "/logging"
      ]
    ],
    [
      "theme",
      [
        "get",
        [
          "$",
          "/user_prefs"
        ],
        "/ui/theme"
      ]
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "database": {
    "host": "localhost",
    "port": 5432
  },
  "logging": {
    "level": "info"
  }
}
```

*Input 1:*
```json
{
  "database": {
    "ssl": true
  },
  "logging": {
    "level": "debug"
  }
}
```

*Input 2:*
```json
{
  "database": {
    "host": "prod.example.com"
  },
  "ui": {
    "theme": "dark"
  }
}
```
**Expected Output:**
```json
{
  "database": {
    "host": "prod.example.com",
    "port": 5432,
    "ssl": true
  },
  "logging": {
    "level": "debug"
  },
  "theme": "dark"
}
```

### Document Versioning Workflow

Document versioning with patch generation and rollback capability.
Demonstrates creating version control for documents with change tracking.


**Script:**
```json
[
  "let",
  [
    [
      "original",
      [
        "get",
        [
          "$inputs"
        ],
        "/0"
      ]
    ],
    [
      "modified",
      [
        "get",
        [
          "$inputs"
        ],
        "/1"
      ]
    ],
    [
      "changes",
      [
        "diff",
        [
          "$",
          "/original"
        ],
        [
          "$",
          "/modified"
        ]
      ]
    ],
    [
      "rollback_patch",
      [
        "diff",
        [
          "$",
          "/modified"
        ],
        [
          "$",
          "/original"
        ]
      ]
    ]
  ],
  [
    "obj",
    [
      "document_id",
      [
        "get",
        [
          "$",
          "/original"
        ],
        "/id"
      ]
    ],
    [
      "version_info",
      [
        "obj",
        [
          "from_version",
          [
            "get",
            [
              "$",
              "/original"
            ],
            "/version"
          ]
        ],
        [
          "to_version",
          [
            "get",
            [
              "$",
              "/modified"
            ],
            "/version"
          ]
        ],
        [
          "change_count",
          [
            "count",
            [
              "$",
              "/changes"
            ]
          ]
        ]
      ]
    ],
    [
      "forward_patch",
      [
        "$",
        "/changes"
      ]
    ],
    [
      "rollback_patch",
      [
        "$",
        "/rollback_patch"
      ]
    ],
    [
      "can_rollback",
      [
        ">",
        [
          "count",
          [
            "$",
            "/rollback_patch"
          ]
        ],
        0
      ]
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "id": "doc_001",
  "version": 1,
  "title": "Original Title",
  "content": "Original content"
}
```

*Input 1:*
```json
{
  "id": "doc_001",
  "version": 2,
  "title": "Updated Title",
  "content": "Updated content",
  "author": "Alice"
}
```
**Expected Output:**
```json
{
  "document_id": "doc_001",
  "version_info": {
    "from_version": 1,
    "to_version": 2,
    "change_count": 4
  },
  "forward_patch": [
    {
      "op": "replace",
      "path": "/content",
      "value": "Updated content"
    },
    {
      "op": "replace",
      "path": "/title",
      "value": "Updated Title"
    },
    {
      "op": "replace",
      "path": "/version",
      "value": 2
    },
    {
      "op": "add",
      "path": "/author",
      "value": "Alice"
    }
  ],
  "rollback_patch": [
    {
      "op": "remove",
      "path": "/author"
    },
    {
      "op": "replace",
      "path": "/content",
      "value": "Original content"
    },
    {
      "op": "replace",
      "path": "/title",
      "value": "Original Title"
    },
    {
      "op": "replace",
      "path": "/version",
      "value": 1
    }
  ],
  "can_rollback": true
}
```

### Functional Pipeline Multi Input

Functional pipeline processing using car/cdr for multiple inputs.
Shows advanced functional programming patterns for processing input sequences.


**Script:**
```json
[
  "let",
  [
    [
      "initial_doc",
      [
        "car",
        [
          "$inputs"
        ]
      ]
    ],
    [
      "patch_sequence",
      [
        "cdr",
        [
          "$inputs"
        ]
      ]
    ],
    [
      "final_doc",
      [
        "reduce",
        [
          "$",
          "/patch_sequence"
        ],
        [
          "lambda",
          [
            "doc",
            "patch"
          ],
          [
            "patch",
            [
              "$",
              "/doc"
            ],
            [
              "$",
              "/patch"
            ]
          ]
        ],
        [
          "$",
          "/initial_doc"
        ]
      ]
    ]
  ],
  [
    "obj",
    [
      "initial_state",
      [
        "$",
        "/initial_doc"
      ]
    ],
    [
      "final_state",
      [
        "$",
        "/final_doc"
      ]
    ],
    [
      "transformations_applied",
      [
        "count",
        [
          "$",
          "/patch_sequence"
        ]
      ]
    ],
    [
      "title_changed",
      [
        "!=",
        [
          "get",
          [
            "$",
            "/initial_doc"
          ],
          "/title"
        ],
        [
          "get",
          [
            "$",
            "/final_doc"
          ],
          "/title"
        ]
      ]
    ]
  ]
]
```

**Multiple Inputs:**

*Input 0:*
```json
{
  "id": "doc_123",
  "title": "Draft",
  "status": "draft",
  "content": "Initial content"
}
```

*Input 1:*
```json
[
  {
    "op": "replace",
    "path": "/status",
    "value": "review"
  }
]
```

*Input 2:*
```json
[
  {
    "op": "replace",
    "path": "/title",
    "value": "Final Document"
  }
]
```

*Input 3:*
```json
[
  {
    "op": "add",
    "path": "/reviewed_by",
    "value": "Editor"
  }
]
```

*Input 4:*
```json
[
  {
    "op": "replace",
    "path": "/status",
    "value": "published"
  }
]
```
**Expected Output:**
```json
{
  "initial_state": {
    "id": "doc_123",
    "title": "Draft",
    "status": "draft",
    "content": "Initial content"
  },
  "final_state": {
    "id": "doc_123",
    "title": "Final Document",
    "status": "published",
    "content": "Initial content",
    "reviewed_by": "Editor"
  },
  "transformations_applied": 4,
  "title_changed": true
}
```

### Data Aggregation Report

Data aggregation and reporting pipeline.
Demonstrates complex data processing for analytics and reporting.


**Script:**
```json
[
  "let",
  [
    [
      "sales_data",
      [
        "get",
        [
          "$input"
        ],
        "/sales"
      ]
    ],
    [
      "total_revenue",
      [
        "reduce",
        [
          "$",
          "/sales_data"
        ],
        [
          "lambda",
          [
            "sum",
            "sale"
          ],
          [
            "+",
            [
              "$",
              "/sum"
            ],
            [
              "get",
              [
                "$",
                "/sale"
              ],
              "/amount"
            ]
          ]
        ],
        0
      ]
    ],
    [
      "high_value_sales",
      [
        "filter",
        [
          "$",
          "/sales_data"
        ],
        [
          "lambda",
          [
            "sale"
          ],
          [
            ">",
            [
              "get",
              [
                "$",
                "/sale"
              ],
              "/amount"
            ],
            1000
          ]
        ]
      ]
    ],
    [
      "north_sales",
      [
        "filter",
        [
          "$",
          "/sales_data"
        ],
        [
          "lambda",
          [
            "sale"
          ],
          [
            "==",
            [
              "get",
              [
                "$",
                "/sale"
              ],
              "/region"
            ],
            "North"
          ]
        ]
      ]
    ],
    [
      "north_total",
      [
        "reduce",
        [
          "$",
          "/north_sales"
        ],
        [
          "lambda",
          [
            "sum",
            "sale"
          ],
          [
            "+",
            [
              "$",
              "/sum"
            ],
            [
              "get",
              [
                "$",
                "/sale"
              ],
              "/amount"
            ]
          ]
        ],
        0
      ]
    ]
  ],
  [
    "obj",
    [
      "summary",
      [
        "obj",
        [
          "total_sales",
          [
            "count",
            [
              "$",
              "/sales_data"
            ]
          ]
        ],
        [
          "total_revenue",
          [
            "$",
            "/total_revenue"
          ]
        ],
        [
          "average_sale",
          [
            "/",
            [
              "$",
              "/total_revenue"
            ],
            [
              "count",
              [
                "$",
                "/sales_data"
              ]
            ]
          ]
        ]
      ]
    ],
    [
      "high_value",
      [
        "obj",
        [
          "count",
          [
            "count",
            [
              "$",
              "/high_value_sales"
            ]
          ]
        ],
        [
          "revenue",
          [
            "reduce",
            [
              "$",
              "/high_value_sales"
            ],
            [
              "lambda",
              [
                "sum",
                "sale"
              ],
              [
                "+",
                [
                  "$",
                  "/sum"
                ],
                [
                  "get",
                  [
                    "$",
                    "/sale"
                  ],
                  "/amount"
                ]
              ]
            ],
            0
          ]
        ]
      ]
    ],
    [
      "north_region",
      [
        "obj",
        [
          "sales_count",
          [
            "count",
            [
              "$",
              "/north_sales"
            ]
          ]
        ],
        [
          "total_revenue",
          [
            "$",
            "/north_total"
          ]
        ]
      ]
    ]
  ]
]
```

**Input:**
```json
{
  "sales": [
    {
      "region": "North",
      "amount": 1500,
      "product": "A"
    },
    {
      "region": "South",
      "amount": 800,
      "product": "B"
    },
    {
      "region": "North",
      "amount": 2000,
      "product": "C"
    },
    {
      "region": "East",
      "amount": 1200,
      "product": "A"
    }
  ]
}
```

**Expected Output:**
```json
{
  "summary": {
    "total_sales": 4,
    "total_revenue": 5500,
    "average_sale": 1375
  },
  "high_value": {
    "count": 3,
    "revenue": 4700
  },
  "north_region": {
    "sales_count": 2,
    "total_revenue": 3500
  }
}
```



## Running the Examples

Each example can be tested using the extracted test files:

```bash
# Generate test files from this documentation
python generate_examples.py

# Run all tests
cd examples && ./run_all.sh

# Run specific example
cd examples/arithmetic/basic_multiplication && ./test.sh
```

---
*This README.md was generated from README.toml - edit that file instead.*
