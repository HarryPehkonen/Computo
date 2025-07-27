# DEBUG_AND_REPL.md

## Computo Debug and REPL Features Documentation

This document provides comprehensive documentation for the debug and REPL (Read-Eval-Print Loop) features of the Computo JSON transformation engine. These features enable productive development, troubleshooting, and performance analysis using a clean conditional compilation architecture.

## Core Architecture Principles

### 1. Conditional Compilation Design
- **Production Builds**: `cmake -B build -DCMAKE_BUILD_TYPE=Release` - Zero debug overhead, minimal binary
- **REPL Builds**: `cmake -B build -DREPL=ON -DCMAKE_BUILD_TYPE=Release` - Full debugging and REPL features
- **Single Codebase**: `#ifdef REPL` blocks provide clean feature separation
- **Clear Distinction**: `NDEBUG` is for assert statements only, `REPL` is for user-facing features

### 2. Performance Philosophy
- **Zero Overhead Production**: Production builds contain NO debug code paths
- **Full Featured Development**: REPL builds include comprehensive debugging infrastructure
- **Clean Library Core**: Core library evaluation engine remains pure and fast
- **Debug Wrapper Pattern**: Debug features wrap library calls, never modify core evaluation

### 3. Feature Separation Strategy
- **Production Binary**: Fast execution, minimal dependencies, automation-friendly
- **REPL Binary**: Interactive development, debugging, profiling, exploration
- **Shared Core**: Both builds use identical core library evaluation engine
- **Clear Use Cases**: Obvious choice between production vs development tools

## Build Configuration

### Production Build
```bash
# Fast execution, minimal binary, zero debug overhead
cmake -B build-prod -DCMAKE_BUILD_TYPE=Release
cmake --build build-prod

# Usage: Direct script execution
./build-prod/computo script.json input.json
```

### REPL Build  
```bash
# Full debugging and REPL features
cmake -B build-repl -DREPL=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-repl

# Usage: Interactive development
./build-repl/computo --repl input.json
./build-repl/computo --debug --trace script.json input.json
```

### Assert Debugging (Both Builds)
```bash
# Enable internal assertion checks for development
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
# NDEBUG is automatically undefined in Debug builds

# Disable assertions for performance testing
cmake -B build-prod -DCMAKE_BUILD_TYPE=Release -DNDEBUG=ON
# NDEBUG removes assert() statements but preserves REPL features
```

## Debug Features Overview (REPL Builds Only)

### REPL Debug Commands (Currently Implemented)

#### Basic REPL Commands
```bash
# Start REPL
computo_repl [input.json]

# Available commands within REPL:
computo> help      # Show available commands
computo> vars      # Show variables in current scope
computo> debug     # Toggle debug mode 
computo> trace     # Toggle trace mode
computo> history   # Show command history
computo> clear     # Clear command history
computo> quit      # Exit REPL
```

#### Script Execution Commands
```bash
# Load and execute script files
computo> run script.json        # Execute JSON script file
computo> run script.jsonc       # Execute JSON with comments (.jsonc)
```

#### Breakpoint Management Commands
```bash
# Set breakpoints
computo> break map              # Break on 'map' operator
computo> break /users           # Break on '/users' variable access
computo> break +                # Break on arithmetic operators

# Remove breakpoints
computo> nobreak map            # Remove 'map' operator breakpoint
computo> nobreak /users         # Remove '/users' variable breakpoint
computo> nobreak                # Remove all breakpoints

# List breakpoints
computo> breaks                 # Show all active breakpoints
```

#### Interactive Debug Session Commands
```bash
# Debug session control (when script is paused at breakpoint)
(debug) step                    # Execute next operation
(debug) continue                # Continue until next breakpoint
(debug) finish                  # Complete execution, clear breakpoints
(debug) where                   # Show current execution location
(debug) vars                    # Show variables in current scope
```

#### Variable Inspection (`vars` command)
```bash
# Example session showing vars command
computo> ["let", [["x", 42], ["y", ["+", ["$", "/x"], 8]]], ["*", ["$", "/x"], ["$", "/y"]]]
2100
computo> vars
Variables in scope:
  x = 42
  y = 50.0
```

