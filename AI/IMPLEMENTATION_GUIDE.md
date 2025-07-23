# IMPLEMENTATION_GUIDE.md

## Comprehensive Implementation Guide for 30-Operator Computo

This guide provides explicit instructions for implementing the full Computo system with 30 operators, CLI tooling, and thread safety while avoiding the over-engineering patterns identified in the original codebase.

## Phase 1: Core Infrastructure & Library (Week 1)

### 1.1 Project Setup
```bash
# Create directory structure
mkdir -p src/operators src/cli include tests

# CMakeLists.txt - Clean and focused
cmake_minimum_required(VERSION 3.15)
project(Computo VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

# Library target - single include design
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp" "src/operators/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/cli/.*")  # Exclude CLI sources
add_library(computo ${LIB_SOURCES})
target_include_directories(computo PUBLIC include)
target_link_libraries(computo nlohmann_json::nlohmann_json)

# CLI target
file(GLOB CLI_SOURCES "src/cli/*.cpp")
add_executable(computo_cli ${CLI_SOURCES})
target_link_libraries(computo_cli computo readline)  # Add readline for REPL
set_target_properties(computo_cli PROPERTIES OUTPUT_NAME computo)

# Test target
add_executable(test_computo tests/test_operators.cpp tests/test_threading.cpp)
target_link_libraries(test_computo computo GTest::gtest_main)
```

### 1.2 Exception Hierarchy (20 lines)
```cpp
// include/computo.hpp - Single public header
#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace computo {

class ComputoException : public std::exception {
public:
    explicit ComputoException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class InvalidOperatorException : public ComputoException {
public:
    explicit InvalidOperatorException(const std::string& op) 
        : ComputoException("Invalid operator: " + op) {}
};

class InvalidArgumentException : public ComputoException {
public:
    explicit InvalidArgumentException(const std::string& msg) 
        : ComputoException("Invalid argument: " + msg) {}
    explicit InvalidArgumentException(const std::string& msg, const std::string& path) 
        : ComputoException("Invalid argument: " + msg + " at " + path) {}
};
```

### 1.3 ExecutionContext (Thread-Safe Design, 60 lines)
```cpp
// In include/computo.hpp
class ExecutionContext {
private:
    std::shared_ptr<const nlohmann::json> input_ptr_;
    std::shared_ptr<const std::vector<nlohmann::json>> inputs_ptr_;
    static const nlohmann::json null_input_;
    
public:
    std::map<std::string, nlohmann::json> variables;
    std::vector<std::string> path;
    
    // Single input constructor
    explicit ExecutionContext(const nlohmann::json& input)
        : input_ptr_(std::make_shared<nlohmann::json>(input)),
          inputs_ptr_(std::make_shared<std::vector<nlohmann::json>>(
              std::vector<nlohmann::json>{input})) {}
    
    // Multiple inputs constructor
    explicit ExecutionContext(const std::vector<nlohmann::json>& inputs)
        : input_ptr_(inputs.empty() ? 
                     std::make_shared<nlohmann::json>(null_input_) :
                     std::make_shared<nlohmann::json>(inputs[0])),
          inputs_ptr_(std::make_shared<std::vector<nlohmann::json>>(inputs)) {}
    
    // Accessors
    const nlohmann::json& input() const { return *input_ptr_; }
    const std::vector<nlohmann::json>& inputs() const { return *inputs_ptr_; }
    
    // Thread-safe context creation for scoping
    ExecutionContext with_variables(const std::map<std::string, nlohmann::json>& vars) const {
        ExecutionContext new_ctx(*inputs_ptr_);
        new_ctx.variables = variables;  // Copy existing
        new_ctx.path = path;           // Copy path
        for (const auto& pair : vars) {
            new_ctx.variables[pair.first] = pair.second;  // Add new
        }
        return new_ctx;
    }
    
    ExecutionContext with_path(const std::string& segment) const {
        ExecutionContext new_ctx = *this;
        new_ctx.path.push_back(segment);
        return new_ctx;
    }
    
    std::string get_path_string() const {
        if (path.empty()) return "/";
        std::string result;
        for (const auto& seg : path) result += "/" + seg;
        return result;
    }
};

// Public API - minimal and thread-safe
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input);
nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs);

} // namespace computo
```

