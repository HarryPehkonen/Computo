# CORE_REQUIREMENTS.md

## Project Overview

Computo is a safe, sandboxed, thread-safe JSON-native data transformation engine that provides a Lisp-like language syntax expressed in JSON. It consists of a clean library (libcomputo) and a feature-rich CLI tool for development and debugging.

## Core Principles

### 1. Clean Separation of Concerns
- **Library**: Thread-safe, minimal, single include file
- **CLI**: Rich debugging features, development environment
- No debugging infrastructure in the library itself
- CLI demonstrates how to extend library functionality

### 2. Functional Consistency
- All operators are n-ary (except `not` which is unary)
- No mixed binary/n-ary behavior
- Consistent argument patterns across all operators

### 3. JSON-Native Design
- All scripts are valid JSON (library) or JSON with comments (CLI only)
- Operator calls: `["operator", arg1, arg2, ...]` (n-ary)
- Literal arrays: `{"array": [item1, item2, ...]}`
- Objects: Standard JSON objects
- Zero ambiguity between operators and data

### 4. Thread Safety
- Pure functions throughout
- No global mutable state
- Exception-based error handling (thread-local)
- Safe for concurrent use across multiple threads

## Library Requirements (libcomputo)

### Single Include Design
- **One header file**: `computo.hpp` 
- **Simple API**: `execute(script, inputs)` and `execute(script, input)`
- **Thread-safe**: Concurrent execution supported
- **No dependencies**: Beyond nlohmann/json and standard library

### Required Operators (30 total)

#### Arithmetic (5 operators - all n-ary)
- `+` - Addition: `["+", a, b, c, ...]`
- `-` - Subtraction: `["-", a, b, c, ...]`
- `*` - Multiplication: `["*", a, b, c, ...]`
- `/` - Division: `["/", a, b, c, ...]`
- `%` - Modulo: `["%", a, b, c, ...]`

#### Comparison (6 operators - all n-ary with chaining)
- `>` - Greater than: `[">", a, b, c]` (a > b && b > c)
- `<` - Less than: `["<", a, b, c]` (a < b && b < c)
- `>=` - Greater or equal: `[">=", a, b, c]`
- `<=` - Less or equal: `["<=", a, b, c]`
- `==` - Equality: `["==", a, b, c]` (all equal)
- `!=` - Inequality: `["!=", a, b]` (binary only)

#### Logical (3 operators)
- `&&` - Logical AND: `["&&", a, b, c, ...]` (n-ary)
- `||` - Logical OR: `["||", a, b, c, ...]` (n-ary)
- `not` - Logical NOT: `["not", a]` (unary only)

#### Data Access (5 operators)
- `$input` - Access first input: `["$input"]`
- '$inputs' - Access all input: '["$inputs"]'
- `$` - Variable access: `["$", "/var_name"]`
- `get` - JSON Pointer access: `["get", object, "/path"]`
- `let` - Variable binding: `["let", [["var", value]], body]`

#### Data Construction (1 operator)
- `obj` - Object creation with variable keys: `["obj", [key_expr, val_expr], ...]`

#### Control Flow (1 operator)  
- `if` - Conditional: `["if", condition, then, else]`

#### Array Operations (7 operators)
- `map` - Transform: `["map", array, lambda]`
- `filter` - Filter: `["filter", array, lambda]`
- `reduce` - Aggregate: `["reduce", array, lambda, initial]`
- `count` - Length: `["count", array]`
- `find` - First match: `["find", array, lambda]`
- `some` - Any match: `["some", array, lambda]`
- `every` - All match: `["every", array, lambda]`

#### Functional Programming (4 operators)
- `car` - First element: `["car", array]`
- `cdr` - Rest elements: `["cdr", array]`  
- `cons` - Prepend: `["cons", item, array]`
- `append` - Concatenate: `["append", array1, array2, ...]`

#### Utilities (3 operators)
- `strConcat` - String concatenation: `["strConcat", a, b, c, ...]`
- `merge` - Object merging: `["merge", obj1, obj2, ...]`
- `approx` - Approximate equality: `["approx", a, b, tolerance]`

