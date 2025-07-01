# Computo - Technical Implementation Details

## Architecture Overview

Computo is implemented as a C++17 library with a recursive interpreter architecture. The system consists of a central evaluation engine that dispatches to operator functions through a registry pattern, with sophisticated context management for variable scoping and error tracking.

## Core Components

### 1. Evaluation Engine

#### Central Evaluate Function
```cpp
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx);
```

**Key Features:**
- **Pass-by-value parameters**: Enables tail call optimization through assignment
- **Trampoline pattern**: Uses `while(true)` loop to implement TCO for `let` and `if`
- **Recursive dispatch**: Handles nested expressions through recursive calls
- **Expression classification**: Distinguishes between literals, operator calls, and array objects

**Algorithm Flow:**
1. **Array Object Detection**: Check for `{"array": [...]}` syntax and evaluate elements
2. **Literal Detection**: Non-arrays and empty arrays are returned as-is
3. **Operator Extraction**: First element of arrays must be string operator name
4. **Special Forms**: Handle `$input`, `$inputs`, `if`, and `let` with TCO
5. **Operator Dispatch**: Look up and call registered operator functions

### 2. Execution Context System

#### ExecutionContext Class
```cpp
class ExecutionContext {
private:
    std::shared_ptr<const nlohmann::json> input_ptr;
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr;
    static const nlohmann::json null_input;
    
public:
    std::map<std::string, nlohmann::json> variables;
    permuto::Options permuto_options;
    std::vector<std::string> evaluation_path;
    // ... methods
};
```

**Design Features:**
- **Smart pointer management**: Uses `std::shared_ptr` for efficient copying and memory safety
- **Multi-input support**: Handles both single and multiple input documents
- **Path tracking**: Maintains execution path for detailed error reporting
- **Variable scoping**: Implements lexical scoping with variable shadowing
- **TCO compatibility**: Assignable design enables tail call optimization

**Context Operations:**
- **`with_variables()`**: Creates new context with additional variable bindings
- **`with_path()`**: Creates new context with extended execution path
- **`get_variable()`**: Variable lookup with JSON Pointer syntax validation
- **`get_path_string()`**: Converts path vector to string for error messages

### 3. Operator Registry System

#### Registry Architecture
```cpp
using OperatorFunc = std::function<nlohmann::json(const nlohmann::json& args, ExecutionContext& ctx)>;
static std::map<std::string, OperatorFunc> operators;
static std::once_flag operators_init_flag;
```

**Key Features:**
- **Thread-safe initialization**: Uses `std::call_once` for race-free startup
- **O(1) lookup**: Hash map provides constant-time operator resolution
- **Function objects**: Lambda-based operator implementations for flexibility
- **Context passing**: Operators receive mutable context for path tracking

**Initialization Pattern:**
```cpp
static void initialize_operators() {
    std::call_once(operators_init_flag, []() {
        operators["+"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
            // Implementation
        };
        // ... other operators
    });
}
```

### 4. Lambda Evaluation System

#### Lambda Structure
```json
["lambda", ["param1", "param2", ...], body_expression]
```

#### Lambda Evaluator
```cpp
static nlohmann::json evaluate_lambda(
    const nlohmann::json& lambda_expr, 
    const std::vector<nlohmann::json>& param_values, 
    ExecutionContext& ctx
);
```

**Implementation Details:**
- **Variable resolution**: Supports lambda variables stored in let bindings
- **Parameter validation**: Strict arity checking with clear error messages
- **Context creation**: Creates new execution context with lambda parameters
- **Scoping**: Implements lexical scoping with proper variable isolation

## Detailed Implementation

### 1. Tail Call Optimization (TCO)

#### Special Forms with TCO
**`if` Operator:**
```cpp
if (op == "if") {
    // ... condition evaluation
    ctx = op_ctx.with_path(is_true ? "then" : "else");
    expr = is_true ? expr[2] : expr[3];
    continue; // TCO - no recursive call
}
```

**`let` Operator:**
```cpp  
if (op == "let") {
    // ... variable binding
    ctx = op_ctx.with_path("body").with_variables(new_vars);
    expr = expr[2];
    continue; // TCO - no recursive call
}
```

**Benefits:**
- **Stack safety**: Prevents stack overflow in deeply nested calls
- **Performance**: Eliminates function call overhead
- **Memory efficiency**: Constant stack space usage

### 2. Array Syntax Revolution

#### Disambiguation Strategy
**Problem**: Traditional Lisp ambiguity between `[1, 2, 3]` as literal vs operator call