### 1.4 Test Infrastructure (50 lines)
```cpp
// tests/test_operators.cpp - String-based JSON testing
#include <gtest/gtest.h>
#include <computo.hpp>
#include <thread>
#include <vector>

using json = nlohmann::json;

class ComputoTest : public ::testing::Test {
protected:
    void SetUp() override {
        input_data = json{{"test", "value"}};
    }
    
    json execute_script(const std::string& script_json) {
        auto script = json::parse(script_json);
        return computo::execute(script, input_data);
    }
    
    json execute_script(const std::string& script_json, const json& input) {
        auto script = json::parse(script_json);
        return computo::execute(script, input);
    }
    
    json input_data;
};

TEST_F(ComputoTest, ExceptionHierarchy) {
    try {
        execute_script(R"(["unknown_operator"])");
        FAIL() << "Should have thrown";
    } catch (const computo::ComputoException& e) {
        SUCCEED();  // Should catch derived type
    }
}

TEST_F(ComputoTest, ExecutionContext) {
    // Test null input handling
    EXPECT_EQ(computo::execute(json(nullptr), json(nullptr)), json(nullptr));
    
    // Test multiple inputs
    std::vector<json> inputs = {json{{"first", 1}}, json{{"second", 2}}};
    auto result = computo::execute(json::array({"$input"}), inputs);
    EXPECT_EQ(result, json{{"first", 1}});
}
```

**CRITICAL:** Do not proceed to Phase 2 until these tests pass and thread safety is verified.

## MANDATORY TESTING REQUIREMENTS AND PHASE GATES

### Testing Philosophy

This implementation follows a **strict phase-gate development model** where each phase must be fully completed, tested, and validated before proceeding to the next phase. No exceptions.

### Universal Testing Requirements

#### Compilation Requirements
- **ZERO WARNINGS**: Code must compile without any warnings using `-Wall -Wextra -Werror`
- **CLEAN BUILD**: No compilation errors, no linking errors, no undefined symbols
- **DEPENDENCY VALIDATION**: All external dependencies must be properly linked and accessible

#### Test Execution Requirements
- **100% TEST PASS RATE**: All tests must pass with no failures, no skips, no exceptions
- **ZERO MEMORY LEAKS**: Valgrind or AddressSanitizer must report clean execution
- **DETERMINISTIC RESULTS**: Tests must produce identical results across multiple runs
- **THREAD SAFETY VALIDATION**: Concurrent execution tests must pass without race conditions

### Test Organization Strategy

#### Test File Structure
```
tests/
├── test_basic_operators.cpp      # +, -, *, /, comparison operators (Phase 4)
├── test_data_access.cpp          # get, $, let operators (Phase 4)  
├── test_logic_construction.cpp   # if, obj, array operations (Phase 4)
├── test_iteration_lambdas.cpp    # map, filter, lambda functionality (Phase 4)
├── test_array_utilities.cpp      # count, merge, array operations (Phase 4)
├── test_json_patch.cpp           # JSON Pointer and patch operations (Phase 4)
├── test_advanced_features.cpp    # Complex integration scenarios (Phase 6)
├── test_threading.cpp            # Thread safety validation (Phase 6)
├── test_cli.cpp                  # CLI and REPL functionality (Phase 5)
└── test_infrastructure.cpp       # Core infrastructure tests (Phase 1)
```

#### Test Coverage Requirements
- **Every Operator**: All 30 operators must have comprehensive test coverage
- **Error Conditions**: Every exception type must have corresponding test cases
- **Edge Cases**: Null values, empty arrays, type mismatches, boundary conditions
- **Integration Scenarios**: Complex nested operations, variable scoping, context management

### Phase-Specific Testing Gates

#### Phase 1 Gate Requirements
**MANDATORY BEFORE PHASE 2:**
```bash
# Must pass compilation
cmake --build build --parallel
echo $?  # Must be 0

# Must pass all infrastructure tests  
cd build && ctest --output-on-failure
# Result: 100% tests passed, 0 tests failed
```

**Required Tests:**
- Exception hierarchy validation
- ExecutionContext thread safety
- Basic evaluation infrastructure
- Memory management (no leaks)

#### Phase 2 Gate Requirements
**MANDATORY BEFORE PHASE 3:**
- Operator registry thread safety tests
- Core evaluation engine validation
- Tail-call optimization verification
- Concurrent execution baseline tests

