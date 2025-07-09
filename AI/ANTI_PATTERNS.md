# ANTI_PATTERNS.md

## What NOT to Do: Lessons from Over-Engineering

This document catalogs specific anti-patterns identified in the original Computo codebase that must be avoided in any clean implementation. Each anti-pattern includes concrete examples and explains why it's problematic.

## 1. Code Duplication Anti-Patterns

### ❌ BAD: Duplicate Helper Functions
**Found in:** `src/operators/array.cpp` and `src/operators/list.cpp`

```cpp
// array.cpp (lines 11-58)
nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, 
                               const nlohmann::json& item, 
                               ExecutionContext& ctx) {
    // 40+ lines of implementation
}

// list.cpp (lines 9-52) - IDENTICAL FUNCTION
nlohmann::json evaluate_lambda(const nlohmann::json& lambda_expr, 
                               const nlohmann::json& item, 
                               ExecutionContext& ctx) {
    // Same 40+ lines of implementation
}
```

**Problems:**
- Maintenance nightmare (fix bugs in two places)
- Violates DRY principle
- Indicates poor code organization
- Clear sign of AI copy-paste development

**✅ GOOD: Single Shared Implementation**
```cpp
// operators.cpp - ONE implementation only
nlohmann::json evaluate_lambda(const nlohmann::json& lambda, 
                               const nlohmann::json& arg, 
                               ExecutionContext& ctx) {
    // Single implementation used by all operators
}
```

### ❌ BAD: Repeated Operator Patterns
**Found in:** `src/operators/comparison.cpp`

```cpp
// Identical structure repeated 6 times
nlohmann::json greater_than_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("...");
    
    // Chained comparison logic (25 lines)
    bool all_true = true;
    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        if (!(a.get<double>() > b.get<double>())) {
            all_true = false;
            break;
        }
    }
    return all_true;
}

nlohmann::json less_than_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    // IDENTICAL structure, only comparison operator changes
}
```

**Problems:**
- Template-generated repetitive code
- Hard to maintain consistency
- Adds unnecessary complexity (chained comparisons rarely needed)

**✅ GOOD: Consistent N-ary Operations**
```cpp
nlohmann::json greater_than_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("> requires at least 2 arguments");
    
    // N-ary chained comparison: [">", a, b, c] means a > b && b > c
    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        if (!a.is_number() || !b.is_number()) {
            throw InvalidArgumentException("> requires numeric arguments");
        }
        if (!(a.get<double>() > b.get<double>())) {
            return false;
        }
    }
    return true;
}
```

## 2. Over-Engineering Anti-Patterns

### ❌ BAD: Unnecessary Memory Management
**Found in:** `include/computo/memory_pool.hpp` (232 lines)

```cpp
class JsonMemoryPool {
private:
    std::vector<std::unique_ptr<nlohmann::json>> objects_;
    std::stack<size_t> available_indices_;
    std::atomic<uint64_t> generation_{1};
    
public:
    class PooledJsonHandle {
        // 130+ lines of RAII wrapper
        // Generation checking
        // Move semantics
        // Complex lifecycle management
    };
    
    PooledJsonHandle acquire() {
        // Complex pooling logic
    }
    
    void return_to_pool(size_t index, uint64_t generation) {
        // Generation validation
        // Thread safety
        // Statistics tracking
    }
};
```

**Problems:**
- Solves non-existent performance problem
- JSON copying is already fast with modern libraries
- Adds 232 lines for questionable benefit
- Complex thread safety for thread-local pools (contradiction)
- Over-engineered RAII for simple data

**✅ GOOD: Use Standard JSON Copying with N-ary Operations**
```cpp
// Just use nlohmann::json directly - it's already efficient
nlohmann::json result = some_json_value;  // Fast copying built-in

// Example: Clean n-ary addition
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("+ requires at least 1 argument");
    
    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("+ requires numeric arguments");
        result += arg.get<double>();
    }
    return result;
}
```

### ❌ BAD: Excessive Builder Patterns
**Found in:** `include/computo/builder.hpp` (455 lines)

