#!/usr/bin/env python3
"""
Validate operator coverage between implementation and documentation
"""

import json
import subprocess
import sys
from pathlib import Path

def get_implemented_operators(computo_binary="./build/computo"):
    """Get list of implemented operators from Computo binary"""
    try:
        result = subprocess.run([computo_binary, "--list-operators"], 
                              capture_output=True, text=True, check=True)
        return json.loads(result.stdout.strip())
    except (subprocess.CalledProcessError, json.JSONDecodeError, FileNotFoundError) as e:
        raise RuntimeError(f"Failed to get implemented operators: {e}")

def get_documented_operators(yaml_file="docs/operators.yaml"):
    """Get list of documented operators from YAML file"""
    extract_script = Path(__file__).parent / "extract-documented-ops.py"
    try:
        result = subprocess.run(["python", str(extract_script), yaml_file], 
                              capture_output=True, text=True, check=True)
        return json.loads(result.stdout.strip())
    except (subprocess.CalledProcessError, json.JSONDecodeError) as e:
        raise RuntimeError(f"Failed to get documented operators: {e}")

def validate_coverage(implemented, documented):
    """Check for missing documentation and extra documentation"""
    implemented_set = set(implemented)
    documented_set = set(documented)
    
    missing_docs = sorted(implemented_set - documented_set)
    extra_docs = sorted(documented_set - implemented_set)
    
    return {
        "total_implemented": len(implemented),
        "total_documented": len(documented),
        "missing_docs": missing_docs,
        "extra_docs": extra_docs,
        "coverage_complete": len(missing_docs) == 0 and len(extra_docs) == 0
    }

def print_report(coverage):
    """Print human-readable coverage report"""
    print("Operator Coverage Report")
    print("=" * 50)
    print(f"Implemented operators: {coverage['total_implemented']}")
    print(f"Documented operators:  {coverage['total_documented']}")
    
    if coverage['coverage_complete']:
        print("✓ Coverage is COMPLETE - all operators documented!")
    else:
        print("✗ Coverage is INCOMPLETE")
        
        if coverage['missing_docs']:
            print(f"\nMissing documentation for {len(coverage['missing_docs'])} operators:")
            for op in coverage['missing_docs']:
                print(f"  - {op}")
        
        if coverage['extra_docs']:
            print(f"\nExtra documentation for {len(coverage['extra_docs'])} operators:")
            for op in coverage['extra_docs']:
                print(f"  - {op}")
    
    print("=" * 50)
    return coverage['coverage_complete']

def main():
    try:
        # Get operator lists
        implemented = get_implemented_operators()
        documented = get_documented_operators()
        
        # Validate coverage
        coverage = validate_coverage(implemented, documented)
        
        # Print report
        success = print_report(coverage)
        
        # Exit with appropriate code
        sys.exit(0 if success else 1)
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
