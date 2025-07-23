# Unique Operator Algorithm and Implementation

## Overview

The `unique` operator implements a sophisticated sliding window algorithm to extract unique elements from sorted arrays. It provides four distinct modes for handling duplicates and supports both simple value uniqueness and field-based uniqueness for object arrays.

## Key Requirements and Limitations

### Critical Limitation: Pre-sorted Data Required
**The unique operator assumes the input array is already sorted.** This is the primary deficiency of the implementation - it will not work correctly on unsorted data. The algorithm relies on the fact that duplicate values are adjacent to each other.

### Input Format
- Requires array container format: `{"array": [...]}`
- Supports 1-3 arguments: `["unique", array]`, `["unique", array, mode]`, or `["unique", array, field, mode]`

## Algorithm Design

### Core Concept: Sliding Window with Two Booleans

The algorithm uses an elegant sliding window approach with two boolean flags:

- **`left`**: Does the current element equal the previous element?
- **`right`**: Does the current element equal the next element?

This two-boolean state completely characterizes the uniqueness context of any element in a sorted array.

### Algorithm Implementation

```cpp
// Sliding window algorithm with two booleans
bool left = false;  // Current item equals previous item?
bool right;         // Current item equals next item?

for (size_t i = 0; i < array.size(); ++i) {
    // Determine right boolean: does current item equal next item?
    if (i == array.size() - 1) {
        right = false; // Last item, no next item to compare
    } else {
        auto current_key = extract_unique_key(array[i], config.field_pointer);
        auto next_key = extract_unique_key(array[i + 1], config.field_pointer);
        right = (current_key == next_key);
    }

    // Apply mode logic using two booleans
    bool should_output = false;
    if (config.mode == "firsts") {
        should_output = !left;  // First occurrence
    } else if (config.mode == "lasts") {
        should_output = !right; // Last occurrence
    } else if (config.mode == "singles") {
        should_output = !left && !right; // Appears exactly once
    } else if (config.mode == "multiples") {
        should_output = (left || right) && !left; // Has duplicates, first occurrence only
    }

    if (should_output) {
        result.push_back(array[i]);
    }

    // Slide the window: right becomes left for next iteration
    left = right;
}
```

## Four Operating Modes

### 1. "firsts" (default)
- **Logic**: `!left`
- **Behavior**: Returns the first occurrence of each unique value
- **Use case**: Standard deduplication
- **Example**: `[1,1,2,2,3]` → `[1,2,3]`

### 2. "lasts"
- **Logic**: `!right`
- **Behavior**: Returns the last occurrence of each unique value
- **Use case**: Keep most recent version of duplicates
- **Example**: `[1,1,2,2,3]` → `[1,2,3]` (but different objects if using field-based uniqueness)

### 3. "singles"
- **Logic**: `!left && !right`
- **Behavior**: Returns only elements that appear exactly once
- **Use case**: Find non-duplicated items
- **Example**: `[1,1,2,2,3]` → `[3]`

### 4. "multiples"
- **Logic**: `(left || right) && !left`
- **Behavior**: Returns first occurrence of elements that have duplicates
- **Use case**: Find items that do have duplicates
- **Example**: `[1,1,2,2,3]` → `[1,2]`

## Field-Based Uniqueness

### JSON Pointer Support
The operator supports field-based uniqueness using JSON Pointer syntax:

```json
["unique", {"array": [
    {"name": "alice", "dept": "eng"},
    {"name": "alice", "dept": "sales"},
    {"name": "bob", "dept": "hr"}
]}, "/name"]
```

### Key Extraction
```cpp
auto extract_unique_key(const nlohmann::json& element, const std::string& field_pointer) -> nlohmann::json {
    if (field_pointer.empty()) {
        return element;  // Simple value uniqueness
    }
    return extract_field_value(element, field_pointer);  // Field-based uniqueness
}
```

### Field Access Implementation
```cpp
auto extract_field_value(const nlohmann::json& obj, const std::string& pointer) -> nlohmann::json {
    if (pointer.empty()) {
        return obj;
    }
    
    try {
        return obj.at(nlohmann::json::json_pointer(pointer));
    } catch (const std::exception&) {
        return {nullptr};  // Field not found, return null
    }
}
```

## Configuration Parsing

### Argument Parsing Logic
```cpp
auto parse_unique_arguments(const nlohmann::json& args) -> UniqueConfig {
    UniqueConfig config;
    
    if (args.size() == 1) {
        config.mode = "firsts";
    } else if (args.size() == 2) {
        std::string second_arg = args[1].get<std::string>();
        if (second_arg == "firsts" || second_arg == "lasts" || 
            second_arg == "multiples" || second_arg == "singles") {
            config.mode = second_arg;
        } else if (second_arg.empty() || second_arg[0] == '/') {
            config.field_pointer = second_arg;
            config.mode = "firsts";
        }
    } else if (args.size() == 3) {
        config.field_pointer = args[1].get<std::string>();
        config.mode = args[2].get<std::string>();
    }
    
    return config;
}
```

## Edge Cases

### Empty Array
- Returns empty array for all modes
- No processing required

### Single Element
- Always included in result for all modes
- Cannot have duplicates by definition

### Missing Fields
- Uses null value for missing object fields
- Consistent with JSON Pointer standard behavior

## Performance Characteristics

### Time Complexity
- **O(n)** single pass through the array
- No additional sorting or multiple passes required
- Minimal memory overhead

### Space Complexity
- **O(k)** where k is the number of unique elements to output
- In-place processing with minimal temporary storage

## Key Strengths

1. **Efficiency**: Single-pass O(n) algorithm
2. **Flexibility**: Four distinct modes for different use cases
3. **Field Support**: JSON Pointer integration for object arrays
4. **Minimal Memory**: No need to store frequency counts or build lookup tables
5. **Elegant Logic**: Two-boolean state machine is simple and maintainable

## Primary Deficiency

**Requires Pre-sorted Input**: The algorithm's fundamental limitation is that it assumes the input array is already sorted. This means:

1. Duplicates must be adjacent for the algorithm to work correctly
2. Unsorted arrays will produce incorrect results
3. The caller must ensure sorting before calling `unique`
4. This creates a dependency on the `sort` operator in most real-world scenarios

## Recommended Usage Pattern

```json
["unique", ["sort", {"array": [3,1,4,1,5,9,2,6,5,3]}]]
```

Always combine with `sort` operator unless you can guarantee the input is already sorted.

## Implementation Location

- **File**: `src/operators/string_array.cpp:454-519`
- **Dependencies**: JSON Pointer library, `extract_field_value` function
- **Tests**: `tests/test_string_array_operations.cpp` (comprehensive test coverage)