```cpp
class ComputoBuilder {
public:
    static ComputoBuilder add(const nlohmann::json& a, const nlohmann::json& b);
    static ComputoBuilder add(std::initializer_list<nlohmann::json> args);
    static ComputoBuilder subtract(const nlohmann::json& a, const nlohmann::json& b);
    static ComputoBuilder multiply(const nlohmann::json& a, const nlohmann::json& b);
    static ComputoBuilder multiply(std::initializer_list<nlohmann::json> args);
    // ... 50+ more methods
    
    static ComputoBuilder zipWith(const nlohmann::json& array1_expr, 
                                  const nlohmann::json& array2_expr, 
                                  const nlohmann::json& lambda_expr);
    static ComputoBuilder mapWithIndex(const nlohmann::json& array_expr, 
                                       const nlohmann::json& lambda_expr);
    // Advanced operations that may never be used
};
```

**Problems:**
- 455 lines to wrap simple JSON array construction
- Many methods for rarely-used operations
- Hides the underlying JSON structure
- Makes tests verbose instead of clear

**✅ GOOD: Direct JSON Construction with N-ary Operations**
```cpp
// Clear, direct, no unnecessary abstraction - showcases n-ary power
json script1 = json::array({"+", 1, 2, 3, 4});  // N-ary addition
EXPECT_EQ(computo::execute(script1, input), 10);

json script2 = json::array({"&&", true, true, false});  // N-ary logical AND
EXPECT_EQ(computo::execute(script2, input), false);

json script3 = json::array({">", 10, 5, 3, 1});  // Chained comparison
EXPECT_EQ(computo::execute(script3, input), true);
```

### ❌ BAD: Complex Type Handling
**Found in:** `src/operators/arithmetic.cpp` (lines 27-42)

```cpp
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    // Complex dual-tracking system
    bool is_int_result = true;
    int64_t int_result = 0;
    double double_result = 0.0;
    
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        
        if (arg.is_number_integer()) {
            int64_t val = arg.get<int64_t>();
            int_result += val;
            double_result += static_cast<double>(val);
        } else if (arg.is_number_float()) {
            is_int_result = false;  // Float encountered
            double val = arg.get<double>();
            double_result += val;
            int_result += static_cast<int64_t>(val);  // Lossy but maintained
        }
    }
    
    return is_int_result ? nlohmann::json(int_result) : nlohmann::json(double_result);
}
```

**Problems:**
- Unnecessary complexity for minimal benefit
- Dual tracking adds cognitive load
- Type preservation rarely matters in practice
- Easy source of subtle bugs

**✅ GOOD: Simple Consistent N-ary Handling**
```cpp
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("+ requires at least 1 argument");
    
    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("+ requires numeric arguments");
        result += arg.get<double>();  // Simple, consistent, n-ary
    }
    return result;
}
```

## 3. Feature Creep Anti-Patterns

### ❌ BAD: Too Many Operators
**Found in:** 35+ operators across 7 files

```cpp
// Excessive operator inventory
"+"  "-"  "*"  "/"  "%"           // Arithmetic (5)
">"  "<"  ">=" "<=" "==" "!="     // Comparison (6)  
"&&" "||" "not"                   // Logical (3)
"map" "filter" "reduce" "find"    // Array basic (4)
"some" "every" "partition"        // Array advanced (3)
"flatMap" "zip" "zipWith"         // Array complex (3)
"car" "cdr" "cons" "append"       // Lisp-style (4)
"chunk" "enumerate"               // Specialized (2)
"strConcat" "patch" "diff"        // String/JSON (3)
"merge" "permuto.apply"           // Integration (2)
```

**Problems:**
- Many operators solve edge cases that rarely occur
- Maintenance burden increases exponentially
- Users overwhelmed by options
- Clear sign of "just in case" development

**✅ GOOD: Well-Chosen 30 Operators with N-ary Consistency**
```cpp
// 30 operators that solve 95% of real problems with consistent n-ary semantics

// Arithmetic (5) - all n-ary
"+"  "-"  "*"  "/"  "%"           

// Comparison (6) - all n-ary with chaining  
">"  "<"  ">=" "<=" "==" "!="     

// Logical (3) - n-ary except 'not'
"&&" "||" "not"                   

// Data access (4)
"$input" "$" "get" "let"          

// Control & construction (2)
"if" "obj"                        

// Array operations (7)
"map" "filter" "reduce" "count" "find" "some" "every"

// Functional programming (4) - for TCO learning
"car" "cdr" "cons" "append"       

// Utilities (3)
"strConcat" "merge" "approx"      
```