#### Debug Infrastructure Foundation
The REPL uses a `DebugExecutionWrapper` pattern that:
- Captures variable state from `let` expressions
- Provides interactive debugging with breakpoints and stepping
- Supports operator suggestion system with Levenshtein distance
- Enables JSON comments parsing for script files
- Maintains zero impact on production builds
- Supports performance timing collection (when enabled)

#### Operator Error Suggestions
```bash
# Automatic suggestions for misspelled operators
computo> ["mpa", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
Error: Invalid operator: mpa. Did you mean 'map'?

computo> ["redcue", {"array": [1, 2, 3, 4]}, ["lambda", ["args"], ["+", ["get", ["$", "/args"], "/0"], ["get", ["$", "/args"], "/1"]]], 0]
Error: Invalid operator: redcue. Did you mean 'reduce'?
```

#### JSON Comments Support
```jsonc
// Script files with .jsonc extension support comments
[
    "let",
    [
        ["data", {"array": [1, 2, 3]}],  // Initial data
        ["doubled", ["map", ["$", "/data"], /* Transform */ ["lambda", ["x"], ["*", ["$", "/x"], 2]]]]
    ],
    ["$", "/doubled"]  // Return transformed data
]
```

### Future Debug Features (Planned Implementation)

The `DebugExecutionWrapper` infrastructure supports future implementation of:

```
Planned Commands:
  trace <expr>    - Show detailed execution path through nested operators
  profile         - Show comprehensive performance timing statistics
  watch <var>     - Monitor variable changes with notifications
```

### Current Debug Output Format

#### Variable Inspection Output
```
# When variables exist
computo> vars
Variables in scope:
  users = [{"age":30,"name":"Alice"},{"age":25,"name":"Bob"}]
  count = 2
  names = ["Alice","Bob"]

# When no variables in scope
computo> vars
No variables in scope
(Execute a 'let' expression to create variables)
```

#### Interactive Debug Session Format
```
computo> break let
Added operator breakpoint: let
computo> run script.json

*** Breakpoint hit ***
Location: /
Operator: let
Expression: ["let",[["x",42],["y",50]],["+",["$","/x"],["$","/y"]]]
Depth: 0
(debug) where
Location: /
Operator: let
Expression: ["let",[["x",42],["y",50]],["+",["$","/x"],["$","/y"]]]
Depth: 0
(debug) vars
No variables in scope
(debug) continue
92.0
computo>
```

#### REPL Session Format
```
Computo REPL v1.0.0
Type 'help' for commands, 'quit' to exit
Use _1, _2, etc. to reference previous commands

computo> run data_script.json
{"result": [2, 4, 6, 8]}
computo> vars
Variables in scope:
  data = {"array":[1,2,3,4]}
  doubled = {"array":[2,4,6,8]}
computo> break map
Added operator breakpoint: map
computo> breaks
Active breakpoints:
  Operator: map
computo> quit
Goodbye!
```

## REPL Features Overview (REPL Builds Only)

### Starting the REPL

#### Basic REPL
```bash
# Start REPL without context (REPL build required)
computo_repl

# REPL with preloaded context files
computo_repl data.json config.json

# Note: --debug and --trace options planned for future implementation
```

#### stdin/stdout Protocol
```bash
# Batch processing multiple scripts
echo '["count", ["$", "/users"]]' | computo data.json
echo '["map", ["$", "/users"], ["lambda", ["u"], ["get", ["$", "/u"], "/name"]]]' | computo data.json

# Pipeline integration
generate_scripts.py | computo data.json | analyze_results.py

# Testing framework integration
cat test_scripts/*.json | computo test_data.json

# Exit on "exit" command or EOF
echo -e '["count", ["$", "/users"]]\nexit' | computo data.json
```

