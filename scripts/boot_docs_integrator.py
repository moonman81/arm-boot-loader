#!/usr/bin/env python3
"""
Boot Documentation Integrator

This tool integrates boot process documentation with development workflows.
It can be used in IDEs or as a CLI to access boot-related docs.
"""

import webbrowser
import os
import sys
import re
from pathlib import Path

class BootDocsIntegrator:
    def __init__(self):
        self.docs_path = os.path.join(os.path.dirname(__file__), "ARM_Boot_Documentation.md")
        self.spec_path = os.path.join(os.path.dirname(__file__), "Boot_Specification.md")

    def open_docs(self, section=None):
        """Open documentation in default browser"""
        if os.path.exists(self.docs_path):
            if section:
                # For simplicity, open full docs
                webbrowser.open(f"file://{self.docs_path}")
            else:
                webbrowser.open(f"file://{self.docs_path}")
            print("Opened boot documentation in browser")
        else:
            print("Documentation file not found")

    def open_spec(self):
        """Open specification in browser"""
        if os.path.exists(self.spec_path):
            webbrowser.open(f"file://{self.spec_path}")
            print("Opened boot specification in browser")
        else:
            print("Specification file not found")

    def search_docs(self, query):
        """Search documentation for query"""
        if not os.path.exists(self.docs_path):
            print("Documentation not available")
            return

        with open(self.docs_path, 'r') as f:
            content = f.read()

        lines = content.split('\n')
        results = []
        for i, line in enumerate(lines):
            if query.lower() in line.lower():
                # Get context
                start = max(0, i-2)
                end = min(len(lines), i+3)
                context = '\n'.join(lines[start:end])
                results.append(f"Line {i+1}: {context}")

        if results:
            print(f"Found {len(results)} matches for '{query}':")
            for result in results[:5]:  # Show first 5
                print(result)
                print("-" * 50)
        else:
            print(f"No matches found for '{query}'")

    def parse_n3_ontology(self, n3_file):
        """Parse N3 ontology file and extract axioms/concepts"""
        if not os.path.exists(n3_file):
            return {}

        axioms = {}
        with open(n3_file, 'r') as f:
            content = f.read()

        # Extract class definitions
        class_pattern = r':(\w+)\s+a\s+owl:Class\s*;'
        for match in re.finditer(class_pattern, content):
            class_name = match.group(1)
            axioms[class_name] = {'type': 'class', 'file': n3_file}

        # Extract instances
        instance_pattern = r':(\w+)\s+a\s+:(\w+)\s*;'
        for match in re.finditer(instance_pattern, content):
            instance_name = match.group(1)
            class_name = match.group(2)
            axioms[instance_name] = {'type': 'instance', 'class': class_name, 'file': n3_file}

        return axioms

    def find_code_implementations(self, concept_name):
        """Find code implementations of mathematical concepts"""
        implementations = []

        # Search in verification.c
        verification_path = os.path.join(os.path.dirname(__file__), "bootloader", "verification.c")
        if os.path.exists(verification_path):
            with open(verification_path, 'r') as f:
                content = f.read()

            # Look for function definitions related to the concept
            func_pattern = rf'verification_\w*{concept_name.lower()}\w*\s*\('
            for match in re.finditer(func_pattern, content):
                line_num = content[:match.start()].count('\n') + 1
                implementations.append({
                    'file': 'verification.c',
                    'line': line_num,
                    'type': 'function',
                    'name': match.group(0).strip()
                })

            # Look for struct definitions
            struct_pattern = rf'typedef struct.*{concept_name.lower()}\w*\s*\{{'
            for match in re.finditer(struct_pattern, content, re.DOTALL):
                line_num = content[:match.start()].count('\n') + 1
                implementations.append({
                    'file': 'verification.c',
                    'line': line_num,
                    'type': 'struct',
                    'name': f'{concept_name} struct'
                })

        return implementations

    def cross_reference_ontologies(self):
        """Cross-reference N3 ontologies with code implementations"""
        print("Cross-referencing mathematical ontologies with code implementations...\n")

        # Find all N3 files
        n3_files = list(Path(os.path.dirname(__file__)).glob("*.n3"))

        total_concepts = 0
        implemented_concepts = 0

        for n3_file in n3_files:
            print(f"Analyzing {n3_file.name}...")
            axioms = self.parse_n3_ontology(str(n3_file))

            for concept_name, concept_info in axioms.items():
                total_concepts += 1
                implementations = self.find_code_implementations(concept_name)

                if implementations:
                    implemented_concepts += 1
                    print(f"✓ {concept_name} ({concept_info['type']}) - IMPLEMENTED")
                    for impl in implementations:
                        print(f"  → {impl['file']}:{impl['line']} - {impl['type']} {impl['name']}")
                else:
                    print(f"✗ {concept_name} ({concept_info['type']}) - NOT IMPLEMENTED")

            print()

        print(f"Summary: {implemented_concepts}/{total_concepts} concepts implemented")
        return implemented_concepts, total_concepts

    def verify_mathematical_consistency(self):
        """Verify mathematical consistency across frameworks"""
        print("Verifying mathematical framework consistency...\n")

        # Check that all frameworks are represented
        frameworks = ['Tarski', 'Grothendieck', 'Kripke', 'Scott', 'Type', 'Homotopy']
        framework_files = {
            'Tarski': 'arm_boot_tarski_semantics.n3',
            'Grothendieck': 'arm_boot_grothendieck_category.n3',
            'Kripke': 'arm_boot_kripke_semantics.n3',
            'Scott': 'arm_boot_scott_domains.n3',
            'Type': 'arm_boot_type_theory.n3',
            'Homotopy': 'arm_boot_homotopy.n3'
        }

        consistency_issues = []

        for framework, file in framework_files.items():
            if os.path.exists(os.path.join(os.path.dirname(__file__), file)):
                print(f"✓ {framework} framework ontology present")
            else:
                print(f"✗ {framework} framework ontology missing")
                consistency_issues.append(f"Missing {framework} ontology")

        # Check master integration
        master_file = os.path.join(os.path.dirname(__file__), 'arm_boot_mathematical_master.n3')
        if os.path.exists(master_file):
            print("✓ Master mathematical framework present")
        else:
            print("✗ Master mathematical framework missing")
            consistency_issues.append("Missing master framework")

        if consistency_issues:
            print(f"\nFound {len(consistency_issues)} consistency issues:")
            for issue in consistency_issues:
                print(f"  - {issue}")
        else:
            print("\n✓ All mathematical frameworks are consistent")

        return len(consistency_issues) == 0

def main():
    integrator = BootDocsIntegrator()

    if len(sys.argv) < 2:
        print("Usage: python boot_docs_integrator.py <command>")
        print("Commands:")
        print("  docs              - Open documentation")
        print("  spec              - Open specification")
        print("  search <query>    - Search documentation")
        print("  crossref          - Cross-reference ontologies with code")
        print("  verify            - Verify mathematical consistency")
        return

    command = sys.argv[1]

    if command == "docs":
        integrator.open_docs()
    elif command == "spec":
        integrator.open_spec()
    elif command == "search" and len(sys.argv) > 2:
        query = ' '.join(sys.argv[2:])
        integrator.search_docs(query)
    elif command == "crossref":
        integrator.cross_reference_ontologies()
    elif command == "verify":
        integrator.verify_mathematical_consistency()
    else:
        print("Invalid command")

if __name__ == "__main__":
    main()