#### Phase 3 Gate Requirements  
**MANDATORY BEFORE PHASE 4:**
- Lambda evaluation correctness
- Truthiness evaluation edge cases
- Numeric type validation
- Utility function completeness

#### Phase 4 Gate Requirements
**MANDATORY BEFORE PHASE 5:**
- All 30 operators implemented and tested
- N-ary operation semantics verified
- Error handling for all operators
- Performance regression tests passing

#### Phase 5 Gate Requirements
**MANDATORY BEFORE PHASE 6:**
- CLI argument parsing validation
- REPL functionality verification
- stdin/stdout protocol testing
- Context loading and management

#### Phase 6 Gate Requirements
**MANDATORY BEFORE COMPLETION:**
- Full system integration testing
- Thread safety under concurrent load
- Performance benchmarks met
- Memory usage within acceptable limits

### Testing Standards and Quality

#### Test Case Requirements
- **Minimum 5 test cases** per operator (positive, negative, edge cases)
- **Descriptive test names** that clearly indicate what is being tested
- **Comprehensive assertions** that validate both result and side effects
- **Independent tests** that do not depend on other test execution order

#### Test Code Quality
- **No test longer than 20 lines** (excluding test data)
- **Clear setup and teardown** for each test case
- **Meaningful test data** that represents real-world usage
- **Consistent testing patterns** across all test files

#### Error Testing Requirements
```cpp
// Example: Every operator must test invalid argument types
TEST_F(ComputoTest, PlusOperatorInvalidTypes) {
    EXPECT_THROW(computo::execute(json::array({"+", "string", 1}), json{}), 
                 computo::InvalidArgumentException);
    EXPECT_THROW(computo::execute(json::array({"+", json{}}), json{}),
                 computo::InvalidArgumentException);
}
```

### Enforcement and Compliance

#### Automated Validation
```bash
# Required command sequence for each phase gate
cmake --build build --parallel && \
cd build && \
ctest --output-on-failure && \
valgrind --leak-check=full --error-exitcode=1 ./test_computo

# All commands must return exit code 0
```

#### Manual Review Checklist
- [ ] All tests pass locally on developer machine
- [ ] Code compiles without warnings on release build
- [ ] Memory analysis shows no leaks or errors
- [ ] Thread safety tests pass under stress conditions
- [ ] Performance meets or exceeds baseline metrics

#### Failure Response Protocol
**If any requirement fails:**
1. **STOP ALL DEVELOPMENT** on subsequent phases
2. **DEBUG AND FIX** the failing requirement
3. **RE-RUN ALL TESTS** to ensure fix doesn't break other functionality
4. **DOCUMENT** the issue and resolution for future reference

**NO EXCEPTIONS:** Do not proceed to the next phase until ALL requirements are met.

### Success Metrics

#### Quantitative Requirements
- **Test Pass Rate**: 100% (no failures, no skips)
- **Code Coverage**: Minimum 95% line coverage for all operator implementations
- **Performance**: No operator regression beyond 10% of baseline
- **Memory**: No memory leaks in any test scenario

#### Qualitative Requirements
- **Test Clarity**: All tests are self-documenting and understandable
- **Test Maintainability**: Tests are easy to modify and extend
- **Test Reliability**: Tests produce consistent results across different environments
- **Test Completeness**: All major use cases and error conditions are covered

This rigorous testing approach ensures that each phase builds on a solid foundation, preventing the accumulation of technical debt and ensuring the final system meets all quality, performance, and reliability requirements.

## Phase 2: Core Evaluation Engine (Week 1 continued)

### 2.1 Operator Registry (Thread-Safe, 40 lines)
```cpp
// src/computo.cpp
#include <computo.hpp>
#include <mutex>
#include <functional>

namespace computo {

// Null input definition
const nlohmann::json ExecutionContext::null_input_ = nlohmann::json(nullptr);

namespace {
    using OperatorFunc = std::function<nlohmann::json(const nlohmann::json&, ExecutionContext&)>;
    std::map<std::string, OperatorFunc> operators;
    std::once_flag init_flag;
    
    // Forward declarations for all operators
    namespace operators {
        // Arithmetic
        nlohmann::json addition(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json subtraction(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json multiplication(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json division(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json modulo(const nlohmann::json& args, ExecutionContext& ctx);
        
        // Comparison
        nlohmann::json greater_than(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json less_than(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json greater_equal(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json less_equal(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json equal(const nlohmann::json& args, ExecutionContext& ctx);
        nlohmann::json not_equal(const nlohmann::json& args, ExecutionContext& ctx);
        
        // ... (forward declarations for all 30 operators)
    }
    
    void initialize_operators() {
        std::call_once(init_flag, []() {
            // Arithmetic
            operators["+"] = operators::addition;
            operators["-"] = operators::subtraction;
            operators["*"] = operators::multiplication;
            operators["/"] = operators::division;
            operators["%"] = operators::modulo;
            
            // Comparison
            operators[">"] = operators::greater_than;
            operators["<"] = operators::less_than;
            operators[">="] = operators::greater_equal;
            operators["<="] = operators::less_equal;
            operators["=="] = operators::equal;
            operators["!="] = operators::not_equal;
            
            // ... (register all 30 operators)
        });
    }
}
```

