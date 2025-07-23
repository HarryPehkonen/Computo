# Array Handling Architecture Guide

## Overview

This document describes the comprehensive architecture for handling arrays and array objects in Computo, designed from scratch for optimal performance, clarity, and correctness.

## Design Philosophy

### The Problem
Computo uses JSON array syntax `["operator", arg1, arg2, ...]` for operator calls, where the first element must be a known operator string. This creates ambiguity: how do you represent a literal array that happens to start with an operator name?

### The Solution
The `{"array": [...]}` syntax serves as an "escape hatch" that forces literal interpretation of array contents, bypassing normal operator evaluation rules.

### Goals
- **Performance**: Minimize overhead for common array operations
- **Safety**: Protect against typos in operator names
- **Clarity**: Make intent explicit and predictable
- **Simplicity**: Reduce special case handling where possible

## Core Evaluation Rules

### Rule 1: Arrays with Known Operator Strings
**Pattern**: `["known_operator", arg1, arg2, ...]`  
**Behavior**: Evaluate as operator call

```json
["+", 1, 2]           → 3
["map", [1,2,3], fn]  → (result of map operation)
```

### Rule 2: Arrays with Unknown Operator Strings  
**Pattern**: `["unknown_string", ...]`  
**Behavior**: **ERROR** - Unknown operator

```json
["hello", "world"]    → ERROR: "hello" is not a known operator
["redcue", arr, fn]   → ERROR: "redcue" is not a known operator (typo protection)
```

### Rule 3: Arrays with Non-String First Elements
**Pattern**: `[non_string, ...]`  
**Behavior**: Treat as literal array

```json
[1, 2, 3]             → [1, 2, 3]
[[], "hello"]         → [[], "hello"]
[null, true, false]   → [null, true, false]
```

### Rule 4: Array Objects (Escape Hatch)
**Pattern**: `{"array": [...]}`  
**Behavior**: Force literal treatment, bypass all evaluation

```json
{"array": ["+", 1, 2]}        → ["+", 1, 2]
{"array": ["hello", "world"]} → ["hello", "world"]
{"array": [1, 2, 3]}          → [1, 2, 3]
```

## Evaluation Semantics

### First-Level vs Embedded-Level Processing

**First Level**: Direct children of `{"array": [...]}` are treated literally  
**Embedded Level**: Deeper nesting follows normal evaluation rules

```json
// First level escaped, embedded level evaluated
{"array": [["+", 1, 2]]}                    → [3]

// First level escaped, embedded array object evaluated  
{"array": [{"array": [1, 2]}, 3]}           → [[1, 2], 3]

// Complex nesting
{"array": [{"array": [["+", 1, 2]]}]}       → [[3]]
```

### Nested Array Object Behavior

Array objects at embedded levels are evaluated normally (unwrapped):

```json
// Outer array object: first level escaped
// Inner array object: embedded level, gets evaluated/unwrapped
{
  "array": [
    {"array": [1, 2]},    // Becomes [1, 2]
    3,
    {"array": [4, 5]}     // Becomes [4, 5]
  ]
}
→ [[1, 2], 3, [4, 5]]
```

## Operator Integration

### Input Handling
Operators should handle both regular arrays and array objects transparently:

```javascript
// Pseudo-code for operator implementation
function map_operator(array_input, lambda) {
    let is_array_object = false;
    let actual_array = array_input;
    
    // Check if input is array object
    if (is_object(array_input) && has_key(array_input, array_key)) {
        actual_array = array_input[array_key];
        is_array_object = true;
        
        // Validate array object structure
        if (!is_array(actual_array)) {
            throw new Error(`Malformed array object: "${array_key}" must contain an array`);
        }
    }
    
    // Perform operation on actual array
    let result = actual_array.map(lambda);
    
    // Return in same format as input
    return is_array_object ? {[array_key]: result} : result;
}
```

### Return Value Conventions
For performance, operators return raw arrays unless input was an array object:

```json
// Input is array object → output is array object
["map", {"array": [1,2,3]}, ["+", ["$"], 1]]  → {"array": [2,3,4]}

// Input is regular array → output is regular array  
["map", [1,2,3], ["+", ["$"], 1]]             → [2,3,4]

// Mixed inputs follow the "array object wins" rule
["concat", {"array": [1,2]}, [3,4]]           → {"array": [1,2,3,4]}
```

### Syntactic Sugar for Mixed Operations
When an operator expects array inputs and encounters a regular array mixed with array objects, the regular array is auto-promoted:

```json
// Auto-promotion: [3,4] becomes {"array": [3,4]} 
["concat", {"array": [1,2]}, [3,4]]           → {"array": [1,2,3,4]}
```

**Auto-promotion occurs when**: Regular array has non-string first element OR unknown operator string as first element.

## Custom Array Keys

### Configuration
Use `--array="custom_key"` to change the array object identifier:

```bash
computo script.json input.json --array="@array"
```

### Behavior with Custom Keys
All array object syntax uses the custom key:

```json
// With --array="@array"
{"@array": ["+", 1, 2]}                      → ["+", 1, 2]
["map", {"@array": [1,2,3]}, lambda]         → {"@array": [result]}
```

### Use Case
Enables output of literal `{"array": ...}` objects when needed:

```json
// Without custom key: impossible to output {"array": [1,2,3]}
// With --array="@array": can output {"array": [1,2,3]} using {"@array": [{"array": [1,2,3]}]}
```

## Error Handling

