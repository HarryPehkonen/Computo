# Comparison Operators

This document details the comparison operators in Computo.

## `>` (Greater Than)

Compares if the first argument is strictly greater than the second, and so on for subsequent arguments (chained comparison).

-   **Syntax**: `[">", <value1>, <value2>, ...]`
-   **Arguments**: Requires at least two numeric arguments.
-   **Returns**: `true` if `value1 > value2 && value2 > value3 ...`, otherwise `false`.

**Examples:**
```json
[">", 5, 3, 1]
// Result: true (5 > 3 && 3 > 1)

[">", 10, 3, 5]
// Result: false (because 3 is not > 5)
```

## `<` (Less Than)

Compares if the first argument is strictly less than the second, and so on for subsequent arguments (chained comparison).

-   **Syntax**: `["<", <value1>, <value2>, ...]`
-   **Arguments**: Requires at least two numeric arguments.
-   **Returns**: `true` if `value1 < value2 && value2 < value3 ...`, otherwise `false`.

**Examples:**
```json
["<", 1, 3, 5]
// Result: true (1 < 3 && 3 < 5)

["<", 3, 5, 3]
// Result: false (because 5 is not < 3)
```

## `>=` (Greater Than or Equal To)

Compares if the first argument is greater than or equal to the second, and so on for subsequent arguments (chained comparison).

-   **Syntax**: `[">=", <value1>, <value2>, ...]`
-   **Arguments**: Requires at least two numeric arguments.
-   **Returns**: `true` if `value1 >= value2 && value2 >= value3 ...`, otherwise `false`.

**Examples:**
```json
[">=", 5, 5, 3]
// Result: true (5 >= 5 && 5 >= 3)

[">=", 10, 5, 8]
// Result: false (because 5 is not >= 8)
```

## `<=` (Less Than or Equal To)

Compares if the first argument is less than or equal to the second, and so on for subsequent arguments (chained comparison).

-   **Syntax**: `["<=", <value1>, <value2>, ...]`
-   **Arguments**: Requires at least two numeric arguments.
-   **Returns**: `true` if `value1 <= value2 && value2 <= value3 ...`, otherwise `false`.

**Examples:**
```json
["<=", 1, 3, 3]
// Result: true (1 <= 3 && 3 <= 3)

["<=", 3, 5, 4]
// Result: false (because 5 is not <= 4)
```

## `==` (Equal To)

Compares if all arguments are equal to each other.

-   **Syntax**: `["==", <value1>, <value2>, ...]`
-   **Arguments**: Requires at least two arguments of any type. Comparison is value-based.
-   **Returns**: `true` if `value1 == value2 && value2 == value3 ...`, otherwise `false`.

**Examples:**
```json
["==", 5, 5, 5]
// Result: true

["==", "hello", "hello", "world"]
// Result: false
```

## `!=` (Not Equal To)

Compares if two arguments are not equal to each other.

-   **Syntax**: `["!=", <value1>, <value2>]`
-   **Arguments**: Requires exactly two arguments of any type. Comparison is value-based.
-   **Returns**: `true` if `value1 != value2`, otherwise `false`.

**Examples:**
```json
["!=", 5, 3]
// Result: true

["!=", "hello", "hello"]
// Result: false
```
