# Computo - Requirements Specification

## Project Overview

Computo is a safe, sandboxed, JSON-native data transformation engine that provides a Lisp-like functional programming language expressed entirely in JSON. It operates on the principle of "code is data" where all programs are valid JSON documents.

## Core Design Principles

### 1. Code as Data
- **All scripts are valid JSON**: Every Computo program must be parseable as standard JSON
- **Unambiguous syntax**: Clear distinction between operator calls `[op, arg1, arg2]` and literal arrays `{"array": [...]}`
- **Immutable operations**: All operations are pure functions that don't modify input data
- **Recursive evaluation**: Expressions are evaluated recursively from the inside out

### 2. Safety and Sandboxing
- **No I/O operations**: No file system access, network operations, or system calls
- **Memory safe**: No pointer arithmetic or manual memory management
- **Deterministic**: Same input always produces same output
- **Exception safety**: Structured error handling with detailed error paths

### 3. JSON-Native Processing
- **First-class JSON support**: Native handling of all JSON data types
- **JSON Pointer integration**: Path-based data access using RFC 6901 JSON Pointer
- **JSON Patch support**: Document transformation using RFC 6902 JSON Patch

## Language Syntax

### Basic Operator Call Syntax
```json
[operator_name, argument1, argument2, ...]
```

### Literal Data Syntax
- **Numbers**: `42`, `3.14`
- **Strings**: `"hello world"`
- **Booleans**: `true`, `false`
- **Null**: `null`
- **Objects**: `{"key": "value"}`
- **Arrays**: `{"array": [item1, item2, item3]}`

### Special Syntax Elements
- **Variable binding**: `["let", [["var1", expr1], ["var2", expr2]], body_expr]`
- **Variable access**: `["$", "/variable_name"]`
- **Lambda expressions**: `["lambda", ["param1", "param2"], body_expr]`

## Data Types

### Primitive Types
- **Numbers**: 64-bit integers and IEEE 754 double-precision floats
- **Strings**: UTF-8 encoded text
- **Booleans**: `true` and `false`
- **Null**: Represents absence of value

### Complex Types
- **Objects**: Unordered collections of key-value pairs with string keys
- **Arrays**: Ordered sequences of values (using array object syntax)
- **Lambdas**: First-class functions with parameter lists and lexical scoping

## Required Operators

### Input/Output Operators
- **`$input`**: Returns the primary input document
- **`$inputs`**: Returns array of all input documents (for multi-input processing)

### Variable and Scoping Operators
- **`let`**: Variable binding with lexical scoping
  - Syntax: `["let", [["var1", expr1], ["var2", expr2]], body_expr]`
  - Sequential binding: Later bindings can reference earlier ones
  - Scoping: Variables shadow outer scope
- **`$`**: Variable lookup using JSON Pointer syntax
  - Syntax: `["$", "/variable_name"]`
  - Path format: Must start with `/`

### Data Access Operators
- **`get`**: JSON Pointer-based data extraction
  - Syntax: `["get", object_expr, "/path/to/data"]`
  - Standards compliance: Implements RFC 6901 JSON Pointer

### Logic and Control Flow Operators
- **`if`**: Conditional execution with truthiness evaluation
  - Syntax: `["if", condition, then_expr, else_expr]`
  - Truthiness rules: `0`, `""`, `null`, `[]`, `{}` are falsy; all others are truthy
- **`&&`**: Logical AND with short-circuit evaluation
  - Syntax: `["&&", expr1, expr2, ...]`
  - Behavior: Returns `false` on first falsy expression, `true` otherwise
- **`||`**: Logical OR with short-circuit evaluation
  - Syntax: `["||", expr1, expr2, ...]`
  - Behavior: Returns `true` on first truthy expression, `false` otherwise

### Data Construction Operators
- **`obj`**: Object creation with support for computed keys
  - Syntax: `["obj", [key1_expr, value1_expr], [key2_expr, value2_expr], ...]`
  - Keys: Must evaluate to strings
- **Array objects**: Literal array creation
  - Syntax: `{"array": [item1, item2, item3]}`
  - Elements: Each element is evaluated as an expression

### Mathematical Operators
- **`+`**: N-ary addition (1 or more arguments)
  - Syntax: `["+", num1, num2, ...]`
  - Type preservation: Returns integer if all inputs are integers
- **`-`**: Binary subtraction
  - Syntax: `["-", minuend, subtrahend]`
- **`*`**: N-ary multiplication (1 or more arguments)
  - Syntax: `["*", num1, num2, ...]`
- **`/`**: Binary division
  - Syntax: `["/", dividend, divisor]`
  - Error handling: Throws exception on division by zero

### Comparison Operators
- **`>`**: N-ary greater than with chained comparisons
  - Syntax: `[">", num1, num2, ...]`
  - Behavior: `a > b > c` means `a > b && b > c`
- **`<`**: N-ary less than with chained comparisons
- **`>=`**: N-ary greater than or equal with chained comparisons
- **`<=`**: N-ary less than or equal with chained comparisons
- **`==`**: N-ary equality (all arguments must be equal)
  - Syntax: `["==", val1, val2, ...]`
- **`!=`**: Binary inequality
  - Syntax: `["!=", val1, val2]`
- **`approx`**: Epsilon-based floating point comparison
  - Syntax: `["approx", left, right, epsilon]`
  - Behavior: Returns `true` if `|left - right| <= epsilon`

### Array/Collection Operators
- **`map`**: Array transformation with lambda
  - Syntax: `["map", array_expr, lambda_expr]`
  - Lambda: `["lambda", ["item"], transform_expr]`
- **`filter`**: Array filtering with predicate lambda
  - Syntax: `["filter", array_expr, predicate_lambda]`
