# Sort Operator Algorithm and Implementation

## Overview

The `sort` operator provides a comprehensive sorting solution for both simple arrays and complex object arrays. It features multiple API patterns, field-based sorting with JSON Pointer syntax, multi-field sorting capabilities, and type-aware comparison for mixed-type arrays.

## Core Architecture

### Dual Sorting Modes

The implementation supports two distinct sorting approaches:

1. **Simple Array Sorting**: Direct value comparison with optional direction
2. **Object Array Sorting**: Field-based sorting with JSON Pointer navigation and multi-field support

### Input Format
- Requires array container format: `{"array": [...]}`
- Flexible argument patterns supporting 1-N arguments

## API Patterns

### Simple Array Sorting
```json
["sort", {"array": [3, 1, 4, 1, 5]}]                    // Ascending (default)
["sort", {"array": [3, 1, 4, 1, 5]}, "desc"]            // Descending
["sort", {"array": [3, 1, 4, 1, 5]}, "asc"]             // Explicit ascending
```

### Object Array Sorting
```json
["sort", {"array": [...]}, "/field"]                     // Single field, ascending
["sort", {"array": [...]}, ["/field", "desc"]]          // Single field, descending
["sort", {"array": [...]}, "/name", ["/age", "desc"]]   // Multi-field
```

## Configuration Architecture

### Field Descriptor Structure
```cpp
struct FieldDescriptor {
    std::string pointer;    // JSON Pointer path
    bool ascending = true;  // Sort direction
};
```

### Sort Configuration
```cpp
struct SortConfig {
    bool is_simple_array;                        // Determines sorting mode
    std::string direction;                       // For simple arrays
    std::vector<FieldDescriptor> fields;         // For object arrays
};
```

## Argument Parsing Algorithm

### Intelligent Mode Detection
```cpp
auto parse_sort_arguments(const nlohmann::json& args) -> SortConfig {
    SortConfig config;
    
    if (args.size() == 1) {
        // Simple array sort, ascending
        config.is_simple_array = true;
        config.direction = "asc";
    } else if (args.size() == 2) {
        // Disambiguation logic
        if (args[1].is_string()) {
            std::string second_arg = args[1].get<std::string>();
            if (second_arg == "desc" || second_arg == "asc") {
                // Simple array with direction
                config.is_simple_array = true;
                config.direction = second_arg;
            } else if (second_arg.empty() || second_arg[0] == '/') {
                // Object array with JSON Pointer field
                config.is_simple_array = false;
                config.fields.push_back(parse_field_descriptor(args[1]));
            }
        } else {
            // Object array with array-format field descriptor
            config.is_simple_array = false;
            config.fields.push_back(parse_field_descriptor(args[1]));
        }
    } else {
        // Multiple fields for object sorting
        config.is_simple_array = false;
        for (size_t i = 1; i < args.size(); ++i) {
            config.fields.push_back(parse_field_descriptor(args[i]));
        }
    }
    
    return config;
}
```

### Field Descriptor Parsing
```cpp
auto parse_field_descriptor(const nlohmann::json& field_spec) -> FieldDescriptor {
    FieldDescriptor desc;
    
    if (field_spec.is_string()) {
        // Simple string field: "/field" -> ascending
        desc.pointer = field_spec.get<std::string>();
        desc.ascending = true;
    } else if (field_spec.is_array()) {
        // Array format: ["/field", "desc"] or ["/field", "asc"]
        desc.pointer = field_spec[0].get<std::string>();
        desc.ascending = true; // default
        
        if (field_spec.size() > 1) {
            std::string direction = field_spec[1].get<std::string>();
            desc.ascending = (direction != "desc");
        }
    }
    
    return desc;
}
```

## Type-Aware Comparison System

### Type Hierarchy
The implementation enforces a consistent type ordering to handle mixed-type arrays:

```
null < numbers < strings < booleans < arrays < objects
```

### Comparison Algorithm
```cpp
auto type_aware_compare(const nlohmann::json& a, const nlohmann::json& b) -> int {
    // Type ordering function
    auto get_type_order = [](const nlohmann::json& val) {
        if (val.is_null()) return 0;
        if (val.is_number()) return 1;
        if (val.is_string()) return 2;
        if (val.is_boolean()) return 3;
        if (val.is_array()) return 4;
        if (val.is_object()) return 5;
        return 6;
    };

    int type_a = get_type_order(a);
    int type_b = get_type_order(b);

    if (type_a != type_b) {
        return type_a - type_b;  // Different types
    }

    // Same type, compare values
    if (a.is_number()) {
        double val_a = a.get<double>();
        double val_b = b.get<double>();
        return (val_a < val_b) ? -1 : (val_a > val_b) ? 1 : 0;
    }
    if (a.is_string()) {
        return a.get<std::string>().compare(b.get<std::string>());
    }
    // ... additional type-specific comparisons
}
```

## Field Access and Navigation

### JSON Pointer Integration
```cpp
auto extract_field_value(const nlohmann::json& obj, const std::string& pointer) -> nlohmann::json {
    if (pointer.empty()) {
        return obj;  // Empty pointer means the value itself
    }
    
    try {
        return obj.at(nlohmann::json::json_pointer(pointer));
    } catch (const std::exception&) {
        return {nullptr};  // Field not found, return null
    }
}
```

