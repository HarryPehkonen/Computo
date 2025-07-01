# Computo Improvement Ideas

This document outlines architectural improvements and patterns that could address the pain points identified in LESSONS.md, focusing on changes that would yield simpler code during development.

## 1. Builder Pattern for JSON Construction

Instead of fighting C++ JSON literal syntax, create a fluent builder to eliminate macro conflicts and make tests cleaner:

```cpp
// Instead of: json{{"array", json::array({1, 2, 3})}}
auto script = ComputoBuilder()
  .array({1, 2, 3})
  .build();

// Or for operators:
auto script = ComputoBuilder()
  .op("map")
    .array({1, 2, 3})
    .lambda("x", ComputoBuilder().op("+").var("x").literal(1))
  .build();
```

**Benefits**:
- Eliminates C++ template argument deduction issues
- Provides type-safe construction
- Makes test code much more readable
- Chainable API feels natural

## 2. Alternative Array Disambiguation

The `{"array": [...]}` syntax is the root of many ergonomic problems. Consider these alternatives:

### Option A: Sigil-based Arrays
```json
["@", 1, 2, 3]  // @ means "literal array"
```

### Option B: Type Hints in Evaluation Context
```cpp
// Mark expected types during parsing
evaluate(expr, ctx.expecting_array())
```

### Option C: Parser-level Disambiguation
```cpp
// Parse-time detection of operator calls vs arrays
bool isOperatorCall = operators.contains(expr[0].get<string>());
```

**Recommendation**: Option A (sigil-based) provides the clearest syntax while maintaining unambiguous parsing.

## 3. Lambda Closure Architecture

The lambda scoping issue needs a more systematic approach with proper lexical environment capture:

```cpp
struct Lambda {
    json params;
    json body;
    std::shared_ptr<Environment> closure;  // Capture environment at creation
};

class Environment {
    std::map<string, json> bindings;
    std::shared_ptr<Environment> parent;  // Lexical scoping chain
    
public:
    json lookup(const string& name) const {
        auto it = bindings.find(name);
        if (it != bindings.end()) return it->second;
        if (parent) return parent->lookup(name);
        throw VariableNotFound(name);
    }
    
    std::shared_ptr<Environment> extend() const {
        auto child = std::make_shared<Environment>();
        child->parent = shared_from_this();
        return child;
    }
};
```

**Benefits**:
- Proper lexical scoping for stored lambdas
- Clear separation of concerns
- Standard closure implementation pattern
- Resolves the 2/57 failing tests

## 4. Dependency Injection for External Integrations

Instead of hard-coding Permuto integration, use dependency injection:

```cpp
class ComputoEngine {
    std::map<string, std::function<json(const json&, Context&)>> external_ops;
    
public:
    void register_external(const string& name, auto op) {
        external_ops[name] = op;
    }
    
    json evaluate_external(const string& op_name, const json& args, Context& ctx) {
        auto it = external_ops.find(op_name);
        if (it == external_ops.end()) {
            throw UnknownOperator(op_name);
        }
        return it->second(args, ctx);
    }
};

// Usage:
engine.register_external("permuto.apply", permuto_apply_impl);
```

**Benefits**:
- Makes testing easier (can inject mock implementations)
- Eliminates build dependency issues
- Allows runtime plugin architecture
- Cleaner separation of concerns

## 5. Macro-Based Test DSL

Create test-specific macros to hide JSON construction complexity:

```cpp
#define COMPUTO_ARRAY(...) json{{"array", json::array({__VA_ARGS__})}}
#define COMPUTO_OP(op, ...) json{op, __VA_ARGS__}
#define COMPUTO_LET(bindings, body) json{"let", bindings, body}
#define COMPUTO_LAMBDA(param, body) json{"lambda", json::array({param}), body}
#define COMPUTO_VAR(name) json{"$", "/" name}

// Tests become much cleaner:
auto script = COMPUTO_OP("map", 
  COMPUTO_ARRAY(1, 2, 3),
  COMPUTO_LAMBDA("x", COMPUTO_OP("+", COMPUTO_VAR("x"), 1))
);
```

**Alternative**: Template-based builders:
```cpp
template<typename... Args>
json op(const string& name, Args&&... args) {
    return json{name, std::forward<Args>(args)...};
}

template<typename... Args>
json array(Args&&... args) {
    return json{{"array", json::array({std::forward<Args>(args)...})}};
}
```

## 6. Enhanced Error Context

Building on the successful path-based error reporting, add more context:

```cpp
class ComputoException : public std::exception {
    string message;
    vector<string> call_stack;
    json problematic_expression;
    
public:
    ComputoException& add_context(const string& op, const json& expr) {
        call_stack.push_back(op);
        if (problematic_expression.is_null()) {
            problematic_expression = expr;
        }
        return *this;
    }
    
    string format_error() const {
        // Include call stack, expression that failed, etc.
    }
};
```

## Priority Assessment

### Highest Impact
1. **Builder Pattern** - Would eliminate most C++ JSON construction pain and make tests dramatically cleaner
2. **Lambda Closure Architecture** - Would solve the lambda scoping issues definitively and bring test success to 100%

### High Impact
3. **Alternative Array Syntax** - Would reduce conceptual overhead for users and simplify the mental model
4. **Dependency Injection** - Would solve external integration testing and build issues

### Medium Impact
5. **Test DSL** - Would improve developer experience but doesn't solve fundamental issues
6. **Enhanced Error Context** - Would improve debugging but current system already works well

## Implementation Strategy

**Phase 1**: Implement builder pattern and test DSL to improve development ergonomics immediately.

**Phase 2**: Redesign lambda architecture with proper closure capture to achieve 100% test success.

**Phase 3**: Consider alternative array syntax if user feedback indicates the current approach is problematic.

**Phase 4**: Add dependency injection for external integrations when more external operators are needed.

## Cost-Benefit Analysis

**Development Time Pain Points vs Fundamental Issues**:
- Test construction issues: **Development tooling problem** - solvable with builder pattern
- External dependency problems: **Build system problem** - solvable with dependency injection  
- Lambda scoping issues: **Fundamental architectural problem** - requires core redesign
- Array syntax ergonomics: **Language design problem** - requires syntax change

The core recursive interpreter design is sound; most pain points are at the language surface level and C++ integration boundaries, making them addressable without major architectural changes.