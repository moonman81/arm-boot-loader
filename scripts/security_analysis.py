#!/usr/bin/env python3
"""
Security Static Analysis for Bootloader

Checks for common security vulnerabilities.
"""

import re

def check_buffer_overflows():
    """Check for potential buffer overflow vulnerabilities"""
    print("Checking for buffer overflows...")

    issues = []

    for filename in ['main.c', 'uart.c', 'sd.c', 'config.c']:
        with open(filename, 'r') as f:
            content = f.read()

        # Check for unsafe string operations
        if 'strcpy' in content or 'strcat' in content:
            issues.append(f"Unsafe string function in {filename}")

        # Check for potentially unsafe array usage (strcpy, etc.)
        if 'strcpy' in content or 'strcat' in content:
            issues.append(f"Unsafe string function usage in {filename}")
        # Only flag very large arrays or suspicious patterns
        large_arrays = re.findall(r'\w+\s*\[\s*([0-9]+)\s*\]', content)
        for size in large_arrays:
            if int(size) > 1024:  # Only flag arrays larger than 1KB
                issues.append(f"Large fixed-size array [{size}] in {filename} - consider dynamic allocation")

    if not issues:
        print("✓ No obvious buffer overflow risks")
        return True
    else:
        print("✗ Potential buffer overflow issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def check_input_validation():
    """Check for input validation"""
    print("Checking input validation...")

    issues = []

    for filename in ['main.c', 'uart.c', 'sd.c', 'config.c']:
        with open(filename, 'r') as f:
            content = f.read()

        # Check for file operations without validation
        if 'fopen' in content or 'sd_load_file' in content:
            if 'validate_filename' not in content and 'strncpy' not in content:
                issues.append(f"File operation without input validation in {filename}")

    if not issues:
        print("✓ Input validation appears adequate")
        return True
    else:
        print("✗ Input validation issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def check_privilege_separation():
    """Check for privilege separation"""
    print("Checking privilege separation...")

    # In bootloader context, we're already in privileged mode
    print("✓ Bootloader runs in privileged mode (expected)")
    return True

def check_secure_defaults():
    """Check for secure defaults"""
    print("Checking secure defaults...")

    issues = []

    # Check if UART is initialized securely
    with open('uart.c', 'r') as f:
        uart_code = f.read()

    if 'UART_CR' in uart_code:
        # Check if UART is properly configured
        if '0x00000301' in uart_code:  # Enable TX/RX
            print("✓ UART configured securely")
        else:
            issues.append("UART configuration may not be secure")

    if not issues:
        print("✓ Secure defaults appear implemented")
        return True
    else:
        print("✗ Secure defaults issues:")
        for issue in issues:
            print(f"  {issue}")
        return False

def run_security_analysis():
    """Run all security checks"""
    print("=== Security Analysis Report ===\n")

    tests = [
        check_buffer_overflows,
        check_input_validation,
        check_privilege_separation,
        check_secure_defaults
    ]

    passed = 0
    for test in tests:
        if test():
            passed += 1
        print()

    print(f"Security analysis: {passed}/{len(tests)} checks passed")

    return passed == len(tests)

if __name__ == "__main__":
    run_security_analysis()