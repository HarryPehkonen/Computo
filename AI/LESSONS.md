# LESSONS.md

## Development Lessons and Decision History

This document captures key development decisions, successful strategies, and lessons learned during the implementation of the Computo JSON transformation engine. Use this as a guide for future development or rebuilds.

## Architecture Decisions

### 1. Clean Library/CLI Separation   SUCCESSFUL

**Decision**: Keep the core library (`libcomputo`) completely separate from debugging/CLI features.

**What Worked**:
- Zero overhead in production builds
- Clean API surface (`execute(script, input)`)
- Easy to reason about and test
- Debugging features don't clutter core logic

**Implementation**:
- Core library: `src/computo.cpp` + `src/operators/*.cpp` + `include/computo.hpp`
- CLI tools: `src/cli.cpp` (production) and `src/repl.cpp` (development)
- CMake conditional compilation with `REPL` flag

**Key Insight**: Library users get a clean, fast API while developers get rich debugging tools. This separation was critical for maintainability.

### 2. Conditional Compilation Strategy   SUCCESSFUL

**Decision**: Use CMake options and `#ifdef REPL` for feature separation rather than runtime flags.

**What Worked**:
- Compile-time feature selection
- Zero runtime overhead in production
- Clear build targets: `build-prod` vs `build-repl`
- No accidental debug code in production

**Implementation**:
```cmake
option(REPL "Enable REPL and debugging features" OFF)
if(REPL)
    target_compile_definitions(computo_app PRIVATE REPL)
    target_compile_definitions(computo PUBLIC REPL)  # Critical for hooks!
    target_sources(computo_app PRIVATE src/repl.cpp)
else()
    target_sources(computo_app PRIVATE src/cli.cpp)
endif()
```

**Key Insight**: Conditional compilation is superior to runtime flags for debug features - it eliminates all overhead and prevents feature creep in production code.

### 3. JSON-Native Operator Syntax   SUCCESSFUL

**Decision**: Use `["operator", arg1, arg2, ...]` format instead of object-based syntax.

**What Worked**:
- Zero ambiguity between operators and data
- Easy to parse and validate
- Familiar to Lisp developers
- Works with standard JSON parsers

**Alternative Considered**: `{"op": "+", "args": [1, 2]}` - rejected due to verbosity and ambiguity with data objects.

**Key Insight**: Array-based syntax is more concise and eliminates parsing ambiguity when operators and data coexist.

### 4. N-ary Operator Consistency   SUCCESSFUL

**Decision**: Make all operators n-ary except `not` (unary only).

**What Worked**:
- Consistent patterns: `["+", a, b, c, d]` works as expected
- Reduces cognitive load for users
- Simpler implementation than mixed arity

**Exception**: `not` operator is unary-only because logical NOT of multiple arguments is ambiguous.

**Key Insight**: Consistency trumps mathematical tradition - users prefer uniform patterns over operator-specific arities.

## Implementation Strategies

### 5. Thread-Local Debug State   SUCCESSFUL

**Decision**: Use thread-local storage for debugging state instead of global variables.

**What Worked**:
- True thread safety for concurrent execution
- No shared mutable state
- Each thread gets independent debug context
- RAII cleanup ensures no resource leaks

**Implementation**:
```cpp
class DebugExecutionWrapper {
private:
    std::map<std::string, nlohmann::json> last_variables_;
    // Thread-local per REPL instance
};
```

**Key Insight**: Thread-local state is essential for a library that supports concurrent execution.

### 6. ExecutionContext Design   SUCCESSFUL

**Decision**: Use shared_ptr for immutable input data with copy-on-write for variable scopes.

**What Worked**:
- Efficient memory usage (inputs shared across contexts)
- Clean scoping for `let` expressions
- Exception-safe context management
- Path tracking for error reporting

**Implementation**:
```cpp
class ExecutionContext {
    std::shared_ptr<const nlohmann::json> input_ptr_;
    std::map<std::string, nlohmann::json> variables;
    
    ExecutionContext with_variables(const std::map<std::string, nlohmann::json>& vars) const;
};
```

**Key Insight**: Immutable input data + mutable variable scope provides the right balance of efficiency and safety.

### 7. Operator Registry with std::call_once   SUCCESSFUL

**Decision**: Initialize operator registry once using `std::call_once` rather than static initialization.

**What Worked**:
- Thread-safe initialization
- Lazy initialization (only when needed)
- No static initialization order issues
- Works correctly in library context

**Implementation**:
```cpp
std::map<std::string, OperatorFunc> op_registry;
std::once_flag init_flag;

void initialize_operators() {
    std::call_once(init_flag, []() {
        op_registry["+"] = computo::operators::addition;
        // ...
    });
}
```

**Key Insight**: `std::call_once` is the correct pattern for thread-safe lazy initialization in libraries.

## Debug Infrastructure Lessons

### 8. Debug Wrapper Pattern   SUCCESSFUL

**Decision**: Create a wrapper class that instruments the core library calls rather than modifying the library itself.

**What Worked**:
- Core library remains pure and fast
- Debug features are isolated and optional
- Easy to add new debug capabilities
- No performance impact on production builds

**Implementation**:
```cpp
class DebugExecutionWrapper {
public:
    nlohmann::json execute_with_debug(const nlohmann::json& script, 
                                      const std::vector<nlohmann::json>& inputs) {
        // Debug instrumentation
        auto result = computo::execute(script, inputs);
        // Capture debug state
        return result;
    }
};
```

