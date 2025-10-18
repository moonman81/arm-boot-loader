#!/usr/bin/env python3
"""
Code Quality Metrics for Bootloader

Calculates various code quality metrics.
"""

import os
import re

def calculate_metrics():
    """Calculate code quality metrics"""
    print("=== Code Quality Metrics ===\n")

    total_lines = 0
    total_functions = 0
    total_comments = 0

    files = ['main.c', 'uart.c', 'sd.c', 'config.c', 'start.S']

    for filename in files:
        if not os.path.exists(filename):
            continue

        with open(filename, 'r') as f:
            lines = f.readlines()

        file_lines = len(lines)
        total_lines += file_lines

        # Count functions
        functions = 0
        if filename.endswith('.c'):
            functions = len(re.findall(r'^\w+\s+\w+\s*\(', '\n'.join(lines), re.MULTILINE))
            total_functions += functions
        elif filename.endswith('.S'):
            functions = len(re.findall(r'^\.global\s+\w+', '\n'.join(lines), re.MULTILINE))
            total_functions += functions

        # Count comments
        comments = 0
        for line in lines:
            if line.strip().startswith('//') or line.strip().startswith('/*'):
                comments += 1
        total_comments += comments

        print(f"{filename}:")
        print(f"  Lines: {file_lines}")
        print(f"  Functions: {functions}")
        print(f"  Comments: {comments}")
        print()

    # Overall metrics
    print("Overall Metrics:")
    print(f"  Total Lines: {total_lines}")
    print(f"  Total Functions: {total_functions}")
    print(f"  Total Comments: {total_comments}")

    if total_lines > 0:
        comment_ratio = (total_comments / total_lines) * 100
        print(f"  Comment Ratio: {comment_ratio:.1f}%")
        if comment_ratio < 10:
            print("  ⚠️  Low comment ratio")
        else:
            print("  ✓ Good comment ratio")

    avg_functions_per_file = total_functions / len(files)
    print(f"  Avg Functions/File: {avg_functions_per_file:.1f}")
    if avg_functions_per_file > 5:
        print("  ⚠️  High function density")
    else:
        print("  ✓ Reasonable function density")

    print("\n✓ Metrics calculation complete")

if __name__ == "__main__":
    calculate_metrics()