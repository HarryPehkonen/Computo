# Engineering Features Documentation for Computo Rewrite

This document catalogs all critical engineering features that must be preserved and properly architected in any complete rewrite of Computo. These features represent core design decisions that define the language's behavior, performance characteristics, and usability.

## Core Architecture Features

### 1. Dual Build System (Production/REPL)
**Current Implementation**: Single CMake build creates both production and REPL variants from same source using conditional compilation (`#ifdef REPL`).

**Critical Aspects**:
- Zero debug overhead in production builds
- Separate libraries: `libcomputo.a` (production) and `libcomputorepl.a` (REPL)
- Identical language semantics between variants
- Build-time feature selection without runtime cost

**Rewrite Considerations**: Maintain clean separation between production and debug infrastructure while sharing core evaluation logic.

### 2. Thread-Safe Design
**Current Implementation**: Pure functional architecture with no global mutable state, thread-local storage for debug features, operator registry with `std::call_once` lazy initialization.

**Critical Aspects**:
- ExecutionContext uses shared_ptr for immutable inputs
- Copy-on-write for variable scopes
- Safe concurrent execution of scripts
- Primary use case: Multi-threaded JSON schema transformations

**Rewrite Considerations**: Essential for production API gateways. Thread safety must be designed in from the ground up. **Global debugger pointer should be dependency-injected rather than using thread-local storage.**

### 3. Memory Management Strategy
**Current Implementation**: Shared pointers for immutable data, automatic unwrapping of array objects, path tracking for error context.

**Critical Aspects**:
- Minimal copying of large data structures
- Automatic memory management without garbage collection overhead
- Path accumulation for detailed error reporting
- Efficient handling of nested data structures

## Language Features

### 4. JSON-Native Syntax
**Current Implementation**: All scripts are valid JSON using array syntax `["operator", arg1, arg2]`.

**Critical Aspects**:
- No custom parser needed - standard JSON libraries work
- Clear distinction between data and code
- Integration with existing JSON tooling
- Array wrapper objects `{"array": [1, 2, 3]}` for disambiguation

**Rewrite Considerations**: This is a foundational design decision that affects every aspect of the language.

### 5. Array Handling System
**Current Implementation**: Complex dual-phase evaluation with wrapper objects and unwrapping.

**Critical Aspects**:
- `{"array": [...]}` as escape hatch for operator/literal ambiguity
- Custom array key support (`--array` flag)
- First-level vs embedded-level evaluation semantics
- Performance optimization for common cases

**Rewrite Considerations**: See ARRAY_HANDLING.md for complete architectural specification.

### 6. N-ary Operator Consistency
**Current Implementation**: All operators accept multiple arguments except `not` (unary only).

**Critical Aspects**:
- Arithmetic: `["+", 1, 2, 3, 4]` → `10`
- Comparison chaining: `[">", a, b, c]` → `a > b && b > c`
- Logical operators: `["&&", cond1, cond2, cond3]`
- Consistent argument handling across operator families

**Rewrite Considerations**: This creates a very regular and predictable operator interface.

### 7. Enhanced Data Access System
**Current Implementation**: JSON Pointer-based navigation with multiple access patterns.

**Critical Aspects**:
- `["$input"]` - Access entire input
- `["$input", "/path"]` - JSON Pointer navigation
- `["$inputs"]` - Multi-input array access
- `["$", "/varname"]` - Variable access with paths
- Default value support for failed lookups

**Rewrite Considerations**: JSON Pointer integration is crucial for complex data manipulation.

### 8. Variable Scoping System
**Current Implementation**: Lexical scoping with `let` expressions and nested scope creation.

**Critical Aspects**:
- `["let", [["var", value]], body]` syntax
- Variable shadowing in nested scopes
- Path-tracked variable access for debugging
- Immutable variable bindings

**Rewrite Considerations**: Scoping rules must be clearly defined and consistently implemented.

## Performance Optimizations

### 9. Tail Call Optimization (TCO)
**Current Implementation**: Trampoline-based evaluator prevents stack overflow in recursive algorithms.

**Critical Aspects**:
- Handles arbitrarily deep recursion
- Optimized for `if` and `let` expressions
- Maintains debugging information in REPL builds
- Zero performance cost in production

**Rewrite Considerations**: Essential for functional programming patterns. The trampoline approach is proven effective.

### 10. Lazy Debug Infrastructure
**Current Implementation**: Debug hooks activated only when debugging enabled, thread-local state.

**Critical Aspects**:
- Pre-evaluation hooks for breakpoints
- Zero overhead in production builds
- Thread-local debug state management
- Instrumentation without core library modification

**Rewrite Considerations**: Debug infrastructure must be completely separate from production evaluation paths.

### 11. Operator Registry Optimization
**Current Implementation**: Thread-safe lazy initialization, categorized operators, efficient name resolution.