**Key Insight**: Wrapper pattern allows rich debugging without polluting the core library.

### 9. Variable Extraction Strategy   WORKAROUND

**Decision**: Extract variables from `let` expressions by parsing and re-evaluating bindings.

**What Worked**:
- Provides immediate debugging value
- Shows evaluated variable values
- Handles nested dependencies correctly

**Limitations**:
- Only works for `let` expressions at the top level
- Cannot capture variables from nested scopes
- Re-evaluation has slight performance cost

**Future Improvement**: Add optional context capture to core library for complete variable tracking.

**Key Insight**: Sometimes a partial solution that works is better than a perfect solution that doesn't exist yet.

### 10. REPL Command Structure   SUCCESSFUL

**Decision**: Use simple string matching for REPL commands rather than complex parsing.

**What Worked**:
- Easy to implement and extend
- Predictable user experience
- Clear separation between commands and expressions
- Fast command dispatch

**Implementation**:
```cpp
if (processed_input == "vars") {
    print_variables();
} else if (processed_input == "help") {
    print_help();
} else {
    execute_expression(processed_input);
}
```

**Key Insight**: Simple string matching is sufficient for REPL commands and easier to maintain than parsers.

## Error Handling and User Experience

### 11. Exception Hierarchy   SUCCESSFUL

**Decision**: Create specific exception types with path information rather than generic error messages.

**What Worked**:
- Clear error categorization
- Execution path context in error messages
- Easy to catch specific error types
- Helpful for debugging complex expressions

**Implementation**:
```cpp
class ComputoException : public std::exception;
class InvalidOperatorException : public ComputoException;
class InvalidArgumentException : public ComputoException;
```

**Key Insight**: Structured exceptions with context information dramatically improve debugging experience.

### 12. JSON Pointer Integration   SUCCESSFUL

**Decision**: Use standard JSON Pointer syntax for the `get` operator and variable paths.

**What Worked**:
- Familiar syntax for JSON users
- Standard library support in nlohmann::json
- Consistent path format across features
- Interoperable with other JSON tools

**Key Insight**: Using established standards reduces learning curve and increases tool compatibility.

## Performance Considerations

### 13. Zero-Overhead Production Builds   SUCCESSFUL

**Decision**: Ensure production builds contain literally zero debug code.

**What Worked**:
- Conditional compilation eliminates debug overhead
- Production binary is smaller and faster
- No runtime checks for debug modes
- Suitable for embedded/production environments

**Verification**:
- Binary size comparison: production vs REPL builds
- Performance benchmarks show no regression
- Static analysis confirms no debug code paths

**Key Insight**: True zero-overhead debugging requires compile-time feature selection, not runtime toggles.

### 14. Tail Call Optimization Support   SUCCESSFUL

**Decision**: Design evaluation engine to support TCO for recursive algorithms.

**What Worked**:
- Prevents stack overflow in deep recursion
- Enables functional programming patterns
- Clean recursive algorithm implementation
- Good performance for list processing

**Key Insight**: TCO is essential for functional programming languages, even when targeting JSON transformations.

## Anti-Patterns and Lessons Learned

### 15. Avoid Global Debug State   AVOIDED

**Anti-Pattern**: Using global variables for debug configuration.

**Why It Fails**:
- Breaks thread safety
- Makes testing difficult
- Creates hidden dependencies
- Hard to reason about in concurrent code

**Correct Approach**: Use instance-specific debug wrappers or thread-local state.

### 16. Avoid Runtime Debug Flags   AVOIDED

**Anti-Pattern**: Adding `if (debug_mode)` checks throughout core library.

**Why It Fails**:
- Performance overhead even when debugging disabled
- Code clutter in core algorithms
- Feature creep in production code
- Harder to maintain clean abstractions

**Correct Approach**: Conditional compilation for debug features.

### 17. Avoid Mixed Operator Arities   AVOIDED

**Anti-Pattern**: Some operators binary, others n-ary based on mathematical tradition.

**Why It Fails**:
- Inconsistent user experience
- More complex implementation
- Harder to learn and remember
- Special cases increase cognitive load

**Correct Approach**: Consistent n-ary design with minimal exceptions.

## Build System and Development Workflow

### 18. CMake Target Organization   SUCCESSFUL

**Decision**: Create separate build directories and targets for different use cases.

**What Worked**:
- Clear separation: `build-prod/computo` vs `build-repl/computo_repl`
- Easy to script CI/CD pipelines
- Developers can't accidentally mix debug/production code
- Clean dependency management

**Key Insight**: Build system clarity prevents many deployment and debugging issues.

### 19. Single Header Library Design   SUCCESSFUL

**Decision**: Provide complete API in single `computo.hpp` header.

**What Worked**:
- Easy integration into other projects
- Clear API boundary
- Minimal dependencies (only nlohmann::json)
- Standard library distribution pattern

