# Computo

A safe, sandboxed JSON transformation engine with Lisp-like syntax expressed in JSON. Features RFC 6902 JSON Patch support for document diffing and patching, plus **Permuto** integration for advanced templating.

## Architecture Overview

- **Computo**: Handles complex programmatic logic (conditionals, loops, calculations, diff/patch operations)
- **Permuto**: Handles simple declarative substitutions and templating using `${path}` syntax
- **JSON Patch**: RFC 6902 compliant diff generation and patch application
- **Code is Data**: All scripts are valid JSON with unambiguous syntax
- **Immutable**: Pure functions that don't modify input data
- **Sandboxed**: No I/O operations or system access

## Quick Start

### Building
```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests (135 tests, 100% passing)
cd build && ctest --verbose
```

### CLI Usage
```bash
# Basic transformation (single input)
./build/computo script.json input.json

# Multiple input files
./build/computo script.json input1.json input2.json input3.json

# Generate JSON Patch from transformation
./build/computo --diff transform_script.json original.json

# With Permuto string interpolation
./build/computo --interpolation script.json input.json

# Available CLI options
./build/computo --missing-key=error --max-depth=32 script.json input.json
```

### Library Usage
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

### System Variables
```json
// Single input access (backward compatible)
["$input"]

// Multiple inputs access (new)
["$inputs"]  // Returns array of all input documents

// Variable access
["$", "/variable_name"]
```

### Essential Operators

#### Data Access & Input Handling
```json
// Access first input (traditional)
["$input"]

// Access all inputs as array (new)
["$inputs"]

// Access specific input by index (new)
["get", ["$inputs"], "/0"]  // First input
["get", ["$inputs"], "/1"]  // Second input

// Variable binding
["let", [["x", 10], ["y", 20]], ["*", ["$", "/x"], ["$", "/y"]]]

// Variable access
["$", "/variable_name"]

// JSON Pointer access
["get", {"key": {"nested": 42}}, "/key/nested"]
```

#### JSON Patch Operations (RFC 6902)
```json
// Generate diff between two documents
["diff", original_doc, modified_doc]
// Returns: [{"op": "replace", "path": "/status", "value": "new"}]

// Apply patch to document
["patch", document, patch_array]
// patch_array must be valid RFC 6902 JSON Patch format

// Example patch operations that can be generated/applied:
// {"op": "add", "path": "/new_field", "value": "new_value"}
// {"op": "remove", "path": "/old_field"}
// {"op": "replace", "path": "/field", "value": "updated_value"}
// {"op": "move", "from": "/old_path", "path": "/new_path"}
// {"op": "copy", "from": "/source", "path": "/destination"}
// {"op": "test", "path": "/field", "value": "expected_value"}
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
- `["approx", a, b, epsilon]` - Approximate equality for floats

### Data Access & Input
- `["$input"]` - Access entire input data (first input if multiple)
- `["$inputs"]` - Access all input documents as array
- `["$", "/path"]` - Access variable by path
- `["get", object, "/json/pointer"]` - Extract data using JSON Pointer
- `["let", bindings, body]` - Create variable scope

### JSON Patch Operations (RFC 6902)
- `["diff", doc_a, doc_b]` - Generate JSON Patch between documents
- `["patch", document, patch_array]` - Apply JSON Patch to document

### Data Construction  
- `["obj", ["key1", val1], ["key2", val2], ...]` - Create object
- `{"array": [item1, item2, ...]}` - Create array (literal syntax)
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

## JSON Patch Workflow Examples

### Basic Diff and Patch
```bash
# Step 1: Create transformation script
echo '["obj", ["id", ["get", ["$input"], "/id"]], ["status", "archived"]]' > transform.json

# Step 2: Generate patch from transformation
echo '{"id": 123, "status": "active"}' > original.json
./build/computo --diff transform.json original.json > patch.json

# Generated patch.json contains:
# [{"op": "replace", "path": "/status", "value": "archived"}]

# Step 3: Create reusable patch application script
echo '["patch", ["get", ["$inputs"], "/0"], ["get", ["$inputs"], "/1"]]' > apply_patch.json

