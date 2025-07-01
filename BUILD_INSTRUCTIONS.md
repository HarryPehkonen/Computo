# Computo Build Instructions

## Quick Start

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build

# Generate docs and test all examples
make test-docs

# Or step by step:
make docs      # Generate documentation 
make doctest   # Run example tests
```

## Available Targets

### Core Build Targets
```bash
make                    # Build computo library and CLI
make test              # Run C++ unit tests
make install           # Install computo system-wide
```

### Documentation Targets  
```bash
make readme            # Generate README.md from README.toml
make examples          # Generate examples from README.toml
make docs              # Generate both (alias: make doc)
make doctest           # Run example tests
make docs-test         # Generate docs + run tests
make test-docs         # Alias for docs-test (more intuitive)
make help-docs         # Show documentation help
```

### Cleanup
```bash
make clean             # Clean build artifacts
cd .. && rm -rf build  # Full clean (build directory)
rm -rf examples        # Clean generated examples
```

## Development Workflow

### Daily Development
```bash
# Edit examples in README.toml
vim ../README.toml

# Test everything
make test-docs

# Commit changes
cd ..
git add README.toml README.md examples/
git commit -m "Update documentation examples"
```

### First-Time Setup
```bash
# Setup Python environment (one time)
./scripts/setup_venv.sh

# Configure build
cmake -B build

# Generate and test documentation  
cd build && make test-docs
```

## Python Virtual Environment

The build system automatically detects and uses the Python virtual environment in `../venv/`. If not found, it uses system Python 3.11+.

## Why No Top-Level Makefile?

This project follows CMake best practices:
- **Single build system** (CMake only)
- **Standard workflow** (`cd build && make target`)
- **IDE compatibility** (VS Code, CLion, etc. expect pure CMake)
- **Clear separation** (build artifacts in build/, source in root)

## Examples

### Generate Fresh Documentation
```bash
cd build
make doc               # Short form
make docs             # Long form  
```

### Test Specific Categories
```bash
cd build
make examples         # Generate examples
cd ../examples/arithmetic && ./run_all.sh  # Test arithmetic only
```

### Full CI/CD Workflow
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cd build
make test             # C++ tests
make test-docs        # Documentation tests
echo "All tests passed!"
``` 