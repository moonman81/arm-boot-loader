# Build System

This document details the build system and development tools used for compiling and testing the bootloader.

## Build Environment

Cross-compilation setup for ARM64 Raspberry Pi targets.

### Prerequisites

Required development tools and dependencies.

| Tool | Version | Purpose | Installation |
|------|---------|---------|--------------|
| aarch64-elf-gcc | 10.0+ | ARM64 cross-compiler | `brew install aarch64-elf-gcc` |
| make | 4.0+ | Build automation | System default |
| python3 | 3.8+ | Test scripts | System default |
| cppcheck | 2.0+ | Static analysis | `brew install cppcheck` |

### Directory Structure

Source code organization.

```
bootloader/
├── *.c, *.h          # Source files
├── *.S               # Assembly files
├── Makefile          # Build configuration
├── linker.ld         # Linker script
├── test_*.py         # Python test scripts
├── static_*.py       # Analysis tools
├── *.bin, *.elf      # Build outputs
└── *.o               # Object files
```

## Build Process

Complete compilation pipeline from source to binary.

### Makefile Targets

Available build operations.

| Target | Command | Output | Description |
|--------|---------|--------|-------------|
| all | `make` | bootloader.bin | Complete build |
| clean | `make clean` | - | Remove build artifacts |
| test | `make test` | Test results | Run test suite |
| static | `make static` | Analysis reports | Static analysis |

### Compilation Flags

Compiler optimization and safety settings.

| Flag Category | Flags | Purpose |
|---------------|-------|---------|
| Optimization | `-Os` | Size optimization |
| Architecture | `-march=armv8-a` | ARM64 instruction set |
| Environment | `-ffreestanding -nostdlib -nostartfiles` | Bare-metal environment |
| Warnings | `-Wall -Wextra -Wno-unused-parameter` | Comprehensive warnings |
| Safety | `-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast` | Embedded casting |

### Linker Configuration

Memory layout and binary generation.

| Section | Address | Purpose |
|---------|---------|---------|
| .text | 0x00000000 | Executable code |
| .data | Auto | Initialized data |
| .bss | Auto | Uninitialized data |
| .stack | 0x00080000 | Runtime stack |
| .heap | 0x00090000 | Dynamic allocation |

## Build Outputs

Generated artifacts and their purposes.

### Binary Files

Final build products.

| File | Size | Purpose | Usage |
|------|------|---------|-------|
| bootloader.bin | ~42KB | Raw binary | SD card boot |
| bootloader.elf | ~45KB | Debug symbols | GDB debugging |

### Intermediate Files

Build process artifacts.

| File Type | Pattern | Purpose |
|-----------|---------|---------|
| Object files | `*.o` | Compiled modules | Linking input |
| Dependency files | `*.d` | Header dependencies | Incremental builds |
| Map files | `*.map` | Memory layout | Linker analysis |

## Testing Framework

Automated testing and validation tools.

### Python Test Scripts

Development and validation utilities.

| Script | Purpose | Execution |
|--------|---------|-----------|
| test_bootloader.py | Integration testing | `python3 test_bootloader.py` |
| test_fat.py | Filesystem testing | `python3 test_fat.py` |
| test_network.py | Network protocol testing | `python3 test_network.py` |
| hardware_test.py | Hardware interface testing | Serial communication |
| verify_fsa.py | FSA verification | State machine validation |

### Static Analysis Tools

Code quality verification scripts.

| Script | Analysis | Output |
|--------|----------|--------|
| static_analysis.py | Code quality metrics | Quality report |
| code_metrics.py | Size and complexity | Statistical analysis |
| security_analysis.py | Vulnerability assessment | Security scorecard |
| validate_ontology.py | Formal specification | Ontology compliance |

### Test Execution

Running the complete test suite.

```bash
# Run all tests
python3 test_bootloader.py

# Run static analysis
python3 static_analysis.py

# Run FSA verification
python3 verify_fsa.py
```

## Development Workflow

Standard development and testing cycle.

### Code Development

Source code modification and compilation.

```bash
# Edit source files
vim main.c

# Build project
make clean && make

# Test compilation
ls -la bootloader.bin
```