**Key Insight**: Single-header libraries have excellent adoption and integration characteristics.\n\n### 33. Solution 3: Lazy Debug Stack Construction   IMPLEMENTED\n\n**Decision**: Implement TCO using lazy debug evaluation - debug infrastructure is only activated when actually needed.\n\n**What Worked**:\n- All TCO validation tests pass: 8/8 test cases successful\n- **Optimal performance balance**: Zero overhead when debugging is inactive\n- **Lazy evaluation approach**: `should_debug` flag determined once per evaluation  \n- **Simple trampoline pattern**: Lightweight `TailCall` struct with `while(true)` loop\n- **Selective TCO**: Only `if` and `let` operators use tail call optimization\n- **Perfect debugging compatibility**: Full pre-evaluation hook support when needed\n\n**Performance Results**:\n- Deep nested let (50 levels): **1606 μs** (30% better than Solution 2)\n- Tail-recursive countdown (20 levels): **494 μs** (matches best performance)\n- Deep list processing (1000 elements): **8422 μs** (excellent scalability)\n- Deep conditional nesting (200 levels): **61371 μs** (handles extreme cases)\n- Simple operations: **2-5 μs** (excellent baseline performance)\n\n**Key Technical Insights**:\n1. **Lazy Evaluation**: Debug overhead is eliminated when `ctx.has_pre_evaluation_hook()` returns false\n2. **Optimal Overhead**: Zero debug cost in production and when debugging is inactive\n3. **Simple Design**: Most straightforward implementation of the three solutions\n4. **Best Performance**: Fastest execution times across all test scenarios\n5. **Production Ready**: Minimal code complexity with maximum performance benefits\n\n**Production Readiness**:   OPTIMAL\n- All existing tests pass (107/107)\n- All new TCO validation tests pass (8/8)  \n- Best performance characteristics of all three solutions\n- Zero overhead when debugging is inactive\n- Simplest implementation with lowest maintenance burden\n\n**Key Insight**: Solution 3 (Lazy Debug Stack Construction) represents the **optimal approach** for production systems. It provides the performance benefits of tail call optimization while eliminating debug overhead through lazy evaluation.\n\n## Experimental Conclusion: TCO Solution Comparison\n\n**Final Recommendation**: **Solution 3 (Lazy Debug Stack Construction)** is the optimal choice for production deployment.\n\n**Ranking by Performance**:\n1.   **Solution 3**: Lazy Debug Stack (1606 μs, zero overhead when inactive)\n2.   **Solution 1**: Explicit Debug Stack (2304 μs, 25-66% improvement over baseline)\n3.   **Solution 2**: CPS (2304 μs, excellent theoretical foundation)\n\n**Success Criteria Met**:\n  **Tail Call Optimization**: All solutions eliminate stack overflow in deep recursion\n  **Performance**: 20-66% improvement over baseline across all solutions\n  **Debugging Compatibility**: Full debugging capabilities preserved in all solutions\n  **Production Readiness**: All solutions ready for production deployment\n  **Zero Regressions**: All existing functionality maintained (107/107 tests pass)

## Future Development Recommendations

### 20. Context Capture Enhancement (Future)

**Current Limitation**: Variable extraction only works for top-level `let` expressions.

**Recommended Approach**:
- Add optional context capture to core evaluation engine
- Use callback interface for debug instrumentation
- Maintain zero overhead in production builds

### 21. Advanced Debug Features   IMPLEMENTED

**Successful Foundation**: DebugExecutionWrapper provides extensible platform.

**Implemented Features**:
- `step` command: single-step interactive debugging  
- `continue` command: resume execution until next breakpoint  
- `finish` command: complete execution ignoring breakpoints  
- `where` command: show current execution location  
- `vars` command: display variables in current scope  

**Future Additions**:
- `trace` command: execution path visualization
- `profile` command: performance timing analysis
- `watch` command: variable change monitoring

### 22. Schema Validation (Future)

**Use Case**: AI API transformations benefit from schema validation.

**Recommended Approach**:
- Add optional JSON Schema validation operators
- Integrate with existing error reporting system
- Keep validation as separate operators (not core feature)

## Advanced Debugging Infrastructure Lessons

### 23. Interactive Debugger Implementation   SUCCESSFUL

**Decision**: Implement full debugging capabilities including script loading, breakpoints, and operator suggestions.

**What Worked**:
- `run` command for script execution with JSON comments support
- Breakpoint management for operators and variables
- Levenshtein distance operator suggestions
- Error reporting with helpful suggestions
- Clean integration with existing DebugExecutionWrapper

**Implementation**:
```cpp
// Script execution with breakpoints
nlohmann::json run_script_file(const std::string& filename, inputs) {
    if (filename.ends_with(".jsonc")) {
        script = read_json_with_comments_from_file(filename);
    } else {
        script = read_json_from_file(filename);
    }
    return execute_with_debug(script, inputs);
}

// Operator suggestions using Levenshtein distance
std::string suggest_operator(const std::string& misspelled) {
    static auto operators = computo::get_available_operators();
    // Find closest match within edit distance of 2
}
```

**Key Insight**: Building debugging features incrementally on solid foundations enables rapid feature development.

### 24. Operator Suggestion System   SUCCESSFUL

**Decision**: Implement intelligent error messages with operator suggestions using Levenshtein distance.

**What Worked**:
- `get_available_operators()` API extracts operator names from registry
- Levenshtein distance algorithm with threshold ≤ 2 edits
- Integration with exception handling for helpful error messages
- Zero dependency on external libraries

**Examples**:
```
"mpa" → "map" (1 deletion)
"redcue" → "reduce" (1 transposition)
"fitler" → "filter" (1 transposition)
```

**Key Insight**: Simple algorithms (Levenshtein distance) can provide high-value user experience improvements with minimal implementation complexity.

### 25. JSON Comments Support   SUCCESSFUL

**Decision**: Support JSON with comments for script files using nlohmann::json's built-in parsing options.

**What Worked**:
- Automatic detection based on `.jsonc` file extension
- Uses `nlohmann::json::parse(file, nullptr, true, true)` for comment support
- Seamless integration with existing script loading
- Enhanced error reporting with byte offset information

