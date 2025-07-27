#!/usr/bin/env python3
"""
Test all YAML examples against Computo engine with multi-input support
"""

import yaml
import json
import subprocess
import tempfile
import os
import sys
from pathlib import Path
from typing import Dict, List, Any, Optional

class ComputoTester:
    def __init__(self, computo_binary="./build/computo"):
        self.computo_binary = Path(computo_binary)
        if not self.computo_binary.exists():
            raise FileNotFoundError(f"Computo binary not found: {computo_binary}")
    
    def test_example(self, expression: Any, inputs: Optional[List[Any]] = None, 
                    expected_result: Optional[Any] = None) -> Dict[str, Any]:
        """Test a single example with optional multi-input support"""
        
        # Create temporary files
        temp_files = []
        
        try:
            # Create script file
            script_file = tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False)
            json.dump(expression, script_file)
            script_file.close()
            temp_files.append(script_file.name)
            
            # Build command
            cmd = [str(self.computo_binary), "--script", script_file.name]
            
            # Create input files if provided
            if inputs is not None:
                for i, input_data in enumerate(inputs):
                    input_file = tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False)
                    json.dump(input_data, input_file)
                    input_file.close()
                    temp_files.append(input_file.name)
                    cmd.append(input_file.name)
            
            # Execute Computo
            try:
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
                
                if result.returncode == 0:
                    try:
                        actual_result = json.loads(result.stdout.strip())
                        success = True
                        error = None
                        
                        # Check expected result if provided
                        if expected_result is not None:
                            # Use strict JSON serialization comparison to avoid type coercion
                            # This ensures 6.0 and 6 are treated as different (which they should be for documentation accuracy)
                            actual_json = json.dumps(actual_result, sort_keys=True)
                            expected_json = json.dumps(expected_result, sort_keys=True)
                            if actual_json != expected_json:
                                success = False
                                error = f"Expected {expected_json}, got {actual_json}"
                    except json.JSONDecodeError:
                        success = False
                        error = f"Invalid JSON output: {result.stdout}"
                        actual_result = None
                else:
                    success = False
                    error = result.stderr.strip()
                    actual_result = None
                    
            except subprocess.TimeoutExpired:
                success = False
                error = "Execution timeout"
                actual_result = None
                
        except Exception as e:
            success = False
            error = f"Setup error: {e}"
            actual_result = None
            
        finally:
            # Cleanup temporary files
            for temp_file in temp_files:
                try:
                    os.unlink(temp_file)
                except OSError:
                    pass
        
        return {
            'success': success,
            'result': actual_result,
            'error': error
        }

def load_operator_examples(yaml_file: str) -> Dict[str, Any]:
    """Load operator examples from YAML file"""
    with open(yaml_file, 'r') as f:
        return yaml.safe_load(f)

def test_all_examples(yaml_file: str = "docs/operators.yaml", verbose: bool = False) -> bool:
    """Test all examples in the YAML file"""
    
    print("Testing Computo Documentation Examples")
    print("=" * 60)
    
    try:
        data = load_operator_examples(yaml_file)
        tester = ComputoTester()
        
        total_tests = 0
        passed_tests = 0
        failed_tests = 0
        
        for operator_name, operator_info in data['operators'].items():
            examples = operator_info.get('examples', [])
            
            print(f"\n{operator_name} ({len(examples)} examples):")
            
            for i, example in enumerate(examples, 1):
                total_tests += 1
                
                name = example.get('name', f'Example {i}')
                expression = json.loads(example['expression'])
                inputs = example.get('inputs')
                expected = example.get('result')
                
                result = tester.test_example(expression, inputs, expected)
                
                if result['success']:
                    status = "✓"
                    passed_tests += 1
                    if verbose:
                        print(f"  {status} {i:2d}. {name}")
                        if result['result'] is not None:
                            print(f"      Result: {json.dumps(result['result'])}")
                else:
                    status = "✗"
                    failed_tests += 1
                    print(f"  {status} {i:2d}. {name}")
                    print(f"      Expression: {example['expression']}")
                    if inputs:
                        print(f"      Inputs: {json.dumps(inputs)}")
                    if expected is not None:
                        print(f"      Expected: {json.dumps(expected)}")
                    print(f"      Error: {result['error']}")
                
                if not verbose and result['success']:
                    print(f"  {status} {i:2d}. {name}")
        
        print("=" * 60)
        print(f"Results: {passed_tests} passed, {failed_tests} failed, {total_tests} total")
        
        if failed_tests == 0:
            print("  All examples passed!")
        else:
            print(f"  {failed_tests} examples failed")
        
        return failed_tests == 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return False

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Test Computo documentation examples")
    parser.add_argument("yaml_file", nargs="?", default="docs/operators.yaml",
                       help="YAML file with operator definitions")
    parser.add_argument("-v", "--verbose", action="store_true",
                       help="Verbose output (show all test results)")
    
    args = parser.parse_args()
    
    success = test_all_examples(args.yaml_file, args.verbose)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