### Malformed Array Objects
```json
{"array": "not an array"}    → ERROR: Malformed array object: "array" must contain an array
{"array": null}              → ERROR: Malformed array object: "array" must contain an array  
{"array": 123}               → ERROR: Malformed array object: "array" must contain an array
```

### Unknown Operators
```json
["unknwon", 1, 2]           → ERROR: Unknown operator "unknwon". Did you mean "unknown"?
["redcue", arr, fn]         → ERROR: Unknown operator "redcue". Did you mean "reduce"?
```

### Validation Points
1. **Parse time**: Validate array object structure
2. **Evaluation time**: Check operator existence
3. **Operator entry**: Validate expected argument types

## Performance Considerations

### Runtime Optimization Flag
Use `--simple-arrays` to disable array object processing entirely:

```bash
# Maximum performance mode - no array object support
computo script.json input.json --simple-arrays
```

When enabled:
- All arrays are treated as literals (Rule 3 only)
- `{"array": [...]}` treated as regular objects
- Operator calls must use known operators (Rule 1 + Rule 2)
- Zero overhead for array object checking

### Performance Optimizations

1. **Fast Path Detection**: Check for array objects only when object has single key matching array key
2. **Operator Registry Caching**: Cache operator lookups to avoid repeated string comparisons  
3. **Lazy Evaluation**: Only validate array object contents when actually processed
4. **Zero-Copy Operations**: Pass array references rather than copying when possible

### Implementation Strategy
```javascript
// Optimized array object detection
function is_array_object(value, array_key) {
    return (
        typeof value === 'object' &&
        value !== null &&
        !Array.isArray(value) &&
        Object.keys(value).length === 1 &&
        array_key in value
    );
}
```

## Implementation Architecture

### Core Components

#### 1. Parser/Evaluator
```javascript
function evaluate(expr, context, array_key = "array") {
    // Fast path: non-arrays
    if (!Array.isArray(expr)) {
        // Check for array objects
        if (is_array_object(expr, array_key)) {
            return evaluate_array_object(expr[array_key], context, array_key);
        }
        return expr;
    }
    
    // Array evaluation logic
    if (expr.length === 0) return expr;
    
    const first = expr[0];
    
    // Rule 1 & 2: String first element  
    if (typeof first === 'string') {
        if (is_known_operator(first)) {
            return evaluate_operator_call(first, expr.slice(1), context);
        } else {
            throw new Error(`Unknown operator "${first}"`);
        }
    }
    
    // Rule 3: Non-string first element
    return expr.map(elem => evaluate(elem, context, array_key));
}
```

#### 2. Array Object Processor
```javascript
function evaluate_array_object(array_contents, context, array_key) {
    // Validate structure
    if (!Array.isArray(array_contents)) {
        throw new Error(`Malformed array object: "${array_key}" must contain an array`);
    }
    
    // Process contents with normal evaluation (embedded level)
    const processed = array_contents.map(elem => evaluate(elem, context, array_key));
    
    // Return unwrapped result
    return processed;
}
```

#### 3. Operator Implementation Pattern
```javascript
function create_array_operator(operation) {
    return function(array_input, ...args) {
        const {actual_array, is_array_object} = unwrap_array_input(array_input);
        const result = operation(actual_array, ...args);
        return is_array_object ? {[current_array_key]: result} : result;
    };
}
```

## Edge Cases and Solutions

### Case 1: Empty Arrays
```json
[]                              → []
{"array": []}                   → []
```

### Case 2: Single Element Arrays  
```json
[1]                             → [1]
["+"]                           → ERROR: Unknown operator "+" (not enough args)
{"array": ["+"]}                → ["+"]
```

### Case 3: Deeply Nested Structures
```json
{
  "array": [
    {
      "array": [
        ["+", 1, 2],
        {"array": ["*", 3, 4]}
      ]
    }
  ]
}
→ [[3, ["*", 3, 4]]]
```

### Case 4: Mixed Data Types
```json
["concat", {"array": [1, "hello"]}, [true, null]]  → {"array": [1, "hello", true, null]}
```

### Case 5: Custom Array Keys in Nested Structures
```json
// With --array="@arr"
{
  "@arr": [
    {"@arr": [1, 2]},
    {"array": [3, 4]}    // This is NOT an array object, it's a literal object
  ]
}
→ [[1, 2], {"array": [3, 4]}]
```

## Migration and Compatibility

### Current Code Assessment
Existing implementations should be audited for:
- Inconsistent array object preservation across operators
- Missing validation of array object structure  
- Complex evaluation heuristics that can be simplified
- Performance overhead in non-array-object cases

### Recommended Migration Steps
1. Implement new evaluation rules with feature flag
2. Add comprehensive error handling and validation
3. Optimize performance with fast-path detection
4. Update operator implementations for consistency
5. Add extensive test coverage for edge cases
6. Enable new system by default

### Testing Strategy
- **Unit Tests**: Each evaluation rule with comprehensive examples
- **Integration Tests**: Complex nested scenarios  
- **Performance Tests**: Benchmark against current implementation
- **Error Tests**: All malformed input scenarios
- **Operator Tests**: Consistent behavior across all array operators

## Conclusion

This architecture provides a clean, performant, and predictable system for array handling that:
- Eliminates ambiguity between operator calls and literal arrays
- Provides safety through typo detection  
- Offers performance optimization opportunities
- Maintains simplicity in common cases
- Supports advanced use cases through escape mechanisms

The key insight is treating `{"array": [...]}` as a true escape hatch rather than a general-purpose array container, allowing most array operations to work with simple, performant native arrays while providing the flexibility needed for edge cases.