#### REPL Session Example
```
$ computo_repl data.json
Computo REPL v1.0.0
Type 'help' for commands, 'quit' to exit
Use _1, _2, etc. to reference previous commands

computo> ["let", [["users", ["$input"]], ["names", ["map", ["$", "/users"], ["lambda", ["u"], ["get", ["$", "/u"], "/name"]]]]], ["$", "/names"]]
["Alice", "Bob"]

computo> vars
Variables in scope:
  names = ["Alice","Bob"]
  users = [{"age":30,"name":"Alice"},{"age":25,"name":"Bob"}]

computo> ["count", ["$", "/names"]]
2

computo> exit
Goodbye!
```

### REPL Expression Shortcuts

#### Variable References
```bash
# Shortcut syntax
users          # Expands to: ["$", "/users"]
config         # Expands to: ["$", "/config"] 
data           # Expands to: ["$", "/data"]

# Original syntax still works
["$", "/users"]
```

#### History References  
```bash
_1             # Result labeled [1] (first expression result)
_2             # Result labeled [2] (second expression result)  
_3             # Result labeled [3] (third expression result)

# Usage example:
computo> ["+", 1, 2]
[1] 3
computo> ["*", 5, 6]
[2] 30
computo> ["+", _1, _2]
[3] 33
computo> ["count", [_1, _2, _3]]
[4] 3
```

#### Simple Arithmetic
```bash
# Simple arithmetic parsing
users + 1      # Expands to: ["+", ["$", "/users"], 1]
count * 2      # Expands to: ["*", ["$", "/count"], 2]
```

### REPL with Readline Support

When compiled with readline support (`COMPUTO_USE_READLINE`):

#### Command History
- **Up/Down arrows**: Navigate command history
- **Ctrl+R**: Reverse search through history
- **History persistence**: Commands saved between sessions

#### Line Editing
- **Ctrl+A**: Move to beginning of line
- **Ctrl+E**: Move to end of line  
- **Ctrl+U**: Delete from cursor to beginning
- **Ctrl+K**: Delete from cursor to end
- **Tab completion**: Variable and operator name completion

## Implementation Architecture

### Conditional Compilation Structure

```cpp
// main.cpp - Single source with clean feature separation
int main(int argc, char* argv[]) {
#ifdef REPL
    return main_repl(argc, argv);
#else
    return main_production(argc, argv);
#endif
}

#ifdef REPL
int main_repl(int argc, char* argv[]) {
    // Full CLI parsing with debug options
    // REPL setup and debugging infrastructure  
    // Interactive features and stdin/stdout protocol
}
#endif

int main_production(int argc, char* argv[]) {
    // Minimal CLI parsing (script and input files only)
    // Direct execution path with zero debug overhead
    // No debug or REPL dependencies
}
```

### Source Organization (Actual Implementation)

```
src/
├── cli.cpp                         # Production CLI (build-prod)
├── repl.cpp                        # REPL with DebugExecutionWrapper (build-repl)
├── computo.cpp                     # Core library - always pure and fast
├── operators/                      # Core operators - no debug code
│   ├── arithmetic.cpp              # +, -, *, /, %
│   ├── comparison.cpp              # >, <, >=, <=, ==, !=
│   ├── logical.cpp                 # &&, ||, not
│   ├── array.cpp                   # map, filter, reduce, count, find, some, every
│   ├── functional.cpp              # car, cdr, cons, append
│   ├── data.cpp                    # $, $input, $inputs, get, let, obj, if
│   ├── utilities.cpp               # strConcat, merge, approx
│   └── stubs.cpp                   # Placeholder implementations
├── benchmark.cpp                   # Performance benchmarks
└── read_json.hpp                   # JSON file reading utility

include/
└── computo.hpp                     # Single header API

tests/
├── test_*.cpp                      # Unit tests for each operator category
└── test_integration.cpp            # End-to-end testing
```

### Build System Integration (Actual Implementation)

