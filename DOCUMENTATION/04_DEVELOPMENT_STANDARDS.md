# Development Standards

This document outlines the standards for code quality, error handling, and performance that all contributions to the Computo project must follow.

## 1. Code Quality and Style

-   **Tooling**: All code must be formatted with `.clang-format` and must compile with zero warnings from `.clang-tidy` using the configurations in the project root.
-   **Clarity**: Code should be clean, readable, and self-documenting where possible. Complex logic should be accompanied by comments explaining the *why*, not the *what*.
-   **Simplicity**: Avoid over-engineering. Prefer simple, direct solutions over complex abstractions unless the complexity is justified by a core requirement (e.g., the TCO trampoline).
-   **Consistency**: Follow the existing patterns in the codebase for naming, structure, and style.

## 2. Error Handling

Clear and helpful error messages are a critical feature of the engine. All error messages must adhere to the following standards:

-   **Standard Format**: `Error at <path>: <specific_problem>. <suggestion_or_explanation>`
-   **Location**: Errors must include the precise execution path (e.g., `/let/body/map`) to the expression that failed.
-   **Specificity**: The message must clearly state what was wrong (e.g., "Expected number, got string").
-   **Context**: When helpful, include the value that caused the error (e.g., `"...got string \"hello\""`).
-   **Guidance**: Whenever possible, provide a concrete suggestion for how to fix the error (e.g., `"Use {\"array\": [...]} for literal arrays."`).
-   **Typo Detection**: For unknown operators, use the Levenshtein distance algorithm to suggest a valid operator if a close match is found.

## 3. Performance

Performance is a key requirement for production use cases. All code should be written with performance in mind, and all contributions will be measured against the project's performance benchmarks.

-   **Zero Overhead Principle**: The production library (`libcomputo`) and CLI (`computo`) must have zero performance overhead from debugging features.
-   **Efficiency**: Operations should be implemented efficiently. For example, use `std::shared_ptr` to avoid unnecessary copying of large data structures in the `ExecutionContext`.
-   **Scalability**: Algorithms should scale predictably. Operations on arrays should ideally be O(n).
-   **Benchmarking**: Any significant new feature or change to a core component must be benchmarked to ensure it does not introduce a performance regression.

## 4. Testing

-   **Comprehensive Coverage**: All new code must be accompanied by tests. The project aims for a line coverage of >95%.
-   **Test Categories**: Tests should cover:
    -   **Positive Cases**: The feature works as expected with valid input.
    -   **Negative Cases**: The feature correctly throws exceptions with invalid input.
    -   **Edge Cases**: The feature handles edge cases like empty arrays, null values, and type boundaries correctly.
-   **Integration Testing**: In addition to unit tests, complex integration tests should be added to validate that features work correctly together in real-world scenarios.
-   **TCO and Thread Safety**: Any changes to the core engine must be validated against the TCO and thread-safety test suites.
