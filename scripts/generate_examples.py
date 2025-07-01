#!/usr/bin/env python3
"""
Generate test examples from README.toml
Creates organized test directories with all necessary files.
"""

import sys
import json
import os
from pathlib import Path

try:
    import tomllib
except ImportError:
    print("Error: tomllib not available. Need Python 3.11+ or install tomli")
    print("Try: pip install tomli")
    sys.exit(1)

def load_toml(file_path: str) -> dict:
    """Load TOML file with line number mapping."""
    # First load the TOML data
    with open(file_path, 'rb') as f:
        data = tomllib.load(f)
    
    # Now create line number mapping by reading the text
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Create a mapping of example names to line numbers
    line_map = {}
    current_line = 0
    in_example = False
    current_name = None
    
    for i, line in enumerate(lines, 1):
        line = line.strip()
        if line.startswith('[[examples]]'):
            in_example = True
            current_line = i
        elif in_example and line.startswith('name = '):
            # Extract name from: name = "example_name"
            name_part = line.split('=', 1)[1].strip()
            current_name = name_part.strip('"\'')
            line_map[current_name] = current_line
            in_example = False
    
    # Add line mapping to data
    data['_line_map'] = line_map
    return data

def sanitize_name(name: str) -> str:
    """Sanitize name for filesystem use."""
    # Replace invalid characters
    safe_name = name.replace(' ', '_').replace('-', '_')
    safe_name = ''.join(c for c in safe_name if c.isalnum() or c in '_')
    return safe_name.lower()

def create_example_directory(example: dict, base_path: Path) -> Path:
    """Create directory structure for an example."""
    category = sanitize_name(example.get('category', 'misc'))
    name = sanitize_name(example['name'])
    
    example_path = base_path / category / name
    example_path.mkdir(parents=True, exist_ok=True)
    
    return example_path

def write_json_file(file_path: Path, data, compact: bool = False):
    """Write JSON file with proper formatting."""
    with open(file_path, 'w', encoding='utf-8') as f:
        if compact:
            json.dump(data, f, separators=(',', ':'))
        else:
            json.dump(data, f, indent=2)

def generate_test_script(example: dict, example_path: Path, example_index: int, line_number: int = None) -> str:
    """Generate test script for an example."""
    name = example['name']
    category = example.get('category', 'misc')
    description = example.get('description', '').strip()
    flags = example.get('flags', [])
    has_expected = 'expected' in example
    multiple_inputs = 'inputs' in example
    
    # Build command
    cmd_parts = ['../../../build/computo']
    cmd_parts.extend(flags)
    cmd_parts.append('script.json')
    
    if multiple_inputs:
        num_inputs = len(example['inputs'])
        cmd_parts.extend([f'input_{i}.json' for i in range(num_inputs)])
    else:
        cmd_parts.append('input.json')
    
    command = ' '.join(f'"{part}"' if ' ' in part else part for part in cmd_parts)
    
    line_info = f" (line {line_number})" if line_number else ""
    script = f'''#!/bin/bash
# Test script for {name}
# Category: {category}
# Generated from README.toml (example #{example_index}){line_info}
# Source: README.toml examples[{example_index}] {f"line {line_number}" if line_number else ""}

set -e

# Colors for output
RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
BLUE='\\033[0;34m'
NC='\\033[0m' # No Color

# Build path
COMPUTO="../../../build/computo"

if [ ! -f "$COMPUTO" ]; then
    echo -e "${{RED}}Error: computo binary not found at $COMPUTO${{NC}}"
    echo "Make sure to build the project first:"
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=Debug"
    echo "  cmake --build build"
    exit 1
fi

echo -e "${{BLUE}}Testing {name} (README.toml example #{example_index}{f' line {line_number}' if line_number else ''})...${{NC}}"
echo -e "Category: {category}"
echo -e "Description: {description.split('.')[0] if description else 'No description'}"
echo -e "Source: README.toml examples[{example_index}]{f' line {line_number}' if line_number else ''}"

echo -e "${{YELLOW}}Running:${{NC}} {command}"

# Run the transformation
RESULT=$({command} 2>&1)
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo -e "${{RED}}✗ FAIL: Command failed with exit code $EXIT_CODE${{NC}}"
    echo "Error output: $RESULT"
    exit 1
fi

echo -e "${{YELLOW}}Result:${{NC}} $RESULT"
'''

    if has_expected:
        script += f'''
# Validate against expected output
if [ -f expected.json ]; then
    EXPECTED=$(cat expected.json)
    
    # Use jq for better JSON comparison if available
    if command -v jq >/dev/null 2>&1; then
        RESULT_NORMALIZED=$(echo "$RESULT" | jq -S -c .)
        EXPECTED_NORMALIZED=$(echo "$EXPECTED" | jq -S -c .)
        
        if [ "$RESULT_NORMALIZED" = "$EXPECTED_NORMALIZED" ]; then
            echo -e "${{GREEN}}✓ PASS: Output matches expected result${{NC}}"
            exit 0
        else
            echo -e "${{RED}}✗ FAIL: Output does not match expected result${{NC}}"
            echo -e "${{YELLOW}}Expected:${{NC}} $EXPECTED_NORMALIZED"
            echo -e "${{YELLOW}}Got:${{NC}}      $RESULT_NORMALIZED"
            exit 1
        fi
    else
        # Fallback to string comparison
        if [ "$RESULT" = "$EXPECTED" ]; then
            echo -e "${{GREEN}}✓ PASS: Output matches expected result${{NC}}"
            exit 0
        else
            echo -e "${{RED}}✗ FAIL: Output does not match expected result${{NC}}"
            echo -e "${{YELLOW}}Expected:${{NC}} $EXPECTED"
            echo -e "${{YELLOW}}Got:${{NC}}      $RESULT"
            echo -e "${{BLUE}}Note: Install jq for better JSON comparison${{NC}}"
            exit 1
        fi
    fi
else
    echo -e "${{YELLOW}}⚠️  No expected output file found${{NC}}"
fi
'''
    else:
        script += '''
echo -e "${YELLOW}ℹ️  No expected output defined for this example${NC}"
'''

    script += '''
echo -e "${GREEN}✓ Script executed successfully${NC}"
'''
    
    return script