### 2.2 Core Evaluation Engine with TCO (80 lines)
```cpp
// Forward declaration
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx);

nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    initialize_operators();
    
    while (true) {  // Trampoline for TCO
        // Array objects: {"array": [...]}
        if (expr.is_object() && expr.contains("array")) {
            if (!expr["array"].is_array()) {
                throw InvalidArgumentException("array object must contain array", ctx.get_path_string());
            }
            nlohmann::json result = nlohmann::json::array();
            for (size_t i = 0; i < expr["array"].size(); ++i) {
                auto element_ctx = ctx.with_path("array[" + std::to_string(i) + "]");
                result.push_back(evaluate(expr["array"][i], element_ctx));
            }
            return result;
        }
        
        // Literals (not arrays)
        if (!expr.is_array()) {
            return expr;
        }
        
        // Empty arrays are invalid
        if (expr.empty()) {
            throw InvalidArgumentException("Empty array", ctx.get_path_string());
        }
        
        // Must start with operator name (string)
        if (!expr[0].is_string()) {
            throw InvalidArgumentException("Array must start with operator name", ctx.get_path_string());
        }
        
        std::string op = expr[0].get<std::string>();
        auto op_ctx = ctx.with_path(op);
        
        // Special case: $input operator
        if (op == "$input") {
            return ctx.input();
        }
        
        // TCO Special Forms
        if (op == "if") {
            if (expr.size() != 4) {
                throw InvalidArgumentException("if requires 3 arguments", op_ctx.get_path_string());
            }
            auto condition = evaluate(expr[1], op_ctx.with_path("condition"));
            bool is_true = is_truthy(condition);  // Forward declaration needed
            ctx = op_ctx.with_path(is_true ? "then" : "else");
            expr = is_true ? expr[2] : expr[3];
            continue;  // TCO
        }
        
        if (op == "let") {
            if (expr.size() != 3) {
                throw InvalidArgumentException("let requires 2 arguments", op_ctx.get_path_string());
            }
            std::map<std::string, nlohmann::json> new_vars;
            for (size_t i = 0; i < expr[1].size(); ++i) {
                const auto& binding = expr[1][i];
                if (!binding.is_array() || binding.size() != 2) {
                    throw InvalidArgumentException("binding must be [var, value]", op_ctx.get_path_string());
                }
                std::string var_name = binding[0].get<std::string>();
                new_vars[var_name] = evaluate(binding[1], op_ctx.with_path("binding[" + std::to_string(i) + "]"));
            }
            ctx = op_ctx.with_path("body").with_variables(new_vars);
            expr = expr[2];
            continue;  // TCO
        }
        
        // Regular operators
        auto it = operators.find(op);
        if (it == operators.end()) {
            throw InvalidOperatorException(op);
        }
        
        // Extract arguments (everything except operator name)
        nlohmann::json args = nlohmann::json::array();
        for (size_t i = 1; i < expr.size(); ++i) {
            args.push_back(expr[i]);
        }
        
        return it->second(args, op_ctx);
    }
}

// Public API implementation
nlohmann::json execute(const nlohmann::json& script, const nlohmann::json& input) {
    ExecutionContext ctx(input);
    return evaluate(script, ctx);
}

nlohmann::json execute(const nlohmann::json& script, const std::vector<nlohmann::json>& inputs) {
    ExecutionContext ctx(inputs);
    return evaluate(script, ctx);
}

} // namespace computo
```

## Phase 3: Shared Utilities (Week 2)

