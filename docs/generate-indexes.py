#!/usr/bin/env python3
"""
Generate alphabetical and task-based operator indexes for GitHub Pages
"""

import yaml
import json
import sys
from pathlib import Path

def load_operators():
    """Load operators from operators.yaml"""
    yaml_path = Path(__file__).parent / "operators.yaml"
    with open(yaml_path, 'r') as f:
        return yaml.safe_load(f)['operators']

def get_operator_categories():
    """Define operator categories for task-based index"""
    return {
        'Arithmetic': ['+', '-', '*', '/', '%'],
        'Comparison': ['>', '<', '>=', '<=', '==', '!='],
        'Logical': ['and', 'or', 'not'],
        'Data Access': ['$input', '$inputs', '$'],
        'Control Flow': ['if', 'let'],
        'Lambda Functions': ['lambda'],
        'Array Operations': ['map', 'filter', 'reduce', 'count', 'find', 'some', 'every', 'append', 'sort', 'reverse', 'unique', 'uniqueSorted', 'zip'],
        'Functional Programming': ['car', 'cdr', 'cons'],
        'Object Operations': ['obj', 'keys', 'values', 'objFromPairs', 'pick', 'omit', 'merge'],
        'String Operations': ['join', 'strConcat'],
        'Utility': ['approx']
    }

def generate_alphabetical_index(operators):
    """Generate alphabetical index markdown"""
    # Sort operators alphabetically, handling special characters
    def sort_key(op_name):
        # Move symbols to front, then alphabetical
        if op_name.startswith(('$', '!', '%', '*', '+', '-', '/', '<', '=', '>', '&', '|')):
            return ('0', op_name)  # Sort symbols first
        return ('1', op_name.lower())  # Then alphabetical
    
    sorted_ops = sorted(operators.keys(), key=sort_key)
    
    content = [
        "---",
        "title: Alphabetical Operator Index",
        "---",
        "",
        "# Alphabetical Operator Index",
        "",
        "Complete list of all Computo operators in alphabetical order:",
        ""
    ]
    
    for op_name in sorted_ops:
        op_data = operators[op_name]
        description = op_data.get('description', f'{op_name} operator')
        # Create a URL-friendly anchor
        anchor = op_name.lower().replace('$', 'dollar-').replace('!', 'not-').replace('=', 'equal').replace('<', 'less').replace('>', 'greater').replace('&', 'and').replace('|', 'or').replace('%', 'mod').replace('*', 'mul').replace('+', 'add').replace('-', 'sub').replace('/', 'div')
        content.append(f"- [`{op_name}`](../LANGUAGE_REFERENCE.md#{anchor}) - {description}")
    
    content.extend(["", "---", f"Total: {len(operators)} operators"])
    
    return '\n'.join(content)

def generate_task_based_index(operators):
    """Generate task-based index markdown"""
    categories = get_operator_categories()
    
    content = [
        "---",
        "title: Task-Based Operator Index",
        "---",
        "",
        "# Task-Based Operator Index",
        "",
        "Operators organized by common use cases and functionality:",
        ""
    ]
    
    for category, op_list in categories.items():
        content.append(f"## {category}")
        content.append("")
        
        for op_name in op_list:
            if op_name in operators:
                op_data = operators[op_name]
                description = op_data.get('description', f'{op_name} operator')
                anchor = op_name.lower().replace('$', 'dollar-').replace('!', 'not-').replace('=', 'equal').replace('<', 'less').replace('>', 'greater').replace('&', 'and').replace('|', 'or').replace('%', 'mod').replace('*', 'mul').replace('+', 'add').replace('-', 'sub').replace('/', 'div')
                content.append(f"- [`{op_name}`](../LANGUAGE_REFERENCE.md#{anchor}) - {description}")
        
        content.append("")
    
    # Add any operators not in categories
    categorized = set()
    for op_list in categories.values():
        categorized.update(op_list)
    
    uncategorized = set(operators.keys()) - categorized
    if uncategorized:
        content.append("## Other")
        content.append("")
        for op_name in sorted(uncategorized):
            op_data = operators[op_name]
            description = op_data.get('description', f'{op_name} operator')
            anchor = op_name.lower().replace('$', 'dollar-').replace('!', 'not-').replace('=', 'equal').replace('<', 'less').replace('>', 'greater').replace('&', 'and').replace('|', 'or').replace('%', 'mod').replace('*', 'mul').replace('+', 'add').replace('-', 'sub').replace('/', 'div')
            content.append(f"- [`{op_name}`](../LANGUAGE_REFERENCE.md#{anchor}) - {description}")
    
    return '\n'.join(content)

def main():
    """Generate both indexes"""
    try:
        operators = load_operators()
        
        # Generate alphabetical index
        alpha_content = generate_alphabetical_index(operators)
        alpha_path = Path("docs/alpha/index.md")
        alpha_path.parent.mkdir(exist_ok=True)
        with open(alpha_path, 'w') as f:
            f.write(alpha_content)
        
        # Generate task-based index
        task_content = generate_task_based_index(operators)
        task_path = Path("docs/task/index.md")
        task_path.parent.mkdir(exist_ok=True)
        with open(task_path, 'w') as f:
            f.write(task_content)
        
        print(f"Generated alphabetical index: {alpha_path}")
        print(f"Generated task-based index: {task_path}")
        print(f"Total operators: {len(operators)}")
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()