#!/usr/bin/env python3
"""
Generate README.md from README.toml
Uses tomllib (Python 3.11+) to parse TOML and create markdown.
"""

import sys
import json
import subprocess
import tempfile
import os
from pathlib import Path

try:
    import tomllib
except ImportError:
    print("Error: tomllib not available. Need Python 3.11+ or install tomli")
    print("Try: pip install tomli")
    sys.exit(1)

def load_toml(file_path: str) -> dict:
    """Load TOML file."""
    with open(file_path, 'rb') as f:
        return tomllib.load(f)

def format_script(script) -> str:
    """Format a Computo script for markdown."""
    return json.dumps(script, indent=2)

def format_json(data) -> str:
    """Format JSON data for markdown."""
    return json.dumps(data, indent=2)

def run_debug_example(example: dict, project_root: Path) -> tuple[str, str]:
    """
    Run a debug example and capture both stdout (JSON) and stderr (debug output).
    Returns (json_result, debug_output) or (None, None) if failed.
    """
    # Check if we should skip running this example
    if example.get('dontrun', False):
        return None, "# Example marked with dontrun=true - execution skipped"
    
    # Check if manual output is provided
    if 'manualrun' in example:
        manual_output = example['manualrun'].strip()
        # Try to extract JSON result from manual output (last line that looks like JSON)
        lines = manual_output.split('\n')
        json_result = None
        for line in reversed(lines):
            line = line.strip()
            if line and (line.startswith('[') or line.startswith('{') or 
                        line.startswith('"') or line.isdigit() or 
                        line in ['true', 'false', 'null'] or
                        (line.startswith('-') and line[1:].isdigit())):
                try:
                    json.loads(line)  # Validate it's JSON
                    json_result = line
                    break
                except json.JSONDecodeError:
                    continue
        return json_result, manual_output
    
    computo_binary = project_root / "build" / "computo"
    if not computo_binary.exists():
        return None, None
    
    flags = example.get('flags', [])
    script = example['script']
    
    # Create temporary directory for files
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        
        # Write script file
        script_file = temp_path / "script.json"
        with open(script_file, 'w') as f:
            json.dump(script, f)
        
        # Build command
        cmd = [str(computo_binary)] + flags + [str(script_file)]
        
        # Handle input files
        if 'inputs' in example:
            # Multiple inputs
            for i, input_data in enumerate(example['inputs']):
                input_file = temp_path / f"input_{i}.json"
                with open(input_file, 'w') as f:
                    json.dump(input_data, f)
                cmd.append(str(input_file))
        else:
            # Single input
            input_data = example.get('input', {})
            input_file = temp_path / "input.json"
            with open(input_file, 'w') as f:
                json.dump(input_data, f)
            cmd.append(str(input_file))
        
        try:
            # Run command with separated stdout/stderr
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10,
                cwd=temp_path
            )
            
            if result.returncode == 0:
                return result.stdout.strip(), result.stderr.strip()
            else:
                # Command failed, return stderr as debug output
                return None, result.stderr.strip()
                
        except (subprocess.TimeoutExpired, subprocess.SubprocessError):
            return None, None

