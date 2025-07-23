# Tail Call Optimization (TCO) Implementation Guide

## Overview

This guide covers the correct implementation of Tail Call Optimization (TCO) in the Computo JSON transformation engine. TCO is critical for preventing stack overflow in deeply recursive scenarios.

## Architecture

### Core Components

1. **TailCall struct**: Contains expression and context for the next evaluation
2. **EvaluationResult struct**: Either contains a final value or a tail call continuation
3. **evaluate_internal()**: Returns EvaluationResult (can be final or tail call)
4. **evaluate()**: Trampoline function that bounces until final result

### Data Structures

```cpp
// Continuation for tail call optimization
struct TailCall {
    nlohmann::json expression;
    ExecutionContext context;
};

// Result type for trampoline pattern
struct EvaluationResult {
    nlohmann::json value;
    bool is_tail_call;
    std::unique_ptr<TailCall> tail_call;
    
    // Constructor for regular result
    explicit EvaluationResult(nlohmann::json val);
    
    // Constructor for tail call
    EvaluationResult(nlohmann::json expr, ExecutionContext ctx);
};
```

## TCO Implementation Patterns

### ❌ WRONG (Causes Stack Overflow)

```cpp
auto op_if(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // Evaluate condition
    auto condition = evaluate(args[0], ctx.with_path("condition"));
    bool is_truthy = computo::is_truthy(condition);
    
    // WRONG: Direct recursive call to evaluate()
    if (is_truthy) {
        return EvaluationResult(evaluate(args[1], ctx.with_path("then")));
    }
    return EvaluationResult(evaluate(args[2], ctx.with_path("else")));
}
```

### ✅ RIGHT (Enables TCO)

```cpp
auto op_if(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // Evaluate condition
    auto condition = evaluate(args[0], ctx.with_path("condition"));
    bool is_truthy = computo::is_truthy(condition);
    
    // RIGHT: Return tail call continuation
    if (is_truthy) {
        return EvaluationResult(args[1], ctx.with_path("then"));
    }
    return EvaluationResult(args[2], ctx.with_path("else"));
}
```

## Key Principles

### 1. Operator Return Types

All operators must return `EvaluationResult` instead of `nlohmann::json`:

```cpp
// Function signature
using OperatorFunction = std::function<EvaluationResult(const nlohmann::json&, ExecutionContext&)>;

// Operator declaration
auto op_if(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult;
```

### 2. Tail Call Detection

An operation is a tail call if:
- It's the last operation in the function
- The result is returned directly (not used in further computation)
- The expression should be evaluated in the same context level

### 3. Trampoline Pattern

The `evaluate()` function implements the trampoline pattern:

```cpp
auto evaluate(const nlohmann::json& expr, const ExecutionContext& ctx) -> nlohmann::json {
    auto result = evaluate_internal(expr, ctx);
    
    // Keep bouncing until we get a final result
    while (result.is_tail_call) {
        result = evaluate_internal(result.tail_call->expression, result.tail_call->context);
    }
    
    return result.value;
}
```

## Control Flow Operators

### If Operator

```cpp
auto op_if(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // Validate arguments
    if (args.size() != 3) {
        throw InvalidArgumentException("if requires exactly 3 arguments", ctx.get_path_string());
    }
    
    // Evaluate condition (NOT a tail call)
    auto condition = evaluate(args[0], ctx.with_path("condition"));
    bool is_truthy = computo::is_truthy(condition);
    
    // Return tail call continuation (IS a tail call)
    if (is_truthy) {
        return EvaluationResult(args[1], ctx.with_path("then"));
    }
    return EvaluationResult(args[2], ctx.with_path("else"));
}
```

### Let Operator

```cpp
auto op_let(const nlohmann::json& args, ExecutionContext& ctx) -> EvaluationResult {
    // Validate arguments
    if (args.size() != 2) {
        throw InvalidArgumentException("let requires exactly 2 arguments", ctx.get_path_string());
    }
    
    // Evaluate bindings (NOT tail calls)
    std::map<std::string, nlohmann::json> new_variables;
    for (const auto& [key, value] : args[0].items()) {
        auto evaluated_value = evaluate(value, ctx.with_path("binding:" + key));
        new_variables[key] = evaluated_value;
    }
    
    // Execute body (IS a tail call)
    auto new_context = ctx.with_variables(new_variables);
    return EvaluationResult(args[1], new_context.with_path("body"));
}
```

## Testing TCO

### Deep Recursion Tests

Create tests that would cause stack overflow without TCO:

```cpp
TEST_F(ComputoTest, TCODeepRecursionIf) {
    // Create deeply nested if expression
    std::string deep_if = R"("result")";
    
    // Build nested if expressions (100+ levels deep)
    for (int i = 0; i < 100; ++i) {
        deep_if = R"(["if", true, )" + deep_if + R"(, "else"])";
    }
    
    // This should not cause stack overflow thanks to TCO
    EXPECT_EQ(execute_script(deep_if), json("result"));
}
```

### Test Coverage Requirements

1. **Single operator deep recursion**: Test each TCO-enabled operator individually
2. **Mixed operator deep recursion**: Test combinations of TCO-enabled operators
3. **Conditional deep recursion**: Test different code paths in deeply nested scenarios
4. **Performance verification**: Ensure TCO doesn't significantly impact performance

## Common Pitfalls

### 1. Wrapping evaluate() Results

```cpp
// WRONG: This defeats TCO
return EvaluationResult(evaluate(expr, ctx));

// RIGHT: Return tail call continuation
return EvaluationResult(expr, ctx);
```

### 2. Intermediate Computations

```cpp
// WRONG: This is not a tail call (result is used in computation)
auto result = evaluate(expr, ctx);
return EvaluationResult(result.get<int>() + 1);

// RIGHT: Do computation first, then make tail call
auto incremented_expr = /* build expression that adds 1 */;
return EvaluationResult(incremented_expr, ctx);
```

### 3. Multiple Return Paths

Ensure ALL return paths in control flow operators use TCO:

```cpp
// WRONG: Only one path uses TCO
if (condition) {
    return EvaluationResult(then_expr, ctx);  // TCO
} else {
    return EvaluationResult(evaluate(else_expr, ctx));  // Not TCO
}

// RIGHT: Both paths use TCO
if (condition) {
    return EvaluationResult(then_expr, ctx);  // TCO
} else {
    return EvaluationResult(else_expr, ctx);  // TCO
}
```

## Verification Checklist

- [ ] All operators return `EvaluationResult` instead of `nlohmann::json`
- [ ] Control flow operators (`if`, `let`) return tail call continuations
- [ ] No direct calls to `evaluate()` in tail positions
- [ ] Deep recursion tests pass without stack overflow
- [ ] TCO doesn't break existing functionality
- [ ] Performance remains acceptable

## Future Operators

When implementing new operators, follow these guidelines:

1. **Regular operators**: Return `EvaluationResult(final_value)` for direct results
2. **Control flow operators**: Return `EvaluationResult(next_expr, new_ctx)` for tail calls
3. **Always test**: Include deep recursion tests for any operator that can recurse

This TCO implementation ensures that Computo can handle deeply nested expressions without stack overflow, making it suitable for complex transformation scenarios.