**Critical Aspects**:
- Single registry lookup per operator type
- Category-based organization (arithmetic, logical, array, etc.)
- Thread-safe initialization using `std::call_once`
- Fast operator name validation

## Operator System Architecture

### 12. Functional Programming Operators
**Current Implementation**: Complete set of higher-order functions with lambda support.

**Critical Aspects**:
- `map`, `filter`, `reduce`, `count`, `find`, `some`, `every`, `zip`
- Lambda expressions: `["lambda", ["param"], body]`
- Function composition and higher-order functions
- Consistent array wrapper handling

**Rewrite Considerations**: These are core to the functional programming paradigm Computo supports.

### 13. Lisp-Style List Operations
**Current Implementation**: Traditional Lisp list manipulation functions.

**Critical Aspects**:
- `car` (first), `cdr` (rest), `cons` (prepend), `append`
- Consistent return type handling
- Empty list validation
- Integration with array wrapper system

### 14. Object Manipulation
**Current Implementation**: Dynamic object construction and manipulation operators.

**Critical Aspects**:
- `obj` - Dynamic object construction with computed keys
- `keys`, `values`, `objFromPairs` for object introspection
- `pick`, `omit` for selective object filtering
- `merge` for object combination

### 15. String and Array Utilities
**Current Implementation**: Comprehensive string processing and array manipulation.

**Critical Aspects**:
- String: `split`, `join`, `trim`, `upper`, `lower`, `strConcat`
- Array: `sort`, `reverse`, `unique`
- Type validation and consistent error handling
- Unicode-aware string operations

## Error Handling System

### 16. Exception Hierarchy
**Current Implementation**: Structured exception types with context information.

**Critical Aspects**:
- `ComputoException` (base class)
- `InvalidOperatorException` with suggestions
- `InvalidArgumentException` with path context
- Levenshtein distance for typo detection

**Rewrite Considerations**: Good error messages are crucial for usability.

### 17. Path-Based Error Reporting
**Current Implementation**: Execution path tracking for precise error location.

**Critical Aspects**:
- Path accumulation: `/let/body/map`
- Context-aware error messages
- Location information for debugging
- Stack trace equivalents for functional code

## CLI and REPL Features

### 18. Production CLI Tool
**Current Implementation**: Efficient script processor with minimal overhead.

**Critical Aspects**:
- Script and input file processing
- Custom array key syntax support
- Comment support (`--comments` flag)
- Performance benchmarking mode (`--perf`)

### 19. Interactive REPL Environment
**Current Implementation**: Full-featured interactive development environment.

**Critical Aspects**:
- Readline integration with history
- Result references (`_1`, `_2` for previous results)
- Variable inspection (`vars` command)
- Script file execution (`run filename`)
- Multi-line input support

### 20. Advanced Debugging System
**Current Implementation**: Comprehensive debugging with breakpoints and inspection.

**Critical Aspects**:
- Operator and variable breakpoints
- Interactive session control
- Step-through execution
- Variable scope inspection
- Execution location tracking

## Input/Output System

### 21. Multi-Input Support
**Current Implementation**: Flexible input handling for different use cases.

**Critical Aspects**:
- Single input: `computo::execute(script, input)`
- Multiple inputs: `computo::execute(script, inputs)`
- Input array access via `$inputs[index]`
- Null input handling for parameterless scripts

### 22. Array Unwrapping System
**Current Implementation**: Final-pass unwrapping of array wrapper objects.

**Critical Aspects**:
- Recursive unwrapping through nested structures
- Custom array key transformation
- Clean output without internal wrapper objects
- Preservation of literal array objects when intended

## Testing Architecture

### 23. Comprehensive Test Suite
**Current Implementation**: Multi-level testing with specialized test categories.

**Critical Aspects**:
- Unit tests for all operator categories
- Thread safety stress testing with concurrent execution
- TCO validation with deep recursion scenarios
- Performance benchmarking with timing validation
- Integration tests for complex nested expressions

### 24. Build System Features
**Current Implementation**: Modern CMake with comprehensive tooling integration.

**Critical Aspects**:
- CMake 3.15+ with C++17 requirements
- Automatic test filtering (production vs REPL)
- Static analysis integration (clang-tidy, clang-format)
- Version header generation
- Cross-platform dependency management

## Advanced Features

### 25. JSON Pointer Integration
**Current Implementation**: Standard RFC 6901 JSON Pointer support throughout.

**Critical Aspects**:
- Path-based navigation: `/path/to/nested/data`
- Error handling for invalid paths
- Default value support for missing paths
- Integration with variable access system

### 26. Lambda System
**Current Implementation**: First-class lambda expressions with proper scoping.

**Critical Aspects**:
- Single-parameter lambda syntax: `["lambda", ["x"], body]`
- Variable capture and lexical scoping
- Lambda composition and currying
- Higher-order function support

