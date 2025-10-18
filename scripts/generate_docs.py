#!/usr/bin/env python3
"""
Automated Documentation Generator for Bootloader

Extracts information from code and generates/updates documentation.
"""

import os
import re
from collections import defaultdict

def extract_fsa_states():
    """Extract FSA states from fsa_monitor.h"""
    states = []

    if os.path.exists('fsa_monitor.h'):
        with open('fsa_monitor.h', 'r') as f:
            content = f.read()

        # Find all STATE_ definitions
        state_matches = re.findall(r'\s+(STATE_\w+),', content)
        states = [match for match in state_matches]

    return states

def extract_functions():
    """Extract function definitions from C files"""
    functions = defaultdict(list)

    c_files = ['main.c', 'uart.c', 'gpio.c', 'timer.c', 'sd.c', 'verification.c']

    for filename in c_files:
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                content = f.read()

            # Find function definitions
            func_matches = re.findall(r'^\w+\s+(\w+)\s*\([^)]*\)\s*{', content, re.MULTILINE)
            functions[filename] = func_matches

    return functions

def generate_fsa_table(states):
    """Generate markdown table for FSA states"""
    table = "| State | Description | Key Features |\n"
    table += "|-------|-------------|--------------|\n"

    # Basic descriptions for known states
    descriptions = {
        'STATE_POWER_ON': ('Initial power-on detection', 'Hardware initialization trigger'),
        'STATE_EARLY_HW_INIT': ('Basic hardware setup', 'UART, GPIO, timer, interrupt initialization'),
        'STATE_BOOTCODE_LOADING': ('bootcode.bin loading', 'SD card access, file validation'),
        'STATE_KERNEL_LOADING': ('Kernel image loading', 'Linux kernel loading'),
        'STATE_SUCCESS': ('Successful boot', 'Completion logging'),
        'STATE_FAILURE': ('Boot failure', 'Error handling and logging'),
    }

    for state in states:
        desc, features = descriptions.get(state, ('State transition', 'Boot process step'))
        table += f"| {state} | {desc} | {features} |\n"

    return table

def update_boot_process_doc():
    """Update BOOT_PROCESS.md with current FSA states"""
    states = extract_fsa_states()

    if not states:
        print("No FSA states found")
        return

    # Read current documentation
    doc_path = '../BOOT_PROCESS.md'
    if not os.path.exists(doc_path):
        print("BOOT_PROCESS.md not found")
        return

    with open(doc_path, 'r') as f:
        content = f.read()

    # Generate new table
    new_table = generate_fsa_table(states)

    # Replace the FSA states table
    pattern = r'\| State \| Description \| Key Features \|.*?(?=\n\n|\n##|\n###|\Z)'
    replacement = f"| State | Description | Key Features |\n|-------|-------------|--------------|\n"

    # Add state entries
    for state in states:
        descriptions = {
            'STATE_POWER_ON': ('Initial power-on detection', 'Hardware initialization trigger'),
            'STATE_EARLY_HW_INIT': ('Basic hardware setup', 'UART, GPIO, timer, interrupt initialization'),
            'STATE_BOOTCODE_LOADING': ('bootcode.bin loading', 'SD card access, file validation'),
            'STATE_KERNEL_LOADING': ('Kernel image loading', 'Linux kernel loading'),
            'STATE_SUCCESS': ('Successful boot', 'Completion logging'),
            'STATE_FAILURE': ('Boot failure', 'Error handling and logging'),
        }
        desc, features = descriptions.get(state, ('State transition', 'Boot process step'))
        replacement += f"| {state} | {desc} | {features} |\n"

    # Update state count in description
    content = re.sub(r'with \d+ states', f'with {len(states)} states', content)

    # Replace table
    content = re.sub(pattern, replacement.rstrip(), content, flags=re.DOTALL)

    # Write back
    with open(doc_path, 'w') as f:
        f.write(content)

    print(f"Updated BOOT_PROCESS.md with {len(states)} FSA states")

def generate_function_inventory():
    """Generate function inventory for documentation"""
    functions = extract_functions()

    inventory = "## Function Inventory\n\n"
    inventory += "Automatically generated function list from codebase.\n\n"

    for filename, funcs in functions.items():
        if funcs:
            inventory += f"### {filename}\n\n"
            for func in funcs:
                inventory += f"- `{func}`\n"
            inventory += "\n"

    return inventory

def main():
    """Main documentation generation"""
    print("=== Automated Documentation Generator ===\n")

    # Update FSA documentation
    print("Updating FSA state documentation...")
    update_boot_process_doc()

    # Generate function inventory
    print("Generating function inventory...")
    inventory = generate_function_inventory()

    # Save to file
    with open('../FUNCTION_INVENTORY.md', 'w') as f:
        f.write(inventory)

    print("✓ Documentation generation complete")

if __name__ == "__main__":
    main()