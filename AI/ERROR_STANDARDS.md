# Error Standards and Implementation Guide

## Purpose

This document establishes comprehensive standards for error handling, error message formatting, and error recovery in Computo. Consistent, helpful error handling is critical for user adoption and debugging complex JSON transformations in production environments.

## Error Classification System

### Error Categories

#### 1. **User Input Errors** (Most Common)
**Characteristics**: Caused by incorrect user input that can be fixed by the user
**Response Strategy**: Helpful error message with specific fix suggestion
**Examples**: Unknown operator, type mismatch, malformed syntax

#### 2. **Data Errors** (Common in Production)
**Characteristics**: Caused by unexpected or malformed input data
**Response Strategy**: Clear explanation of what was expected vs found, with fallback suggestions
**Examples**: Missing required fields, null values where data expected, invalid JSON Pointer paths

#### 3. **Logic Errors** (User Design Issues)
**Characteristics**: Valid syntax but impossible/contradictory logic
**Response Strategy**: Explain why the operation cannot be completed, suggest alternative approach
**Examples**: Division by zero, empty array operations, infinite recursion

#### 4. **System Errors** (Rare, System-Level)
**Characteristics**: Internal system failures or resource exhaustion
**Response Strategy**: Include technical details for bug reports, suggest workarounds when possible
**Examples**: Memory allocation failures, stack overflow, thread safety violations

## Error Message Standards

### Standard Error Message Format

```
Error at <path>: <specific_problem>. <suggestion_or_explanation>
```

#### Components:

1. **Error at**: Always start with location information
2. **Path**: Precise execution path using JSON Pointer syntax
3. **Specific Problem**: Exactly what went wrong in user-friendly terms
4. **Suggestion**: Actionable advice for fixing the error (when possible)

### Location Path Standards

#### Path Format: JSON Pointer Style
- **Root level**: `/` 
- **Array indices**: `/0`, `/1`, `/2`
- **Object keys**: `/key`, `/users`, `/settings`
- **Nested paths**: `/let/body/map`, `/if/condition`, `/users/0/name`

#### Path Examples:
```
/                           // Error at root expression
/let                        // Error in let expression
/let/body                   // Error in let body
/let/body/map               // Error in map within let body
/let/body/map/lambda/body   // Error in lambda body within map
/users/0/transform          // Error in first user's transform
```

### Specific Error Message Templates

#### Unknown Operator Errors

**Template**: `Error at <path>: Unknown operator "<operator>". Did you mean "<suggestion>"?`

**Examples**:
```
Error at /: Unknown operator "filer". Did you mean "filter"?
Error at /let/body: Unknown operator "redcue". Did you mean "reduce"?
Error at /map/lambda/body: Unknown operator "stConcat". Did you mean "strConcat"?
```

**Implementation Requirements**:
- Use Levenshtein distance ≤ 2 for suggestions
- Prioritize operators from the same category (array, arithmetic, etc.)
- Limit suggestions to top 3 matches
- If no good matches, suggest checking operator documentation

#### Type Mismatch Errors

**Template**: `Error at <path>: Expected <expected_type>, got <actual_type> <actual_value>. <type_conversion_suggestion>`

**Examples**:
```
Error at /+/1: Expected number, got string "hello". Numbers cannot be added to strings.
Error at /map/0: Expected array, got object {"key": "value"}. Use {"array": [...]} syntax for arrays.
Error at /filter/lambda: Expected lambda expression, got number 42. Use ["lambda", ["param"], body] syntax.
```

**Type Descriptions**:
- `number` (not "integer", "float", "double")
- `string` 
- `boolean`
- `array` (for both native arrays and array objects)
- `object`
- `null`
- `lambda expression`

#### Array Object Errors

**Template**: `Error at <path>: <specific_array_issue>. <array_syntax_reminder>`

**Examples**:
```
Error at /data: Array object malformed: "array" key must contain an array, got string "hello". Use {"array": [...]} syntax.
Error at /map/0: Expected array or array object, got regular object. Use {"array": [...]} to create array objects.
Error at /: Cannot evaluate array starting with unknown operator "hello". Use {"array": ["hello", ...]} for literal arrays.
```

#### JSON Pointer Path Errors

**Template**: `Error at <path>: Path "<json_pointer>" <path_issue>. <path_suggestion>`

**Examples**:
```
Error at /$input: Path "/users/5/name" not found in input. Input has 3 users (indices 0-2).
Error at /$: Path "/missing_var" not found in variables. Available variables: x, y, counter.
Error at /$input: Path "/user/age" not found. Did you mean "/users/0/age"?
```

#### Lambda Expression Errors

