# Build System Optimization Guide

## Problem Solved

The original CMake configuration included clang-tidy as part of every compilation, which caused extremely slow builds (often timing out after 2+ minutes). This was happening because:

1. **clang-tidy was enabled globally** via `CMAKE_CXX_CLANG_TIDY`
2. **Every source file was analyzed during compilation** instead of as a separate step
3. **Hundreds of thousands of warnings** were being processed during each build

## New Build System

### Fast Development Builds (Default)

```bash
# Fast compilation without any linting/formatting (default)
cmake -B build
cmake --build build -j$(nproc)
```

This now completes in seconds instead of minutes.

### Code Quality as Separate Targets

#### Available Targets

```bash
# Format all source code
make format

# Check formatting without modifying files
make format-check

# Run static analysis with clang-tidy
make lint

# Run all code quality checks
make quality
```

#### Optional: Enable Linting During Build

```bash
# Enable clang-tidy during compilation (slow but thorough)
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# Enable clang-format during compilation
cmake -B build -DENABLE_CLANG_FORMAT=ON
cmake --build build
```

## Recommended Workflow

### Development Workflow
```bash
# 1. Fast development builds
cmake -B build
cmake --build build -j$(nproc)

# 2. Run tests
./build/test_computo

# 3. Before committing, check code quality
cd build
make format          # Fix formatting
make lint            # Check for issues
```

### CI/CD Workflow
```bash
# 1. Build with quality checks enabled
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# 2. Run separate quality targets
cd build
make format-check    # Fail if formatting is wrong
make lint            # Fail if linting issues exist
```

## Performance Comparison

| Build Type | Time | Use Case |
|------------|------|----------|
| **Development (new)** | ~15-30 seconds | Daily development |
| **With linting (old)** | 2+ minutes (often timeout) | Never practical |
| **Quality targets (new)** | ~30-60 seconds | Pre-commit checks |

## Technical Details

### CMake Options Added

- `ENABLE_CLANG_TIDY=ON/OFF` - Enable clang-tidy during compilation
- `ENABLE_CLANG_FORMAT=ON/OFF` - Enable clang-format during compilation

### Custom Targets Added

- `format` - Apply clang-format to all source files
- `format-check` - Verify formatting (CI-friendly, exits with error)
- `lint` - Run clang-tidy static analysis
- `quality` - Combined target running both format-check and lint

### Source File Collection

The build system automatically collects all relevant files:
```cmake
set(ALL_SOURCE_FILES
    ${COMPUTO_LIB_SOURCES}
    ${COMPUTO_HEADERS}
    src/cli.cpp
    src/repl.cpp
)
file(GLOB_RECURSE TEST_FILES tests/*.cpp tests/*.hpp)
list(APPEND ALL_SOURCE_FILES ${TEST_FILES})
```

## Benefits

1. **⚡ Fast Development Builds**: No more waiting for linting during compilation
2. **🎯 Targeted Quality Checks**: Run linting/formatting only when needed
3. **🔧 Flexible CI Integration**: Choose appropriate checks for different contexts
4. **🚀 Parallel Builds**: Leverage all CPU cores with `make -j$(nproc)`
5. **📊 Clear Status Messages**: Know exactly which tools are available

## Migration Notes

### Old Workflow
```bash
# Before: Slow build with forced linting
cmake -B build && cmake --build build
# Often timed out or took 2+ minutes
```

### New Workflow
```bash
# Now: Fast build for development
cmake -B build && cmake --build build -j$(nproc)
# Completes in 15-30 seconds

# Separate quality checks when needed
cd build && make quality
```

## Troubleshooting

### Missing Tools
If clang-format or clang-tidy are not found:
```bash
# Ubuntu/Debian
sudo apt install clang-format clang-tidy

# macOS
brew install clang-format
```

### Parallel Build Issues
If parallel builds cause issues:
```bash
# Use fewer cores
cmake --build build -j2

# Or single-threaded
cmake --build build
```