- **`reduce`**: Array reduction with accumulator
  - Syntax: `["reduce", array_expr, reducer_lambda, initial_value]`
  - Lambda: `["lambda", ["accumulator", "item"], body_expr]`
- **`find`**: Find first matching element
  - Syntax: `["find", array_expr, predicate_lambda]`
  - Returns: First matching element or `null`
- **`some`**: Test if any element matches predicate
- **`every`**: Test if all elements match predicate
- **`flatMap`**: Map and flatten results
- **`count`**: Get array length
- **`car`**: Get first element (head)
- **`cdr`**: Get all but first element (tail)
- **`cons`**: Prepend element to array
- **`append`**: Concatenate multiple arrays
- **`chunk`**: Split array into chunks of specified size
- **`partition`**: Split array into truthy/falsy groups based on predicate
- **`zip`**: Combine two arrays into pairs
- **`zipWith`**: Combine two arrays with lambda function
- **`mapWithIndex`**: Map with index as second parameter
- **`enumerate`**: Convert array to index-value pairs

### String Operations
- **`concat`**: String concatenation with type coercion
  - Syntax: `["concat", expr1, expr2, ...]`
  - Behavior: Converts all arguments to strings and concatenates

### Object Operations
- **`merge`**: Object merging (right-to-left precedence)
  - Syntax: `["merge", obj1, obj2, ...]`

### Template Processing (Permuto Integration)
- **`permuto.apply`**: Bridge to Permuto template engine
  - Syntax: `["permuto.apply", template_expr, context_expr]`
  - Behavior: Applies Permuto template processing with given context

### Document Transformation
- **`diff`**: Generate JSON Patch between two documents
  - Syntax: `["diff", document_a, document_b]`
  - Returns: JSON Patch array per RFC 6902
- **`patch`**: Apply JSON Patch to document
  - Syntax: `["patch", document, patch_array]`
  - Returns: Transformed document

## Lambda System

### Lambda Syntax
```json
["lambda", ["param1", "param2", ...], body_expression]
```

### Lambda Features
- **Multiple parameters**: Support for 0, 1, or more parameters
- **Lexical scoping**: Lambdas capture variables from their definition context
- **First-class values**: Lambdas can be stored in variables and passed as arguments
- **Variable resolution**: Lambda variables can be resolved from let bindings

### Lambda Usage Patterns
- **Array processing**: Used with `map`, `filter`, `reduce`, etc.
- **Conditional logic**: Used with `find`, `some`, `every`
- **Data transformation**: Used with `zipWith`, `mapWithIndex`

## Execution Model

### Evaluation Strategy
- **Recursive descent**: Expressions evaluated from inside out
- **Tail call optimization**: Special handling for `let` and `if` to prevent stack overflow
- **Lazy evaluation**: Short-circuit evaluation for logical operators

### Error Handling
- **Structured exceptions**: Hierarchy of exception types
- **Error paths**: Detailed execution context in error messages
- **Graceful degradation**: Predictable error behavior

### Multi-Input Support
- **Backward compatibility**: Single input API preserved
- **Multiple inputs**: Support for processing multiple documents
- **Input access**: `$input` for primary, `$inputs` for all inputs

## API Requirements

### Core Functions
```cpp
// Single input API (backward compatibility)
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& options);

// Multiple inputs API
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& options);
```

### Exception Hierarchy
```cpp
ComputoException (base)
├── InvalidScriptException
├── InvalidOperatorException
├── InvalidArgumentException
└── PatchFailedException
```

## Command Line Interface

### Basic Usage
```bash
computo script.json input.json
computo script.json input1.json input2.json
```

### Options
- **`--help, -?`**: Show help message
- **`--interpolation`**: Enable Permuto string interpolation
- **`--diff`**: Generate JSON patch between input and result
- **`--pretty=N`**: Pretty-print JSON with N spaces
- **`--comments`**: Allow comments in script files

### Examples
```bash
computo transform.json data.json
computo --pretty=2 script.json input1.json input2.json
computo --interpolation --diff transform.json data.json
```

## Integration Requirements

### Permuto Integration
- **Template processing**: Bridge to Permuto templating engine
- **Options passing**: Support for Permuto configuration options
- **Error handling**: Proper exception translation

### JSON Library Integration
- **nlohmann/json**: Primary JSON library dependency
- **JSON Pointer**: Native JSON Pointer support
- **JSON Patch**: Native JSON Patch support

### Build System
- **CMake**: Modern CMake configuration
- **C++17**: Minimum C++ standard
- **Package management**: Standard CMake package configuration

## Performance Requirements

### Memory Management
- **Smart pointers**: Use of `std::shared_ptr` for context management
- **Copy-on-write**: Efficient context copying for scoping
- **Exception safety**: RAII-compliant resource management

### Execution Efficiency
- **Tail call optimization**: Prevention of stack overflow in recursive calls
- **Short-circuit evaluation**: Efficient logical operator evaluation
- **Operator registry**: O(1) operator lookup via `std::map`

## Testing Requirements

### Test Coverage
- **Unit tests**: Comprehensive operator testing
- **Integration tests**: End-to-end script execution
- **Error cases**: Exception handling validation
- **Edge cases**: Boundary condition testing

### Test Framework
- **Google Test**: Primary testing framework
- **Automated testing**: CMake test integration
- **Continuous testing**: CTest configuration

## Quality Requirements

### Code Quality
- **Warnings as errors**: Strict compiler settings
- **Static analysis**: Comprehensive warning flags
- **Code style**: Consistent formatting and naming

### Documentation
- **API documentation**: Comprehensive interface documentation
- **Examples**: Practical usage examples
- **Error messages**: Clear, actionable error descriptions