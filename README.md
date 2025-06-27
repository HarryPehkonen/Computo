# Computo

A safe, sandboxed JSON transformation engine with Lisp-like syntax expressed in JSON.

## Quick Start

```bash
# Build
cmake -B build && cmake --build build

# Transform JSON
./build/computo script.json input.json

# With string interpolation
./build/computo --interpolation script.json input.json
```

## Basic Example

**script.json:**
```json
["map", {"array": [1, 2, 3]}, ["lambda", ["x"], ["*", ["$", "/x"], 2]]]
```

**input.json:**
```json
{}
```

**Output:**
```json
[2, 4, 6]
```

## Features

- **22 operators**: arithmetic, logic, arrays, objects, templates
- **Functional programming**: map, filter, reduce with lambda expressions  
- **Type preservation**: numbers stay numbers, booleans stay booleans
- **Template integration**: Works with Permuto for advanced templating
- **Safe execution**: Sandboxed with no file system or network access

## Core Operators

| Category | Operators |
|----------|-----------|
| **Math** | `+`, `-`, `*`, `/` |
| **Logic** | `if`, `>`, `<`, `>=`, `<=`, `==`, `!=`, `approx` |
| **Data** | `$input`, `$`, `let`, `get`, `obj` |
| **Arrays** | `map`, `filter`, `reduce`, `find`, `some`, `every`, `flatMap` |
| **Templates** | `permuto.apply` |

## Documentation

- **Learning Guide**: See `book/00_index.md` for comprehensive tutorials
- **Implementation**: See `PROGRESS.md` for technical details
- **Reference**: See `CLAUDE.md` for development guidance

## Testing

```bash
cd build && ctest --verbose
```

103 tests covering all operators and edge cases.