### 3.1 Common Operator Utilities (30 lines)
```cpp
// src/operators/shared.hpp (internal header)
#pragma once
#include <computo.hpp>

namespace computo::operators {

// Truthiness evaluation - consistent across all operators
bool is_truthy(const nlohmann::json& value) {
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_number()) return value.get<double>() != 0.0;
    if (value.is_string()) return !value.get<std::string>().empty();
    if (value.is_null()) return false;
    if (value.is_array() || value.is_object()) return !value.empty();
    return false;
}

// Lambda evaluation - shared by array operators
nlohmann::json evaluate_lambda(const nlohmann::json& lambda, const nlohmann::json& arg, ExecutionContext& ctx) {
    if (!lambda.is_array() || lambda.size() != 3 || lambda[0] != "lambda") {
        throw InvalidArgumentException("Invalid lambda expression");
    }
    if (!lambda[1].is_array() || lambda[1].size() != 1) {
        throw InvalidArgumentException("Lambda must have exactly one parameter");
    }
    std::string param = lambda[1][0].get<std::string>();
    std::map<std::string, nlohmann::json> vars = {{param, arg}};
    return evaluate(lambda[2], ctx.with_variables(vars));
}

// Numeric validation - shared by arithmetic/comparison operators
void validate_numeric_args(const nlohmann::json& args, const std::string& op_name) {
    for (const auto& arg : args) {
        if (!arg.is_number()) {
            throw InvalidArgumentException(op_name + " requires numeric arguments");
        }
    }
}

} // namespace computo::operators
```

## Phase 4: Operator Implementation (Week 2-3)

### 4.1 Arithmetic Operators (N-ary, 125 lines)
```cpp
// src/operators/arithmetic.cpp
#include "shared.hpp"

namespace computo::operators {

nlohmann::json addition(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("+ requires at least 1 argument");
    
    double result = 0.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("+ requires numeric arguments");
        result += arg.get<double>();
    }
    return result;
}

nlohmann::json subtraction(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("- requires at least 1 argument");
    
    auto first = evaluate(args[0], ctx);
    if (!first.is_number()) throw InvalidArgumentException("- requires numeric arguments");
    
    if (args.size() == 1) {
        return -first.get<double>();  // Unary negation
    }
    
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) throw InvalidArgumentException("- requires numeric arguments");
        result -= arg.get<double>();
    }
    return result;
}

nlohmann::json multiplication(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("* requires at least 1 argument");
    
    double result = 1.0;
    for (const auto& arg_expr : args) {
        auto arg = evaluate(arg_expr, ctx);
        if (!arg.is_number()) throw InvalidArgumentException("* requires numeric arguments");
        result *= arg.get<double>();
    }
    return result;
}

nlohmann::json division(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.empty()) throw InvalidArgumentException("/ requires at least 1 argument");
    
    auto first = evaluate(args[0], ctx);
    if (!first.is_number()) throw InvalidArgumentException("/ requires numeric arguments");
    
    if (args.size() == 1) {
        return 1.0 / first.get<double>();  // Reciprocal
    }
    
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) throw InvalidArgumentException("/ requires numeric arguments");
        double divisor = arg.get<double>();
        if (divisor == 0.0) throw InvalidArgumentException("Division by zero");
        result /= divisor;
    }
    return result;
}

nlohmann::json modulo(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("% requires at least 2 arguments");
    
    auto first = evaluate(args[0], ctx);
    if (!first.is_number()) throw InvalidArgumentException("% requires numeric arguments");
    
    double result = first.get<double>();
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (!arg.is_number()) throw InvalidArgumentException("% requires numeric arguments");
        double divisor = arg.get<double>();
        if (divisor == 0.0) throw InvalidArgumentException("Modulo by zero");
        result = std::fmod(result, divisor);
    }
    return result;
}

} // namespace computo::operators
```

