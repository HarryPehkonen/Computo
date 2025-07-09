# TCO Implementation Experimental Plan

## Overview

This document outlines a systematic experimental approach to implement and compare three different methods of achieving Tail Call Optimization (TCO) while preserving debugging capabilities in the Computo JSON transformation engine.

## Current Baseline State

### Git Information
- **Baseline Commit**: `206a10b23df24b594fc22a702330b15e6b59b889`
- **Commit Message**: "update LESSONS.md"
- **Date**: Current working state
- **Branch**: `main`

### Tracked Files (43 total)
```
Source Files (22):
- include/computo.hpp
- src/benchmark.cpp
- src/cli.cpp
- src/computo.cpp
- src/operators/arithmetic.cpp
- src/operators/array.cpp
- src/operators/comparison.cpp
- src/operators/data.cpp
- src/operators/declarations.hpp
- src/operators/functional.cpp
- src/operators/logical.cpp
- src/operators/shared.hpp
- src/operators/stubs.cpp
- src/operators/utilities.cpp
- src/read_json.hpp
- src/repl.cpp

Test Files (16):
- tests/test_arithmetic.cpp
- tests/test_array.cpp
- tests/test_cli.cpp
- tests/test_comparison.cpp
- tests/test_data.cpp
- tests/test_debug.cpp
- tests/test_debugging.cpp
- tests/test_functional.cpp
- tests/test_get.cpp
- tests/test_infrastructure.cpp
- tests/test_integration.cpp
- tests/test_let.cpp
- tests/test_obj.cpp
- tests/test_performance.cpp
- tests/test_repl.cpp
- tests/test_thread_safety.cpp
- tests/test_utilities.cpp
- tests/test_zip_object_creation.cpp

Documentation Files (5):
- ANTI_PATTERNS.md
- CLAUDE.md
- CLEAN_ARCHITECTURE.md
- CORE_REQUIREMENTS.md
- DEBUG_AND_REPL.md
- IMPLEMENTATION_GUIDE.md
- LESSONS.md (will accumulate findings across experiments)
- README.md
```

### Current TCO Problem
- **Issue**: Pre-evaluation hook system for debugging broke original TCO implementation
- **Current State**: Neither proper TCO nor complete debugging functionality
- **Conflict**: Debugging requires recursion depth tracking; TCO eliminates recursion

## Experimental Framework

### Git Workflow Strategy

#### 1. Baseline Preservation
```bash
# Tag current state for easy restoration
git tag pre-tco-experiment

# Create experiment tracking branch
git checkout -b tco-experiments

# Ensure clean working directory
git status --porcelain  # Should show only untracked files
```

#### 2. Per-Experiment Branches
```bash
# Solution 1: Explicit Debug Stack
git checkout -b tco-solution-1-explicit-debug-stack
# Implementation work here...
git add -A && git commit -m "Implement TCO Solution 1: Explicit Debug Stack"

# Return to baseline for Solution 2
git checkout main
git reset --hard pre-tco-experiment

# Solution 2: Continuation Passing Style  
git checkout -b tco-solution-2-continuation-passing
# Implementation work here...
git add -A && git commit -m "Implement TCO Solution 2: Continuation Passing Style"

# Return to baseline for Solution 3
git checkout main
git reset --hard pre-tco-experiment

# Solution 3: Lazy Debug Stack Construction
git checkout -b tco-solution-3-lazy-debug-stack
# Implementation work here...
git add -A && git commit -m "Implement TCO Solution 3: Lazy Debug Stack Construction"
```

#### 3. Restoration Commands
```bash
# Return to exact pre-experiment state
git checkout main
git reset --hard pre-tco-experiment

# Alternative: Return to any specific experiment
git checkout tco-solution-1-explicit-debug-stack
git checkout tco-solution-2-continuation-passing  
git checkout tco-solution-3-lazy-debug-stack

# Clean untracked files if needed
git clean -fd
```

### File Tracking Strategy

#### Files to Modify in Each Experiment
- `src/computo.cpp` - Core evaluation engine
- `src/operators/data.cpp` - `if_operator` and `let_binding` for TCO
- `include/computo.hpp` - Any new data structures
- `tests/test_performance.cpp` - TCO validation tests
- `LESSONS.md` - Cumulative findings (preserved across experiments)

#### Files to EXCLUDE from Reset
- `LESSONS.md` - Will accumulate insights from all three approaches
- `TCO_PLAN.md` - This experimental plan document
- Build artifacts and temporary files (already gitignored)

## Three TCO Implementation Strategies

### Solution 1: Explicit Debug Stack with TCO Trampoline

#### Approach
Replace recursive `evaluate()` with trampoline loop that maintains explicit debug stack.

