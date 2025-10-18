#!/usr/bin/env python3
import sys
from rdflib import Graph

def validate_n3(file_path):
    g = Graph()
    try:
        g.parse(file_path, format='n3')
        print("N3 file is valid.")
        print(f"Number of triples: {len(g)}")
    except Exception as e:
        print(f"Error parsing N3 file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python validate_n3.py <file>")
        sys.exit(1)
    validate_n3(sys.argv[1])