### ❌ BAD: Unnecessary Configuration
**Found in:** Complex options and debugging infrastructure

```cpp
// Over-engineered configuration
class ExecutionContext {
    permuto::Options permuto_options;
    std::vector<std::string> evaluation_path;
    // Debugging hooks
    // Performance monitoring
    // Statistics collection
};

// Global debugging infrastructure
class GlobalDebugStats;
class DebuggerScope;
thread_local DebugContext;
```

**Problems:**
- Solving problems users don't have
- Adds complexity to every operation
- Thread-local storage for single-threaded use case
- Performance monitoring for fast operations

**✅ GOOD: Simple Context**
```cpp
class ExecutionContext {
    nlohmann::json input_data;
    std::map<std::string, nlohmann::json> variables;
    std::vector<std::string> path;  // For error reporting only
};
```

## 4. API Design Anti-Patterns

### ❌ BAD: Multiple API Variants
**Found in:** 8 different execute functions

```cpp
// Overloaded API variants
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input, const permuto::Options& options);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& options);
nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input);
nlohmann::json execute_move(nlohmann::json&& script, const nlohmann::json& input, const permuto::Options& options);
nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs);
nlohmann::json execute_move(nlohmann::json&& script, const std::vector<nlohmann::json>& inputs, const permuto::Options& options);
```

**Problems:**
- Analysis paralysis for users
- More code to maintain and test
- Move semantics optimization for already-fast operations
- Multiple input support for edge cases

**✅ GOOD: Minimal Thread-Safe APIs**
```cpp
// Library: Clean and minimal
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);

// Examples showcasing n-ary power
execute(json::array({"+", 1, 2, 3, 4}), json{});  // N-ary addition
execute(json::array({">", 10, 5, 3}), json{});    // Chained comparison
execute(json::array({"&&", true, true, false}), json{});  // N-ary logical
```

### ❌ BAD: Inconsistent Operator Behavior
**Found in:** Mixed binary/n-ary operators

```cpp
// Inconsistent argument handling (OLD ANTI-PATTERN)
"+" -> n-ary (1 to N arguments)
"-" -> binary only (exactly 2 arguments)  // INCONSISTENT!
"*" -> n-ary (1 to N arguments)  
"/" -> binary only (exactly 2 arguments)  // INCONSISTENT!
"==" -> n-ary with chaining
">" -> n-ary with chaining
```

**Problems:**
- Users can't predict operator behavior
- Some operators have complex argument validation
- Inconsistent error messages
- No clear design principle

**✅ GOOD: Consistent N-ary Operations**
```cpp
// All operators use n-ary semantics (except "not" and "!=")
"+" -> n-ary: ["+", a, b, c, d, ...]     // Sum all arguments
"-" -> n-ary: ["-", a, b, c, ...]       // a - b - c - ...
"*" -> n-ary: ["*", a, b, c, ...]       // Multiply all arguments
"/" -> n-ary: ["/", a, b, c, ...]       // a / b / c / ...
">" -> chained: [">", a, b, c]          // a > b && b > c
"==" -> n-ary: ["==", a, b, c]          // All arguments equal
"&&" -> n-ary: ["&&", a, b, c, ...]     // All arguments truthy
"||" -> n-ary: ["||", a, b, c, ...]     // Any argument truthy
```

## 5. Testing Anti-Patterns

### ❌ BAD: Over-Complex Test Infrastructure
**Found in:** 4,414 lines of tests with builder patterns

```cpp
// Verbose builder-based tests
TEST_F(NaryOperatorsTest, AdditionNaryWithExpressions) {
    json script = json::array({
        "+", 
        json::array({"+", 1, 2}),  // 3
        json::array({"*", 2, 3}),  // 6
        5                          // 5
    });
    EXPECT_EQ(computo::execute(script, input_data), 14);
}

// Or even worse - builder pattern tests
auto script = CB::add({
    CB::add(1, 2).build(),
    CB::multiply(2, 3).build(),
    5
}).build();
```

**Problems:**
- Tests are harder to read than the functionality they test
- Over-testing edge cases that don't matter
- Builder patterns hide the actual JSON structure

