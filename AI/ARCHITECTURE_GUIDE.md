# Architecture Guide

This document details the key architectural patterns that must be followed for a successful implementation of Computo, as well as the anti-patterns that must be strictly avoided.

## Core Architectural Patterns

### Recursive Interpreter with Tail Call Optimization (TCO)

**Pattern**: The core evaluation engine is a recursive interpreter. To prevent stack overflow on deeply nested expressions (a key requirement for functional programming), it must be implemented with a trampoline pattern for tail call optimization.

**Implementation**: The main `evaluate` function enters a `while(true)` loop. Control flow operators like `if` and `let` do not make a direct recursive call; instead, they return a `TailCall` struct containing the next expression and context to be evaluated. The loop then "bounces" to the next evaluation, keeping the stack flat.

```cpp
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    while (true) {  // Trampoline loop for TCO
        // Handle array objects: {"array": [...]}
        if (expr.is_object() && expr.contains("array")) {
            // Evaluate each element and return literal array
        }
        
        // Handle special forms (if, let) with TCO
        if (op == "if") {
            // TCO: Update ctx and expr, then continue
            ctx = new_context;
            expr = chosen_branch;
            continue;
        }
        
        // Handle regular operators normally
        return operator_func(args, ctx);
    }
}
```

**Why it Works**: This is essential for supporting functional programming paradigms and ensures the engine is robust against complex, deeply nested scripts.

### Thread-Safe Operator Registry

**Pattern**: A central, static registry holds mappings from operator names (e.g., `"+"`) to their corresponding C++ function implementations.

**Implementation**: The registry is a `std::map`. To ensure thread-safe initialization in a multi-threaded environment, it is populated within a `std::call_once` block. This guarantees that the registry is initialized exactly once, on the first use, without race conditions.

```cpp
namespace {
    std::map<std::string, OperatorFunc> operators;
    std::once_flag init_flag;
    
    void initialize_operators() {
        std::call_once(init_flag, []() {
            // Arithmetic operators
            operators["+"] = addition_operator;
            operators["-"] = subtraction_operator;
            operators["*"] = multiplication_operator;
            operators["/"] = division_operator;
            operators["%"] = modulo_operator;
            
            // Comparison operators  
            operators[">"] = greater_than_operator;
            operators["<"] = less_than_operator;
            // ... etc for all operators
        });
    }
}
```

**Why it Works**: Provides O(1) lookup for operators, is easy to extend, scales efficiently to larger operator sets, and is guaranteed to be thread-safe.

### Immutable Execution Context

**Pattern**: All state required for an expression's evaluation is contained within an `ExecutionContext` object. This object is treated as immutable.

**Implementation**: When a new scope is created (e.g., in a `let` expression), a *new* context is created by copying the parent context and adding the new variables. To make this efficient, input data is stored in `std::shared_ptr`s, avoiding deep copies of large JSON objects.

```cpp
class ExecutionContext {
    std::shared_ptr<const nlohmann::json> input_ptr;          // First input
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr; // All inputs
    std::map<std::string, nlohmann::json> variables;          // Let bindings
    std::vector<std::string> path;                            // Error reporting
    
public:
    const nlohmann::json& input() const { return *input_ptr; }
    const std::vector<nlohmann::json>& inputs() const { return *inputs_ptr; }
    
    // Thread-safe context copying for scoping
    ExecutionContext with_variables(const std::map<std::string, nlohmann::json>& vars) const;
    ExecutionContext with_path(const std::string& segment) const;
};
```

**Why it Works**: This is fundamental to thread safety. Since contexts are not shared or mutated across threads, there are no race conditions. It also makes variable scoping clean and predictable.

### Clean Library/CLI Separation

**Pattern**: The core transformation logic is completely contained within `libcomputo`. All user-facing features like the REPL, debugging, command history, and interactive control are in `libcomputorepl` and the `computo_repl` executable.

**Implementation**: The build system uses conditional compilation (`-DREPL`). The core library is compiled without this flag. The REPL library is compiled *with* it, which enables hooks and debugging code paths that are completely absent from the production version.

```cpp
// Library: computo.hpp - Single include, minimal API
namespace computo {
    // Core API - thread-safe, no debugging
    nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
    nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
    
    // Exception hierarchy - thread-local
    class ComputoException : public std::exception;
    class InvalidOperatorException : public ComputoException;
    class InvalidArgumentException : public ComputoException;
}

// CLI: Separate executable with debugging features
// - REPL with readline
// - Tracing and profiling  
// - Breakpoints and variable watching
// - JSON comment parsing
```