#### Key Components
```cpp
struct DebugFrame {
    std::string operator_name;
    nlohmann::json expression;
    std::vector<std::string> path;
    std::map<std::string, nlohmann::json> variables;
    int depth;
};

nlohmann::json evaluate_with_explicit_debug_stack(nlohmann::json expr, ExecutionContext ctx) {
    std::vector<DebugFrame> debug_stack;
    
    while (true) {  // TCO trampoline
        // Build debug frame
        DebugFrame frame = create_debug_frame(expr, ctx, debug_stack.size());
        debug_stack.push_back(frame);
        
        // Call pre-evaluation hook with simulated stack
        if (ctx.has_pre_evaluation_hook()) {
            EvaluationAction action = call_debug_hook(frame);
            if (action == EvaluationAction::PAUSE) {
                handle_debug_pause(debug_stack);
            }
        }
        
        // Handle tail-recursive forms
        if (is_tail_call_form(expr)) {
            std::tie(expr, ctx) = prepare_tail_call(expr, ctx);
            continue;  // No stack growth!
        } else {
            auto result = handle_non_tail_call(expr, ctx);
            debug_stack.pop_back();
            return result;
        }
    }
}
```

#### Files to Modify
- `src/computo.cpp`: Replace `evaluate()` function
- `src/operators/data.cpp`: Modify `if_operator` and `let_binding`
- `include/computo.hpp`: Add `DebugFrame` struct

#### Expected Benefits
- True TCO with no stack growth
- Complete debugging functionality preserved
- Explicit control over debug information
- Minimal performance overhead when debugging disabled

#### Potential Challenges
- Manual stack management complexity
- Memory overhead of debug frames
- Ensuring debug fidelity matches real recursion

### Solution 2: Continuation Passing Style (CPS)

#### Approach
Transform recursive calls into explicit continuations, enabling both TCO and debugging.

#### Key Components
```cpp
struct Continuation {
    std::function<nlohmann::json(nlohmann::json)> k;
    DebugFrame debug_info;
};

nlohmann::json evaluate_cps(nlohmann::json expr, ExecutionContext ctx, 
                            std::vector<Continuation>& cont_stack) {
    while (true) {
        // Call debug hook with continuation stack info
        call_debug_hook_with_continuations(expr, ctx, cont_stack);
        
        if (is_if_expression(expr)) {
            auto condition = evaluate_cps(get_condition(expr), ctx, cont_stack);
            if (is_truthy(condition)) {
                expr = get_then_branch(expr);  // TCO
                continue;
            } else {
                expr = get_else_branch(expr);  // TCO
                continue;
            }
        }
        
        if (is_let_expression(expr)) {
            // Evaluate bindings and continue with body
            ctx = evaluate_let_bindings(expr, ctx);
            expr = get_let_body(expr);  // TCO
            continue;
        }
        
        // Handle other operators...
    }
}
```

#### Files to Modify
- `src/computo.cpp`: Transform to CPS style
- `src/operators/data.cpp`: CPS-compatible operators
- `include/computo.hpp`: Add continuation types

#### Expected Benefits
- Elegant theoretical foundation
- Natural debugging with continuation stack
- Potential for advanced control flow
- Academic soundness

#### Potential Challenges
- Complex transformation of existing code
- Learning curve for CPS concepts
- Potential performance overhead of continuations
- Integration with existing operator system

### Solution 3: Lazy Debug Stack Construction

#### Approach
Use lightweight call tracking with on-demand debug stack reconstruction.

#### Key Components
```cpp
struct TailCall { 
    nlohmann::json expr; 
    ExecutionContext ctx; 
    std::string op_name;
    std::chrono::high_resolution_clock::time_point timestamp;
};

nlohmann::json evaluate_lazy_debug(nlohmann::json expr, ExecutionContext ctx) {
    std::vector<TailCall> call_chain;  // Lightweight tracking
    
    while (true) {
        call_chain.push_back({expr, ctx, get_operator_name(expr), std::chrono::high_resolution_clock::now()});
        
        if (breakpoint_hit(expr, ctx)) {
            // Reconstruct full debug stack on demand
            auto debug_stack = reconstruct_debug_info(call_chain);
            handle_debug_session(debug_stack);
        }
        
        // Continue with TCO
        if (is_tail_form(expr)) {
            std::tie(expr, ctx) = transform_tail_call(expr, ctx);
            continue;
        } else {
            return handle_base_case(expr, ctx);
        }
    }
}

std::vector<DebugFrame> reconstruct_debug_info(const std::vector<TailCall>& call_chain) {
    std::vector<DebugFrame> debug_stack;
    for (size_t i = 0; i < call_chain.size(); ++i) {
        const auto& call = call_chain[i];
        DebugFrame frame;
        frame.operator_name = call.op_name;
        frame.expression = call.expr;
        frame.path = call.ctx.path;
        frame.variables = call.ctx.variables;
        frame.depth = i;
        debug_stack.push_back(frame);
    }
    return debug_stack;
}
```

