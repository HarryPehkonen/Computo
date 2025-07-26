# Computo Syntax Verification Process

This document describes the automated documentation validation system for Computo, which ensures that all operator documentation remains accurate and complete.

## Theory of Operation

### The Problem

Traditional documentation approaches suffer from several critical issues:
- **Documentation drift**: Examples become outdated when implementation changes
- **Missing coverage**: New operators may not be documented
- **Ghost features**: Documentation describes non-existent functionality
- **Untested examples**: Documented examples may not actually work

### The Solution

Computo uses a **YAML-based documentation system** with **automated validation** that:

1. **Defines operators in structured YAML** with examples and expected results
2. **Tests every example** against the real Computo engine
3. **Validates coverage** by comparing implemented vs documented operators
4. **Generates documentation** automatically from validated definitions
5. **Prevents documentation drift** through CI/CD integration

### Core Principles

- **Single source of truth**: YAML file contains all operator specifications
- **Executable documentation**: Every example is tested against real engine
- **Complete coverage**: System ensures all operators are documented
- **Multi-input support**: Full testing of complex scenarios with multiple input files
- **Self-validation**: Uses Computo itself for validation logic

## System Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Computo       │    │   operators.yaml │    │   Validation    │
│   Binary        │    │                  │    │   Scripts       │
│                 │    │  - Definitions   │    │                 │
│ --list-operators│───▶│  - Examples      │───▶│ - Coverage      │
│                 │    │  - Expected      │    │ - Testing       │
│                 │    │    Results       │    │ - Generation    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │                        │
                                ▼                        ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │  Input Files     │    │  Generated      │
                       │  (Multi-input    │    │  Documentation  │
                       │   scenarios)     │    │                 │
                       └──────────────────┘    └─────────────────┘
```

## Components

### 1. Computo CLI Enhancement

**File**: `src/main.cpp`, `src/cli_args.cpp`
**Purpose**: Added `--list-operators` switch to output JSON array of all implemented operators

```bash
./build/computo --list-operators
# Output: ["!=","$","$input","$inputs","+","-",...]
```

### 2. YAML Operator Definitions

**File**: `docs/operators.yaml`
**Purpose**: Single source of truth for all operator documentation

**Structure**:
```yaml
operators:
  "operator_name":
    description: "Human-readable description"
    syntax: 'Formal syntax specification'
    examples:
      - name: "Example description"
        expression: 'JSON expression as string'
        inputs: [input1_data, input2_data, ...]  # Optional for multi-input
        result: expected_result                   # Optional for validation
```

**Key Features**:
- **Multi-input support**: `inputs` array provides data for multiple input files
- **Expected results**: Automatic validation of actual vs expected output
- **Named examples**: Clear identification of test scenarios
- **Comprehensive coverage**: All 46 operators with 66+ examples

### 3. Validation Scripts

#### extract-documented-ops.py
**Purpose**: Extract sorted list of operators from YAML file
```bash
python docs/extract-documented-ops.py
# Output: ["!=", "$", "$input", ...]
```

#### validate-coverage.py
**Purpose**: Cross-validate implemented vs documented operators
```bash
python docs/validate-coverage.py
# Output: Coverage report with missing/extra operators
```

#### test-examples.py
**Purpose**: Test all YAML examples against real Computo engine
```bash
python docs/test-examples.py [-v]
# Tests all 66+ examples with multi-input support
```

#### generate-reference.py
**Purpose**: Generate Markdown documentation from YAML
```bash
python docs/generate-reference.py -o docs/LANGUAGE_REFERENCE.md
# Generates categorized operator reference
```

### 4. Workflow Orchestration

#### validate-all.sh
**Purpose**: Complete validation workflow
```bash
./docs/validate-all.sh
# Runs: coverage validation → example testing → reporting
```

## Multi-Input Testing

The system fully supports testing operators that require multiple input files:

**YAML Example**:
```yaml
- name: "Multi-input addition"
  expression: '["+", ["$inputs", "/0"], ["$inputs", "/1"]]'
  inputs: [10, 20]
  result: 30
```

**Test Process**:
1. Create temporary `input1.json` with `10`
2. Create temporary `input2.json` with `20`
3. Execute: `computo --script script.json input1.json input2.json`
4. Validate result equals `30`

## Validation Workflow

### 1. Coverage Validation
```bash
# Get implemented operators
./build/computo --list-operators > implemented.json

