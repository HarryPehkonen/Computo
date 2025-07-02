# Computo Documentation Scripts

This directory contains the Python scripts used to generate documentation from the single source of truth: `README.toml`.

## Overview

The documentation generation system ensures that all examples in the README are verified to work exactly as described. This prevents AI systems from being misled by outdated or broken examples.

## Files

- `generate_readme.py` - Generates `README.md` from `README.toml`
- `generate_examples.py` - Generates test examples and directory structure from `README.toml`
- `requirements.txt` - Python dependencies (minimal - uses standard library)
- `setup_venv.sh` - Sets up Python virtual environment

## Requirements

- Python 3.11+ (for built-in `tomllib` support)
- Virtual environment at `../venv/` (created by `setup_venv.sh`)

## Usage

### Quick Start

```bash
# From project root - set up environment once
./scripts/setup_venv.sh

# Use CMake targets (recommended)
make docs-test    # Generate docs and run all tests
make docs         # Generate README.md and examples
make readme       # Generate README.md only
make examples     # Generate examples only
make doctest      # Run example tests only
```

### Manual Usage

```bash
# Activate virtual environment
source venv/bin/activate

# Generate README.md
python scripts/generate_readme.py

# Generate examples
python scripts/generate_examples.py

# Run tests
cd examples && ./run_all.sh
```

## README.toml Syntax and Features

### File Structure

```toml
[meta]
title = "Computo"
subtitle = "A safe, sandboxed JSON transformation engine..."
human_docs_url = "https://example.com/docs"
human_docs_repo = "https://github.com/example/docs"

[intro]
description = """
Multi-line description that supports {human_docs_url} interpolation...
"""

[[examples]]
name = "example_name"
category = "category_name"
description = "Example description..."
script = ["operator", "arg1", "arg2"]
input = {"key": "value"}
expected = {"result": "value"}
# ... optional fields
```

### Example Fields Reference

#### Required Fields

- `name` - Unique identifier for the example (becomes directory name)
- `category` - Groups examples in documentation and file structure
- `description` - Explanation of what the example demonstrates
- `script` - The Computo JSON script to execute
- `expected` - Expected output result for validation

#### Input Data Fields

```toml
# Single input (most common)
input = {"key": "value"}

# Multiple inputs (for multi-input examples)
inputs = [
    {"first": "input"},
    {"second": "input"}
]

# No input (script-only examples)
input = {}
```

#### Execution Control Fields

```toml
# Command line flags
flags = ["--debug", "--trace", "--profile"]

# Skip automatic execution (for interactive/special examples)
dontrun = true

# Provide manually captured output instead of auto-execution
manualrun = """
$ computo --interactive script.json input.json
DEBUG: Debug mode enabled [INTERACTIVE]
...complete interactive session...
42
"""
```

#### Special Use Cases

```toml
# CLI usage examples (documentation only)
cli_example = "./build/computo --pretty=2 script.json input.json"

# C++ builder examples (documentation only)
cpp_example = """
// ComputoBuilder code example
auto builder = CB::add(15, 27);
"""
```

### Debug and Interactive Examples

The system supports sophisticated debugging examples:

#### Debug Flags
All debug-related flags automatically enable debug mode:
- `--debug` - Basic timing and enhanced error reporting
- `--trace` - Execution tracing with operation flow
- `--profile` - Performance profiling with timing analysis
- `--interactive` - Interactive debugging mode
- `--break-on=OPERATOR` - Set breakpoints on operators
- `--watch=VARIABLE` - Monitor variable changes
- `--slow-threshold=MS` - Report slow operations
- `--debug-level=LEVEL` - Control verbosity (error|warning|info|debug|verbose)

#### Interactive Examples with `manualrun`

For examples requiring user interaction:

```toml
[[examples]]
name = "interactive_debugging"
category = "debugging"
description = "Interactive debugging session example"
script = ["let", [["x", 42]], ["+", ["$", "/x"], 1]]
input = {}
expected = 43
flags = ["--interactive", "--break-on=+"]
manualrun = """
$ computo --interactive --break-on=+ script.json input.json
🔍 Debug mode enabled [INTERACTIVE]

BREAKPOINT: + at /+
Hit count: 1
Arguments: [["$", "/x"], 1]

Variables in scope:
  x: 42

Debug Commands:
  (c)ontinue  (s)tep  (i)nspect <var>  (e)val <expr>  (h)elp  (q)uit
> c
Continuing execution...
SUCCESS: EXECUTION SUCCESSFUL in 2ms
==========================================

RESULT:
==========
43
"""
```

#### Examples that Cannot Run Automatically

```toml
[[examples]]
name = "special_example"
category = "debugging"
description = "Example requiring special handling"
script = ["some", "complex", "operation"]
input = {}
expected = "result"
dontrun = true
flags = ["--interactive"]
```

### Generated Output Behavior

#### README.md Generation

- **Script Display**: JSON script formatted with proper indentation
- **Input Display**: Single input or multiple inputs clearly labeled
- **Flags Display**: Command line flags shown if present
- **Debug Output**: For debug examples, captured output displayed in code blocks
- **Manual Output**: `manualrun` content appears as debug output section
- **Expected Output**: JSON result formatted with proper indentation

#### Test Generation