**✅ GOOD: Simple Direct Tests**
```cpp
TEST(ComputoTest, BasicArithmetic) {
    EXPECT_EQ(execute(json::array({"+", 2, 3}), json{}), 5);
    EXPECT_EQ(execute(json::array({"-", 5, 2}), json{}), 3);
}
```

### ❌ BAD: Excessive Edge Case Testing
**Found in:** 103 individual test cases

```cpp
// Testing every possible edge case
TEST_F(ErrorHandlingEdgeCasesTest, AdditionInvalidTypes) {
    // Test addition with string argument
    json script = json::array({"+", "hello", 2});
    EXPECT_THROW(computo::execute(script, input_data), computo::InvalidArgumentException);
    
    // Test addition with boolean argument
    json script2 = json::array({"+", 1, true});
    EXPECT_THROW(computo::execute(script2, input_data), computo::InvalidArgumentException);
    
    // Test addition with null argument
    json script3 = json::array({"+", 1, json(nullptr)});
    EXPECT_THROW(computo::execute(script3, input_data), computo::InvalidArgumentException);
    
    // Test addition with array argument
    json script4 = json::array({"+", 1, json::array({1, 2})});
    EXPECT_THROW(computo::execute(script4, input_data), computo::InvalidArgumentException);
    
    // Test addition with object argument
    json script5 = json::array({"+", 1, json::object()});
    EXPECT_THROW(computo::execute(script5, input_data), computo::InvalidArgumentException);
}
```

**Problems:**
- Testing obvious error conditions
- More test code than implementation code
- Diminishing returns on test coverage

**✅ GOOD: Essential Error Testing**
```cpp
TEST(ComputoTest, ErrorHandling) {
    EXPECT_THROW(execute(json::array({"unknown"}), json{}), InvalidOperatorException);
    EXPECT_THROW(execute(json::array({"+", "hello", 2}), json{}), InvalidArgumentException);
}
```

## 6. File Organization Anti-Patterns

### ❌ BAD: Excessive File Granularity
**Found in:** Operators split across 7 files

```
src/operators/
├── arithmetic.cpp      (4 operators)
├── array.cpp          (13 operators)  
├── comparison.cpp     (6 operators)
├── list.cpp           (4 operators)
├── logical.cpp        (3 operators)
├── string.cpp         (1 operator)
└── utility.cpp        (4 operators)
```

**Problems:**
- Code duplication between files
- Hard to find operator implementations
- Build complexity
- Over-organization for small codebase

**✅ GOOD: Simple Organization**
```
src/
├── computo.cpp        (evaluation engine, TCO)
└── operators.cpp      (all 12 operators)
```

## Prevention Strategies

### 1. Size-Based Rules
- Any function > 20 lines: simplify or split
- Any file > 200 lines: question organization
- Any operator > 15 lines: find simpler approach

### 2. Duplication Detection
```bash
# Run after each operator implementation
grep -n "nlohmann::json.*_operator" src/*.cpp | sort
# Should show no duplicate function signatures
```

### 3. Feature Justification
Before implementing any operator, answer:
- What specific real-world problem does this solve?
- Can it be solved with existing operators?
- Will 80% of users need this?
- Is the implementation under 15 lines?

### 4. Complexity Checks
- Can a new developer understand this in 5 minutes?
- Are there nested loops or complex conditionals?
- Does error handling exceed 2 lines?
- Are there more than 3 levels of indentation?

## 7. Updated Anti-Patterns for 30-Operator System

### ❌ BAD: Debugging Features in Library Code
**Problem:** Mixing debugging infrastructure into the core library

```cpp
// DON'T DO THIS - debugging in library
namespace computo {
    // Global debugging state
    thread_local bool debug_enabled = false;
    thread_local std::vector<std::string> trace_log;
    
    nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
        if (debug_enabled) {
            trace_log.push_back("Executing: " + script.dump());
        }
        // ... evaluation with debugging hooks throughout
    }
}
```

**Problems:**
- Complicates library API
- Performance overhead for all users
- Thread-local storage complexity
- Violates single responsibility principle