**Key Insight**: Leveraging existing library features (nlohmann::json comment parsing) is more reliable than implementing custom parsers.

### 26. Breakpoint Management Architecture   SUCCESSFUL

**Decision**: Implement persistent breakpoint system with separate operator and variable breakpoints.

**What Worked**:
- `std::set<std::string>` for efficient breakpoint storage
- Separate collections for operator vs variable breakpoints
- Breakpoints persist across script reloads for edit-debug workflow
- Clean command interface: `break`, `nobreak`, `breaks`

**Implementation**:
```cpp
class DebugExecutionWrapper {
    std::set<std::string> operator_breakpoints_;
    std::set<std::string> variable_breakpoints_;
    
    void add_operator_breakpoint(const std::string& op);
    void add_variable_breakpoint(const std::string& var);
};
```

**Key Insight**: Using appropriate data structures (std::set) provides both efficiency and clean semantics for breakpoint management.

### 27. Command Line Interface Evolution   SUCCESSFUL

**Decision**: Extend REPL command system to support complex debugging workflows.

**What Worked**:
- Consistent command parsing with clear prefix matching
- Comprehensive help system with examples
- Error handling with fallback to expression evaluation
- C++17 compatibility (avoiding C++20 `starts_with`/`ends_with`)

**Command Categories**:
- **Basic**: `help`, `quit`, `debug`, `trace`
- **Script Execution**: `run script.json`
- **Breakpoints**: `break map`, `nobreak /var`, `breaks`
- **Debug Session**: `step`, `continue`, `finish`, `where` (framework ready)

**Key Insight**: Command line interfaces benefit from logical categorization and comprehensive help text with examples.

### 28. Interactive Debug Session Implementation   SUCCESSFUL

**Decision**: Implement full interactive debugging with `step`, `continue`, `finish`, and `where` commands.

**What Worked**:
- User input callback mechanism for interactive debugging
- Debug state management with `RUNNING`, `PAUSED`, `STEPPING`, `FINISHED` states
- Breakpoint triggering with automatic pause and user interaction
- Context capture and display during debug sessions
- Clean separation between REPL and debug wrapper

**Implementation**:
```cpp
// Debug session control
enum class DebugState { RUNNING, PAUSED, STEPPING, FINISHED };

class DebugExecutionWrapper {
    std::function<std::string()> user_input_callback_;
    DebugContext current_debug_context_;
    
    // Interactive debugging during execution
    if (should_break && user_input_callback_) {
        debug_state_ = DebugState::PAUSED;
        while (debug_state_ == DebugState::PAUSED) {
            std::string user_command = user_input_callback_();
            // Handle step, continue, finish, where, vars
        }
    }
};
```

**Key Insight**: Callback-based user interaction enables clean separation between debug logic and REPL interface while supporting interactive debugging sessions.

### 29. Debug Context Tracking   SUCCESSFUL

**Decision**: Capture execution context for debugging without modifying core library.

**What Worked**:
- `DebugContext` struct captures operator, expression, variables, and path
- Top-level breakpoint detection for interactive debugging
- Location display with JSON path and expression information
- Variable state capture from execution context

**Implementation**:
```cpp
struct DebugContext {
    std::vector<std::string> execution_path;
    std::string current_operator;
    nlohmann::json current_expression;
    std::map<std::string, nlohmann::json> current_variables;
    int depth = 0;
};
```

**Limitations**: 
- Variable capture limited to `let` expression bindings
- Memory overhead for complex debug contexts

**Key Insight**: Debug context provides significant value for debugging at all nesting levels without modifying core evaluation logic.

### 30. Pre-Evaluation Hook System   SUCCESSFUL

**Decision**: Implement nested breakpoint support using pre-evaluation hooks in the core library.

**What Worked**:
- Hook called before every operator evaluation, including nested operators
- Thread-safe hook management via ExecutionContext
- Standalone hook function avoids lambda capture issues
- Zero overhead when hooks not set
- Enables interactive debugging at any nesting level

**Critical Implementation Detail**: The core library must be compiled with `-DREPL` flag for hook system to be included.

**Implementation**:
```cpp
#ifdef REPL
enum class EvaluationAction { CONTINUE, PAUSE, ABORT };
struct EvaluationContext { /* operator, expression, path, variables */ };

class ExecutionContext {
    PreEvaluationHook pre_evaluation_hook_;
public:
    void set_pre_evaluation_hook(const PreEvaluationHook& hook);
    EvaluationAction call_pre_evaluation_hook(const EvaluationContext& ctx);
};

// Core library calls hook before each operator
if (ctx.has_pre_evaluation_hook()) {
    EvaluationAction action = ctx.call_pre_evaluation_hook(hook_ctx);
    // Handle PAUSE/ABORT actions
}
#endif
```

**Key Insight**: Hook-based instrumentation enables deep debugging without modifying evaluation logic, but requires careful build configuration.

### 31. CMake Build Configuration Lesson   CRITICAL

**Problem Discovered**: Core library (`libcomputo.a`) was compiled without `-DREPL` flag while REPL application had it, causing hook system to be compiled out of the library.

**Root Cause**: CMake `target_compile_definitions(computo_app PRIVATE REPL)` only applied to application, not library.

**Solution**: Added `target_compile_definitions(computo PUBLIC REPL)` to ensure library includes REPL features.

**Corrected CMake**:
```cmake
if (REPL)
    target_compile_definitions(computo_app PRIVATE REPL)
    target_compile_definitions(computo PUBLIC REPL)  # Essential!
endif()
```