**Why it Works**: Guarantees zero performance overhead from debugging features in the production library, keeping it minimal and fast. It provides a clear separation of concerns between the engine and the tools.

## N-ary Operator Consistency

All operators that can logically accept multiple arguments should be implemented as n-ary with consistent behavior:

```cpp
// All operators follow consistent n-ary pattern
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 1) throw InvalidArgumentException("+ requires at least 1 argument");
    
    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("+ requires numeric arguments");
        result += arg.get<double>();
    }
    return result;
}

// Comparison with chaining: [">", a, b, c] means a > b && b > c
nlohmann::json greater_than_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("> requires at least 2 arguments");
    
    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        if (!(a.get<double>() > b.get<double>())) return false;
    }
    return true;
}
```

**Why this works**: Consistent behavior across all operators, natural syntax for common operations, eliminates the complexity of mixed binary/n-ary systems, and enables clean functional programming patterns.

## Recommended File Organization

### Balanced Structure
```
src/
  computo.cpp                    # Core evaluation engine (200 lines)
  operators/
    arithmetic.cpp               # +, -, *, /, % (125 lines)
    comparison.cpp               # >, <, >=, <=, ==, != (150 lines)
    logical.cpp                  # &&, ||, not (75 lines)
    array.cpp                    # map, filter, reduce, count, find, some, every (175 lines)
    functional.cpp               # car, cdr, cons, append (100 lines)
    utility.cpp                  # strConcat, merge, approx (75 lines)
    data.cpp                     # $input, $, get, let, obj, if (150 lines)
  cli/
    main.cpp                     # Argument parsing, main loop (150 lines)
    repl.cpp                     # Interactive REPL with readline (300 lines)
    debugger.cpp                 # Tracing, profiling, breakpoints (350 lines)

include/
  computo.hpp                    # Single public API (150 lines)

tests/
  test_operators.cpp             # All operator tests (400 lines)
  test_threading.cpp             # Thread safety tests (100 lines)
  test_cli.cpp                   # CLI feature tests (300 lines)
```

**Why this organization works**: Related operators grouped together, shared utilities prevent duplication, manageable file sizes (75-200 lines each), clear separation of library vs CLI code, and easy to locate and maintain operators.

### Shared Utilities Pattern
```cpp
// In operators/shared.hpp (internal only)
namespace computo::operators {
    // Shared by all operator files
    nlohmann::json evaluate_lambda(const nlohmann::json& lambda, 
                                   const nlohmann::json& arg, 
                                   ExecutionContext& ctx);
    
    bool is_truthy(const nlohmann::json& value);
    
    void validate_numeric_args(const nlohmann::json& args, const std::string& op_name);
}
```

## Thread Safety Implementation

### Core Requirements
```cpp
// NO global mutable state
namespace computo {
    namespace {
        // Registry initialized once, then read-only
        std::map<std::string, OperatorFunc> operators;
        std::once_flag init_flag;
    }
    
    // All evaluation state in ExecutionContext (stack-local)
    // Pure functions throughout
    // Exception-based error handling (thread-local)
}
```

### Thread-Safe Patterns
```cpp
// GOOD: Thread-local evaluation context
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);  // Stack-local, no sharing
    return evaluate(script, ctx);
}

// GOOD: Pure operator functions
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    // No global state access
    // No side effects
    // Deterministic results
}

// GOOD: Context copying for scoping
ExecutionContext new_ctx = ctx.with_variables(new_vars);  // Immutable copy
```

## Variable Keys in Object Construction

### Enhanced obj Operator
```cpp
nlohmann::json obj_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    nlohmann::json result = nlohmann::json::object();
    
    for (const auto& pair : args) {
        if (!pair.is_array() || pair.size() != 2) {
            throw InvalidArgumentException("obj requires [key_expr, value_expr] pairs");
        }
        
        // Evaluate BOTH key and value expressions
        auto key_result = evaluate(pair[0], ctx);
        auto value_result = evaluate(pair[1], ctx);
        
        // Key must evaluate to string
        if (!key_result.is_string()) {
            throw InvalidArgumentException("obj key must evaluate to string");
        }
        
        std::string key = key_result.get<std::string>();
        result[key] = value_result;
    }
    return result;
}
```

**Usage Examples**:
```json
// Static keys (old way still works)
["obj", ["name", "John"], ["age", 30]]

// Variable keys (new capability)
["obj", [["$", "/field_name"], ["$", "/field_value"]]]

// Mixed static and variable keys
["obj", ["type", "user"], [["$", "/dynamic_key"], ["get", ["$input"], "/value"]]]
```

