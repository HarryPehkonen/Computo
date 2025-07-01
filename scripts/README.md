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

## Architecture

### Single Source of Truth: `README.toml`

All documentation content is stored in `README.toml`:

- **Meta information** - Title, subtitle, URLs
- **Introduction text** - Architecture overview, installation
- **Examples** - 100+ verified examples across 16 categories
- **Documentation sections** - (planned for future expansion)

### Generated Outputs

1. **`README.md`** - Complete documentation for AI consumption
2. **`examples/`** - Executable test directories with:
   - `script.json` - Computo transformation script
   - `input.json` / `input_N.json` - Input data files
   - `expected.json` - Expected output
   - `test.sh` - Executable test script
   - `README.md` - Example documentation

### Verification Process

1. Examples are extracted from TOML with exact input/output
2. Test scripts are generated that run actual Computo binary
3. Output is compared against expected results using `jq` for JSON comparison
4. Tests include line number references back to source TOML
5. Master test runner provides comprehensive reporting

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

## Future Enhancements

- Migrate more documentation content to structured TOML sections
- Add performance benchmarking examples
- Add error scenario examples for AI training
- Integration with CI/CD for automated verification 