### 4.2 Comparison Operators (N-ary with chaining, 150 lines)
```cpp
// src/operators/comparison.cpp
#include "shared.hpp"

namespace computo::operators {

nlohmann::json greater_than(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("> requires at least 2 arguments");
    
    // Chained comparison: [">", a, b, c] means a > b && b > c
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

nlohmann::json less_than(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("< requires at least 2 arguments");
    
    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto a = evaluate(args[i], ctx);
        auto b = evaluate(args[i + 1], ctx);
        if (!a.is_number() || !b.is_number()) {
            throw InvalidArgumentException("< requires numeric arguments");
        }
        if (!(a.get<double>() < b.get<double>())) {
            return false;
        }
    }
    return true;
}

// Similar implementations for >=, <=

nlohmann::json equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() < 2) throw InvalidArgumentException("== requires at least 2 arguments");
    
    // All arguments must be equal to each other
    auto first = evaluate(args[0], ctx);
    for (size_t i = 1; i < args.size(); ++i) {
        auto arg = evaluate(args[i], ctx);
        if (first != arg) {  // nlohmann::json has built-in equality
            return false;
        }
    }
    return true;
}

nlohmann::json not_equal(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() != 2) throw InvalidArgumentException("!= requires exactly 2 arguments");
    
    auto a = evaluate(args[0], ctx);
    auto b = evaluate(args[1], ctx);
    return a != b;
}

} // namespace computo::operators
```

### 4.3 Continue with remaining operator categories...

**Critical Implementation Rules for All Operators:**

1. **Size Limits**: Each operator < 25 lines
2. **N-ary Consistency**: All operators accept multiple arguments (except `not`, `!=`)
3. **Error Handling**: Simple exception throwing, clear messages
4. **No Duplication**: Use shared utilities from `shared.hpp`
5. **Thread Safety**: Pure functions, no global state
6. **NO EMOJIS**: No emoji characters in any code, error messages, or output

## Phase 5: CLI Implementation (Week 4)

### 5.1 Main CLI Application
```cpp
// src/cli/main.cpp
#include <computo.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

struct CLIOptions {
    bool trace = false;
    bool profile = false;
    bool comments = false;
    bool repl = false;
    std::string break_on_operator;
    std::string watch_variable;
    int pretty_spaces = 0;
    std::vector<std::string> files;
};

CLIOptions parse_arguments(int argc, char* argv[]) {
    CLIOptions opts;
    
    // Parse switches (must come before files)
    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        std::string arg = argv[i];
        
        if (arg == "--trace") {
            opts.trace = true;
        } else if (arg == "--profile") {
            opts.profile = true;
        } else if (arg == "--comments") {
            opts.comments = true;
        } else if (arg == "--repl") {
            opts.repl = true;
        } else if (arg.starts_with("--break-on=")) {
            opts.break_on_operator = arg.substr(11);
        } else if (arg.starts_with("--watch=")) {
            opts.watch_variable = arg.substr(8);
        } else if (arg.starts_with("--pretty=")) {
            opts.pretty_spaces = std::stoi(arg.substr(9));
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            exit(1);
        }
        i++;
    }
    
    // Remaining arguments are files
    while (i < argc) {
        opts.files.push_back(argv[i++]);
    }
    
    return opts;
}

int main(int argc, char* argv[]) {
    try {
        CLIOptions opts = parse_arguments(argc, argv);
        
        if (opts.repl) {
            // Start REPL (potentially with context files)
            return run_repl(opts);
        }
        
        if (opts.files.empty()) {
            std::cerr << "Usage: computo [options] script.json [context.json ...]" << std::endl;
            return 1;
        }
        
        // Load script
        auto script = load_json_file(opts.files[0], opts.comments);
        
        // Load context files
        std::vector<nlohmann::json> inputs;
        for (size_t i = 1; i < opts.files.size(); ++i) {
            inputs.push_back(load_json_file(opts.files[i], opts.comments));
        }
        
        // Execute with debugging if requested
        nlohmann::json result;
        if (opts.trace || opts.profile || !opts.break_on_operator.empty()) {
            result = execute_with_debugging(script, inputs, opts);
        } else {
            result = inputs.empty() ? 
                computo::execute(script, nlohmann::json(nullptr)) :
                computo::execute(script, inputs);
        }
        
        // Output result
        if (opts.pretty_spaces > 0) {
            std::cout << result.dump(opts.pretty_spaces) << std::endl;
        } else {
            std::cout << result.dump() << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

### 5.2 REPL Implementation with Readline
```cpp
// src/cli/repl.cpp
#include <readline/readline.h>
#include <readline/history.h>
#include <computo.hpp>
#include <iostream>