### Nested Field Support
The implementation supports arbitrarily deep field navigation:
- `/user/name` - Navigate to nested object field
- `/scores/0` - Access array elements by index
- `/metadata/tags/2/label` - Complex nested paths

## Multi-Field Sorting

### Comparator Generation
```cpp
auto create_multi_field_comparator(const std::vector<FieldDescriptor>& fields) 
    -> std::function<bool(const nlohmann::json&, const nlohmann::json&)> {
    
    return [fields](const nlohmann::json& a, const nlohmann::json& b) -> bool {
        for (const auto& field : fields) {
            auto val_a = extract_field_value(a, field.pointer);
            auto val_b = extract_field_value(b, field.pointer);
            
            int cmp = type_aware_compare(val_a, val_b);
            if (cmp != 0) {
                return field.ascending ? (cmp < 0) : (cmp > 0);
            }
            // Fields are equal, continue to next field
        }
        return false; // All fields equal
    };
}
```

### Sequential Field Processing
The multi-field algorithm processes fields in order:
1. Compare by first field
2. If equal, compare by second field
3. Continue until difference found or all fields exhausted

## Main Sort Implementation

### Orchestration Logic
```cpp
auto sort_op(const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    // Validate arguments and extract array
    auto array_container = evaluate(args[0], ctx);
    auto array = array_container["array"];
    
    // Parse arguments to determine sorting strategy
    SortConfig config = parse_sort_arguments(args);
    
    // Make a copy to sort
    auto result = array;
    
    if (config.is_simple_array) {
        // Simple array sorting
        bool ascending = (config.direction == "asc");
        std::sort(result.begin(), result.end(), 
            [ascending](const nlohmann::json& a, const nlohmann::json& b) {
                int cmp = type_aware_compare(a, b);
                return ascending ? (cmp < 0) : (cmp > 0);
            });
    } else {
        // Object array sorting with fields
        auto comparator = create_multi_field_comparator(config.fields);
        std::sort(result.begin(), result.end(), comparator);
    }
    
    return nlohmann::json::object({ { "array", result } });
}
```

## Edge Cases and Error Handling

### Missing Fields
- Missing object fields return `null` values
- `null` values sort before all other types
- Provides predictable behavior for incomplete data

### Type Consistency
- Mixed-type arrays are handled gracefully
- Type hierarchy ensures stable, predictable ordering
- No runtime type errors during comparison

### Empty Arrays
- Empty arrays are handled without special cases
- Return empty result immediately

## Performance Characteristics

### Time Complexity
- **Simple Arrays**: O(n log n) - standard comparison sort
- **Object Arrays**: O(n log n × f) where f is the number of fields
- **Field Access**: O(d) where d is the JSON Pointer depth

### Space Complexity
- **O(n)** for result array copy
- **O(1)** additional space for comparators
- No significant memory overhead for field descriptors

### Algorithm Choice
Uses `std::sort` (typically Introsort):
- Average case: O(n log n)
- Worst case: O(n log n) 
- Stable: No (but this can be addressed if needed)

## Key Strengths

1. **Flexible API**: Multiple calling patterns for different use cases
2. **Type Safety**: Consistent handling of mixed-type data
3. **JSON Pointer Integration**: Standard path syntax for field navigation
4. **Multi-Field Support**: Complex sorting requirements with multiple criteria
5. **Performance**: Optimal O(n log n) complexity
6. **Robustness**: Graceful handling of missing fields and edge cases

## Potential Deficiencies

### Stability
- Uses `std::sort` which is not stable
- Relative order of equal elements may change
- Could be addressed by switching to `std::stable_sort` if needed

### Field Validation
- No compile-time validation of JSON Pointer paths
- Invalid paths silently return `null` rather than error
- Could benefit from optional strict mode for validation

### Memory Usage
- Creates full copy of input array for sorting
- Could be optimized for in-place sorting in some scenarios
- Trade-off between safety and memory efficiency

## Usage Examples

### Simple Sorting
```json
// Numbers
["sort", {"array": [3, 1, 4, 1, 5]}]
// Result: [1, 1, 3, 4, 5]

// Mixed types
["sort", {"array": [null, 42, "hello", true, []]}]
// Result: [null, 42, "hello", true, []]
```

### Object Sorting
```json
// Single field
["sort", {"array": [
    {"name": "charlie", "age": 30},
    {"name": "alice", "age": 25},
    {"name": "bob", "age": 35}
]}, "/name"]
// Result: sorted by name ascending

// Multi-field
["sort", {"array": [...]}, "/dept", ["/salary", "desc"]]
// Result: sorted by department ascending, then salary descending
```

### Complex Nested Fields
```json
["sort", {"array": [
    {"user": {"profile": {"score": 85}}},
    {"user": {"profile": {"score": 92}}},
    {"user": {"profile": {"score": 78}}}
]}, "/user/profile/score"]
// Result: sorted by nested score field
```

## Implementation Location

- **File**: `src/operators/string_array.cpp:157-380`
- **Dependencies**: `std::sort`, JSON Pointer library
- **Tests**: `tests/test_string_array_operations.cpp` (comprehensive coverage including edge cases)
- **Related**: Works optimally with `unique` operator for complete deduplication workflows