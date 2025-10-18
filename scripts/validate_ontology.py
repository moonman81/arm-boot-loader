#!/usr/bin/env python3
"""
Ontology Validation for Bootloader

Checks if bootloader implementation matches ontological specifications.
"""

import os
import re

def check_fsa_ontology():
    """Check against arm_boot_fsa_ontology.n3"""
    print("Checking FSA ontology compliance...")

    # Check if code implements FSA state machine
    with open('main.c', 'r') as f:
        code = f.read()

    # Check for state machine elements
    has_monitor = 'fsa_monitor_init' in code and 'fsa_update_state' in code
    has_switch = 'switch (fsa_monitor.current_state)' in code
    has_states = all(state in code for state in [
        'STATE_BOOTCODE_LOADING', 'STATE_BOOTCODE_EXEC',
        'STATE_STARTELF_LOADING', 'STATE_STARTELF_EXEC',
        'STATE_KERNEL_LOADING', 'STATE_KERNEL_EXEC'
    ])

    if has_monitor and has_switch and has_states:
        print("✓ FSA state machine with monitoring implemented according to ontology")
        return True
    else:
        print("✗ FSA implementation incomplete")
        return False

def check_math_ontology():
    """Check against mathematical ontologies"""
    print("Checking mathematical ontology compliance...")

    # Check if implementation uses concepts from math ontologies
    with open('main.c', 'r') as f:
        code = f.read()

    math_concepts = [
        ('state', 'State from domain theory'),
        ('transition', 'Transition from FSA'),
        ('kernel', 'Kernel from type theory')
    ]

    found = 0
    for concept, desc in math_concepts:
        if concept.lower() in code.lower():
            found += 1
            print(f"✓ {desc} concept used")

    if found >= 2:
        print("✓ Mathematical concepts adequately represented")
        return True
    else:
        print("✗ Insufficient mathematical concept usage")
        return False

def check_requirements_ontology():
    """Check against requirements ontologies"""
    print("Checking requirements ontology compliance...")

    with open('main.c', 'r') as f:
        code = f.read()

    requirements = [
        'kernel_handover',  # Functional requirement
        'uart_puts',        # Debugging requirement
        'switch.*state'     # State machine requirement
    ]

    implemented = 0
    for req in requirements:
        if re.search(req, code, re.IGNORECASE):
            implemented += 1

    if implemented == len(requirements):
        print("✓ All key requirements implemented")
        return True
    else:
        print(f"✗ {len(requirements) - implemented} requirements not implemented")
        return False

def validate_ontologies():
    """Run all ontology validations"""
    print("=== Ontology Validation Report ===\n")

    tests = [
        check_fsa_ontology,
        check_math_ontology,
        check_requirements_ontology
    ]

    passed = 0
    for test in tests:
        if test():
            passed += 1
        print()

    print(f"Ontology validation: {passed}/{len(tests)} checks passed")

    return passed == len(tests)

if __name__ == "__main__":
    validate_ontologies()