#### Files to Modify
- `src/computo.cpp`: Implement lazy tracking and reconstruction
- `src/operators/data.cpp`: Minimal changes for tail call detection
- `include/computo.hpp`: Add `TailCall` struct and reconstruction functions

#### Expected Benefits
- Minimal memory overhead during normal execution
- Full debug info available when needed
- Clean separation of concerns
- Good performance when debugging inactive

#### Potential Challenges
- Complexity of on-demand reconstruction
- Potential latency when debugging activates
- Edge cases in partial stack reconstruction
- Maintaining call chain accuracy

## Standardized Test Framework

### Deep Recursion Tests
Create tests that will definitely cause stack overflow without TCO:

```cpp
// Fibonacci - Classic recursive test
TEST(TCOTest, DeepFibonacci) {
    json script = create_recursive_fibonacci_script(50);
    auto result = exec(script);
    EXPECT_EQ(result, expected_fibonacci_50);
}

// List processing - Functional programming test
TEST(TCOTest, DeepListProcessing) {
    json large_list = create_list(10000);
    json script = json::array({
        "reduce",
        json::object({{"array", large_list}}),
        json::array({"lambda", json::array({"acc", "x"}), 
                    json::array({"+", json::array({"$", "/acc"}), json::array({"$", "/x"})})
                   }),
        0
    });
    auto result = exec(script);
    EXPECT_EQ(result, 49995000);  // Sum of 0..9999
}

// Nested let expressions - Variable scoping test
TEST(TCOTest, DeepNestedLet) {
    json script = create_nested_let_script(1000);  // 1000 levels deep
    auto result = exec(script);
    EXPECT_EQ(result, 1000);
}

// Conditional recursion - Control flow test
TEST(TCOTest, DeepConditionalRecursion) {
    json script = create_countdown_script(5000);  // Countdown from 5000
    auto result = exec(script);
    EXPECT_EQ(result, 0);
}
```

### Debugging Functionality Tests
Ensure all debugging features work with each TCO approach:

```cpp
// Breakpoint tests
TEST(TCODebugTest, BreakpointsInDeepRecursion) {
    debug_wrapper.add_operator_breakpoint("if");
    json script = create_recursive_fibonacci_script(20);
    // Should hit breakpoints without stack overflow
}

// Variable inspection tests  
TEST(TCODebugTest, VariableInspectionInRecursion) {
    json script = create_nested_let_script(100);
    // Test vars command at various depths
}

// Stepping tests
TEST(TCODebugTest, SteppingThroughRecursion) {
    json script = create_countdown_script(50);
    // Test step, continue, finish commands
}
```

### Performance Benchmarks
Measure and compare performance across all approaches:

```cpp
// Execution time tests
TEST(TCOPerfTest, ArithmeticPerformance) {
    auto baseline_time = measure_current_implementation();
    auto tco_time = measure_tco_implementation();
    EXPECT_LT(tco_time, baseline_time * 1.1);  // No more than 10% slower
}

// Memory usage tests
TEST(TCOPerfTest, MemoryUsage) {
    auto memory_before = get_memory_usage();
    run_deep_recursion_test();
    auto memory_after = get_memory_usage();
    EXPECT_LT(memory_after - memory_before, 1024 * 1024);  // Less than 1MB growth
}

// Debug overhead tests
TEST(TCOPerfTest, DebugOverhead) {
    auto no_debug_time = measure_without_debugging();
    auto debug_time = measure_with_debugging();
    EXPECT_LT(debug_time, no_debug_time * 2.0);  // Less than 2x overhead
}
```

## Metrics Collection Strategy

### Performance Metrics
- **Execution Time**: Microseconds per operation
- **Memory Usage**: Peak memory consumption during recursion  
- **Stack Depth**: Actual vs simulated stack depth
- **Debug Overhead**: Performance cost of debugging features

### Code Complexity Metrics
- **Lines of Code**: Changes required for each approach
- **Cyclomatic Complexity**: Control flow complexity
- **Maintainability Index**: Subjective code clarity assessment
- **Integration Complexity**: How well it fits with existing architecture

### Debugging Fidelity Metrics
- **Feature Completeness**: All debugging features work identically
- **Information Accuracy**: Debug info matches recursive execution
- **Response Time**: Latency when debugging activates
- **User Experience**: Subjective quality of debugging session

## Experimental Protocol

### Phase 1: Baseline Measurement
1. **Performance Baseline**: Measure current implementation performance
2. **Create Recursive Tests**: Implement deep recursion test cases
3. **Document Current Behavior**: Capture exact current debugging behavior
4. **Commit Baseline**: `git tag pre-tco-baseline-measurements`