**✅ GOOD: Clean Library/CLI Separation**
```cpp
// Library: computo.hpp - minimal and focused
namespace computo {
    nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
    nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);
}

// CLI: separate debugging wrapper
nlohmann::json execute_with_debugging(const nlohmann::json& script, 
                                      const std::vector<nlohmann::json>& inputs,
                                      const CLIOptions& opts) {
    // CLI-only debugging logic here
}
```

### ❌ BAD: Mixed Binary/N-ary Operator Complexity
**Found in:** Inconsistent operator argument handling

```cpp
// DON'T DO THIS - inconsistent argument patterns
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() == 2) {
        // Binary addition with one implementation
        return evaluate(args[0], ctx).get<double>() + evaluate(args[1], ctx).get<double>();
    } else if (args.size() > 2) {
        // N-ary addition with different implementation
        double result = 0.0;
        for (const auto& arg : args) {
            result += evaluate(arg, ctx).get<double>();
        }
        return result;
    } else {
        throw InvalidArgumentException("+ requires at least 2 arguments");
    }
}
```

**Problems:**
- Users can't predict operator behavior
- Inconsistent error messages
- Complex implementation with multiple code paths
- Different semantics for same operator

**✅ GOOD: Consistent N-ary Implementation**
```cpp
nlohmann::json addition_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("+ requires at least 1 argument");
    
    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("+ requires numeric arguments");
        result += arg.get<double>();
    }
    return result;
}
```

### ❌ BAD: CLI Argument Parsing Overengineering  
**Problem:** Complex argument parsing for simple CLI

```cpp
// DON'T DO THIS - overengineered CLI parsing
class CLIArgumentParser {
private:
    std::map<std::string, CLIFlag> flags;
    std::vector<CLIOption> options;
    std::vector<CLIPositional> positionals;
    
public:
    CLIArgumentParser& add_flag(const std::string& name, const std::string& description, bool* target);
    CLIArgumentParser& add_option(const std::string& name, const std::string& description, 
                                  const std::string& default_value, std::string* target);
    CLIArgumentParser& add_positional(const std::string& name, bool required, std::string* target);
    
    ParseResult parse(int argc, char* argv[]);
    void print_help() const;
    void print_usage() const;
    
    // 200+ lines of complex parsing logic
};
```

**Problems:**
- Over-abstracted for simple needs
- More complex than the actual functionality
- External dependency or large implementation
- Harder to debug argument issues

**✅ GOOD: Simple Manual Parsing**
```cpp
struct CLIOptions {
    bool trace = false;
    bool profile = false;
    bool repl = false;
    std::string break_on_operator;
    int pretty_spaces = 0;
    std::vector<std::string> files;
};

CLIOptions parse_arguments(int argc, char* argv[]) {
    CLIOptions opts;
    int i = 1;
    
    // Simple switch parsing - switches before files
    while (i < argc && argv[i][0] == '-') {
        std::string arg = argv[i];
        if (arg == "--trace") opts.trace = true;
        else if (arg == "--repl") opts.repl = true;
        else if (arg.starts_with("--pretty=")) opts.pretty_spaces = std::stoi(arg.substr(9));
        // ... simple, direct parsing
        i++;
    }
    
    // Remaining args are files
    while (i < argc) opts.files.push_back(argv[i++]);
    return opts;
}
```

### ❌ BAD: Thread Safety Through Complex Synchronization
**Problem:** Using locks and complex thread synchronization

```cpp
// DON'T DO THIS - complex thread synchronization
namespace computo {
    std::mutex operator_registry_mutex;
    std::shared_mutex context_cache_mutex;
    std::atomic<bool> operators_initialized{false};
    
    thread_local std::unique_ptr<DebugContext> debug_context;
    
    nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
        std::shared_lock<std::shared_mutex> lock(context_cache_mutex);
        
        if (!operators_initialized.load()) {
            lock.unlock();
            std::unique_lock<std::mutex> init_lock(operator_registry_mutex);
            if (!operators_initialized.load()) {
                initialize_operators();
                operators_initialized.store(true);
            }
        }
        
        // Complex thread-local debugging setup
        if (!debug_context) {
            debug_context = std::make_unique<DebugContext>();
        }
        
        // ... evaluation with multiple lock points
    }
}
```

**Problems:**
- Complex lock hierarchy prone to deadlocks
- Performance overhead from synchronization
- Thread-local storage complicates library
- Debugging state doesn't belong in library