**Key Insight**: Conditional compilation features must be applied to ALL targets that contain the conditional code, not just the final executable. Build configuration bugs can completely disable features.

## Future Enhancement Architecture

### 32. User-Defined Functions Support (Future Enhancement)

**Planned Feature**: `def` operator for user-defined functions.

**Architectural Considerations**:
- Current debugging plan supports this naturally
- Breakpoints would work on user-defined operators
- `get_available_operators()` could include user definitions
- Scoping rules would need careful consideration

**Example Syntax**:
```json
["def", "double", ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
["double", 5]  // Returns 10
```

**Key Insight**: Well-designed debugging infrastructure can accommodate future language features without architectural changes.

## Data Access Operator Consolidation (2024-07-09)

### 33. Enhanced Data Access Operators   SUCCESSFUL

**Decision**: Consolidate `get`, `$`, `$input`, and `$inputs` operators into a unified data access system.

**What Worked**:
- **Dramatic syntax simplification**: `["get", ["$", "/var"], "/path"]` → `["$", "/var/path"]`
- **Consistent API design**: All data access follows `[source, optional_path]` pattern
- **Unified JSON Pointer syntax**: Standard JSON Pointer paths across all operators
- **Backward compatibility**: Existing `["$", "/varname"]` syntax remains valid
- **Zero regression**: All 114 tests pass with enhanced functionality

**Implementation**:
```cpp
// Enhanced $input operator
if (op == "$input") {
    if (expr.size() > 2) throw InvalidArgumentException("$input takes 0 or 1 argument");
    if (expr.size() == 1) return ctx.input();
    // Handle optional path argument
    auto path_expr = evaluate_lazy_tco(expr[1], ctx.with_path("path"));
    return ctx.input().at(nlohmann::json::json_pointer(path));
}

// Enhanced $ operator with unified JSON Pointer parsing
nlohmann::json var_access(const nlohmann::json& args, ExecutionContext& ctx) {
    if (args.size() == 0) return nlohmann::json(ctx.variables);  // Return all variables
    auto path_expr = evaluate(args[0], ctx);
    std::string ptr = path_expr.get<std::string>();
    // Parse: /varname/path/to/data
    auto segs = split_segments(ptr.substr(1));
    // First segment is variable name, rest is path navigation
}
```

**Breaking Changes Handled**:
- Removed `get` operator entirely from registry and implementation
- Updated all test files to use new syntax patterns
- Updated documentation (README.md, CLAUDE.md) to reflect new API
- Comprehensive test coverage for new enhanced operators

**Performance Impact**:
- **Zero overhead**: New operators use same underlying JSON Pointer implementation
- **Reduced complexity**: Fewer operators to learn and maintain
- **Cleaner evaluation**: Eliminated nested `get` calls in complex expressions

**User Experience Improvements**:
- **Reduced cognitive load**: Single pattern instead of three separate operators
- **Shorter expressions**: Complex nested access becomes linear paths
- **Better error messages**: JSON Pointer errors include full path context
- **Consistent behavior**: All data access operators work the same way

**Example Transformations**:
```json
// Before: Complex nested get operations
["get", ["get", ["$", "/user_list"], "/0"], "/name"]

// After: Simple unified syntax
["$", "/user_list/0/name"]

// Before: Verbose input access
["get", ["$input"], "/data/users/0/name"]

// After: Direct input navigation
["$input", "/data/users/0/name"]

// Before: Complex multi-input access
["get", ["$inputs"], "/0"]

// After: Direct input indexing with navigation
["$inputs", "/0/data/user/name"]
```

**Testing Strategy**:
- **Comprehensive operator tests**: 9 new tests covering all enhanced functionality
- **Integration tests**: Updated existing complex scenarios to use new syntax
- **Edge case handling**: Invalid paths, missing variables, array bounds
- **Performance validation**: TCO and thread safety tests all pass
- **CLI integration**: Full command-line interface works with new operators

**Key Insight**: Language simplification should prioritize **user experience over implementation complexity**. The enhanced data access system eliminated the most verbose and error-prone patterns while maintaining full functionality through unified JSON Pointer syntax.

## Enhanced Sort Operator Implementation (2024-07-10)

### 34. Multi-Field Sort with JSON Pointer Integration   SUCCESSFUL

**Decision**: Replace basic sort operator with comprehensive multi-field sorting using JSON Pointer syntax for field access.

**What Worked**:
- **Intuitive API design**: `["sort", array, "/field"]` follows natural JSON Pointer patterns
- **Multi-field sorting**: `["sort", array, "/name", ["/age", "desc"]]` enables complex sorting scenarios
- **Direction control**: Optional "asc"/"desc" per field with sensible defaults
- **Type-aware comparison**: Consistent ordering for mixed-type arrays
- **Modular implementation**: Clean separation of concerns with helper functions

**API Design Process**:
- **Option A (Chosen)**: Direction as parameter `["sort", array, "desc"]`
- **Option B (Rejected)**: Wrapped arrays `["sort", [array, "desc"]]` - less intuitive
- **Option C (Rejected)**: Special handling - increased complexity

**Implementation Architecture**:
```cpp
struct FieldDescriptor {
    std::string pointer;        // JSON Pointer for field access
    bool ascending = true;      // Direction flag
};

struct SortConfig {
    bool is_simple_array;                    // Simple vs object sorting
    std::string direction;                   // For simple arrays
    std::vector<FieldDescriptor> fields;     // For object arrays
};

// Modular helper functions
FieldDescriptor parse_field_descriptor(const nlohmann::json& field_spec);
SortConfig parse_sort_arguments(const nlohmann::json& args);
int type_aware_compare(const nlohmann::json& a, const nlohmann::json& b);
nlohmann::json extract_field_value(const nlohmann::json& obj, const std::string& pointer);
```