**Template**: `Error at <path>: Lambda <lambda_issue>. <lambda_syntax_reminder>`

**Examples**:
```
Error at /map/lambda: Lambda parameter "x" not found in scope. Available parameters: item, index.
Error at /filter/lambda: Lambda must have exactly one parameter, got 3: ["x", "y", "z"].
Error at /reduce/lambda: Lambda body returned string "hello", expected boolean for filter condition.
```

#### Argument Count Errors

**Template**: `Error at <path>: <operator> expects <expected_count>, got <actual_count>. <usage_example>`

**Examples**:
```
Error at /: Operator "+" expects at least 1 argument, got 0. Usage: ["+", number1, number2, ...]
Error at /not: Operator "not" expects exactly 1 argument, got 3. Usage: ["not", boolean_expression]
Error at /if: Operator "if" expects 3 arguments (condition, true_branch, false_branch), got 2.
```

#### Variable Scope Errors

**Template**: `Error at <path>: Variable "<var_name>" <scope_issue>. <scope_explanation>`

**Examples**:
```
Error at /let/body: Variable "x" not found in scope. Variables must be defined before use in let expressions.
Error at /$: Variable "counter" not found. Available variables in current scope: x, y, temp.
Error at /lambda/body: Variable "data" not found. Lambda scope contains only: item (from map).
```

#### Recursion and Stack Errors

**Template**: `Error at <path>: <recursion_issue>. <recursion_solution>`

**Examples**:
```
Error at /lambda: Maximum recursion depth exceeded (1000 levels). Check for infinite recursion in lambda.
Error at /if: Circular dependency detected in variable evaluation. Variables: x -> y -> x.
```

## Typo Detection and Suggestions

### Suggestion Algorithm

#### For Unknown Operators:
1. **Calculate Levenshtein distance** to all known operators
2. **Filter candidates** with distance ≤ 2
3. **Prioritize by category** (same category as context suggests)
4. **Return top 3 matches** in order of likelihood

#### For Variable Names:
1. **Check current scope** first (closest matches)
2. **Check parent scopes** if no close matches in current scope
3. **Include JSON Pointer path suggestions** for complex paths

#### For JSON Pointer Paths:
1. **Check available keys** at each path level
2. **Suggest corrections** for each path component
3. **Provide context** about available alternatives

### Suggestion Quality Standards

#### Good Suggestions:
- **Contextually Relevant**: Operator category matches usage context
- **Common Alternatives**: Prioritize frequently used operators
- **Syntactically Valid**: All suggestions produce valid expressions

#### Avoid Poor Suggestions:
- **Too Many Options**: Maximum 3 suggestions per error
- **Irrelevant Matches**: Don't suggest operators from unrelated categories
- **Ambiguous Corrections**: If uncertain, explain the context instead

## Error Context and Recovery

### Providing Useful Context

#### For Complex Expressions:
Include the problematic sub-expression in error messages:
```
Error at /let/body/map/lambda/body: Unknown operator "filer" in expression ["filer", ["$", "/item"], ["lambda", ["x"], [">", ["$", "/x/age"], 18]]]. Did you mean "filter"?
```

#### For Data-Dependent Errors:
Show the actual data that caused the problem:
```
Error at /users/0/age: Expected number, got string "twenty-five". Input data: {"name": "Alice", "age": "twenty-five"}. Use ["toNumber", value] to convert strings.
```

#### For Path Navigation Errors:
Show what was actually available:
```
Error at /$input: Path "/users/5" not found. Available users: [{"name": "Alice"}, {"name": "Bob"}] (indices 0-1).
```

### Recovery Suggestions

#### Immediate Fixes:
**Operator Typos**: `Did you mean "filter"?`
**Type Issues**: `Use ["toString", value] to convert numbers to strings.`
**Array Syntax**: `Use {"array": [...]} syntax for literal arrays.`

#### Structural Guidance:
**Lambda Issues**: `Lambda syntax: ["lambda", ["param"], body_expression]`
**Let Expression**: `Let syntax: ["let", [["var", value]], body_expression]`
**Conditional Logic**: `If syntax: ["if", condition, true_branch, false_branch]`

#### Documentation References:
**Complex Operators**: `See documentation for "reduce" operator examples.`
**Array Operations**: `See array handling guide for literal vs operator arrays.`
**Variable Scoping**: `See variable scoping rules in documentation.`

## Error Propagation Standards

### Exception Hierarchy

#### Base Exception: `ComputoException`
- **Purpose**: Base class for all Computo-specific errors
- **Info**: Execution path, basic error message
- **Usage**: Catch-all for Computo errors