**✅ GOOD: Thread Safety Through Pure Functions**
```cpp
namespace computo {
    namespace {
        std::map<std::string, OperatorFunc> operators;
        std::once_flag init_flag;
        
        void initialize_operators() {
            std::call_once(init_flag, []() {
                operators["+"] = operators::addition;
                // ... one-time initialization
            });
        }
    }
    
    nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
        initialize_operators();  // Thread-safe, efficient
        ExecutionContext ctx(input);  // Stack-local, no sharing
        return evaluate(script, ctx);  // Pure function
    }
}
```

### ❌ BAD: Variable Keys as String Literals Only
**Problem:** Restricting object keys to string literals

```cpp
// DON'T DO THIS - static keys only
nlohmann::json obj_operator(const nlohmann::json& args, ExecutionContext& ctx) {
    nlohmann::json result = nlohmann::json::object();
    
    for (const auto& pair : args) {
        if (!pair.is_array() || pair.size() != 2) {
            throw InvalidArgumentException("obj requires [key, value] pairs");
        }
        
        // Only accept literal string keys
        if (!pair[0].is_string()) {
            throw InvalidArgumentException("obj key must be a literal string");
        }
        
        std::string key = pair[0].get<std::string>();
        auto value = evaluate(pair[1], ctx);
        result[key] = value;
    }
    return result;
}
```

**Problems:**
- Limits dynamic object construction
- Prevents AI API schema translation use cases
- Inconsistent with expression evaluation elsewhere
- Reduces language expressiveness

**✅ GOOD: Variable Keys Through Expression Evaluation**
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

### ❌ BAD: String-Based JSON Tests Without Raw Strings
**Problem:** Complex C++ JSON construction in tests

```cpp
// DON'T DO THIS - complex JSON construction
TEST_F(ComputoTest, ComplexTest) {
    nlohmann::json script = nlohmann::json::array({
        "let",
        nlohmann::json::array({
            nlohmann::json::array({"key", "name"}),
            nlohmann::json::array({"value", "Alice"})
        }),
        nlohmann::json::array({
            "obj",
            nlohmann::json::array({
                nlohmann::json::array({"$", "/key"}),
                nlohmann::json::array({"$", "/value"})
            })
        })
    });
    
    nlohmann::json expected = nlohmann::json::object();
    expected["name"] = "Alice";
    
    EXPECT_EQ(execute(script, json{}), expected);
}
```

**Problems:**
- Verbose and hard to read
- Easy to make syntax errors
- Doesn't show actual JSON structure
- Difficult to debug when tests fail

**✅ GOOD: Raw String JSON Tests**
```cpp
TEST_F(ComputoTest, ComplexTest) {
    std::string script_json = R"(
        ["let", [["key", "name"], ["value", "Alice"]], 
         ["obj", [["$", "/key"], ["$", "/value"]]]]
    )";
    
    auto script = nlohmann::json::parse(script_json);
    nlohmann::json expected = json{{"name", "Alice"}};
    
    EXPECT_EQ(execute(script, json{}), expected);
}
```

## Prevention Strategies for 30-Operator System

### 1. Architecture Enforcement
- **Library/CLI Separation**: No debugging code in library headers
- **Thread Safety**: Pure functions only, no shared mutable state
- **Single Include**: All library API in one `computo.hpp` file
- **NO EMOJIS**: No emoji characters in any code, documentation, error messages, or CLI output

### 2. Operator Consistency Rules
- **N-ary Pattern**: All operators except `not` and `!=` accept multiple arguments
- **Error Messages**: Consistent format across all operators
- **Implementation Size**: Each operator < 25 lines

### 3. CLI Quality Gates
- **Simple Parsing**: Manual argument parsing for straightforward needs
- **Feature Separation**: Debugging features only in CLI, not library
- **Readline Integration**: REPL uses standard readline library

### 4. Testing Quality Metrics
- **String-Based Tests**: Use raw string JSON for readability
- **Thread Safety Tests**: Verify concurrent execution safety
- **Integration Tests**: Test CLI and library separately

These anti-patterns represent real problems found in AI-assisted development. Avoiding them leads to cleaner, more maintainable code that solves actual problems rather than anticipated ones.