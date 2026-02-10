# Computo Language Reference

This document provides complete specifications for all Computo operators, generated from validated examples.

## Operator Categories

## Arithmetic Operators

### `+` - Addition (n-ary)

**Syntax:** `["+", number, number, ...]`

**Examples:**

**Basic addition:**
```json
["+", 1, 2, 3]
```
*Result:* `6`

**Single argument (identity):**
```json
["+", 5]
```
*Result:* `5`

**Multi-input addition:**
```json
["+", ["$inputs", "/0"], ["$inputs", "/1"]]
```
*Inputs:* [10, 20]
*Result:* `30`

### `-` - Subtraction (n-ary) or unary negation

**Syntax:** `["-", number, number, ...] or ["-", number]`

**Examples:**

**Basic subtraction:**
```json
["-", 10, 3, 2]
```
*Result:* `5`

**Unary negation:**
```json
["-", 5]
```
*Result:* `-5`

### `*` - Multiplication (n-ary)

**Syntax:** `["*", number, number, ...]`

**Examples:**

**Basic multiplication:**
```json
["*", 2, 3, 4]
```
*Result:* `24`

### `/` - Division (n-ary) or reciprocal

**Syntax:** `["/", number, number, ...] or ["/", number]`

**Examples:**

**Basic division:**
```json
["/", 20, 2, 2]
```
*Result:* `5`

**Reciprocal:**
```json
["/", 4]
```
*Result:* `0.25`

### `%` - Modulo operation

**Syntax:** `["%", number, number]`

**Examples:**

**Basic modulo:**
```json
["%", 20, 6]
```
*Result:* `2`


## Comparison Operators

### `>` - Greater than (supports chaining)

**Syntax:** `[">", number, number, ...]`

**Examples:**

**Chained comparison:**
```json
[">", 5, 3, 1]
```
*Result:* `true`

### `<` - Less than (supports chaining)

**Syntax:** `["<", number, number, ...]`

**Examples:**

**Chained comparison:**
```json
["<", 1, 3, 5]
```
*Result:* `true`

### `>=` - Greater than or equal (supports chaining)

**Syntax:** `[">=", number, number, ...]`

**Examples:**

**Chained comparison:**
```json
[">=", 5, 5, 3]
```
*Result:* `true`

### `<=` - Less than or equal (supports chaining)

**Syntax:** `["<=", number, number, ...]`

**Examples:**

**Chained comparison:**
```json
["<=", 1, 2, 2]
```
*Result:* `true`

### `==` - Equality (all arguments must be equal)

**Syntax:** `["==", value, value, ...]`

**Examples:**

**All equal:**
```json
["==", 2, 2, 2]
```
*Result:* `true`

### `!=` - Inequality (binary only)

**Syntax:** `["!=", value, value]`

**Examples:**

**Not equal:**
```json
["!=", 1, 2]
```
*Result:* `true`


## Logical Operators

### `and` - Logical AND (all must be truthy)

**Syntax:** `["and", expr, expr, ...]`

**Examples:**

**All truthy:**
```json
["and", true, 1, "non-empty"]
```
*Result:* `true`

### `or` - Logical OR (any must be truthy)

**Syntax:** `["or", expr, expr, ...]`

**Examples:**

**Any truthy:**
```json
["or", false, 0, 3]
```
*Result:* `true`

### `not` - Logical NOT (unary)

**Syntax:** `["not", expr]`

**Examples:**

**Negation:**
```json
["not", false]
```
*Result:* `true`


## Data Access Operators

### `$input` - Access input data

**Syntax:** `["$input"] or ["$input", json_pointer]`

**Examples:**

**Full input:**
```json
["$input"]
```
*Inputs:* [{"name": "Alice", "age": 30}]
*Result:* `{"name": "Alice", "age": 30}`

**Specific field:**
```json
["$input", "/name"]
```
*Inputs:* [{"name": "Alice", "age": 30}]
*Result:* `"Alice"`

**Nested access:**
```json
["$input", "/users/0/name"]
```
*Inputs:* [{"users": [{"name": "Bob", "age": 25}]}]
*Result:* `"Bob"`

### `$inputs` - Access multiple input files

**Syntax:** `["$inputs"] or ["$inputs", json_pointer]`

**Examples:**

**All inputs:**
```json
["$inputs"]
```
*Inputs:* [{"name": "Alice"}, {"name": "Bob"}]
*Result:* `[{"name": "Alice"}, {"name": "Bob"}]`

**Specific input:**
```json
["$inputs", "/1"]
```
*Inputs:* [{"name": "Alice"}, {"name": "Bob"}]
*Result:* `{"name": "Bob"}`

**Nested in specific input:**
```json
["$inputs", "/1/name"]
```
*Inputs:* [{"name": "Alice", "age": 30}, {"name": "Bob", "age": 25}]
*Result:* `"Bob"`