**Solution**: Explicit array object syntax
```json
{"array": [1, 2, 3]}  // Literal array
["+", 1, 2, 3]        // Operator call
```

**Implementation:**
```cpp
if (expr.is_object() && expr.contains("array")) {
    if (!expr["array"].is_array()) {
        throw InvalidArgumentException("array object must contain an actual array");
    }
    // Evaluate each element and return as JSON array
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < expr["array"].size(); ++i) {
        result.push_back(evaluate(expr["array"][i], ctx.with_path("array[" + std::to_string(i) + "]")));
    }
    return result;
}
```

### 3. N-ary Operator Implementation

#### Addition Operator Example
```cpp
operators["+"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    if (args.size() < 1) {
        throw InvalidArgumentException("+ operator requires at least 1 argument");
    }
    
    // Type-preserving arithmetic
    bool all_integers = values[0].is_number_integer();
    int64_t int_result = all_integers ? values[0].get<int64_t>() : 0;
    double double_result = values[0].get<double>();
    
    for (size_t i = 1; i < values.size(); ++i) {
        if (all_integers && values[i].is_number_integer()) {
            int_result += values[i].get<int64_t>();
            double_result += values[i].get<double>();
        } else {
            all_integers = false;
            double_result += values[i].get<double>();
        }
    }
    
    return all_integers ? nlohmann::json(int_result) : nlohmann::json(double_result);
};
```

**Features:**
- **Variable arity**: Accepts 1 or more arguments
- **Type preservation**: Returns integer if all inputs are integers
- **Error handling**: Validates argument types with clear messages

### 4. Truthiness Implementation

#### Consistent Truthiness Logic
```cpp
bool is_true = false;
if (condition.is_boolean()) {
    is_true = condition.get<bool>();
} else if (condition.is_number()) {
    if (condition.is_number_integer()) {
        is_true = condition.get<int64_t>() != 0;
    } else {
        is_true = condition.get<double>() != 0.0;
    }
} else if (condition.is_string()) {
    is_true = !condition.get<std::string>().empty();
} else if (condition.is_null()) {
    is_true = false;
} else if (condition.is_array() || condition.is_object()) {
    is_true = !condition.empty();
}
```

**Applied in:**
- `if` conditional evaluation
- `filter` predicate evaluation  
- `&&` and `||` logical operators
- `find`, `some`, `every` predicates

### 5. Error Handling System

#### Exception Hierarchy
```cpp
class ComputoException : public std::exception {
    explicit ComputoException(const std::string& message);
};

class InvalidArgumentException : public ComputoException {
    explicit InvalidArgumentException(const std::string& message, const std::string& path);
};
```

#### Path-Aware Error Reporting
```cpp
ExecutionContext op_ctx = ctx.with_path(op);
throw InvalidArgumentException("Invalid argument count", op_ctx.get_path_string());
```

**Error Path Examples:**
- `/let/body/map/lambda[0]/$` - Variable lookup in map lambda
- `/if/condition/get` - JSON pointer access in condition
- `/obj/key[1]/concat` - String concatenation in object key

### 6. Multi-Input Architecture

#### Input Management
```cpp
class ExecutionContext {
private:
    std::shared_ptr<const nlohmann::json> input_ptr;              // Primary input (backward compatibility)
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr; // All inputs
};
```

#### Input Operators
```cpp
// $input - backward compatibility
if (op == "$input") {
    return ctx.input();
}

// $inputs - new multi-input support
if (op == "$inputs") {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& input_doc : ctx.inputs()) {
        result.push_back(input_doc);
    }
    return result;
}
```

### 7. Permuto Integration

#### Template Processing Bridge
```cpp
operators["permuto.apply"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    if (args.size() != 2) {
        throw InvalidArgumentException("permuto.apply operator requires exactly 2 arguments");
    }
    
    auto template_json = evaluate(args[0], ctx);
    auto context_json = evaluate(args[1], ctx);
    
    try {
        return permuto::apply(template_json, context_json, ctx.permuto_options);
    } catch (const permuto::PermutoException& e) {
        throw InvalidArgumentException("Permuto error: " + std::string(e.what()));
    }
};
```

**Integration Features:**
- **Options passing**: Permuto configuration through execution context
- **Exception translation**: Converts Permuto exceptions to Computo exceptions
- **Template evaluation**: Both template and context are evaluated expressions

### 8. JSON Patch Integration