```cmake
# CMakeLists.txt
option(REPL "Enable REPL and debugging features" OFF)

# Core library - always minimal and fast
add_library(computo ${COMPUTO_SOURCES})
target_include_directories(computo PUBLIC ${INCLUDE_DIR})
target_link_libraries(computo PUBLIC nlohmann_json::nlohmann_json)

# Conditional executable based on REPL flag
if(REPL)
    # REPL-enabled build: computo_repl binary
    add_executable(computo_app src/repl.cpp src/benchmark.cpp)
    target_link_libraries(computo_app PRIVATE computo ${READLINE_LIB})
    target_compile_definitions(computo_app PRIVATE REPL)
    set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo_repl)
else()
    # Production CLI build: computo binary
    add_executable(computo_app src/cli.cpp src/benchmark.cpp)
    target_link_libraries(computo_app PRIVATE computo)
    set_target_properties(computo_app PROPERTIES OUTPUT_NAME computo)
endif()
```

### Performance Design

#### Production Builds (Zero Overhead)
```cpp
// src/cli.cpp - No debug code paths exist in binary
int main(int argc, char* argv[]) {
    // Direct execution - fastest possible
    auto result = computo::execute(script, inputs);
    std::cout << result.dump(2) << std::endl;
    return 0;
}
```

#### REPL Builds (Debug Infrastructure)
```cpp
// src/repl.cpp - Debug wrapper around clean library call
void execute_expression(const std::string& input) {
    nlohmann::json script = nlohmann::json::parse(input);
    
    // Configure debug wrapper based on current modes
    debug_wrapper_.enable_trace(trace_mode_);
    debug_wrapper_.enable_profiling(debug_mode_);
    
    // Extract variables from let expressions for debugging
    debug_wrapper_.extract_variables_from_let(script, input_data_);
    
    // Execute with debug wrapper
    auto result = debug_wrapper_.execute_with_debug(script, input_data_);
    
    std::cout << result.dump(2) << std::endl;
}
```

### Thread Safety Design

#### Core Library Thread Safety
- **Pure Functions**: Core evaluation engine remains pure in both builds
- **No Shared State**: Operator registry initialized once, then read-only
- **Concurrent Execution**: Multiple threads can execute simultaneously

#### Debug Thread Safety (REPL Builds Only)
```cpp
#ifdef REPL
// Thread-local debugger instances
thread_local std::unique_ptr<Debugger> current_debugger;

// RAII scope management
class DebugScope {
public:
    DebugScope(Debugger* debugger) {
        previous_debugger_ = current_debugger.get();
        current_debugger.reset(debugger);
    }
    
    ~DebugScope() {
        current_debugger.reset(previous_debugger_);
    }
};
#endif
```

### Memory Management

#### Production Builds
- **Zero Debug Memory**: No debug structures allocated
- **Minimal Memory Footprint**: Only core evaluation data structures
- **No Cleanup Overhead**: No debug resources to manage

#### REPL Builds  
```cpp
#ifdef REPL
// RAII compliance for debug resources
class DebugScope {
    std::unique_ptr<Debugger> debugger_;
public:
    DebugScope(std::unique_ptr<Debugger> debugger) : debugger_(std::move(debugger)) {}
    ~DebugScope() { /* Automatic cleanup */ }
};

// Smart pointer usage
std::unique_ptr<computo::Debugger> debugger;
std::shared_ptr<ExecutionContext> debug_context;
#endif
```

## Debug Feature Implementation Details

### DebugExecutionWrapper Pattern (Currently Implemented)

#### Wrapper Architecture
```cpp
class DebugExecutionWrapper {
private:
    bool trace_enabled_ = false;
    bool step_mode_ = false;
    bool profiling_enabled_ = false;
    std::map<std::string, nlohmann::json> last_variables_;
    
public:
    // Configuration methods
    void enable_trace(bool enabled);
    void enable_profiling(bool enabled);
    
    // Execution with debugging
    nlohmann::json execute_with_debug(const nlohmann::json& script, 
                                      const std::vector<nlohmann::json>& inputs);
    
    // Variable extraction from let expressions
    void extract_variables_from_let(const nlohmann::json& script, 
                                    const std::vector<nlohmann::json>& inputs);
    
    // State access
    const std::map<std::string, nlohmann::json>& get_last_variables() const;
};
```

