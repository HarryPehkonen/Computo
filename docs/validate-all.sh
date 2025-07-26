#!/bin/bash
set -e

# Complete Computo Documentation Validation Workflow
# This script validates operator coverage and tests all documented examples

echo "Computo Documentation Validation"
echo "================================="

# Check if Computo binary exists
if [ ! -f "./build/computo" ]; then
    echo "Error: Computo binary not found at ./build/computo"
    echo "Please build Computo first: cmake --build build"
    exit 1
fi

# Check if operators.yaml exists
if [ ! -f "docs/operators.yaml" ]; then
    echo "Error: operators.yaml not found at docs/operators.yaml"
    exit 1
fi

echo "1. Validating operator coverage..."
echo "-----------------------------------"

# Run coverage validation
if python docs/validate-coverage.py; then
    echo "✓ Operator coverage validation passed"
else
    echo "✗ Operator coverage validation failed"
    exit 1
fi

echo ""
echo "2. Testing all documentation examples..."
echo "-----------------------------------------"

# Run example tests
if python docs/test-examples.py; then
    echo "✓ All documentation examples passed"
else
    echo "✗ Some documentation examples failed"
    exit 1
fi

echo ""
echo "3. Summary"
echo "----------"
echo "✓ All validations passed!"
echo "  - Operator coverage is complete"
echo "  - All examples execute correctly"
echo "  - Documentation is accurate and up-to-date"

echo ""
echo "To add this to CI/CD, run: docs/validate-all.sh"
echo "To test specific examples with verbose output: python docs/test-examples.py -v"
echo "To check operator coverage: python docs/validate-coverage.py"