- **Automatic Execution**: Default behavior for most examples
- **Manual Output**: `manualrun` examples display output and extract JSON result
- **Skip Execution**: `dontrun = true` examples validate files but don't execute
- **Debug Handling**: Debug examples capture stderr separately from stdout
- **JSON Validation**: Results compared using `jq` for accurate JSON comparison
- **Timeout Protection**: 30-second timeout prevents hanging tests

### Categories and Organization

Examples are organized by category:

- `arithmetic` - Basic mathematical operations
- `comparison` - Comparison and equality operators
- `logical` - Boolean logic operations
- `conditional` - If-then-else control flow
- `data-access` - Input access and JSON pointers
- `multiple-inputs` - Multi-input processing
- `data-construction` - Object and array creation
- `array-operations` - Array transformation functions
- `functional-lists` - Functional programming operations
- `lambda-functions` - Lambda expressions and closures
- `json-patch` - RFC 6902 JSON Patch operations
- `permuto` - Template and interpolation features
- `real-world` - Complex practical examples
- `cli-usage` - Command-line interface examples
- `cpp-builder` - C++ API documentation
- `debugging` - Debug features and tools

### Advanced Features

#### Multi-line Scripts

```toml
script = [
    "let",
    [
        ["variable1", "value1"],
        ["variable2", "value2"]
    ],
    ["complex", "nested", "operation"]
]
```

#### Template Interpolation

The `description` field supports variable interpolation:

```toml
description = """
See the full documentation at {human_docs_url}
Repository: {human_docs_repo}
"""
```

#### Line Number Tracking

The system automatically tracks line numbers in README.toml and includes them in:
- Generated test scripts
- Error messages
- Test runner output
- Debug information

### Error Handling and Validation

#### JSON Validation
- All `script`, `input`, `inputs`, and `expected` fields validated as proper JSON
- Scripts can use either direct JSON or string format (parsed automatically)
- Validation errors include example name and line number

#### Test Validation
- Scripts must execute successfully
- Output must match expected JSON exactly
- Debug examples capture and validate debug output
- Missing binary detection with helpful error messages

#### Debug Output Processing
- Automatic extraction of JSON results from debug output
- Support for all JSON types (objects, arrays, numbers, strings, booleans, null)
- Fallback handling for extraction failures
- Preservation of manual debug sessions

## Architecture

### Single Source of Truth: `README.toml`

All documentation content is stored in `README.toml`:

- **Meta information** - Title, subtitle, URLs
- **Introduction text** - Architecture overview, installation
- **Examples** - 130+ verified examples across 16 categories
- **Documentation sections** - (planned for future expansion)

### Generated Outputs

1. **`README.md`** - Complete documentation for AI consumption
2. **`examples/`** - Executable test directories with:
   - `script.json` - Computo transformation script
   - `input.json` / `input_N.json` - Input data files
   - `expected.json` - Expected output
   - `test.sh` - Executable test script with proper error handling
   - `README.md` - Example documentation

### Verification Process

1. Examples are extracted from TOML with exact input/output
2. Test scripts are generated that run actual Computo binary
3. Output is compared against expected results using `jq` for JSON comparison
4. Tests include line number references back to source TOML
5. Master test runner provides comprehensive reporting
6. Special handling for interactive and debug examples

### Debug Output Integration

1. **Live Capture**: Debug examples run with actual binary to capture real output
2. **Manual Override**: `manualrun` provides hand-crafted debug sessions
3. **Smart Extraction**: JSON results extracted from debug output automatically
4. **Proper Display**: Debug output appears in README as formatted code blocks
5. **Validation**: Even manual examples validate extracted results

## Integration with Build System

The scripts are integrated with CMake build system:

- CMake detects and uses Python virtual environment automatically
- Documentation targets depend on `README.toml` for rebuilding
- Example tests depend on built `computo` binary
- Proper dependency tracking ensures minimal rebuilds

## Benefits

- **Guaranteed Accuracy**: All examples are verified to work
- **Single Source of Truth**: One file contains all example code
- **AI-Friendly**: Generated README optimized for AI comprehension
- **Developer-Friendly**: Clear structure and automated testing
- **Maintainable**: Changes to examples automatically propagate
- **Debug-Aware**: Sophisticated debugging examples with real output
- **Interactive-Safe**: No hanging on interactive examples
- **Comprehensive**: Covers all Computo features with working examples

## Troubleshooting

### Common Issues

**Examples failing to run:**
- Ensure `computo` binary is built: `cmake --build build`
- Check Python version: `python --version` (need 3.11+)
- Verify virtual environment: `source venv/bin/activate`

**JSON parsing errors:**
- Validate TOML syntax with a TOML parser
- Check for trailing commas in JSON
- Ensure proper escaping in multi-line strings

**Interactive examples hanging:**
- Use `manualrun` for interactive examples
- Add `dontrun = true` for special cases
- Verify timeout settings in test scripts

**Debug output not appearing:**
- Ensure debug flags are set correctly
- Check that `computo` binary supports debug features
- Verify debug output goes to stderr, not stdout

## Future Enhancements

- Migrate more documentation content to structured TOML sections
- Add performance benchmarking examples
- Add error scenario examples for AI training
- Integration with CI/CD for automated verification
- Enhanced debug output formatting and processing
- Support for more complex interactive scenarios 