### Input Handling
- **Single input**: `execute(script, input)` → `$input` available
- **Multiple inputs**: `execute(script, inputs_vector)` → `$input` = first, `$inputs` = array of all
- **No inputs**: `execute(script, json(nullptr))` → `$input` = null

### Lambda Expressions
- Format: `["lambda", ["param"], body]`
- Single parameter only
- Variable access: `["$", "/param"]`
- Proper lexical scoping with TCO support

### Error Handling (Thread-Safe)
- Exception hierarchy:
  - `ComputoException` (base)
  - `InvalidOperatorException`
  - `InvalidArgumentException`
- Clear error messages with path information
- Thread-local exception handling (no shared error state)

## CLI Requirements (computo tool)

### Command Line Interface
```bash
computo [options] script.json [context1.json context2.json ...]
```

**Switch Order**: All switches must come before filenames

### Required CLI Options
- `--trace` - Show execution trace
- `--profile` - Show performance statistics  
- `--break-on=OPERATOR` - Break execution at operator (auto-enters interactive mode)
- `--watch=VARIABLE` - Watch variable changes
- `--pretty=N` - Pretty print with N spaces indent
- `--comments` - Enable JSON with comments parsing
- `--repl` - Start interactive REPL (can load context files)

### CLI-Only Features
- **JSON Comments**: Parse JSON with comments using nlohmann::json flags
- **Interactive Debugging**: REPL with readline support, history
- **Variable Inspection**: Watch variables, break on operators
- **Performance Monitoring**: Execution timing, operation counts

### Context File Handling
- **No context files**: `$input = null`
- **Multiple files**: `$input = first_file`, `$inputs = [all_files]`
- **REPL with context**: `computo --repl context.json` preloads context

## Implementation Constraints

### Code Organization
```
src/
  computo.cpp           # Core evaluation engine, TCO
  operators/            # Operator implementations (organized by category)
    arithmetic.cpp      # +, -, *, /, %
    comparison.cpp      # >, <, >=, <=, ==, !=  
    logical.cpp         # &&, ||, not
    array.cpp          # map, filter, reduce, count, find, some, every
    functional.cpp     # car, cdr, cons, append
    utility.cpp        # strConcat, merge, approx
    data.cpp          # $input, $, get, let, obj, if
  cli/
    main.cpp           # CLI argument parsing, debugging features
    repl.cpp           # Interactive REPL with readline
    debugger.cpp       # Tracing, profiling, breakpoints

include/
  computo.hpp          # Single public header
```

### Dependencies
- **Library**: `nlohmann/json`, standard library only
- **CLI**: Add `readline` for REPL functionality
- **No debugging dependencies** in library

### Size Guidelines (Relaxed for 30 operators)
- **Library implementation**: < 1500 lines total
- **Any single operator**: < 25 lines
- **CLI implementation**: < 800 lines total
- **Test code**: < 1500 lines total

### Thread Safety Requirements
- No global mutable state
- Pure evaluation functions
- Operator registry initialized with `std::call_once`
- Exception-based error handling (thread-local)

### Style Requirements
- **NO EMOJIS**: No emoji characters in any code, documentation, error messages, or CLI output
- Consistent error message formatting
- Clean, readable code without unnecessary decorations

## Success Criteria

The implementation is successful if:
1. All 30 operators work correctly with n-ary semantics
2. Library is thread-safe for concurrent use
3. Single `computo.hpp` include provides complete API
4. CLI provides rich debugging without cluttering library
5. Variable keys work in `obj` operator
6. TCO prevents stack overflow in deep recursion
7. REPL provides productive development experience

## Use Case Validation

**Primary Use Case**: AI API Schema Translation
- Unified schema ↔ Mistral API format
- Unified schema ↔ Anthropic API format  
- Complex conditional transformations
- Object restructuring with variable keys
- Array processing with functional operators

**Secondary Use Case**: Functional Programming Learning
- Tail call optimization exploration
- Lisp-style list processing with `car`, `cdr`, `cons`, `append`
- Complex recursive algorithms

This specification balances practical functionality with learning opportunities while maintaining clean architecture separation between library and tooling.