#### Variable Capture Implementation
```cpp
// Extract and evaluate variables from let expressions
void extract_variables_from_let(const nlohmann::json& script, 
                                const std::vector<nlohmann::json>& inputs) {
    last_variables_.clear();
    if (script.is_array() && script.size() > 0 && script[0].is_string()) {
        std::string op = script[0].get<std::string>();
        if (op == "let" && script.size() == 3 && script[1].is_array()) {
            computo::ExecutionContext ctx(inputs);
            
            // Evaluate each binding in sequence
            for (const auto& binding : script[1]) {
                if (binding.is_array() && binding.size() == 2 && binding[0].is_string()) {
                    std::string var_name = binding[0].get<std::string>();
                    nlohmann::json evaluated_value = computo::evaluate(binding[1], ctx);
                    last_variables_[var_name] = evaluated_value;
                    ctx.variables[var_name] = evaluated_value;  // For subsequent bindings
                }
            }
        }
    }
}
```

### Future Debug Features (Planned)

#### Breakpoint System (Planned)
- Operator breakpoints: `breakpoint map`
- Path breakpoints: `breakpoint /let/body/map`
- Conditional breakpoints with custom logic

#### Performance Profiling (Planned)
- Operator-level timing statistics
- Slow operation detection with configurable thresholds
- Memory usage tracking

#### Interactive Debugging (Planned)
- Step-through execution
- Variable watching with change notifications
- Execution path visualization

## REPL Implementation Details

### Expression Parsing Pipeline

#### Parse Priority Order
1. **Empty/Whitespace**: Return null
2. **History References**: `_1`, `_2`, `_3`
3. **Variable Shortcuts**: `users` → `["$", "/users"]`
4. **Simple Arithmetic**: `users + 1` → `["+", ["$", "/users"], 1]`
5. **Full JSON**: Parse as complete JSON expression

#### Error Handling
```cpp
// Graceful error handling in REPL
try {
    auto result = computo::execute(parsed_expression, context);
    // Add to history and display
} catch (const computo::ComputoException& e) {
    std::cout << "Error: " << e.what() << std::endl;
    // Continue REPL session
}
```

### Context Management

#### Input Loading
```cpp
// Single input file
ComputoREPL repl(debugger.get());
repl.load_context(input_data);

// Multiple input files  
repl.load_contexts({input1, input2, input3});
// $input = input1, $inputs = [input1, input2, input3]
```

#### Variable Scope
- **Persistent Context**: Loaded files remain available throughout session
- **Expression Results**: History values accessible via `_1` (result [1]), `_2` (result [2]), `_3` (result [3]), etc.
- **Clean Separation**: REPL variables don't interfere with script execution

### Readline Integration

#### Conditional Compilation
```cpp
#ifdef COMPUTO_USE_READLINE
    // Full readline support with history
    char* input = readline(prompt.c_str());
    add_history(input);
#else
    // Fallback to basic input
    std::getline(std::cin, input);
#endif
```

#### History Management
- **Session History**: Commands saved during current session
- **Persistent History**: History file for command persistence (future enhancement)
- **History Filtering**: Empty and duplicate commands filtered out

## CLI Integration Patterns

### Argument Parsing Strategy

#### Debug-First Parsing
```cpp
// Debug options parsed before other options
// This ensures debug infrastructure is set up early
bool debug_enabled = false;
std::unique_ptr<computo::Debugger> debugger = nullptr;

// Parse debug options first
if (arg == "--debug" || arg == "--trace" || arg == "--profile") {
    if (!debugger) {
        debugger = std::make_unique<computo::Debugger>();
    }
    // Configure debug features
}
```

#### Option Validation
- **Mutual Dependencies**: Some options require debug mode
- **Sensible Defaults**: Reasonable behavior when options conflict
- **Clear Error Messages**: Helpful guidance for incorrect usage

### Error Handling Integration

#### Enhanced Error Reporting
```cpp
void print_enhanced_error(const std::exception& e, computo::Debugger* debugger) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    
    if (debugger && debugger->is_tracing_enabled()) {
        std::cerr << "EXECUTION TRACE:" << std::endl;
        std::cerr << debugger->get_execution_report() << std::endl;
    }
    
    // Additional debugging tips
}
```