### Phase 2: Solution 1 Implementation
1. **Create Branch**: `git checkout -b tco-solution-1-explicit-debug-stack`
2. **Implement Explicit Debug Stack**: Modify core evaluation engine
3. **Run Test Suite**: Verify all tests pass including new recursive tests
4. **Collect Metrics**: Performance, memory, complexity measurements
5. **Update LESSONS.md**: Document findings, pros/cons, challenges
6. **Commit**: `git commit -m "Complete TCO Solution 1: Explicit Debug Stack"`

### Phase 3: Solution 2 Implementation  
1. **Return to Baseline**: `git checkout main && git reset --hard pre-tco-experiment`
2. **Create Branch**: `git checkout -b tco-solution-2-continuation-passing`
3. **Implement CPS**: Transform evaluation to continuation-passing style
4. **Run Test Suite**: Same tests as Solution 1
5. **Collect Metrics**: Same metrics as Solution 1
6. **Update LESSONS.md**: Add comparative analysis
7. **Commit**: `git commit -m "Complete TCO Solution 2: Continuation Passing Style"`

### Phase 4: Solution 3 Implementation
1. **Return to Baseline**: `git checkout main && git reset --hard pre-tco-experiment` 
2. **Create Branch**: `git checkout -b tco-solution-3-lazy-debug-stack`
3. **Implement Lazy Construction**: On-demand debug stack reconstruction
4. **Run Test Suite**: Same tests as previous solutions
5. **Collect Metrics**: Same metrics as previous solutions  
6. **Update LESSONS.md**: Final comparative analysis
7. **Commit**: `git commit -m "Complete TCO Solution 3: Lazy Debug Stack Construction"`

### Phase 5: Analysis and Decision
1. **Compare All Metrics**: Performance, complexity, maintainability
2. **Document Final Recommendation**: Based on empirical evidence
3. **Create Summary**: Best approach for Computo architecture
4. **Plan Implementation**: Detailed steps for chosen solution

## Success Criteria

### Functional Requirements
- ✅ **TCO Effectiveness**: No stack overflow on 1000+ level recursion
- ✅ **Debugging Completeness**: All debugging features work identically
- ✅ **Performance**: No more than 20% performance degradation  
- ✅ **Memory Efficiency**: Reasonable memory usage for debug structures

### Quality Requirements
- ✅ **Code Clarity**: Implementation is understandable and maintainable
- ✅ **Integration**: Fits cleanly with existing architecture
- ✅ **Documentation**: Thorough analysis in LESSONS.md
- ✅ **Testability**: All features remain testable

### Comparative Requirements
- ✅ **Objective Analysis**: Metrics-based comparison of all three approaches
- ✅ **Trade-off Documentation**: Clear understanding of pros/cons
- ✅ **Recommendation**: Data-driven decision on best approach
- ✅ **Reproducibility**: Other developers can repeat experiments

## Risk Mitigation

### Potential Risks
1. **Breaking Existing Functionality**: Extensive test suite prevents regression
2. **Performance Degradation**: Baseline measurements provide clear targets
3. **Debugging Quality Loss**: Standardized debugging tests ensure fidelity
4. **Implementation Complexity**: Incremental approach with git safety net

### Mitigation Strategies
1. **Git Safety Net**: Easy restoration to baseline with `git reset --hard pre-tco-experiment`
2. **Comprehensive Testing**: All existing tests must pass for each solution
3. **Incremental Development**: Small, testable changes with frequent commits
4. **Documentation**: Detailed analysis prevents knowledge loss

## Post-Experiment Actions

### Immediate Actions
1. **Choose Best Solution**: Based on empirical evidence
2. **Clean Implementation**: Polish chosen approach for production
3. **Update Documentation**: Reflect final implementation in all docs
4. **Archive Experiments**: Keep branches for future reference

### Long-term Considerations
1. **Monitor Performance**: Ensure TCO benefits are realized in production
2. **User Feedback**: Collect feedback on debugging experience
3. **Future Enhancements**: Use LESSONS.md insights for future development
4. **Knowledge Sharing**: Share findings with broader development community

## Conclusion

This experimental framework provides a rigorous, scientific approach to solving the TCO+debugging conflict in Computo. By implementing and comparing three distinct approaches with standardized tests and metrics, we'll have empirical evidence to make the best architectural decision for the project's long-term success.

The systematic git workflow ensures we can safely experiment while preserving the ability to return to our baseline state. The comprehensive test framework ensures we don't lose any existing functionality while gaining the benefits of proper tail call optimization.

Most importantly, the detailed documentation in LESSONS.md will capture valuable insights that benefit future development decisions and serve as a reference for other developers facing similar architectural challenges.