#### Diff Generation
```cpp
operators["diff"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    auto document_a = evaluate(args[0], ctx);
    auto document_b = evaluate(args[1], ctx);
    
    try {
        return nlohmann::json::diff(document_a, document_b);
    } catch (const nlohmann::json::exception& e) {
        throw InvalidArgumentException("Failed to generate JSON diff: " + std::string(e.what()));
    }
};
```

#### Patch Application
```cpp
operators["patch"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    auto document_to_patch = evaluate(args[0], ctx);
    auto patch_array = evaluate(args[1], ctx);
    
    try {
        return document_to_patch.patch(patch_array);
    } catch (const nlohmann::json::exception& e) {
        throw PatchFailedException(std::string(e.what()));
    }
};
```

## Build System Architecture

### CMake Configuration
```cmake
cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
```

### Dependencies
- **nlohmann/json**: JSON parsing and manipulation
- **Permuto**: Template processing library  
- **Google Test**: Unit testing framework

### Build Targets
- **`computo`**: Core library
- **`computo_cli`**: Command-line interface
- **`computo_tests`**: Test suite

### Compiler Settings
```cmake
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()
```

## Command Line Interface

### Argument Parsing
```cpp
// Flag parsing with position tracking
for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-?") {
        print_help(argv[0]);
        return 0;
    } else if (arg == "--interpolation") {
        enable_interpolation = true;
        script_arg = i + 1;
    }
    // ... other flags
}
```

### Multi-Input Processing
```cpp
std::vector<json> inputs;
for (int i = script_arg + 1; i < argc; ++i) {
    json input;
    input_file >> input;
    inputs.push_back(input);
}

// Execute with appropriate API
if (inputs.empty()) {
    result = computo::execute(script, json(nullptr));
} else {
    result = computo::execute(script, inputs);
}
```

### Output Formatting
```cpp
if (diff_mode) {
    json patch = nlohmann::json::diff(inputs[0], result);
    std::cout << patch.dump(pretty_indent) << std::endl;
} else {
    std::cout << result.dump(pretty_indent) << std::endl;
}
```

## Performance Optimizations

### 1. Memory Management
- **Smart pointers**: Efficient context copying with shared ownership
- **Copy-on-write**: Minimal copying through shared references
- **RAII**: Automatic resource management and exception safety

### 2. Execution Efficiency
- **Operator registry**: O(1) operator lookup
- **Short-circuit evaluation**: Early termination in logical operators
- **Type-preserving arithmetic**: Avoids unnecessary floating-point conversion

### 3. Context Management
- **Path building**: Incremental path construction for error reporting
- **Variable scoping**: Efficient variable map copying and overlay
- **Lambda context**: Isolated execution environments for lambda calls

## Testing Architecture

### Test Structure
```cpp
class ComputoTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    json input_data;
};
```

### Test Categories
- **Literal values**: Basic data type handling
- **Operator functionality**: Individual operator testing
- **Error conditions**: Exception handling validation
- **Integration scenarios**: Complex nested expressions
- **Lambda system**: Parameter binding and scoping
- **Multi-input processing**: Multiple document handling

### Test Execution
```cmake
add_test(NAME ComputoTests COMMAND computo_tests)
```

## Security Considerations

### Sandboxing
- **No I/O operations**: All operators are pure functions
- **No system access**: No file system, network, or process operations
- **Memory safety**: RAII and smart pointers prevent memory leaks
- **Exception safety**: Structured exception handling prevents crashes

### Input Validation
- **JSON parsing**: Strict JSON syntax validation
- **Operator validation**: Unknown operators throw exceptions
- **Type checking**: Runtime type validation with clear error messages
- **Bounds checking**: Array and object access validation

## Extension Points

### Adding New Operators
```cpp
operators["new_operator"] = [](const nlohmann::json& args, ExecutionContext& ctx) -> nlohmann::json {
    // Validate arguments
    if (args.size() != expected_count) {
        throw InvalidArgumentException("new_operator requires exactly N arguments");
    }
    
    // Evaluate arguments as needed
    auto arg1 = evaluate(args[0], ctx);
    
    // Implement operator logic
    // Return result
};
```

### Custom Exception Types
```cpp
class CustomException : public ComputoException {
public:
    explicit CustomException(const std::string& message) 
        : ComputoException("Custom error: " + message) {}
};
```

### Context Extensions
```cpp
// Add new fields to ExecutionContext for additional state
// Implement corresponding accessor methods
// Update context copying methods as needed
```

This technical architecture enables Computo to provide a safe, efficient, and extensible JSON-native functional programming environment with sophisticated features like tail call optimization, lexical scoping, and multi-input processing.