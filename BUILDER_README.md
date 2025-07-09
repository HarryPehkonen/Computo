# ComputoBuilder - Fluent API for Test Construction

The ComputoBuilder provides a fluent, type-safe API for constructing Computo JSON expressions in C++ tests, eliminating the verbose and error-prone manual JSON construction.

## Quick Start

```cpp
#include <computo/computo.hpp>
using CB = computo::ComputoBuilder;

// Instead of: json{{"array", json::array({1, 2, 3})}}
auto array_script = CB::array({1, 2, 3});

// Instead of: json::array({"+", 2, 3})
auto add_script = CB::add(2, 3);

// Instead of complex nested JSON
auto complex_script = CB::map(
    CB::array({1, 2, 3}),
    CB::lambda("x", CB::add(CB::var("x"), 1))
);
```

## Key Benefits

### 1. **Readability**
```cpp
// OLD: Unreadable nested JSON
json old_syntax = json::array({
    "map",
    json{{"array", json::array({1, 2, 3})}},
    json::array({"lambda", json::array({"x"}), json::array({"+", json::array({"$", "/x"}), 1})})
});

// NEW: Self-documenting code
auto new_syntax = CB::map(
    CB::array({1, 2, 3}),
    CB::lambda("x", CB::add(CB::var("x"), 1))
);
```

### 2. **Type Safety**
- Compile-time error checking
- IDE autocomplete support
- No manual bracket balancing

### 3. **Pain Points Solved**
- GOOD: `json{{"array", json::array({1,2,3})}}` → `CB::array({1,2,3})`
- GOOD: `json::array({"+", a, b})` → `CB::add(a, b)`
- GOOD: `json::array({"$", "/x"})` → `CB::var("x")`
- BENEFIT: Complex nesting becomes readable
- BENEFIT: Template argument deduction issues eliminated
- BENEFIT: Macro conflicts avoided

## API Reference

### Literals
```cpp
CB::number(3.14)
CB::string("hello")
CB::boolean(true)
CB::null()
```

### Arrays
```cpp
CB::array({1, 2, 3})
CB::empty_array()
```

### Arithmetic
```cpp
CB::add(2, 3)
CB::add({1, 2, 3, 4})  // N-ary addition
CB::subtract(5, 3)
CB::multiply(4, 3)
CB::divide(10, 2)
```

### Object Construction
```cpp
CB::obj()
    .add_field("name", "test")
    .add_field("value", 42)
    .add_field("computed", CB::add(10, 5))
```

### Variables and Input
```cpp
CB::input()           // ["$input"]
CB::var("x")          // ["$", "/x"]
CB::get(CB::input(), "/path")  // JSON Pointer access
```

### Control Flow
```cpp
CB::if_then_else(condition, then_expr, else_expr)
CB::let({{"x", 10}, {"y", 20}}, CB::add(CB::var("x"), CB::var("y")))
```

### Lambdas and Array Operations
```cpp
CB::lambda("x", CB::add(CB::var("x"), 1))
CB::map(array_expr, lambda_expr)
CB::filter(array_expr, lambda_expr)
CB::count(array_expr)
```

### Comparison
```cpp
CB::equal(a, b)
CB::not_equal(a, b)
CB::less_than(a, b)
CB::greater_than(a, b)
```

### Generic Operators
```cpp
CB::op("custom_operator").arg(value1).arg(value2)
CB::op("map") << CB::array({1,2,3}) << lambda_expr  // Fluent chaining
```

## Usage in Tests

### Basic Test Pattern
```cpp
TEST_F(ComputoTest, ExampleTest) {
    json input = {{"numbers", {1, 2, 3}}};
    
    auto script = CB::map(
        CB::get(CB::input(), "/numbers"),
        CB::lambda("x", CB::multiply(CB::var("x"), 2))
    );
    
    json expected = {2, 4, 6};
    EXPECT_EQ(computo::execute(script, input), expected);
}
```

### Complex Nested Operations
```cpp
TEST_F(ComputoTest, ComplexExample) {
    auto script = CB::let(
        {
            {"data", CB::get(CB::input(), "/numbers")},
            {"threshold", 3}
        },
        CB::map(
            CB::filter(
                CB::var("data"), 
                CB::lambda("x", CB::greater_than(CB::var("x"), CB::var("threshold")))
            ),
            CB::lambda("x", CB::multiply(CB::var("x"), 2))
        )
    );
    
    // Clear intent: filter numbers > 3, then double them
}
```

## Implementation Files

- **Header**: `include/computo/builder.hpp`
- **Integration**: Included via `#include <computo/computo.hpp>`
- **Tests**: `tests/test_builder.cpp` and `tests/builder_examples.cpp`
- **Alias**: `using CB = computo::ComputoBuilder;` for brevity

## Integration

The builder is automatically included when you include the main computo header:

```cpp
#include <computo/computo.hpp>  // Includes builder.hpp
using CB = computo::ComputoBuilder;
```

## Current Status

- COMPLETE: Core arithmetic, array, and object operations
- COMPLETE: Lambda expressions and variable binding
- COMPLETE: Input access and JSON Pointer operations
- COMPLETE: Conditional logic and comparisons
- COMPLETE: Fluent chaining with `<<` operator
- WARNING: Some advanced operators may need implementation in core engine
- WARNING: String literal forms for special operators not yet supported

## Future Enhancements

- Template-based automatic type conversion
- More specialized operator methods
- Integration with testing frameworks for better error messages
- Support for all Computo operators as they're implemented

The ComputoBuilder successfully eliminates the major pain points identified in LESSONS.md while maintaining full compatibility with the existing Computo JSON format.