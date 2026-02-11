# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Computo is a sandboxed, thread-safe JSON-native data transformation engine with Lisp-like syntax. It provides:
- **libcomputo** - a minimal C++ static library for JSON transformations
- **computo** - a CLI with script execution, interactive REPL, and debugging
- A human-friendly "sugar syntax" that bidirectionally transpiles to/from JSON

The language uses JSOM (not nlohmann/json) as its JSON library. Scripts are JSON arrays where the first element is the operator name: `["+", 1, 2, 3]`.

## Build Commands

```bash
# Standard development build
cmake -B build && cmake --build build -j$(nproc)

# Clean release build
./build.sh

# Run all tests
ctest --test-dir build --output-on-failure

# Run a specific test by name
ctest --test-dir build -R test_arithmetic --verbose

# Code formatting and linting (separate from build)
cd build && make format        # auto-format with clang-format
cd build && make format-check  # check formatting (CI)
cd build && make lint          # clang-tidy static analysis
cd build && make quality       # all quality checks

# Build with sanitizers
cmake -B build -DENABLE_ASAN=ON -DENABLE_UBSAN=ON && cmake --build build -j$(nproc)
```

Build artifacts: `libcomputo.a`, `computo` (CLI), `test_computo` (test binary).

## Architecture

### Core Evaluation Engine (`src/computo.cpp`)

Recursive interpreter with a **trampoline pattern for tail call optimization (TCO)**. The `evaluate()` function runs in a `while(true)` loop. Special forms (`if`, `let`) return a `TailCall` struct instead of recursing, and the loop bounces to the next evaluation, keeping the stack flat.

Key types in `include/computo.hpp`:
- `ExecutionContext` - immutable context passed through evaluation. New scopes created via `with_variables()`. Input data stored as `shared_ptr` (no deep copies).
- `EvaluationResult` - variant distinguishing regular results from `TailCall` continuations.
- `DebugContext` - breakpoint/trace system, passed as optional parameter.

### Operator System (`src/operators/`)

All operators registered in a thread-safe `std::map` initialized with `std::call_once`. Each operator is a function taking `(args, context, debug_ctx)`. Operators are split across files by category:
- `arithmetic.cpp`, `comparison.cpp`, `logical.cpp` - math/logic
- `data_access.cpp` - `$input`, `$`, `let`, `get`
- `control_flow.cpp` - `if`
- `array_ops.cpp` - `map`, `filter`, `reduce`, `count`, `find`, `some`, `every`
- `object_ops.cpp` - `obj`, `keys`, `values`, `pick`, `omit`, `merge`
- `functional_ops.cpp` - `car`, `cdr`, `cons`, `append`
- `string_utility_ops.cpp` - `join`, `strConcat`, `split`, `contains`, etc.
- `sort_utils.cpp` - `sort`, `reverse`, `unique`, `zip`
- `shared.cpp` / `shared.hpp` - common utilities (lambda evaluation, truthiness, etc.)

### Sugar Syntax (`src/sugar_parser.cpp`, `src/sugar_writer.cpp`)

Bidirectional transpiler between human-friendly syntax and JSON. Parser converts sugar to JSON; writer converts JSON back to sugar.

### CLI (`src/main.cpp`, `src/cli_args.cpp`, `src/repl.cpp`)

Entry point handles argument parsing, file execution, piped input, and REPL mode with readline support. Debug features (breakpoints, tracing) live only in the CLI layer.

## Key Design Principles

- **All operators are n-ary** with consistent behavior. `["+", 1, 2, 3]` sums all args. `[">", 10, 5, 3]` chains as `10 > 5 && 5 > 3`.
- **Thread safety via immutability** - no global mutable state. ExecutionContext is copied for new scopes, operator registry is initialize-once-then-read-only.
- **No emojis** in code, error messages, or output.
- **Error messages** follow: `Error at <path>: <problem>. <suggestion>` with JSON Pointer paths and typo suggestions via Levenshtein distance.
- **Tests use raw string JSON** (`R"(["+", 1, 2])"` parsed with JSOM) rather than C++ JSON construction.

## Testing

Tests use Google Test. Main test binary is `test_computo`. Additional specialized binaries: `test_memory_safety`, `test_thread_safety`, `test_performance` (disabled by default).

C++17. Code style: LLVM base, 4-space indent, 100-column limit (see `.clang-format`).
