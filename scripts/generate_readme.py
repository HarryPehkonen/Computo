#!/usr/bin/env python3
"""
Generate README.md from README.toml
Uses tomllib (Python 3.11+) to parse TOML and create markdown.
"""

import sys
import json
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

def generate_example_section(example: dict) -> str:
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
    
    return f"""### {name.replace('_', ' ').title()}

{description}

**Script:**
```json
{format_script(script)}
```

{input_section}{flags_section}**Expected Output:**
```json
{format_json(expected)}
```

"""

def generate_examples_by_category(examples: list) -> str:
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
            markdown += generate_example_section(example)
    
    return markdown

def generate_readme(toml_data: dict) -> str:
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

{generate_examples_by_category(examples)}

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
        readme_content = generate_readme(toml_data)
        
        with open(readme_file, 'w', encoding='utf-8') as f:
            f.write(readme_content)
        
        print(f"✅ Generated {readme_file} from {toml_file}")
        print(f"📊 Included {len(toml_data.get('examples', []))} examples")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()