### `$` - Variable access

**Syntax:** `["$", json_pointer]`

**Examples:**

**Variable access:**
```json
["let", {"x": 42}, ["$", "/x"]]
```
*Result:* `42`

### `let` - Variable binding

**Syntax:** `["let", bindings, expression]`

**Examples:**

**Object syntax:**
```json
["let", {"x": 10, "y": 20}, ["+", ["$", "/x"], ["$", "/y"]]]
```
*Result:* `30`

**Array syntax:**
```json
["let", [["x", 10]], ["+", ["$", "/x"], 5]]
```
*Result:* `15`

**With input data:**
```json
["let", {"user": ["$input", "/users/0"]}, ["$", "/user/name"]]
```
*Inputs:* [{"users": [{"name": "Alice", "age": 30}]}]
*Result:* `"Alice"`


## Control Flow Operators

### `if` - Conditional expression

**Syntax:** `["if", condition, then_expr, else_expr]`

**Examples:**

**True condition:**
```json
["if", [">", 5, 3], "yes", "no"]
```
*Result:* `"yes"`

**False condition:**
```json
["if", ["<", 5, 3], "yes", "no"]
```
*Result:* `"no"`

### `lambda` - Lambda function

**Syntax:** `["lambda", [param1, param2, ...], body]`

**Examples:**

**No parameters:**
```json
["lambda", [], 42]
```
*Result:* `[[], 42]`

**One parameter:**
```json
["lambda", ["x"], ["+", ["$", "/x"], 1]]
```
*Result:* `[["x"], ["+", ["$", "/x"], 1]]`

**Multiple parameters:**
```json
["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]]
```
*Result:* `[["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]]`


## Object Operations Operators

### `obj` - Object construction (dynamic keys and values)

**Syntax:** `["obj", key_expr, value_expr, ...]`

**Examples:**

**Static keys:**
```json
["obj", "name", "Alice", "age", 30]
```
*Result:* `{"name": "Alice", "age": 30}`

**Dynamic key:**
```json
["obj", ["strConcat", "user_", "123"], "Alice"]
```
*Result:* `{"user_123": "Alice"}`

**Variable-based construction:**
```json
["let", {"key": "name", "value": "Bob"}, ["obj", ["$", "/key"], ["$", "/value"]]]
```
*Result:* `{"name": "Bob"}`

### `keys` - Get object keys

**Syntax:** `["keys", object]`

**Examples:**

**Object keys:**
```json
["keys", {"a": 1, "b": 2}]
```
*Result:* `{"array": ["a", "b"]}`

### `values` - Get object values

**Syntax:** `["values", object]`

**Examples:**

**Object values:**
```json
["values", {"a": 1, "b": 2}]
```
*Result:* `{"array": [1, 2]}`

### `objFromPairs` - Create object from key-value pairs

**Syntax:** `["objFromPairs", array_of_pairs]`

**Examples:**

**From pairs:**
```json
["objFromPairs", {"array": [["a", 1], ["b", 2]]}]
```
*Result:* `{"a": 1, "b": 2}`

### `pick` - Select specific object keys

**Syntax:** `["pick", object, array_of_keys]`

**Examples:**

**Pick keys:**
```json
["pick", {"a": 1, "b": 2, "c": 3}, {"array": ["a", "c"]}]
```
*Result:* `{"a": 1, "c": 3}`

### `omit` - Remove specific object keys

**Syntax:** `["omit", object, array_of_keys]`

**Examples:**

**Omit keys:**
```json
["omit", {"a": 1, "b": 2, "c": 3}, {"array": ["b"]}]
```
*Result:* `{"a": 1, "c": 3}`

### `merge` - Merge objects (later objects override earlier)

**Syntax:** `["merge", object, object, ...]`

**Examples:**

**Basic merge:**
```json
["merge", {"a": 1}, {"b": 2}]
```
*Result:* `{"a": 1, "b": 2}`

**Override values:**
```json
["merge", {"a": 1, "b": 2}, {"b": 3, "c": 4}]
```
*Result:* `{"a": 1, "b": 3, "c": 4}`


## Array Operations Operators

### `map` - Array mapping with lambda

**Syntax:** `["map", array, lambda]`

**Examples:**

**Double values:**
```json
["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
```
*Result:* `{"array": [2, 4, 6]}`

### `filter` - Array filtering with lambda

**Syntax:** `["filter", array, lambda]`

**Examples:**

**Filter greater than 2:**
```json
["filter", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]
```
*Result:* `{"array": [3, 4]}`

### `reduce` - Array reduction with lambda

**Syntax:** `["reduce", array, lambda, initial]`

**Examples:**

**Sum array:**
```json
["reduce", {"array": [1, 2, 3]}, ["lambda", ["acc", "item"], ["+", ["$", "/acc"], ["$", "/item"]]], 0]
```
*Result:* `6`