#### Graceful Degradation
- **Debug Failures**: Debug errors don't prevent script execution
- **Missing Dependencies**: Fallback behavior when readline unavailable
- **Resource Limits**: Sensible limits on debug data collection

## Performance Considerations

### Debug Overhead Management

#### Conditional Execution
```cpp
// Debug operations only when needed
if (debugger && debugger->is_tracing_enabled()) {
    debugger->trace_operation(op_name, args);
}

// Zero overhead when debugging disabled
if (!debugger) {
    // Direct execution path with no debug overhead
    return computo::execute(script, input);
}
```

#### Memory Efficiency
- **Bounded Collections**: Debug data structures have size limits
- **Lazy Initialization**: Debug structures created only when needed
- **Automatic Cleanup**: RAII ensures timely resource release

### Profiling Accuracy

#### High-Resolution Timing
```cpp
// Precise timing for performance measurement
auto start = std::chrono::high_resolution_clock::now();
auto result = evaluate_operator(args, context);
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
```

#### Statistical Accuracy
- **Multiple Measurements**: Statistics based on multiple executions
- **Outlier Detection**: Identify and report unusual performance spikes
- **Context Awareness**: Performance correlated with input data characteristics

## Integration Testing

### Debug Feature Testing

#### Unit Tests
```cpp
TEST(DebuggerTest, BasicBreakpoints) {
    auto debugger = std::make_unique<computo::Debugger>();
    debugger->set_breakpoint_on_operator("map");
    
    // Test breakpoint triggering
    bool breakpoint_hit = false;
    debugger->set_breakpoint_handler([&](const DebugContext& ctx) {
        breakpoint_hit = true;
        return BreakpointAction::CONTINUE;
    });
    
    // Execute script with map operator
    // Verify breakpoint was hit
    EXPECT_TRUE(breakpoint_hit);
}
```

#### Integration Tests
- **CLI Option Parsing**: Verify all debug options work correctly
- **REPL Session Testing**: Test complete REPL interactions
- **Performance Testing**: Verify profiling data accuracy
- **Error Handling**: Test error conditions with debug enabled

### Cross-Platform Testing

#### Readline Availability
```cpp
// Test both with and without readline
#ifdef COMPUTO_USE_READLINE
    // Test full readline functionality
#else
    // Test fallback input handling
#endif
```

#### Thread Safety Validation
- **Concurrent Debugging**: Multiple threads with independent debuggers
- **Global Statistics**: Verify atomic operations work correctly
- **Resource Management**: No memory leaks under concurrent load

## Style and Quality Guidelines

### Code Organization

#### Small, Focused Functions
```cpp
// GOOD: Small, single-purpose functions
std::string format_duration(std::chrono::duration<double, std::milli> duration) {
    return std::to_string(duration.count()) + "ms";
}

void print_operator_stats(const OperatorStats& stats) {
    std::cout << stats.name << ": " << stats.call_count << " calls, "
              << "avg " << format_duration(stats.average_time()) << std::endl;
}
```

#### Clean Interfaces
```cpp
// GOOD: Clear, minimal interfaces
class Debugger {
public:
    void enable_execution_trace(bool enabled);
    void set_breakpoint_on_operator(const std::string& op_name);
    void add_variable_watch(const std::string& var_name);
    
    // No complex configuration objects or overloaded APIs
};
```

### Error Handling

#### Descriptive Error Messages
```cpp
// GOOD: Clear, actionable error messages
if (!script.is_array()) {
    throw InvalidArgumentException(
        "Script must be a JSON array starting with operator name. "
        "Example: [\"+\", 1, 2] or [\"map\", array, lambda]"
    );
}
```

#### NO EMOJIS
```cpp
// GOOD: Professional, clean messages
std::cout << "Execution successful in " << duration << "ms" << std::endl;
std::cout << "Breakpoint hit at operator: " << op_name << std::endl;

// NEVER USE: Emojis or visual decorations
// std::cout << "  Execution successful!" << std::endl;  // NO
// std::cout << "  Debug mode enabled" << std::endl;    // NO
```