**Why this is important**: Essential for AI API schema translation, enables dynamic object restructuring, maintains backwards compatibility, and has simple implementation without complexity.

## Testing Strategy

### String-Based JSON Tests
```cpp
TEST(ComputoTest, ArithmeticOperators) {
    std::string script_json = R"(["+", 1, 2, 3])";
    auto script = nlohmann::json::parse(script_json);
    
    EXPECT_EQ(computo::execute(script, json{}), 6);
}

TEST(ComputoTest, VariableKeys) {
    std::string script_json = R"(
        ["let", [["key", "name"], ["value", "Alice"]], 
         ["obj", [["$", "/key"], ["$", "/value"]]]]
    )";
    auto script = nlohmann::json::parse(script_json);
    
    json expected = json{{"name", "Alice"}};
    EXPECT_EQ(computo::execute(script, json{}), expected);
}
```

**Why string-based tests work**: Easier to read than C++ JSON construction, closer to actual usage patterns, simpler to write and maintain, and shows real JSON syntax.

## Key Anti-Patterns to Avoid

###   Global Mutable State
**Description**: Using global or `thread_local` variables for managing state (like debug flags or execution traces) within the core library.

**Why it Fails**: It breaks thread safety, introduces hidden dependencies, and makes the code difficult to reason about and test.

**Correct Approach**: Pass all state through the `ExecutionContext`. For debugging, use a wrapper pattern or dependency-injected hooks that are not part of the core library's function signatures.

###   Mixing Binary and N-ary Operators
**Description**: Implementing some operators (like `+`) as n-ary but others (like `-`) as strictly binary, forcing users to nest calls: `["-", ["-", 10, 5], 2]`.

**Why it Fails**: It creates an inconsistent and unpredictable user experience. Users cannot easily guess how an operator will behave.

**Correct Approach**: All operators that can logically accept multiple arguments should be implemented as n-ary with consistent behavior (e.g., `["-", 10, 5, 2]` works as expected).

###   Debugging Logic in the Core Library
**Description**: Placing `if (debug_enabled)` checks, logging, or timing code directly within the main evaluation path of `libcomputo`.

**Why it Fails**: It adds performance overhead (even if the check is false), clutters the core logic, and violates the principle of a clean separation of concerns.

**Correct Approach**: Use conditional compilation (`#ifdef REPL`) to add hooks to the evaluation engine. The debugging tools can then attach to these hooks, but the hooks themselves are compiled out of the production library.

###   Over-Engineering with Unnecessary Abstractions
**Description**: Creating complex builder patterns, memory pools, or elaborate type systems to solve problems that do not exist in the core requirements.

**Why it Fails**: It adds massive complexity for little to no real-world benefit. For example, `nlohmann::json` is already highly optimized for copying, so a custom memory pool is unnecessary.

**Correct Approach**: Keep the design simple and direct. Use standard library features and the `nlohmann::json` library to their full potential before inventing custom solutions.

###   Mixed Binary/N-ary Complexity
```cpp
// DON'T DO THIS - inconsistent behavior
if (op == "+") {
    if (args.size() == 2) {
        // Binary addition
    } else {
        // N-ary addition with different logic
    }
}
```

###   Debugging in Library Code
```cpp
// DON'T DO THIS - library should be clean
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    if (debug_enabled) {  // NO - no debugging in library
        log_trace("Executing: " + script.dump());
    }
    // ...
}
```

###   Global Mutable State
```cpp
// DON'T DO THIS - breaks thread safety
thread_local DebugContext debug_state;  // NO - complicates library
std::atomic<size_t> operation_count;    // NO - unnecessary tracking
```

###   Emojis and Visual Decorations
```cpp
// DON'T DO THIS - clutters professional code
throw InvalidArgumentException("  Invalid operator: " + op);  // NO EMOJIS
std::cout << "  Execution complete!" << std::endl;           // NO EMOJIS
log_message("  Debugging mode enabled");                     // NO EMOJIS
```

**Problems**: Unprofessional in business/academic environments, can cause encoding issues in different terminals, clutters error messages and logs, and inconsistent rendering across systems.

**  GOOD: Clean, Professional Messages**
```cpp
throw InvalidArgumentException("Invalid operator: " + op);
std::cout << "Execution complete." << std::endl;
log_message("Debugging mode enabled");
```

This architecture successfully scales the proven patterns from the minimal implementation to support all operators, CLI tooling, and thread safety while maintaining the clean separation that prevents over-engineering.
