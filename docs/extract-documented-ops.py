#!/usr/bin/env python3
"""
Extract documented operators from YAML file
"""

import yaml
import json
import sys
from pathlib import Path

def extract_operators(yaml_file):
    """Extract sorted list of operators from YAML file"""
    with open(yaml_file, 'r') as f:
        data = yaml.safe_load(f)
    
    operators = list(data['operators'].keys())
    operators.sort()
    return operators

def main():
    if len(sys.argv) > 1:
        yaml_file = sys.argv[1]
    else:
        # Default to operators.yaml in same directory
        yaml_file = Path(__file__).parent / "operators.yaml"
    
    try:
        operators = extract_operators(yaml_file)
        print(json.dumps(operators))
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
