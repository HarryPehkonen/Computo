# Building Computo

This guide provides comprehensive instructions for compiling, testing, and understanding the build process for the Computo project.

## Prerequisites

-   **C++17 Compiler**: A modern C++ compiler (GCC, Clang, MSVC) that supports C++17.
-   **CMake**: Version 3.15 or higher.
-   **nlohmann/json**: The JSON library for C++. The build system will attempt to find it if installed.
-   **Google Test**: The testing framework. The build system will attempt to find it if installed.
-   **readline** (Optional): For an enhanced REPL experience with history and line editing. The build will proceed without it, but the REPL will have basic input capabilities.
-   **Clang-Tidy** (Optional): For static code analysis. If found, it will be run automatically during compilation.

## Unified Build Process

The project uses a unified build system that creates all production and debugging targets in a single run. There is no need for special flags like `-DREPL=ON`.

To build the entire project, run the following commands from the project root:

```bash
# 1. Configure the build system
cmake -B build

# 2. Compile all targets
cmake --build build
```

This will create a `build/` directory containing all the compiled artifacts.

## Build Artifacts

The unified build process generates the following targets:

### Libraries

-   `libcomputo.a`: The core, production-ready static library. It is lightweight and contains no debugging or REPL-specific code, making it ideal for linking into production applications.
-   `libcomputorepl.a`: The enhanced static library for development. It includes all the necessary hooks and code (compiled with the `-DREPL` flag) to support the interactive debugger and REPL.

### Executables

-   `computo`: The production command-line interface (CLI). It links against `libcomputo.a` and is optimized for fast, non-interactive script execution.
-   `computo_repl`: The interactive Read-Eval-Print Loop (REPL) for development and debugging. It links against `libcomputorepl.a` and provides features like breakpoints, variable inspection, and command history.

### Test Suites

-   `test_computo`: The test suite for the production library. It links against `libcomputo.a` and verifies the correctness and performance of the core engine.
-   `test_computo_repl`: The test suite for the REPL/debug library. It links against `libcomputorepl.a` and validates the debugging features and interactive functionality.

## Testing

The project uses CTest, CMake's testing tool, to run the test suites.

To run all tests after building the project, execute the following command from the `build` directory:

```bash
cd build
ctest --output-on-failure
```

This command will automatically discover and run both `computo_tests` and `computo_repl_tests`.

-   `computo_tests`: Validates the core functionality of the production library.
-   `computo_repl_tests`: Ensures that the debugging and REPL-specific features work as expected.

For more detailed output, you can use the `--verbose` flag:

```bash
ctest --verbose
```

## Code Quality

Code quality is enforced automatically during the build process if the necessary tools are found.

-   **Clang-Format**: Code formatting is defined in the `.clang-format` file. It is recommended to integrate this with your editor to format code automatically.
-   **Clang-Tidy**: Static analysis is configured via the `.clang-tidy` file. If `clang-tidy` is installed and found by CMake, it will run on every source file during compilation, reporting warnings for potential bugs, performance issues, and style violations.

A clean build should produce no `clang-tidy` warnings.