### Testing Cycle

Validation and verification process.

```bash
# Run static analysis
python3 static_analysis.py

# Run driver tests (on hardware)
python3 hardware_test.py

# Validate FSA
python3 verify_fsa.py

# Check code metrics
python3 code_metrics.py
```

### Debugging

Development debugging workflow.

```bash
# Build with debug symbols
make

# Examine binary size
stat -f%z bootloader.bin

# Check for compilation warnings
make 2>&1 | grep -i warn
```

## Cross-Compilation Details

ARM64 bare-metal compilation specifics.

### Toolchain Configuration

Compiler and linker setup.

| Component | Configuration | Purpose |
|-----------|----------------|---------|
| Target | `aarch64-elf` | ARM64 little-endian |
| ABI | `aapcs64` | ARM64 calling convention |
| Float | Soft float | No hardware floating point |
| Exceptions | No exceptions | Bare-metal environment |

### Assembly Integration

Startup code and low-level initialization.

| File | Purpose | Entry Point |
|------|---------|-------------|
| start.S | ARM64 startup | `_start` function |
| linker.ld | Memory layout | Section placement |
| crt0 equivalent | Runtime initialization | Hardware setup |

## Build Optimization

Size and performance optimization techniques.

### Compiler Optimizations

Code generation settings.

| Optimization | Flag | Effect |
|--------------|------|--------|
| Size | `-Os` | Minimize binary size |
| Function sections | `-ffunction-sections` | Per-function optimization |
| Data sections | `-fdata-sections` | Per-data optimization |
| Linker GC | `-Wl,--gc-sections` | Remove unused code |

### Size Reduction

Binary size minimization strategies.

| Technique | Implementation | Size Impact |
|-----------|----------------|-------------|
| Array bounds reduction | Smaller data structures | -8KB |
| Function simplification | Remove complex features | -5KB |
| Dead code elimination | Linker garbage collection | -2KB |
| String compression | Shared string literals | -1KB |

## Continuous Integration

Automated build and test pipeline.

### Pre-commit Hooks

Development-time validation.

```bash
#!/bin/bash
# Pre-commit hook script
make clean && make
python3 static_analysis.py
python3 validate_ontology.py
```

### CI Pipeline Stages

Automated testing pipeline.

| Stage | Tools | Criteria |
|-------|-------|----------|
| Build | make, aarch64-gcc | Compilation success |
| Static Analysis | cppcheck, custom scripts | No critical issues |
| Ontology Validation | RDF validation | Specification compliance |
| Performance Testing | Timing measurements | Performance benchmarks |
| Documentation | Markdown generation | Updated docs |

## Troubleshooting

Common build issues and solutions.

### Compilation Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `aarch64-elf-gcc not found` | Missing toolchain | Install cross-compiler |
| `undefined reference` | Missing source file | Check Makefile SOURCES |
| `section overflow` | Binary too large | Reduce array sizes |
| `invalid instruction` | Wrong architecture | Check -march flag |

### Runtime Issues

| Issue | Symptom | Debugging |
|-------|---------|-----------|
| No UART output | Hardware issue | Check GPIO pinout |
| SD card not detected | Driver issue | Test with known good card |
| Kernel not loading | File issue | Verify kernel8.img presence |
| Boot hangs | State machine | Check FSA state transitions |

### Performance Issues

| Problem | Indicator | Optimization |
|---------|-----------|--------------|
| Large binary | >50KB | Reduce data structures |
| Slow boot | >5 seconds | Profile state transitions |
| High memory usage | Heap exhaustion | Optimize allocations |
| Compilation slow | >30 seconds | Use incremental builds |

## Build Statistics

Current build metrics and performance.

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Binary size | 42KB | <50KB | ✅ Good |
| Build time | <10s | <30s | ✅ Good |
| Object count | 20+ | - | ✅ Modular |
| Warning count | <5 | 0 | ⚠️ Minor |
| Test coverage | 80% | 90% | 🔄 Improving |

The build system provides a complete development environment with cross-compilation, automated testing, static analysis, and comprehensive validation tools for reliable bootloader development.