def create_example_readme(example: dict) -> str:
    """Create README for individual example."""
    name = example['name']
    category = example.get('category', 'misc')
    description = example.get('description', '')
    flags = example.get('flags', [])
    
    readme = f'''# {name.replace('_', ' ').title()}

**Category:** {category}

## Description

{description}

## Files

- `script.json` - The Computo transformation script
- `input.json` - Input data (or multiple `input_N.json` files)
- `expected.json` - Expected output result
- `test.sh` - Executable test script
- `README.md` - This documentation

## Usage

```bash
# Run the test
./test.sh

# Or run manually
'''
    
    if flags:
        readme += f'''../../../build/computo {' '.join(flags)} script.json input.json
'''
    else:
        readme += '''../../../build/computo script.json input.json
'''
    
    readme += '''```

## Script Content

```json
'''
    # Handle both direct JSON and string formats for README display
    script_for_display = example['script']
    if isinstance(script_for_display, str):
        # Parse string as JSON (for triple-quoted multiline scripts)
        try:
            script_for_display = json.loads(script_for_display)
        except json.JSONDecodeError:
            # If parsing fails, display as-is (shouldn't happen with valid examples)
            pass
    readme += json.dumps(script_for_display, indent=2)
    readme += '''
```
'''
    
    if 'expected' in example:
        readme += '''
## Expected Output

```json
'''
        readme += json.dumps(example['expected'], indent=2)
        readme += '''
```
'''
    
    return readme