int run_repl(const CLIOptions& opts) {
    std::cout << "Computo REPL - Type 'exit' or Ctrl+D to quit" << std::endl;
    
    // Load context files if provided
    std::vector<nlohmann::json> context_inputs;
    for (size_t i = 0; i < opts.files.size(); ++i) {
        context_inputs.push_back(load_json_file(opts.files[i], opts.comments));
        std::cout << "Loaded context[" << i << "] from " << opts.files[i] << std::endl;
    }
    
    char* line;
    while ((line = readline("computo> ")) != nullptr) {
        std::string input(line);
        
        if (input == "exit" || input == "quit") {
            free(line);
            break;
        }
        
        if (!input.empty()) {
            add_history(line);
            
            try {
                auto script = nlohmann::json::parse(input);
                nlohmann::json result;
                
                if (context_inputs.empty()) {
                    result = computo::execute(script, nlohmann::json(nullptr));
                } else {
                    result = computo::execute(script, context_inputs);
                }
                
                if (opts.pretty_spaces > 0) {
                    std::cout << result.dump(opts.pretty_spaces) << std::endl;
                } else {
                    std::cout << result.dump() << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        }
        
        free(line);
    }
    
    return 0;
}
```

## Phase 6: Testing and Thread Safety Validation (Week 4)

### 6.1 Comprehensive Operator Tests
```cpp
// tests/test_operators.cpp - String-based JSON tests
TEST_F(ComputoTest, ArithmeticOperators) {
    // N-ary addition
    EXPECT_EQ(execute_script(R"(["+", 1, 2, 3])"), 6);
    
    // N-ary multiplication
    EXPECT_EQ(execute_script(R"(["*", 2, 3, 4])"), 24);
    
    // Subtraction with multiple args
    EXPECT_EQ(execute_script(R"(["-", 10, 3, 2])"), 5);
}

TEST_F(ComputoTest, ComparisonOperators) {
    // Chained comparison
    EXPECT_EQ(execute_script(R"([">", 5, 3, 1])"), true);
    EXPECT_EQ(execute_script(R"([">", 5, 3, 4])"), false);
    
    // Equality with multiple args
    EXPECT_EQ(execute_script(R"(["==", 5, 5, 5])"), true);
    EXPECT_EQ(execute_script(R"(["==", 5, 5, 6])"), false);
}

TEST_F(ComputoTest, VariableKeysInObj) {
    std::string script = R"(
        ["let", [["key", "name"], ["value", "Alice"]], 
         ["obj", [["$", "/key"], ["$", "/value"]]]]
    )";
    
    json expected = json{{"name", "Alice"}};
    EXPECT_EQ(execute_script(script), expected);
}

TEST_F(ComputoTest, FunctionalProgramming) {
    // Test car, cdr, cons for TCO learning
    EXPECT_EQ(execute_script(R"(["car", {"array": [1, 2, 3]}])"), 1);
    EXPECT_EQ(execute_script(R"(["cdr", {"array": [1, 2, 3]}])"), json::array({2, 3}));
    EXPECT_EQ(execute_script(R"(["cons", 0, {"array": [1, 2, 3]}])"), json::array({0, 1, 2, 3}));
}
```

### 6.2 Thread Safety Tests
```cpp
// tests/test_threading.cpp
TEST(ThreadSafetyTest, ConcurrentExecution) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    json script = json::array({"+", t, i});
                    json result = computo::execute(script, json{});
                    if (result == t + i) {
                        success_count++;
                    }
                } catch (...) {
                    // Thread safety failure
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);
}
```

## Success Criteria and Quality Gates

### Code Quality Metrics
- **Library code**: < 1500 lines total
- **CLI code**: < 800 lines total  
- **Test code**: < 800 lines total
- **Any single operator**: < 25 lines
- **No code duplication**: Shared utilities only

### Functional Requirements
- All 30 operators work with n-ary semantics
- Variable keys work in `obj` operator
- Thread safety verified with concurrent tests
- CLI provides all debugging features
- REPL has readline support and history

### Performance Requirements
- TCO prevents stack overflow on deep recursion
- Thread-safe execution without locks in hot path
- Operator lookup is O(1)
- Context copying is efficient with shared pointers

### Anti-Pattern Prevention
- **No debugging in library code**
- **No global mutable state**
- **No code duplication between operator files**
- **No mixed binary/n-ary complexity**
- **No elaborate abstractions for simple operations**

This implementation guide scales the proven minimal approach to support the full feature set while maintaining the clean architecture and avoiding the over-engineering patterns that plagued the original codebase.
