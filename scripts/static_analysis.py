#!/usr/bin/env python3
"""
Static Analysis for Bootloader

Runs various static analysis tools on the codebase.
"""

import subprocess
import os
import re

def run_cppcheck():
    """Run cppcheck static analysis"""
    print("Running cppcheck...")
    result = subprocess.run([
        'cppcheck',
        '--enable=all',
        '--std=c99',
        '--suppress=missingIncludeSystem',
        '--suppress=unusedFunction',
        '--inline-suppr',
        'main.c', 'uart.c', 'sd.c', 'config.c'
    ], capture_output=True, text=True)

    if result.returncode == 0:
        print("✓ cppcheck passed")
        return True
    else:
        print("✗ cppcheck found issues:")
        print(result.stdout)
        print(result.stderr)
        return False

def check_code_complexity():
    """Check code complexity metrics"""
    print("Checking code complexity...")

    issues = []

    for filename in ['main.c', 'uart.c', 'sd.c', 'config.c']:
        if not os.path.exists(filename):
            continue

        with open(filename, 'r') as f:
            lines = f.readlines()

        # Count lines
        total_lines = len(lines)

        # Count functions
        functions = len(re.findall(r'^\w+\s+\w+\s*\(', '\n'.join(lines), re.MULTILINE))

        # Check for long functions
        in_function = False
        func_lines = 0
        for line in lines:
            if re.match(r'^\w+\s+\w+\s*\(', line.strip()):
                in_function = True
                func_lines = 0
            elif in_function and line.strip() == '}':
                if func_lines > 50:
                    issues.append(f"Function too long in {filename}: {func_lines} lines")
                in_function = False
            elif in_function:
                func_lines += 1

        print(f"  {filename}: {total_lines} lines, {functions} functions")

    if not issues:
        print("✓ Code complexity OK")
        return True
    else:
        print("✗ Code complexity issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def check_naming_conventions():
    """Check naming conventions"""
    print("Checking naming conventions...")

    issues = []

    for filename in ['main.c', 'uart.c', 'sd.c', 'config.c']:
        if not os.path.exists(filename):
            continue

        with open(filename, 'r') as f:
            content = f.read()

        # Check for non-snake_case functions
        functions = re.findall(r'^\w+\s+(\w+)\s*\(', content, re.MULTILINE)
        for func in functions:
            if not re.match(r'^[a-z][a-z0-9_]*$', func):
                issues.append(f"Function {func} in {filename} not in snake_case")

    if not issues:
        print("✓ Naming conventions OK")
        return True
    else:
        print("✗ Naming convention issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def check_documentation_accuracy():
    """Check if documentation matches implementation"""
    print("Checking documentation accuracy...")

    issues = []

    # Check FSA state count in documentation vs code
    if os.path.exists('../BOOT_PROCESS.md'):
        with open('../BOOT_PROCESS.md', 'r') as f:
            doc_content = f.read()

        # Count states in documentation
        doc_states = len(re.findall(r'\| STATE_\w+ \|', doc_content))

        # Count states in fsa_monitor.h
        if os.path.exists('fsa_monitor.h'):
            with open('fsa_monitor.h', 'r') as f:
                header_content = f.read()
            code_states = len(re.findall(r'\s+STATE_\w+,', header_content))

            if doc_states != code_states:
                issues.append(f"FSA state count mismatch: docs have {doc_states}, code has {code_states}")

    # Check if documented functions exist in code
    documented_functions = [
        'uart_init', 'gpio_set_function', 'timer_init', 'verification_init'
    ]

    for func in documented_functions:
        found = False
        for filename in ['main.c', 'uart.c', 'gpio.c', 'verification.c']:
            if os.path.exists(filename):
                with open(filename, 'r') as f:
                    if func in f.read():
                        found = True
                        break
        if not found:
            issues.append(f"Documented function {func} not found in codebase")

    if not issues:
        print("✓ Documentation accuracy OK")
        return True
    else:
        print("✗ Documentation accuracy issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def run_static_analysis():
    """Run all static analysis"""
    print("=== Static Analysis Report ===\n")

    tests = [
        run_cppcheck,
        check_code_complexity,
        check_naming_conventions,
        check_documentation_accuracy
    ]

    passed = 0
    for test in tests:
        if test():
            passed += 1
        print()

    print(f"Static analysis: {passed}/{len(tests)} checks passed")

    return passed == len(tests)

if __name__ == "__main__":
    run_static_analysis()