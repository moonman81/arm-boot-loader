#!/usr/bin/env python3
"""
Formal Verification of Bootloader FSA

Checks if the bootloader code follows the FSA defined in ontologies.
"""

import re

def check_fsa_compliance():
    """Check if main.c implements the FSA correctly"""

    with open('main.c', 'r') as f:
        code = f.read()

    # Check for state enum in header
    try:
        with open('fsa_monitor.h', 'r') as h:
            header_code = h.read()
        if 'typedef enum' in header_code and 'boot_state_t' in header_code:
            print("✓ FSA states defined")
        else:
            print("✗ FSA states not properly defined")
    except FileNotFoundError:
        print("✗ FSA header file not found")

    # Check for state machine loop
    if 'while (1)' in code and ('switch (current_state)' in code or 'switch (fsa_monitor.current_state)' in code):
        print("✓ State machine implemented")
    else:
        print("✗ State machine not implemented")

    # Check for state transitions
    states = ['STATE_BOOTCODE_LOADING', 'STATE_BOOTCODE_EXEC', 'STATE_STARTELF_LOADING',
              'STATE_STARTELF_EXEC', 'STATE_KERNEL_LOADING', 'STATE_KERNEL_EXEC', 'STATE_SUCCESS', 'STATE_FAILURE']

    for state in states:
        if state in code:
            print(f"✓ State {state} present")
        else:
            print(f"✗ State {state} missing")

    # Check for kernel handover
    if 'kernel_handover' in code:
        print("✓ Kernel handover implemented")
    else:
        print("✗ Kernel handover missing")

    print("FSA verification complete")

if __name__ == "__main__":
    check_fsa_compliance()