#### Specific Exceptions:

##### `InvalidOperatorException : ComputoException`
- **Purpose**: Unknown or misnamed operators
- **Info**: Operator name, suggestions, execution path
- **Usage**: Typo detection and operator validation

##### `InvalidArgumentException : ComputoException`
- **Purpose**: Wrong argument types or counts
- **Info**: Expected vs actual types, argument position, usage examples
- **Usage**: Type validation and argument checking

##### `DataAccessException : ComputoException`
- **Purpose**: JSON Pointer path errors, missing data
- **Info**: Attempted path, available alternatives, input structure
- **Usage**: Input data validation and path navigation

##### `VariableException : ComputoException`
- **Purpose**: Variable scope and binding errors
- **Info**: Variable name, current scope, availability
- **Usage**: Let expression and lambda parameter validation

##### `EvaluationException : ComputoException`
- **Purpose**: Runtime logic errors (division by zero, recursion)
- **Info**: Operation attempted, context, recovery suggestions
- **Usage**: Mathematical and logical operation errors

### Error Aggregation

#### For Multiple Errors:
When multiple errors occur in a single expression, report the first error encountered with context about the overall operation:

```
Error at /let/body/map: Invalid argument in map operation. First error at /let/body/map/lambda/body: Unknown operator "filer". Fix this error first, then re-evaluate for additional issues.
```

#### For Nested Errors:
Preserve the full path but focus on the deepest error:
```
Error at /let/body/map/lambda/body/+: Expected number, got string "hello" at argument 1. In context: map operation over array [{"value": "hello"}, {"value": "world"}].
```

## Implementation Requirements

### Error Message Construction

#### Path Tracking:
- **Build path incrementally** during evaluation
- **Use JSON Pointer syntax** for consistency
- **Include array indices** for array operations
- **Preserve full path** even in nested operations

#### Message Assembly:
```cpp
// Pseudo-code for error message construction
std::string build_error_message(
    const std::string& path,
    const std::string& error_type,
    const std::string& details,
    const std::string& suggestion = ""
) {
    std::string message = "Error at " + path + ": " + details;
    if (!suggestion.empty()) {
        message += ". " + suggestion;
    }
    return message;
}
```

#### Context Preservation:
- **Include original expression** when helpful for debugging
- **Show actual vs expected values** for type errors
- **Provide data samples** for data access errors
- **Maintain operator context** for nested operations

### Error Detection Points

#### Parse Time:
- **JSON syntax validation**
- **Array object structure validation**
- **Basic operator name checking**

#### Evaluation Time:
- **Operator existence validation**
- **Argument type checking**
- **Argument count validation**
- **Data access path validation**

#### Runtime:
- **Mathematical operation errors** (division by zero)
- **Logical operation errors** (invalid comparisons)
- **Resource exhaustion** (recursion limits)
- **Variable scope violations**

## Testing Standards for Error Handling

### Error Message Quality Tests

#### Template Compliance:
- All error messages follow standard format
- Path information is accurate and helpful
- Suggestions are relevant and actionable

#### User Experience Tests:
- Error messages tested with actual users
- Common errors have helpful recovery paths
- Error message length is reasonable (< 200 characters typically)

#### Typo Detection Tests:
- Known typos return helpful suggestions
- Edge cases (very short names, special characters) handled gracefully
- Suggestion quality measured by user success rate

### Error Recovery Tests

#### User Workflow Tests:
- Users can fix 80%+ of errors using only error messages
- Error-fix cycles don't introduce new error types
- Recovery suggestions actually work when followed

#### Stress Testing:
- Deep nesting produces useful error paths
- Large data sets don't obscure error messages
- Complex expressions maintain error clarity

## Internationalization Considerations (Future)

### Message Structure:
- Error message templates should be extractable for translation
- Technical terms (operator names, types) should remain in English
- Path information should be locale-independent

### Cultural Adaptation:
- Error message tone should be adaptable to different cultural preferences
- Suggestion strategies may need cultural customization
- Documentation references should be translatable

## Conclusion

Excellent error handling is a critical differentiator for Computo. Users should feel confident that when something goes wrong, they'll understand what happened and know how to fix it. Every error message is an opportunity to teach users about Computo's capabilities while helping them solve their immediate problem.

The investment in comprehensive error handling pays dividends in:
- **Reduced Support Burden**: Users self-service most issues
- **Faster Adoption**: New users feel supported through learning curve
- **Production Reliability**: Debugging production issues becomes tractable
- **Developer Productivity**: Less time spent figuring out what went wrong

Any rewrite of Computo should prioritize error handling as a first-class feature, not an afterthought to be added later.
