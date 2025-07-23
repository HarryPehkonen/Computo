# CLEAN_ARCHITECTURE.md

## Architectural Principles That Work

Based on the analysis of the existing Computo implementation, these architectural decisions have proven successful and should be retained in any clean implementation while scaling to the full 30-operator requirement.

## Core Architecture (KEEP THESE)

### 1. Recursive Interpreter with Tail Call Optimization
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

**Why this works:**
- Prevents stack overflow on deep recursion
- Essential for functional programming with `car`, `cdr`, recursive algorithms
- Simple to implement and understand
- Efficient for nested expressions

### 2. Thread-Safe Operator Registry Pattern
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
            // ... etc for all 30 operators
        });
    }
}
```

**Why this works:**
- O(1) operator lookup for all 30 operators
- Thread-safe initialization with `std::call_once`
- Easy to add new operators
- Clean separation of concerns
- Scales efficiently to larger operator sets

### 3. ExecutionContext for Thread-Safe State Management
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

**Why this works:**
- Immutable context copying prevents race conditions
- Shared pointers for efficient input data sharing
- Variable scoping is clean and predictable
- Path tracking enables excellent error messages
- Pass-by-value enables TCO
- Thread-safe: no shared mutable state

### 4. N-ary Operator Consistency
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

**Why this works:**
- Consistent behavior across all operators
- Natural syntax for common operations
- Eliminates the complexity of mixed binary/n-ary systems
- Enables clean functional programming patterns

### 5. Clean Library/CLI Separation
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

**Why this works:**
- Library stays focused and lightweight
- CLI provides rich development experience
- No debugging overhead in production library use
- Clear separation of concerns
- Easy to package and distribute

## File Organization (BALANCED APPROACH)

### Recommended Structure
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

**Total Lines: ~2400 (within reasonable bounds for 30 operators + CLI)**

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

**Why this organization works:**
- Related operators grouped together
- Shared utilities prevent duplication
- Manageable file sizes (75-200 lines each)
- Clear separation of library vs CLI code
- Easy to locate and maintain operators

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

**Usage Examples:**
```json
// Static keys (old way still works)
["obj", ["name", "John"], ["age", 30]]

// Variable keys (new capability)
["obj", [["$", "/field_name"], ["$", "/field_value"]]]

// Mixed static and variable keys
["obj", ["type", "user"], [["$", "/dynamic_key"], ["get", ["$input"], "/value"]]]
```

**Why this is important:**
- Essential for AI API schema translation
- Enables dynamic object restructuring
- Maintains backwards compatibility
- Simple implementation without complexity

## Testing Strategy (Simplified)

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

**Why string-based tests work:**
- Easier to read than C++ JSON construction
- Closer to actual usage patterns
- Simpler to write and maintain
- Shows real JSON syntax

## Anti-Patterns to Avoid (UPDATED)

### 1. Mixed Binary/N-ary Complexity
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

### 2. Debugging in Library Code
```cpp
// DON'T DO THIS - library should be clean
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    if (debug_enabled) {  // NO - no debugging in library
        log_trace("Executing: " + script.dump());
    }
    // ...
}
```

### 3. Global Mutable State
```cpp
// DON'T DO THIS - breaks thread safety
thread_local DebugContext debug_state;  // NO - complicates library
std::atomic<size_t> operation_count;    // NO - unnecessary tracking
```

### 4. Emojis and Visual Decorations
```cpp
// DON'T DO THIS - clutters professional code
throw InvalidArgumentException("❌ Invalid operator: " + op);  // NO EMOJIS
std::cout << "✅ Execution complete!" << std::endl;           // NO EMOJIS
log_message("🔍 Debugging mode enabled");                     // NO EMOJIS
```

**Problems:**
- Unprofessional in business/academic environments
- Can cause encoding issues in different terminals
- Clutters error messages and logs
- Inconsistent rendering across systems

**✅ GOOD: Clean, Professional Messages**
```cpp
throw InvalidArgumentException("Invalid operator: " + op);
std::cout << "Execution complete." << std::endl;
log_message("Debugging mode enabled");
```

This architecture successfully scales the proven patterns from the minimal implementation to support 30 operators, CLI tooling, and thread safety while maintaining the clean separation that prevents over-engineering.