def generate_master_test_runner(examples: list, base_path: Path, line_map: dict = None):
    """Generate master test runner script."""
    # Group by category with indices
    categories = {}
    for example_index, example in enumerate(examples):
        category = sanitize_name(example.get('category', 'misc'))
        if category not in categories:
            categories[category] = []
        categories[category].append((example_index, example))
    
    script = '''#!/bin/bash
# Master test runner for all Computo examples
# Generated from README.toml

set -e

SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"

# Colors for output
RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
BLUE='\\033[0;34m'
NC='\\033[0m' # No Color

# Counters
TOTAL=0
PASSED=0
FAILED=0

echo -e "${BLUE}Running all Computo examples...${NC}"
echo "===================================="

# Function to run a single test
run_test() {
    local category="$1"
    local example="$2"
    local test_path="$3"
    local toml_ref="$4"
    
    TOTAL=$((TOTAL + 1))
    echo -n "  ${example} (${toml_ref})... "
    
    cd "$test_path"
    if timeout 30s ./test.sh > test_output.log 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
        echo "    📄 Source: ${toml_ref}"
        echo "    📋 Error log: $test_path/test_output.log"
        # Show first few lines of error for quick debugging
        echo "    🔍 Error preview:"
        head -3 test_output.log | sed 's/^/       /'
    fi
    cd - > /dev/null
}

'''
    
    for category, category_examples in sorted(categories.items()):
        script += f'''
echo
echo -e "${{YELLOW}}Testing {category} examples...({len(category_examples)} examples)${{NC}}"
echo "{'='*50}"

'''
        for example_index, example in category_examples:
            example_name = sanitize_name(example['name'])
            line_number = line_map.get(example['name']) if line_map else None
            line_ref = f" line {line_number}" if line_number else ""
            script += f'''run_test "{category}" "{example_name}" "{category}/{example_name}" "README.toml[{example_index}]{line_ref}"
'''
    
    script += '''
echo
echo "===================================="
echo -e "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}, $TOTAL total"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed.${NC}"
    echo "Check individual test logs for details."
    exit 1
fi
'''
    
    runner_path = base_path / "run_all.sh"
    with open(runner_path, 'w') as f:
        f.write(script)
    os.chmod(runner_path, 0o755)

def create_examples_from_toml(toml_data: dict, output_dir: str = "examples"):
    """Create all example files from TOML data."""
    examples = toml_data.get('examples', [])
    base_path = Path(output_dir)
    
    # Clean and recreate directory
    if base_path.exists():
        import shutil
        shutil.rmtree(base_path)
    base_path.mkdir(exist_ok=True)
    
    print(f"📁 Creating {len(examples)} examples in {output_dir}/")
    
    for example_index, example in enumerate(examples):
        example_path = create_example_directory(example, base_path)
        name = sanitize_name(example['name'])
        
        # Write script.json - handle both direct JSON and string formats
        script_data = example['script']
        if isinstance(script_data, str):
            # Parse string as JSON (for triple-quoted multiline scripts)
            try:
                script_data = json.loads(script_data)
            except json.JSONDecodeError as e:
                raise ValueError(f"Failed to parse script as JSON in example '{example['name']}': {e}")
        write_json_file(example_path / "script.json", script_data)
        
        # Write input files
        if 'inputs' in example:
            # Multiple input files
            for i, input_data in enumerate(example['inputs']):
                write_json_file(example_path / f"input_{i}.json", input_data)
        else:
            # Single input file
            input_data = example.get('input', {})
            write_json_file(example_path / "input.json", input_data)
        
        # Write expected.json if available
        if 'expected' in example:
            write_json_file(example_path / "expected.json", example['expected'])
        
        # Write test script
        line_map = toml_data.get('_line_map', {})
        line_number = line_map.get(example['name'])
        test_script = generate_test_script(example, example_path, example_index, line_number)
        test_path = example_path / "test.sh"
        with open(test_path, 'w') as f:
            f.write(test_script)
        os.chmod(test_path, 0o755)
        
        # Write example README
        example_readme = create_example_readme(example)
        with open(example_path / "README.md", 'w') as f:
            f.write(example_readme)
        
        print(f"  ✅ {example.get('category', 'misc')}/{name} (example #{example_index})")
    
    # Generate master test runner
    line_map = toml_data.get('_line_map', {})
    generate_master_test_runner(examples, base_path, line_map)
    print(f"  ✅ run_all.sh")

def main():
    # Work from project root (parent directory)
    project_root = Path(__file__).parent.parent
    toml_file = project_root / "README.toml"
    
    if not toml_file.exists():
        print(f"❌ Error: {toml_file} not found")
        sys.exit(1)
    
    try:
        toml_data = load_toml(str(toml_file))
        # Change to project root for output
        os.chdir(project_root)
        create_examples_from_toml(toml_data)
        
        examples_count = len(toml_data.get('examples', []))
        categories = set(ex.get('category', 'misc') for ex in toml_data.get('examples', []))
        
        print(f"")
        print(f"🎉 Successfully created {examples_count} examples!")
        print(f"📂 Categories: {', '.join(sorted(categories))}")
        print(f"")
        print(f"🚀 To run all tests:")
        print(f"   cd examples && ./run_all.sh")
        print(f"")
        print(f"🔍 To run specific example:")
        print(f"   cd examples/category/example_name && ./test.sh")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()