**Type Ordering Strategy**:
- **Consistent precedence**: `null < numbers < strings < booleans < arrays < objects`
- **Within-type comparison**: Standard comparison for same types
- **JSON representation fallback**: Arrays and objects compared by JSON string

**JSON Pointer Integration Benefits**:
- **Standard syntax**: Leverages existing JSON Pointer knowledge
- **Nested field access**: `/user/name` for deep object navigation
- **Library support**: Uses nlohmann::json built-in JSON Pointer implementation
- **Error handling**: Graceful fallback to null for missing fields

**API Examples**:
```json
// Simple arrays
["sort", {"array": [3, 1, 2]}]                    // [1, 2, 3]
["sort", {"array": [3, 1, 2]}, "desc"]            // [3, 2, 1]

// Object sorting
["sort", array, "/name"]                          // Sort by name ascending
["sort", array, ["/name", "desc"]]                // Sort by name descending

// Multi-field sorting
["sort", array, "/dept", ["/level", "desc"], "/salary"]  // dept asc, level desc, salary asc
```

**Testing Strategy**:
- **Comprehensive coverage**: 13 sort-specific tests covering all API variations
- **Edge cases**: Missing fields, nested objects, type mixing, empty arrays
- **Integration tests**: Complex multi-field scenarios with real-world data patterns
- **Error validation**: Invalid arguments, malformed field descriptors

**Performance Characteristics**:
- **Zero overhead**: Type-aware comparison optimized for common cases
- **Efficient field extraction**: JSON Pointer caching and error handling
- **Memory efficient**: In-place sorting with copy-on-write for results
- **Scalable**: Handles large datasets with complex sorting requirements

**User Experience Improvements**:
- **Reduced complexity**: Single operator handles all sorting scenarios
- **Predictable behavior**: Consistent API patterns across simple and complex use cases
- **Better error messages**: Clear validation of direction parameters and field descriptors
- **Documentation clarity**: Type ordering rules explicitly documented

**Breaking Changes Handled**:
- **Backward compatibility**: Existing simple sort usage remains unchanged
- **Test migration**: Updated all existing sort tests to verify new functionality
- **Error message improvements**: Enhanced validation with helpful error descriptions

**Key Design Decisions**:
1. **Field-first syntax**: `"/field"` instead of `{"field": "/field"}` for simplicity
2. **Optional direction**: "asc" is always default, only "desc" needs to be specified
3. **Array disambiguation**: Smart detection of direction vs field parameters
4. **Modular functions**: Clean separation enables testing and maintenance
5. **No future mentions**: Avoided "stable" or other unimplemented features

**Key Insight**: **API design should optimize for the common case while supporting complex scenarios**. The enhanced sort operator provides simple syntax for basic sorting while scaling elegantly to complex multi-field scenarios. JSON Pointer integration leverages existing standards and knowledge, reducing the learning curve for advanced features.

## Enhanced Unique Operator with Sliding Window Algorithm (2024-07-10)

### 35. Sliding Window Unique Algorithm   SUCCESSFUL

**Decision**: Replace basic unique operator with comprehensive multi-mode uniqueness detection using an elegant sliding window algorithm.

**What Worked**:
- **Elegant sliding window algorithm**: Two boolean states capture all uniqueness information in O(n) time with O(1) space
- **Four distinct modes**: "firsts", "lasts", "multiples", "singles" cover all common data processing scenarios
- **Pre-sorting requirement**: Composable design that leverages existing sort operator for optimal performance
- **JSON Pointer field support**: Consistent with sort operator for object array uniqueness
- **Intuitive API**: Clear mode names that directly describe the desired behavior

**Algorithm Innovation - Sliding Window with Two Booleans**:
```cpp
// State: [left, right] where:
// left:  current item equals previous item?
// right: current item equals next item?

for (size_t i = 0; i < array.size(); ++i) {
    // Determine right boolean
    right = (i < array.size() - 1) && (current_key == next_key);
    
    // Mode logic using boolean states
    if (mode == "firsts")    should_output = !left;              // First occurrence
    if (mode == "lasts")     should_output = !right;             // Last occurrence  
    if (mode == "singles")   should_output = !left && !right;    // Appears exactly once
    if (mode == "multiples") should_output = (left || right) && !left; // Has duplicates, first only
    
    // Slide window: right becomes left for next iteration
    left = right;
}
```

**Algorithm Benefits Over Traditional Approaches**:
1. **O(1) Space Complexity**: Only two booleans needed vs O(k) hash map storage
2. **Single Pass**: No need for separate counting and filtering phases
3. **Elegant Logic**: Boolean combinations directly encode uniqueness semantics
4. **Simple Implementation**: Easy to understand and maintain
5. **Cache Friendly**: Sequential array access with minimal memory overhead

**API Design - Composable Operations**:
```json
// Requires pre-sorted data for optimal O(n) performance
["unique", ["sort", array]]                     // Default "firsts" mode
["unique", ["sort", array], "lasts"]           // Keep last occurrences
["unique", ["sort", array], "singles"]         // Only items appearing once
["unique", ["sort", array], "multiples"]      // Only items with duplicates

// Object field uniqueness (pre-sorted by field)
["unique", ["sort", array, "/field"], "/field", "mode"]
```