### Documentation Requirements

#### Code Comments
```cpp
// GOOD: Explain WHY, not WHAT
// Use readline for better editing experience when available
#ifdef COMPUTO_USE_READLINE
    char* input = readline(prompt.c_str());
#endif

// Thread-local storage ensures each thread has independent debug state
thread_local std::unique_ptr<Debugger> thread_debugger;
```

#### API Documentation
- **Function Purpose**: What does this function accomplish?
- **Parameter Requirements**: Valid input ranges and types
- **Exception Behavior**: What exceptions can be thrown?
- **Thread Safety**: Is this function thread-safe?

## Anti-Patterns to Avoid

### Debug Infrastructure in Core Library

#### DON'T: Always-Present Debug Code
```cpp
// BAD: Debug hooks always present in core evaluation
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    if (debug_enabled) {  // NO - overhead even when false
        log_trace("Evaluating: " + expr.dump());
    }
    
    auto start = std::chrono::high_resolution_clock::now();  // NO - always timing
    auto result = do_evaluation(expr, ctx);
    auto end = std::chrono::high_resolution_clock::now();    // NO - overhead
    
    GlobalDebugStats::increment_operations();  // NO - atomic operation overhead
    return result;
}
```

#### DO: Conditional Compilation
```cpp
// GOOD: Zero overhead when REPL not defined
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
#ifdef REPL
    return evaluate_with_debug(expr, ctx);
#else
    // Direct evaluation - no debug code paths exist
    auto it = operators.find(op);
    return it->second(args, ctx);
#endif
}

#ifdef REPL
nlohmann::json evaluate_with_debug(nlohmann::json expr, ExecutionContext ctx) {
    // Full debug instrumentation only when REPL build
    auto debugger = get_current_debugger();
    if (debugger) {
        debugger->trace_operation(expr);
    }
    return do_evaluation(expr, ctx);
}
#endif
```

### Confusing NDEBUG with Feature Flags

#### DON'T: Mix Assert Debugging with User Features
```cpp
// BAD: Using NDEBUG for user-facing features  
#ifndef NDEBUG
    bool enable_repl_features = true;        // NO - wrong flag
    bool enable_performance_profiling = true; // NO - not for asserts
#endif

// BAD: Using custom debug flag for assertions
#ifdef REPL
    assert(input.is_object());               // NO - use NDEBUG for asserts
#endif
```

#### DO: Clear Separation of Concerns
```cpp
// GOOD: NDEBUG only for assert statements
#ifndef NDEBUG
    assert(input.is_object());               // YES - internal debugging
    assert(script.is_array());               // YES - developer assertions
#endif

// GOOD: REPL flag only for user-facing features
#ifdef REPL
    bool enable_repl_mode = true;            // YES - user feature
    bool enable_interactive_debugging = true; // YES - user feature
    bool enable_performance_profiling = true; // YES - user feature
#endif
```

### Global Debug State (REPL Builds)

#### DON'T: Shared Mutable Debug State
```cpp
#ifdef REPL
// BAD: Global debug state breaks thread safety
namespace computo {
    bool global_debug_enabled = false;           // NO
    std::vector<std::string> global_trace_log;   // NO
    DebugContext current_debug_context;          // NO
}
#endif
```

#### DO: Thread-Local Debug Management
```cpp
#ifdef REPL
// GOOD: Thread-local debug state
thread_local std::unique_ptr<Debugger> current_debugger;

class DebugScope {
    Debugger* previous_debugger_;
public:
    DebugScope(Debugger* debugger) {
        previous_debugger_ = current_debugger.get();
        current_debugger.reset(debugger);
    }
    
    ~DebugScope() {
        current_debugger.reset(previous_debugger_);
    }
};
#endif
```

### Complex Debug Configuration

#### DON'T: Over-Engineered Configuration
```cpp
// BAD: Complex configuration objects
class DebugConfiguration {
    std::map<std::string, DebugOption> options;
    std::vector<DebugProfile> profiles;
    DebugFilter filter_chain;
    // 200+ lines of configuration complexity
};
```