# Get documented operators  
python docs/extract-documented-ops.py > documented.json

# Cross-validate
python docs/validate-coverage.py
```

### 2. Example Testing
```bash
# Test all examples against real engine
python docs/test-examples.py

# Verbose output
python docs/test-examples.py -v
```

### 3. Documentation Generation
```bash
# Generate reference documentation
python docs/generate-reference.py -o docs/LANGUAGE_REFERENCE.md
```

## Adding New Operators

### Process
1. **Implement operator** in Computo source code
2. **Add to YAML**: Update `docs/operators.yaml` with operator specification
3. **Add examples**: Include comprehensive examples with expected results
4. **Validate**: Run `./docs/validate-all.sh` to ensure completeness
5. **Generate docs**: Update generated documentation

### Example Addition
```yaml
"new_operator":
  description: "Does something useful"
  syntax: '["new_operator", arg1, arg2]'
  examples:
    - name: "Basic usage"
      expression: '["new_operator", "hello", "world"]'
      result: "hello world"
    - name: "With input data"
      expression: '["new_operator", ["$input", "/field"], "suffix"]'
      inputs: [{"field": "prefix"}]
      result: "prefix suffix"
```

## CI/CD Integration

### Recommended Workflow
```yaml
# .github/workflows/docs.yml
- name: Validate Documentation
  run: |
    cmake --build build
    ./docs/validate-all.sh
    
- name: Update Documentation
  run: |
    python docs/generate-reference.py -o docs/LANGUAGE_REFERENCE.md
    git add docs/LANGUAGE_REFERENCE.md
    git commit -m "Update generated documentation"
```

### Make Targets
```makefile
docs: docs-gather docs-coverage docs-validate docs-generate

docs-gather:
	./build/computo --list-operators > docs/implemented-operators.json

docs-coverage:
	python docs/validate-coverage.py

docs-validate:
	python docs/test-examples.py

docs-generate:
	python docs/generate-reference.py -o docs/LANGUAGE_REFERENCE.md

docs-clean:
	rm -f docs/implemented-operators.json docs/LANGUAGE_REFERENCE.md
```

## Benefits

### For Developers
- **Impossible to have undocumented operators**: Coverage validation catches them
- **Impossible to have broken examples**: All examples tested against engine
- **Easy maintenance**: Human-readable YAML format
- **Comprehensive testing**: Multi-input scenarios fully supported

### For Users
- **Accurate documentation**: All examples guaranteed to work
- **Complete coverage**: Every operator documented
- **Clear examples**: Named scenarios with expected results
- **Up-to-date information**: Automatically generated from validated sources

### For Project
- **Documentation as code**: YAML definitions version-controlled
- **Automated validation**: CI/CD prevents documentation drift
- **Self-documenting**: System demonstrates Computo's capabilities
- **Maintainable**: Simple tools, clear separation of concerns

## Error Detection

The system catches these common documentation problems:

### Missing Documentation
```
Operator Coverage Report
========================
Missing documentation for 2 operators:
  - new_feature
  - experimental_op
```

### Failed Examples
```
✗ 42. Advanced usage
    Expression: ["broken_op", "invalid", "args"]
    Expected: "success"
    Error: Unknown operator: broken_op
```

### Incorrect Results
```
✗ 15. Mathematical operation
    Expected: 42
    Got: 24
    Error: Expected 42, got 24
```

## Troubleshooting

### Common Issues

**Coverage validation fails**: New operators implemented but not documented
- Solution: Add operator to `docs/operators.yaml`

**Example tests fail**: Implementation changed but YAML not updated
- Solution: Update expected results in YAML

**Multi-input test fails**: Incorrect input format
- Solution: Verify `inputs` array format matches expected JSON

**Documentation generation fails**: Invalid YAML syntax
- Solution: Validate YAML with `python -c "import yaml; yaml.safe_load(open('docs/operators.yaml'))"`

## Future Enhancements

- **Performance testing**: Benchmark examples for performance regression
- **Fuzz testing**: Generate random valid expressions for testing
- **Schema validation**: JSON Schema for YAML structure validation
- **Interactive docs**: Generate interactive documentation with executable examples
- **API documentation**: Generate API documentation for C++ library

---

This documentation validation system ensures that Computo's documentation remains accurate, complete, and useful for all users while minimizing maintenance overhead for developers.