def generate_example_section(example: dict, project_root: Path = None) -> str:
    """Generate markdown for a single example."""
    name = example['name']
    description = example.get('description', '')
    script = example['script']
    expected = example['expected']
    
    # Input handling
    if 'inputs' in example:
        input_section = "**Multiple Inputs:**\n"
        for i, inp in enumerate(example['inputs']):
            input_section += f"\n*Input {i}:*\n```json\n{format_json(inp)}\n```\n"
    else:
        input_data = example.get('input', {})
        if input_data:
            input_section = f"**Input:**\n```json\n{format_json(input_data)}\n```\n\n"
        else:
            input_section = ""
    
    # Flags
    flags_section = ""
    if 'flags' in example:
        flags_section = f"**Flags:** `{' '.join(example['flags'])}`\n\n"
    
    # Debug output section (for debug examples)
    debug_section = ""
    flags = example.get('flags', [])
    has_debug_flags = any('--debug' in flag or '--trace' in flag or '--profile' in flag or 
                         '--interactive' in flag or '--break-on' in flag or '--watch' in flag or
                         '--slow-threshold' in flag or '--debug-level' in flag for flag in flags)
    has_manual_run = 'manualrun' in example
    
    if (has_debug_flags or has_manual_run) and project_root:
        json_result, debug_output = run_debug_example(example, project_root)
        if debug_output:
            # For manual runs, don't truncate since they're carefully crafted
            if 'manualrun' in example:
                debug_section = f"""**Debug Output:**
```
{debug_output}
```

"""
            else:
                # Truncate auto-generated debug output to first 15 lines for readability
                debug_lines = debug_output.split('\n')
                if len(debug_lines) > 15:
                    debug_lines = debug_lines[:15] + ['... (truncated for brevity)']
                formatted_debug = '\n'.join(debug_lines)
                
                debug_section = f"""**Debug Output:**
```
{formatted_debug}
```

"""
    
    return f"""### {name.replace('_', ' ').title()}

{description}

**Script:**
```json
{format_script(script)}
```

{input_section}{flags_section}{debug_section}**Expected Output:**
```json
{format_json(expected)}
```

"""

def generate_examples_by_category(examples: list, project_root: Path = None) -> str:
    """Group examples by category and generate sections."""
    categories = {}
    for example in examples:
        category = example.get('category', 'misc')
        if category not in categories:
            categories[category] = []
        categories[category].append(example)
    
    markdown = ""
    for category, category_examples in sorted(categories.items()):
        markdown += f"## {category.replace('-', ' ').title()} Examples\n\n"
        for example in category_examples:
            markdown += generate_example_section(example, project_root)
    
    return markdown

def generate_readme(toml_data: dict, project_root: Path = None) -> str:
    """Generate complete README.md content."""
    meta = toml_data.get('meta', {})
    intro = toml_data.get('intro', {})
    examples = toml_data.get('examples', [])
    
    # Format intro description with variables
    intro_text = intro.get('description', '')
    if 'human_docs_url' in meta:
        intro_text = intro_text.format(
            human_docs_url=meta.get('human_docs_url', '#'),
            human_docs_repo=meta.get('human_docs_repo', '#')
        )
    
    readme = f"""# {meta.get('title', 'Computo')}

{meta.get('subtitle', '')}

{intro_text}

## Quick Start

### Building
```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest --verbose
```

### CLI Usage
```bash
# Basic transformation (single input)
./build/computo script.json input.json

# Multiple input files
./build/computo script.json input1.json input2.json input3.json

# With flags
./build/computo --interpolation script.json input.json
```

{generate_examples_by_category(examples, project_root)}

## Running the Examples

Each example can be tested using the extracted test files:

```bash
# Generate test files from this documentation
python generate_examples.py

# Run all tests
cd examples && ./run_all.sh

# Run specific example
cd examples/arithmetic/basic_multiplication && ./test.sh
```

---
*This README.md was generated from README.toml - edit that file instead.*
"""
    
    return readme

def main():
    # Work from project root (parent directory)
    project_root = Path(__file__).parent.parent
    toml_file = project_root / "README.toml"
    readme_file = project_root / "README.md"
    
    if not toml_file.exists():
        print(f"Error: {toml_file} not found")
        sys.exit(1)
    
    try:
        toml_data = load_toml(toml_file)
        
        # Check for debug examples
        examples = toml_data.get('examples', [])
        debug_examples = [ex for ex in examples if any('--debug' in flag for flag in ex.get('flags', []))]
        
        if debug_examples:
            computo_binary = project_root / "build" / "computo"
            if computo_binary.exists():
                print(f"🔍 Found {len(debug_examples)} debug examples - capturing live debug output...")
            else:
                print(f"⚠️  Found {len(debug_examples)} debug examples but computo binary not found at {computo_binary}")
                print("   Debug output will not be included in documentation.")
        
        readme_content = generate_readme(toml_data, project_root)
        
        with open(readme_file, 'w', encoding='utf-8') as f:
            f.write(readme_content)
        
        print(f"✅ Generated {readme_file} from {toml_file}")
        print(f"📊 Included {len(examples)} examples")
        
        if debug_examples and computo_binary.exists():
            print(f"🔧 Debug examples include live output from computo binary")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()