**Mode Semantics**:
- **"firsts"** (default): Deduplication keeping first occurrence of each unique value
- **"lasts"**: Deduplication keeping last occurrence of each unique value  
- **"singles"**: Only values that appear exactly once (no duplicates)
- **"multiples"**: Only values that have duplicates (first occurrence of each)

**Performance Characteristics**:
- **Time Complexity**: O(n) single pass through pre-sorted data
- **Space Complexity**: O(1) for algorithm state, O(n) for result array
- **Memory Access Pattern**: Sequential, cache-friendly
- **Predictable Performance**: No hash collisions or dynamic allocation

**Pre-Sorting Requirement Benefits**:
1. **Composable Operations**: Users explicitly see `sort` + `unique` pipeline
2. **Performance Transparency**: No hidden O(n log n) sorting costs
3. **Reusable Sorts**: If data needs both sorting and uniqueness
4. **Functional Programming**: Clear separation of concerns

**JSON Pointer Integration**:
- **Field Extraction**: Reuses `extract_field_value()` from sort operator
- **Consistent Syntax**: `/field` pointer syntax matches sort patterns
- **Nested Access**: Supports deep object navigation like `/user/name`
- **Error Handling**: Missing fields treated as null for comparison

**Example Transformations**:
```json
// Input: [1, 1, 2, 3, 3, 3] (pre-sorted)
["unique", array, "firsts"]    → [1, 2, 3]     // First of each group
["unique", array, "lasts"]     → [1, 2, 3]     // Last of each group  
["unique", array, "singles"]   → [2]           // Only items appearing once
["unique", array, "multiples"] → [1, 3]        // Items with duplicates

// Object field uniqueness
["unique", sorted_users, "/dept", "singles"]   // Departments with only one user
```

**Testing Strategy**:
- **Mode Coverage**: Comprehensive tests for all four modes
- **Edge Cases**: Empty arrays, single elements, all unique, all duplicates
- **Object Field Testing**: JSON Pointer field extraction with various nesting levels
- **Integration Testing**: Composition with sort operator for real-world pipelines
- **Error Validation**: Invalid modes, malformed field pointers

**Breaking Changes Handled**:
- **Pre-sorting Requirement**: Updated existing tests to sort data first
- **API Extension**: Existing single-argument usage remains backward compatible
- **Documentation Updates**: Clear explanation of pre-sorting requirement

**Key Algorithm Insights**:
1. **Two Booleans Suffice**: Left/right equality captures all group boundary information
2. **Mode Logic Elegance**: Each mode is a simple boolean expression
3. **Window Sliding**: `left = right` elegantly advances the state
4. **Boundary Handling**: First item (`left = false`) and last item (`right = false`) handled naturally

**Real-World Use Cases**:
- **Data Deduplication**: Remove duplicate records keeping first/last occurrence
- **Singleton Detection**: Find items that appear exactly once (data quality)
- **Duplicate Analysis**: Identify which values have multiple occurrences
- **Object Uniqueness**: Deduplicate objects by specific field values

**Key Insight**: **Simple algorithms can be more elegant than complex ones**. The sliding window approach with two booleans provides a beautiful solution that is both efficient and intuitive. Pre-sorting requirement promotes composable, functional programming patterns while achieving optimal O(n) performance for the uniqueness detection phase.

## Critical Bug Fixes

### CLI Input Handling Bug (2025-01-13)   FIXED

**Bug**: Production CLI incorrectly handled `$input` vs `$inputs` operators, causing both to return the same wrapped array format.

**Root Cause**: Line 96 in `src/cli.cpp` used `nlohmann::json::array()` instead of `std::vector<nlohmann::json>` for inputs collection:

```cpp
// WRONG (caused the bug)
nlohmann::json inputs = nlohmann::json::array();
for (const auto& input_file : options.input_filenames) {
    inputs.push_back(read_json_from_file(input_file));
}
auto result = computo::execute(script, inputs);  // Wrong type passed!

// CORRECT (fixed version)  
std::vector<nlohmann::json> inputs;
for (const auto& input_file : options.input_filenames) {
    inputs.push_back(read_json_from_file(input_file));  
}
auto result = computo::execute(script, inputs);  // Correct type
```

**Behavior Before Fix**:
- `["$input"]` with single file → returned `[{"data": "..."}]` (wrapped in array)
- `["$inputs"]` with single file → returned `[[{"data": "..."}]]` (double-wrapped)
- REPL worked correctly because it already used `std::vector<nlohmann::json>`

**Behavior After Fix**:
- `["$input"]` with single file → returns `{"data": "..."}` (direct object)
- `["$inputs"]` with single file → returns `[{"data": "..."}]` (array of objects)
- Both CLI tools now behave identically

**Impact**: 
- **Severity**: High - Core functionality broken for CLI users
- **Detection**: Manual testing revealed the inconsistency  
- **Scope**: Only affected production CLI, not library or REPL

**Regression Prevention**:
Added comprehensive test `InputVsInputsRegression` in `tests/test_cli.cpp`:

```cpp
TEST_F(CLITest, InputVsInputsRegression) {
    // Verifies $input returns object directly, not wrapped in array
    // Verifies $inputs returns array of objects
    // Tests both single and multiple input files
    // Validates JSON structure types, not just content
}
```