# Step 4: Apply patch to any document
./build/computo apply_patch.json original.json patch.json
# Result: {"id": 123, "status": "archived"}
```

### Advanced Multi-Document Processing
```json
// Script to merge updates from multiple sources
["let", [
    ["original", ["get", ["$inputs"], "/0"]],
    ["updates", ["get", ["$inputs"], "/1"]],
    ["patch_ops", ["diff", ["$", "/original"], ["$", "/updates"]]]
  ],
  ["obj",
    ["original_document", ["$", "/original"]],
    ["updated_document", ["$", "/updates"]],
    ["patch_operations", ["$", "/patch_ops"]],
    ["can_apply_safely", ["==", ["count", ["$", "/patch_ops"]], 1]]
  ]
]
```

### Document Versioning and Rollback
```json
// Generate rollback patch (reverse diff)
["diff", 
  ["get", ["$inputs"], "/1"],  // new version
  ["get", ["$inputs"], "/0"]   // original version
]
// This creates a patch that rolls back from new to original
```

## Multiple Input Processing

### Accessing Multiple Inputs
```json
// Working with multiple data sources
["obj",
  ["user_data", ["get", ["$inputs"], "/0"]],
  ["preferences", ["get", ["$inputs"], "/1"]],
  ["session_info", ["get", ["$inputs"], "/2"]],
  ["total_inputs", ["count", ["$inputs"]]]
]
```

### Data Merging from Multiple Sources
```json
// Merge user profiles from different systems
["let", [
    ["profile1", ["get", ["$inputs"], "/0"]],
    ["profile2", ["get", ["$inputs"], "/1"]]
  ],
  ["obj",
    ["user_id", ["get", ["$", "/profile1"], "/id"]],
    ["name", ["get", ["$", "/profile1"], "/name"]],
    ["email", ["get", ["$", "/profile2"], "/email"]],
    ["preferences", ["get", ["$", "/profile2"], "/settings"]],
    ["last_login", ["get", ["$", "/profile1"], "/last_seen"]]
  ]
]
```

### Cross-Document Validation
```json
// Compare documents for consistency
["let", [
    ["doc1", ["get", ["$inputs"], "/0"]],
    ["doc2", ["get", ["$inputs"], "/1"]],
    ["differences", ["diff", ["$", "/doc1"], ["$", "/doc2"]]]
  ],
  ["obj",
    ["are_identical", ["==", ["count", ["$", "/differences"]], 0]],
    ["difference_count", ["count", ["$", "/differences"]]],
    ["changes_summary", ["$", "/differences"]]
  ]
]
```

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

### Configuration Synchronization
```json
// Sync configuration changes between environments
["let", [
    ["prod_config", ["get", ["$inputs"], "/0"]],
    ["staging_config", ["get", ["$inputs"], "/1"]],
    ["sync_patch", ["diff", ["$", "/prod_config"], ["$", "/staging_config"]]]
  ],
  ["obj",
    ["requires_sync", [">", ["count", ["$", "/sync_patch"]], 0]],
    ["patch_operations", ["$", "/sync_patch"]],
    ["staging_after_sync", ["if", 
      ["$", "/requires_sync"],
      ["patch", ["$", "/staging_config"], ["$", "/sync_patch"]],
      ["$", "/staging_config"]
    ]]
  ]
]
```

### Data Migration Pipeline
```json
// Transform data format between versions
["let", [
    ["old_format", ["get", ["$inputs"], "/0"]],
    ["migration_rules", ["get", ["$inputs"], "/1"]]
  ],
  ["permuto.apply",
    {
      "version": "2.0",
      "user_id": "${/old_format/id}",
      "profile": {
        "full_name": "${/old_format/name}",
        "contact": {
          "email": "${/old_format/email_address}",
          "phone": "${/old_format/phone_number}"
        }
      },
      "settings": "${/migration_rules/default_settings}",
      "migrated_at": "${/migration_rules/timestamp}"
    },
    ["obj",
      ["old_format", ["$", "/old_format"]],
      ["migration_rules", ["$", "/migration_rules"]]
    ]
  ]
]
```

## Error Handling

Computo provides structured exceptions with debugging information:

- `ComputoException` - Base exception class
- `InvalidScriptException` - Malformed JSON scripts
- `InvalidOperatorException` - Unknown operators
- `InvalidArgumentException` - Wrong argument types/counts
- `PatchFailedException` - JSON Patch application failures

### JSON Patch Error Scenarios
```json
// These operations may throw PatchFailedException:

// 1. Invalid patch format
["patch", {"a": 1}, "not_an_array"]  // Throws: patch must be array

// 2. Failed test operation
["patch", {"value": 42}, {"array": [
  {"op": "test", "path": "/value", "value": 99},  // This will fail
  {"op": "replace", "path": "/value", "value": 100}
]}]

// 3. Invalid path in patch
["patch", {"a": 1}, {"array": [
  {"op": "replace", "path": "/nonexistent", "value": 2}
]}]

// 4. Malformed patch operation
["patch", {"a": 1}, {"array": [
  {"op": "invalid_operation", "path": "/a", "value": 2}
]}]
```

## CLI Reference

### Basic Usage
```bash
# Single input transformation
computo script.json input.json

# Multiple input processing  
computo script.json input1.json input2.json input3.json

# No input (script only)
computo script.json
```

### Diff Mode
```bash
# Generate patch from transformation
computo --diff transform_script.json original.json

# Error: --diff requires exactly one input
computo --diff script.json input1.json input2.json  # ❌ Fails
```

### Permuto Integration
```bash
# Enable string interpolation
computo --interpolation script.json input.json

# Permuto options
computo --missing-key=error script.json input.json
computo --max-depth=32 script.json input.json
computo --start="{{{" --end="}}}" script.json input.json
```

### Combined Options
```bash
# Interpolation + diff mode
computo --interpolation --diff transform.json input.json

# Multiple flags
computo --interpolation --missing-key=error --max-depth=16 script.json input.json
```

## Performance & Limits

- **Input Size**: No hard limits, bounded by available memory
- **Recursion Depth**: Configurable via Permuto options (default: 100)
- **Array Operations**: Optimized for large datasets
- **JSON Patch**: Supports all RFC 6902 operations
- **Memory Usage**: Immutable operations create new objects, original data unchanged

## Dependencies

- **nlohmann/json** - Core JSON library with JSON Patch support
- **Permuto** - Template processing library  
- **Google Test** - Testing framework (for development)

## Project Structure

```
computo/
├── CMakeLists.txt           # Build configuration
├── README.md               # This documentation
├── CLAUDE.md                # AI development guidance
├── TECHNICAL_DETAILS.md     # Implementation details
├── PermutoREADME.md        # Permuto documentation
├── book/                   # Learning guide
│   └── 00_index.md
├── cli/                    # Command-line tool
├── include/computo/        # Public headers
├── src/                    # Library implementation
└── tests/                  # Test suite (135 tests, 100% passing)
```

## Development & Testing

- **Test Coverage**: 130 comprehensive tests covering all operators and edge cases
- **JSON Patch Compliance**: Full RFC 6902 implementation with round-trip testing
- **Error Handling**: Comprehensive exception testing for all failure modes
- **Multi-Input Support**: Extensive testing of multiple input scenarios
- **Backward Compatibility**: All existing scripts work unchanged

## Safety & Security

- **Sandboxed execution**: No file system or network access
- **Memory safe**: Built-in recursion limits and cycle detection
- **Input validation**: All operator arguments are validated
- **Type preservation**: Maintains JSON data types throughout transformations
- **Immutable operations**: Original data never modified
- **RFC Compliance**: JSON Patch operations follow standard specifications

## Migration Guide

### From Single Input to Multiple Inputs
```json
// Old way (still works)
["get", ["$input"], "/field"]

// New way for multiple inputs
["get", ["$inputs"], "/0/field"]  // First input
["get", ["$inputs"], "/1/field"]  // Second input

// Backward compatibility guaranteed
// $input is equivalent to: ["get", ["$inputs"], "/0"]
```

### Adding Diff/Patch to Existing Workflows
```json
// Before: Direct transformation
["obj", ["status", "processed"]]

// After: Generate patch for the same transformation
["diff", ["$input"], ["obj", ["status", "processed"]]]

// Or apply external patches
["patch", ["$input"], ["get", ["$inputs"], "/1"]]  // Apply patch from second input
```