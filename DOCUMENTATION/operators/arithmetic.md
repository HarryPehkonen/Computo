# Arithmetic Operators

This document details the arithmetic operators in Computo.

## `+` (Addition)

Performs n-ary addition.

-   **Syntax**: `["+", <number1>, <number2>, ...]`
-   **Arguments**: Requires at least one numeric argument.
-   **Returns**: The sum of all arguments.

**Examples:**
```json
["+", 1, 2, 3, 4, 5] 
// Result: 15

["+", 10.5, 2.5]
// Result: 13.0
```

## `-` (Subtraction)

Performs n-ary subtraction or unary negation.

-   **Syntax**: `["-", <number1>, <number2>, ...]`
-   **Arguments**: Requires at least one numeric argument.
-   **Behavior**:
    -   With one argument, it performs unary negation (`-number1`).
    -   With multiple arguments, it subtracts each subsequent number from the first (`number1 - number2 - number3 - ...`).
-   **Returns**: The result of the subtraction or negation.

**Examples:**
```json
["-", 10, 3, 2]
// Result: 5

["-", 5]
// Result: -5
```

## `*` (Multiplication)

Performs n-ary multiplication.

-   **Syntax**: `["*", <number1>, <number2>, ...]`
-   **Arguments**: Requires at least one numeric argument.
-   **Returns**: The product of all arguments.

**Examples:**
```json
["*", 2, 3, 4]
// Result: 24
```

## `/` (Division)

Performs n-ary division or calculates the reciprocal.

-   **Syntax**: `["/", <number1>, <number2>, ...]`
-   **Arguments**: Requires at least one numeric argument. Division by zero will throw an `InvalidArgumentException`.
-   **Behavior**:
    -   With one argument, it calculates the reciprocal (`1 / number1`).
    -   With multiple arguments, it divides the first number by each subsequent number (`number1 / number2 / number3 / ...`).
-   **Returns**: The result of the division.

**Examples:**
```json
["/", 20, 2, 2]
// Result: 5

["/", 4]
// Result: 0.25
```

## `%` (Modulo)

Performs n-ary modulo.

-   **Syntax**: `["%", <number1>, <number2>, ...]`
-   **Arguments**: Requires at least two numeric arguments. Modulo by zero will throw an `InvalidArgumentException`.
-   **Behavior**: It calculates the remainder of the first number divided by the second, then the remainder of that result divided by the third, and so on.
-   **Returns**: The final remainder.

**Examples:**
```json
["%", 20, 6] 
// Result: 2

["%", 20, 6, 5]
// Result: 2 (20 % 6 = 2, then 2 % 5 = 2)
```