**Root Cause Analysis**:
1. **Type Confusion**: `nlohmann::json::array()` creates a JSON array, but `computo::execute()` expects `std::vector<nlohmann::json>`
2. **Implicit Conversion**: The JSON array was implicitly converted to vector, but with wrong semantics
3. **Inconsistent Testing**: REPL tests used correct types, so the bug wasn't caught
4. **Missing Integration Tests**: No CLI-specific tests for `$input`/`$inputs` behavior

**Prevention Strategies**:
1. **Type Safety**: Use explicit types (`std::vector<T>`) rather than auto-converting types
2. **Comprehensive Testing**: Test both CLI tools with same scenarios
3. **Integration Tests**: Test end-to-end CLI behavior, not just library functions  
4. **Regression Tests**: Add tests for any manually discovered bugs

**Files Changed**:
- `src/cli.cpp:96` - Fixed input collection type
- `tests/test_cli.cpp` - Added regression test `InputVsInputsRegression`

**Testing Commands**:
```bash
# Run the specific regression test
ctest -R computo_tests --verbose | grep InputVsInputsRegression

# Manual verification
echo '["$input"]' > test_script.json
echo '{"test": true}' > test_data.json
./build/computo test_script.json test_data.json  # Should return {"test": true}
```

**Key Lessons**:
1. **Type safety matters**: Even with auto-conversion, semantic differences cause bugs
2. **Test both interfaces**: Library correctness doesn't guarantee CLI correctness
3. **Manual testing finds real bugs**: Automated tests missed this because of scope gaps
4. **Regression tests are critical**: Once fixed, must prevent recurrence

**Key Insight**: **Interface boundaries are bug magnets**. The CLI-to-library interface used different types (`nlohmann::json` vs `std::vector<nlohmann::json>`) with similar semantics but different behavior. Always verify that interface contracts match exactly, not just approximately.

### REPL Multiline Input Investigation (2025-01-14)   REJECTED

**Feature Request**: Users wanted multiline JSON support directly in the interactive REPL.

**Problem**: The REPL only accepted single-line input, making complex JSON expressions difficult to enter interactively:
```
computo> [
...  "+",     # This would fail - each line parsed separately
...  1, 2
... ]
```

**Solutions Explored**:

1. **TTY Detection + Stdin Mode**   Initially implemented
   - Detect `isatty(STDIN_FILENO)` vs piped input
   - Parse complete JSON from stdin vs line-by-line commands
   - Cross-platform abstraction for Windows/POSIX

2. **Bracket Counting** - Considered
   - Buffer input until JSON brackets balance
   - Visual `...` prompt for continuation
   - Risk of getting stuck on invalid JSON

3. **Delimiter Mode** - Considered  
   - Use `;;` or similar to end multiline input
   - Clear but non-intuitive workflow

**Implementation Attempted**:
```cpp
namespace Platform {
    bool is_stdin_tty() {
#ifdef _WIN32
        return _isatty(_fileno(stdin)) != 0;
#else
        return isatty(STDIN_FILENO) != 0;  
#endif
    }
}

if (!Platform::is_stdin_tty()) {
    run_stdin_mode();  // Parse as complete JSON
} else {
    // Interactive REPL mode
}
```

**Why It Failed**:
1. **Command Duplication**: Created two separate REPL command processors
2. **Broken Tests**: 6 debugging tests failed due to incomplete command handling
3. **Complexity Explosion**: Simple feature required extensive refactoring
4. **Maintenance Burden**: Two code paths to maintain for same functionality

**Existing Solutions Realized**:
The feature request was already solved by existing workflows:

1. **File-based multiline**: `vim script.json` → `computo> run script.json`  
2. **CLI mode**: `computo script.json input.json`  
3. **Temp file workflow**: `echo "JSON" > temp.json && computo_repl temp.json`  

**Decision**: **REVERT** - Complexity not justified by user benefit.

**Key Lessons**:
1. **Scope discipline matters**: REPLs should be Read-Eval-Print Loops, not editors
2. **Existing solutions first**: Check if the problem is already solved before building
3. **Feature creep detection**: When a "simple" feature breaks existing tests, reconsider
4. **Separation of concerns**: Editors edit, REPLs evaluate - don't mix responsibilities
5. **User workflows**: Users already had good multiline workflows via files and run command

**Alternative Recommendation**: **Editor Integration**
- VSCode/Vim plugins for Computo syntax highlighting
- Send selection to REPL via pipe: `:!echo '%' | computo_repl`
- Language Server Protocol support for any editor

**Key Insight**: **Sometimes the best feature is the one you don't implement**. When a feature request requires significant complexity but existing workflows already solve the problem adequately, it's often better to improve documentation and tooling around existing solutions rather than add new features.

## Conclusion

The key to this project's success was maintaining clean architectural boundaries while building comprehensive development tools:

1. **Library/CLI separation** kept the core fast and clean
2. **Conditional compilation** eliminated debug overhead
3. **Consistent operator design** reduced cognitive load
4. **Thread-safe patterns** enabled concurrent use
5. **Standard JSON integration** leveraged existing tools
6. **Incremental debugging features** built on solid foundations
7. **Intelligent error reporting** improved developer experience

The debug infrastructure demonstrates how to add rich development features without compromising production performance. The wrapper pattern, conditional compilation, and incremental feature development approach should be the foundation for all future capabilities.

**Critical Success Factors**:
- **Early architectural decisions** enable later feature development
- **Leveraging existing libraries** (nlohmann::json, readline) provides robust functionality
- **Simple algorithms** (Levenshtein distance) can provide high-value features
- **Incremental development** allows rapid iteration and testing
- **Clean interfaces** make features discoverable and usable