**Desired Enhancement**: **Multi-parameter lambda support**: `["lambda", ["x", "y", "z"], body, dataSource]` with named variable access instead of awkward `/var/0`, `/var/1` syntax. Data source specification at the end for performance optimization.

### 27. Custom Truthiness Rules
**Current Implementation**: Consistent truthiness evaluation across all conditional operators.

**Critical Aspects**:
- Boolean: standard true/false
- Numbers: non-zero is truthy (including negative numbers)
- Strings: non-empty is truthy
- Arrays/Objects: non-empty is truthy
- Null/undefined: always falsy

**Rewrite Considerations**: **Keep simple, throw errors for questionable comparisons**. Comparisons like `{} vs null` should error rather than having complex truthiness rules. Design should be easily changeable if reasonable use cases emerge.

### 28. Performance Benchmarking
**Current Implementation**: Built-in performance testing with statistical analysis.

**Critical Aspects**:
- Automatic benchmark execution
- Statistical analysis (min/max/average/percentiles)
- Complex algorithm benchmarks (recursion, iteration, etc.)
- Memory usage and execution time profiling

**Enhancement Goals**: **Keep current algorithms for comparison**, but make extensible to add new performance tests for specific use cases and different transformation patterns.

## Critical Implementation Details

### 29. Type System
**Current Implementation**: JSON-native typing with comprehensive validation.

**Critical Aspects**:
- Native JSON types only (no custom types)
- Consistent type validation across all operators
- Detailed type information in error messages
- IEEE 754 double precision for all numbers

### 30. Comment Support
**Current Implementation**: Optional JSON comment parsing for script files.

**Critical Aspects**:
- Script-only comment support (not in input data)
- Standard `//` and `/* */` comment syntax
- Parse error handling with precise location information
- Optional activation via command-line flag

### 31. Extension Points
**Current Implementation**: Modular architecture supporting future extensions.

**Critical Aspects**:
- Category-based operator organization
- Plugin-style operator registration
- Hook system for evaluation interception
- Wrapper pattern for non-intrusive feature addition

## Architectural Principles for Rewrite

### Core Design Principles
1. **Functional Purity**: No global mutable state, immutable data structures
2. **Performance First**: Zero-cost abstractions, lazy evaluation where beneficial
3. **Error Clarity**: Precise error messages with full context
4. **Thread Safety**: Safe concurrent execution without locks in hot paths
5. **Extensibility**: Clean interfaces for adding new operators and features

### Technology Considerations
- **Language Choice**: Must support functional programming paradigms efficiently
- **Memory Management**: Automatic management without GC pauses
- **JSON Integration**: Native or very efficient JSON handling
- **Concurrency**: Thread-safe by design, not by addition
- **Debugging**: First-class debugging support without production overhead

### Migration Strategy
Any rewrite should:
1. Maintain 100% semantic compatibility with current language
2. Preserve all performance characteristics
3. Support gradual migration of existing scripts
4. Maintain or improve error message quality
5. Keep the same CLI and API interfaces

## Future JSON Transformation Enhancements

### 32. Enhanced JSON Processing Features
**Priority**: Performance-first, but must be useful for production JSON transformations.

**High-Value Features**:
- **Null/Undefined Coalescing**: `["coalesce", val1, val2, val3]` - first non-null value
- **Type Coercion**: `["toString", value]`, `["toNumber", value]`, `["toBoolean", value]`
- **Advanced Object Merging**: `["merge", "deep", obj1, obj2]` with conflict resolution strategies
- **Schema Validation**: `["validate", schema, data]` for input/output validation
- **Path-Based Operations**: `["transformPaths", object, pathTransformPairs]` for bulk operations
- **Array Restructuring**: `["groupBy", array, keyFunction]`, `["partition", array, predicate]`

**Design Principles**:
- **Fail Fast**: Malformed input causes immediate error
- **Performance First**: Optimize for speed while maintaining usefulness
- **Clean Abstraction**: Complex implementations acceptable with thorough documentation and tests
- **No Backward Compatibility**: Aggressive redesign allowed for optimal API

### 33. Dependency Injection Architecture
**Critical Change**: Replace global debugger pointer with dependency-injected debugging interface.

**Benefits**:
- Improved testability and isolation
- Cleaner architecture separation
- Multiple debug contexts in same process
- Better integration with production monitoring

## Conclusion

These features represent the complete engineering foundation of Computo. Each feature exists for specific design reasons and contributes to the overall goals of:
- **Functional programming paradigm** with clean, composable operations
- **High performance** with minimal overhead and efficient memory usage
- **Developer experience** with excellent error messages and debugging tools
- **Production readiness** with thread safety and reliability
- **Extensibility** for future language evolution

A successful rewrite must preserve all these characteristics while potentially modernizing the implementation approach and improving maintainability.
