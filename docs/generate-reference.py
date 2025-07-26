#!/usr/bin/env python3
"""
Generate Markdown documentation from YAML operator definitions
"""

import yaml
import json
import sys
from pathlib import Path

def escape_markdown(text):
    """Escape special markdown characters"""
    # Basic escaping for code blocks and special chars
    return text.replace('`', '\\`').replace('|', '\\|')

def format_example(example):
    """Format a single example as markdown"""
    name = example.get('name', 'Example')
    expression = example['expression']
    inputs = example.get('inputs')
    result = example.get('result')
    
    markdown = f"**{name}:**\n"
    markdown += f"```json\n{expression}\n```\n"
    
    if inputs:
        markdown += f"*Inputs:* {json.dumps(inputs)}\n"
    
    if result is not None:
        markdown += f"*Result:* `{json.dumps(result)}`\n"
    
    return markdown

def generate_operator_section(operator_name, operator_info):
    """Generate markdown section for a single operator"""
    description = operator_info.get('description', 'No description')
    syntax = operator_info.get('syntax', 'No syntax specified')
    examples = operator_info.get('examples', [])
    
    # Escape operator name if it contains special chars
    display_name = escape_markdown(operator_name)
    
    markdown = f"### `{display_name}` - {description}\n\n"
    markdown += f"**Syntax:** `{escape_markdown(syntax)}`\n\n"
    
    if examples:
        markdown += "**Examples:**\n\n"
        for example in examples:
            markdown += format_example(example) + "\n"
    
    return markdown

def generate_reference_documentation(yaml_file):
    """Generate complete reference documentation from YAML"""
    with open(yaml_file, 'r') as f:
        data = yaml.safe_load(f)
    
    operators = data['operators']
    
    # Group operators by category
    categories = {
        'Arithmetic': ['+', '-', '*', '/', '%'],
        'Comparison': ['>', '<', '>=', '<=', '==', '!='],
        'Logical': ['and', 'or', 'not'],
        'Data Access': ['$input', '$inputs', '$', 'let'],
        'Control Flow': ['if', 'lambda'],
        'Object Operations': ['obj', 'keys', 'values', 'objFromPairs', 'pick', 'omit', 'merge'],
        'Array Operations': ['map', 'filter', 'reduce', 'count', 'find', 'some', 'every'],
        'Functional Programming': ['car', 'cdr', 'cons', 'append'],
        'String Operations': ['strConcat', 'join'],
        'Array Manipulation': ['sort', 'reverse', 'unique', 'uniqueSorted', 'zip'],
        'Utilities': ['approx']
    }
    
    markdown = """# Computo Language Reference

This document provides complete specifications for all Computo operators, generated from validated examples.

## Operator Categories

"""
    
    for category, operator_list in categories.items():
        markdown += f"## {category} Operators\n\n"
        
        for op_name in operator_list:
            if op_name in operators:
                markdown += generate_operator_section(op_name, operators[op_name])
            else:
                print(f"Warning: Operator '{op_name}' not found in YAML", file=sys.stderr)
        
        markdown += "\n"
    
    # Add any operators not in categories
    documented_ops = set(operators.keys())
    categorized_ops = set()
    for ops in categories.values():
        categorized_ops.update(ops)
    
    uncategorized = documented_ops - categorized_ops
    if uncategorized:
        markdown += "## Other Operators\n\n"
        for op_name in sorted(uncategorized):
            markdown += generate_operator_section(op_name, operators[op_name])
    
    # Add footer
    markdown += f"""
---

*This documentation was automatically generated from `operators.yaml` and validated against the Computo engine.*

*Total operators documented: {len(operators)}*
"""
    
    return markdown

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate Markdown reference from YAML")
    parser.add_argument("yaml_file", nargs="?", default="docs/operators.yaml",
                       help="YAML file with operator definitions")
    parser.add_argument("-o", "--output", help="Output file (default: stdout)")
    
    args = parser.parse_args()
    
    try:
        markdown = generate_reference_documentation(args.yaml_file)
        
        if args.output:
            with open(args.output, 'w') as f:
                f.write(markdown)
            print(f"Documentation written to {args.output}")
        else:
            print(markdown)
            
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