### `count` - Array length

**Syntax:** `["count", array]`

**Examples:**

**Array count:**
```json
["count", {"array": [1, 2, 3]}]
```
*Result:* `3`

### `find` - Find first element matching predicate

**Syntax:** `["find", array, lambda]`

**Examples:**

**Find first > 2:**
```json
["find", {"array": [1, 2, 3, 4]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]
```
*Result:* `3`

### `some` - Test if any element matches predicate

**Syntax:** `["some", array, lambda]`

**Examples:**

**Any > 2:**
```json
["some", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 2]]]
```
*Result:* `true`

### `every` - Test if all elements match predicate

**Syntax:** `["every", array, lambda]`

**Examples:**

**All > 0:**
```json
["every", {"array": [1, 2, 3]}, ["lambda", ["x"], [">", ["$", "/x"], 0]]]
```
*Result:* `true`


## Functional Programming Operators

### `car` - First element of array

**Syntax:** `["car", array]`

**Examples:**

**First element:**
```json
["car", {"array": [1, 2, 3]}]
```
*Result:* `1`

### `cdr` - All elements except first

**Syntax:** `["cdr", array]`

**Examples:**

**Rest elements:**
```json
["cdr", {"array": [1, 2, 3]}]
```
*Result:* `{"array": [2, 3]}`

### `cons` - Prepend element to array

**Syntax:** `["cons", element, array]`

**Examples:**

**Prepend element:**
```json
["cons", 0, {"array": [1, 2]}]
```
*Result:* `{"array": [0, 1, 2]}`

### `append` - Concatenate arrays

**Syntax:** `["append", array, array, ...]`

**Examples:**

**Concatenate two arrays:**
```json
["append", {"array": [1, 2]}, {"array": [3, 4]}]
```
*Result:* `{"array": [1, 2, 3, 4]}`

**Concatenate multiple arrays:**
```json
["append", {"array": [1]}, {"array": [2, 3]}, {"array": [4]}]
```
*Result:* `{"array": [1, 2, 3, 4]}`


## String Operations Operators

### `strConcat` - String concatenation

**Syntax:** `["strConcat", string, string, ...]`

**Examples:**

**Basic concatenation:**
```json
["strConcat", "Hello", " ", "World"]
```
*Result:* `"Hello World"`

### `join` - Join array elements into string

**Syntax:** `["join", array, separator]`

**Examples:**

**Join with space:**
```json
["join", {"array": ["hello", "world"]}, " "]
```
*Result:* `"hello world"`

**Join with comma:**
```json
["join", {"array": ["a", "b", "c"]}, ", "]
```
*Result:* `"a, b, c"`


## Array Manipulation Operators

### `sort` - Array sorting

**Syntax:** `["sort", array] or ["sort", array, direction] or ["sort", array, field, ...]`

**Examples:**

**Natural sort:**
```json
["sort", {"array": [3, 1, 4]}]
```
*Result:* `{"array": [1, 3, 4]}`

**Descending sort:**
```json
["sort", {"array": [3, 1, 4]}, "desc"]
```
*Result:* `{"array": [4, 3, 1]}`

### `reverse` - Reverse array elements

**Syntax:** `["reverse", array]`

**Examples:**

**Reverse array:**
```json
["reverse", {"array": [1, 2, 3]}]
```
*Result:* `{"array": [3, 2, 1]}`

### `unique` - Remove duplicate elements

**Syntax:** `["unique", array]`

**Examples:**

**Remove duplicates:**
```json
["unique", {"array": [1, 2, 2, 3, 3, 3]}]
```
*Result:* `{"array": [1, 2, 3]}`

### `uniqueSorted` - Remove duplicates from sorted array (optimized)

**Syntax:** `["uniqueSorted", array] or ["uniqueSorted", array, mode]`

**Examples:**

**Basic deduplication:**
```json
["uniqueSorted", {"array": [1, 1, 2, 2, 3]}]
```
*Result:* `{"array": [1, 2, 3]}`

### `zip` - Pair corresponding elements from arrays

**Syntax:** `["zip", array1, array2, ...]`

**Examples:**

**Zip two arrays:**
```json
["zip", {"array": [1, 2]}, {"array": ["a", "b"]}]
```
*Result:* `{"array": [[1, "a"], [2, "b"]]}`


## Utilities Operators

### `approx` - Approximate equality for numbers

**Syntax:** `["approx", number1, number2, tolerance]`

**Examples:**

**Within tolerance:**
```json
["approx", 0.1, 0.10001, 0.001]
```
*Result:* `true`

**Outside tolerance:**
```json
["approx", 0.1, 0.11, 0.001]
```
*Result:* `false`



---

*This documentation was automatically generated from `operators.yaml` and validated against the Computo engine.*

*Total operators documented: 46*