#### DO: Simple Debug Options
```cpp
// GOOD: Simple, direct configuration
class Debugger {
public:
    void enable_execution_trace(bool enabled);
    void enable_performance_profiling(bool enabled);
    void set_log_level(LogLevel level);
    void set_slow_operation_threshold(std::chrono::milliseconds threshold);
};
```

### Debugging Performance Impact

#### DON'T: Always-On Debug Overhead
```cpp
// BAD: Debug overhead even when disabled
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    DebugTimer timer;  // NO - overhead even when debug disabled
    auto result = do_evaluation(expr, ctx);
    timer.record("evaluation");  // NO - always recording
    return result;
}
```

#### DO: Zero-Overhead When Disabled
```cpp
// GOOD: Zero debug overhead when not debugging
nlohmann::json evaluate(nlohmann::json expr, ExecutionContext ctx) {
    // Direct evaluation with no debug overhead
    return do_evaluation(expr, ctx);
}

// Debug timing handled in CLI wrapper only when requested
```

## Success Criteria

### Build System Requirements

#### Production Builds
-   Zero debug overhead: No debug code compiled into binary
-   Minimal dependencies: No readline or debug library requirements
-   Fast compilation: Only core library and production main compiled
-   Small binary size: Minimal footprint for distribution

#### REPL Builds
-   Full debug infrastructure: All debugging features available
-   Conditional compilation: `#ifdef REPL` blocks work correctly
-   Readline integration: Command history and editing when available
-   Clear build distinction: Different CMake targets for different purposes

### Functional Requirements

#### Production Binary
-   Direct script execution: `computo script.json input.json`
-   Zero debug overhead: Fastest possible execution
-   Minimal CLI: Only essential arguments supported
-   Error handling: Clean error messages without debug context

#### REPL Binary Features
-   Interactive REPL: Command prompt with expression evaluation
-   Debug features: Tracing, profiling, breakpoints, variable watching
-   stdin/stdout protocol: Batch processing and pipeline integration
-   Context loading: Preload input files for REPL sessions

### Quality Requirements

#### Code Organization
-   Clean feature separation: `#ifdef REPL` blocks clearly marked
-   Single source base: No code duplication between builds
-   Small, focused functions: < 25 lines per function
-   Clear interfaces: Obvious APIs with minimal dependencies

#### Performance Design
-   Production builds: Literally zero debug overhead
-   REPL builds: Debug features only active when requested
-   Thread safety: Both builds support concurrent execution
-   Memory efficiency: Minimal footprint in production builds

#### Style Guidelines
-   NDEBUG separation: Assert statements controlled by NDEBUG only
-   REPL feature flags: User features controlled by REPL flag only
-   NO EMOJIS: Professional, clean code and output
-   Consistent error handling: Clear, actionable error messages

### Integration Requirements

#### CMake Build System
-   Simple build commands: Clear cmake options for each build type
-   Dependency management: REPL builds add readline, production builds don't
-   Source organization: Conditional compilation of debug source files
-   Target naming: Clear distinction between production and REPL binaries

#### Cross-Platform Support
-   Readline optional: Graceful fallback when readline unavailable
-   Consistent behavior: Same functionality across different platforms
-   Resource management: Proper cleanup on all systems
-   Compiler compatibility: Works with different C++ compilers

### User Experience

#### Production Use Cases
-   CI/CD integration: Fast, reliable script execution
-   Automation scripts: Simple, predictable command-line interface
-   Performance testing: Zero overhead for accurate measurements
-   Distribution: Small binaries suitable for embedded environments

#### Development Use Cases
-   Interactive exploration: REPL for testing expressions
-   Script debugging: Breakpoints, tracing, variable inspection
-   Performance analysis: Profiling and slow operation detection
-   Batch testing: stdin/stdout protocol for test automation

This conditional compilation architecture provides a clean solution that delivers zero-overhead production builds while maintaining full debugging capabilities for development, using industry-standard `#ifdef` patterns for feature separation.
