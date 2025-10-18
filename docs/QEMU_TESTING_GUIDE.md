# QEMU Testing Guide for ARM Native Bootloader

## Overview

This guide describes how to test the ARM native bootloader in QEMU without requiring physical Raspberry Pi hardware or GPU. The bootloader runs in headless mode with all output through the UART serial console.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Boot Modes](#boot-modes)
4. [Testing Procedures](#testing-procedures)
5. [Troubleshooting](#troubleshooting)
6. [QEMU Limitations](#qemu-limitations)
7. [Advanced Testing](#advanced-testing)

---

## Prerequisites

### Required Software

1. **QEMU** (version 5.0 or later)
   - macOS: `brew install qemu`
   - Ubuntu/Debian: `sudo apt-get install qemu-system-arm`
   - Arch Linux: `sudo pacman -S qemu-arch-extra`

2. **AArch64 Cross-Compiler**
   - Already required for building the bootloader
   - `aarch64-elf-gcc` or `aarch64-linux-gnu-gcc`

3. **Make**
   - Standard build tool

### Verify Installation

```bash
# Check QEMU is installed
make qemu-check

# Should output:
# QEMU found: QEMU emulator version X.X.X
```

---

## Quick Start

### Build and Run

```bash
# 1. Build the bootloader
cd bootloader
make clean
make

# 2. Run in QEMU (headless mode)
make qemu-test

# Alternative: Use the boot script
./qemu_boot.sh
```

### Expected Output

You should see UART output similar to:

```
Custom Raspberry Pi Bootloader v1.0
Complete BSP Initialized
Detected Raspberry Pi: Raspberry Pi 3 Model B
SoC: BCM2837, 4 cores, 1024MB RAM
Capabilities: ETH WiFi BT
Memory allocation working
Power-on detected
Early hardware initialization...
Selecting bootcode source...
Loading bootcode...
...
```

### Exit QEMU

Press: **Ctrl-A** then **X**

---

## Boot Modes

### 1. Direct Bootloader Boot (Default)

Boots the bootloader binary directly in QEMU without SD card.

```bash
make qemu-test
```

**Use case**: Testing bootloader initialization, hardware detection, driver tests

**QEMU command**:
```bash
qemu-system-aarch64 -M raspi3b -kernel bootloader.bin -serial stdio -nographic
```

### 2. Bootloader with SD Card Image

Boots the bootloader with an emulated SD card containing kernel and DTB.

```bash
# First, create an SD card image (see Advanced Testing)
./qemu_boot.sh --sd sdcard.img
```

**Use case**: Testing full boot flow including kernel loading

### 3. Direct Kernel Boot (Bypass Bootloader)

Boots a kernel directly, bypassing the bootloader.

```bash
./qemu_boot.sh --kernel kernel8.img --dtb bcm2710-rpi-3-b.dtb
```

**Use case**: Testing kernel without bootloader involvement

### 4. Debug Mode

Runs with QEMU debug output enabled.

```bash
make qemu-debug

# Or
./qemu_boot.sh --debug
```

**Output**: Creates `qemu.log` with detailed CPU and interrupt traces

---

## Testing Procedures

### Test 1: Basic Boot Verification

**Objective**: Verify bootloader boots and initializes hardware

**Procedure**:
```bash
make qemu-test
```

**Expected Results**:
- ✓ UART output appears
- ✓ "Custom Raspberry Pi Bootloader v1.0" message
- ✓ Hardware detection succeeds
- ✓ Model detected as "Raspberry Pi 3 Model B"
- ✓ SoC detected as "BCM2837"
- ✓ Memory allocation working
- ✓ Driver tests run (basic tests may fail due to QEMU limitations)

**Pass Criteria**: Boot reaches FSA state machine and progresses through states

---

### Test 2: FSA State Machine Progression

**Objective**: Verify all boot states are traversed correctly

**Procedure**:
```bash
make qemu-test 2>&1 | grep -E "STATE_|state"
```

**Expected State Sequence**:
```
STATE_POWER_ON
STATE_EARLY_HW_INIT
STATE_BOOTCODE_SOURCE_SELECT
STATE_BOOTCODE_LOADING
STATE_BOOTCODE_VALIDATION
STATE_BOOTCODE_EXEC
STATE_BOOTCODE_CONFIG_PARSE
STATE_CORE_DRIVER_INIT
STATE_BSP_DRIVER_INIT
STATE_HW_VALIDATION
STATE_SECURITY_ATTESTATION
STATE_FIRMWARE_MEASUREMENT
STATE_BOOT_POLICY_VALIDATION
STATE_TRUSTED_EXECUTION_INIT
STATE_CONFIG_LOADING
STATE_CONFIG_PARSING
STATE_CONFIGURATION_COHERENCE_CHECK
STATE_DEPENDENCY_GRAPH_ANALYSIS
STATE_CONFIG_VALIDATION
STATE_SEMANTIC_VALIDATION
STATE_CONSISTENCY_CHECK
STATE_CONFIG_APPLICATION
STATE_STARTELF_SOURCE_SELECT
STATE_STARTELF_LOADING
STATE_STARTELF_VALIDATION
STATE_STARTELF_EXEC
STATE_KERNEL_SOURCE_SELECT
STATE_KERNEL_LOADING (may fail without SD card)
```

**Pass Criteria**: All states execute without crashes

---

### Test 3: UART Communication

**Objective**: Verify bidirectional UART works in QEMU

**Procedure**:
```bash
# QEMU automatically connects UART0 to stdio
make qemu-test

# Type characters - they should be echoed (if interactive mode is enabled)
```

**Expected Results**:
- ✓ Output appears on terminal
- ✓ Characters display correctly
- ✓ No garbled text
- ✓ ANSI colors work (if supported by terminal)

**Pass Criteria**: Clean UART output with no corruption

---

### Test 4: Timer and Interrupts

**Objective**: Verify timer subsystem works in QEMU

**Procedure**:
```bash
make qemu-test 2>&1 | grep -i timer
```

**Expected Results**:
- ✓ Timer initializes successfully
- ✓ Boot time measurement works
- ✓ Delay functions work (10ms delay between FSA states)

**Pass Criteria**: Timer-related messages appear

---

### Test 5: Memory Subsystem

**Objective**: Verify memory allocation works

**Procedure**:
```bash
make qemu-test 2>&1 | grep -i memory
```

**Expected Results**:
- ✓ "Memory allocation working" message
- ✓ No memory corruption
- ✓ BSS cleared correctly

**Pass Criteria**: Memory allocation succeeds

---

### Test 6: Hardware Detection

**Objective**: Verify model detection via mailbox

**Procedure**:
```bash
make qemu-test 2>&1 | grep -E "Detected|SoC:|Capabilities"
```

**Expected Results**:
```
Detected Raspberry Pi: Raspberry Pi 3 Model B
SoC: BCM2837, 4 cores, 1024MB RAM
Capabilities: ETH WiFi BT
```

**Pass Criteria**: Model detected correctly as Pi 3B with BCM2837

---

### Test 7: Driver Initialization

**Objective**: Verify all drivers initialize (even if some fail in QEMU)

**Procedure**:
```bash
make qemu-test 2>&1 | grep -i "init"
```

**Expected Results**:
- ✓ UART init
- ✓ Memory init
- ✓ GPIO init
- ✓ Timer init
- ✓ Interrupt init
- ✓ Mailbox init
- ✓ Clock init
- ⚠ DMA init (may fail - not emulated)
- ⚠ I2C init (may fail - not emulated)
- ⚠ SPI init (may fail - not emulated)
- ⚠ PWM init (may fail - not emulated)
- ⚠ USB init (may fail - limited emulation)
- ⚠ Ethernet init (may fail - not emulated)

**Pass Criteria**: Core drivers (UART, Timer, Memory, Interrupt) succeed

---

### Test 8: Security Subsystem

**Objective**: Verify security attestation and measurements

**Procedure**:
```bash
make qemu-test 2>&1 | grep -E "security|attestation|measurement"
```

**Expected Results**:
- ✓ "Performing security attestation..." appears
- ✓ "Measuring firmware integrity..." appears
- ✓ Security checks pass (may use stub implementations)

**Pass Criteria**: Security states execute without blocking boot

---

### Test 9: Verification System

**Objective**: Verify formal verification checks

**Procedure**:
```bash
make qemu-test 2>&1 | grep -E "verification|validation|semantic"
```

**Expected Results**:
- ✓ "Performing semantic validation..." appears
- ✓ Verification checks pass

**Pass Criteria**: Verification states complete successfully

---

### Test 10: Boot Time Measurement

**Objective**: Measure boot performance in QEMU

**Procedure**:
```bash
make qemu-test 2>&1 | grep -E "boot time|Boot successful"
```

**Expected Results**:
```
Total boot time: XXX ms
```

**Typical Range**: 50-500ms in QEMU (much faster than real hardware due to no I/O delays)

**Pass Criteria**: Boot time is measured and reported

---

## Troubleshooting

### Issue: QEMU not found

**Symptom**:
```
make: qemu-system-aarch64: command not found
```

**Solution**:
```bash
# macOS
brew install qemu

# Linux
sudo apt-get install qemu-system-arm
```

---

### Issue: No output in terminal

**Symptom**: QEMU starts but no text appears

**Causes**:
1. Wrong machine type (must be `raspi3b` not `raspi3`)
2. Serial not connected (need `-serial stdio`)
3. Bootloader binary corrupted

**Solution**:
```bash
# Rebuild bootloader
make clean && make

# Use correct command
qemu-system-aarch64 -M raspi3b -kernel bootloader.bin -serial stdio -nographic
```

---

### Issue: Boot hangs after initialization

**Symptom**: Boot stops after "Loading bootcode..." or "Loading kernel..."

**Cause**: Waiting for SD card which doesn't exist in basic QEMU mode

**Solution**: This is expected behavior. The bootloader is trying to load from SD card. Either:

1. **Accept the behavior**: The boot demonstrates initialization works
2. **Create SD card image**: Follow "Advanced Testing" section below
3. **Modify config**: Set `kernel_source` to skip SD card requirement

---

### Issue: Driver tests fail

**Symptom**: Messages like "DMA test failed" or "SPI test failed"

**Cause**: QEMU doesn't emulate all peripherals

**Solution**: This is expected. QEMU limitations are documented below. Focus on testing core functionality (UART, Timer, Memory, Interrupts).

---

### Issue: Garbled or missing characters

**Symptom**: UART output has missing or corrupted characters

**Cause**:
1. Terminal not handling raw output correctly
2. UART baud rate mismatch (should auto-negotiate in QEMU)

**Solution**:
```bash
# Use proper terminal
make qemu-test

# Or add explicit serial options
qemu-system-aarch64 -M raspi3b -kernel bootloader.bin \
    -serial stdio -nographic -monitor none
```

---

### Issue: QEMU crashes or errors

**Symptom**:
```
qemu-system-aarch64: Unsupported machine type
```

**Cause**: Old QEMU version (need 5.0+)

**Solution**: Update QEMU
```bash
brew upgrade qemu  # macOS
```

---

## QEMU Limitations

QEMU `raspi3b` machine emulation has limitations compared to real hardware:

### ✓ Fully Emulated
- **UART0 (PL011)**: Full support, bidirectional
- **System Timer**: Accurate emulation
- **ARM Generic Timer**: Full support
- **Interrupt Controller (GIC-400)**: Full support
- **Mailbox**: Basic support (limited properties)
- **GPIO**: Partial (registers work, no physical I/O)
- **Memory**: Full support
- **CPU**: ARMv8 Cortex-A53 emulation

### ⚠ Partially Emulated
- **SD Card (EMMC)**: Works with `-drive` parameter, limited
- **USB**: Limited USB 2.0 controller emulation
- **Mailbox Properties**: Subset of real hardware
- **Clock Management**: Simplified

### ✗ Not Emulated
- **Ethernet (GENET)**: No network hardware emulation
- **Wi-Fi/Bluetooth**: Not supported
- **DMA Controller**: Not emulated
- **PWM**: Not emulated
- **I2C**: Not emulated
- **SPI**: Not emulated
- **Hardware Watchdog**: Not emulated
- **True RNG**: Not emulated
- **VC4 GPU/VideoCore**: Not emulated (no framebuffer)

### Impact on Testing

| Feature | Real Hardware | QEMU | Testing Strategy |
|---------|---------------|------|------------------|
| UART | ✓ | ✓ | Full testing in QEMU |
| Timer | ✓ | ✓ | Full testing in QEMU |
| Interrupts | ✓ | ✓ | Full testing in QEMU |
| Memory | ✓ | ✓ | Full testing in QEMU |
| GPIO | ✓ | ⚠ | Register access only |
| SD Card | ✓ | ⚠ | Limited (need SD image) |
| USB | ✓ | ⚠ | Very limited |
| Ethernet | ✓ | ✗ | Hardware test only |
| DMA | ✓ | ✗ | Hardware test only |
| I2C/SPI | ✓ | ✗ | Hardware test only |
| Security | ✓ | ⚠ | Logic testing only |

**Recommendation**: Use QEMU for initial development and logic testing. Use real hardware for full integration testing.

---

## Advanced Testing

### Creating an SD Card Image

To test full boot flow with kernel loading:

```bash
# 1. Create empty SD card image (64MB)
dd if=/dev/zero of=sdcard.img bs=1M count=64

# 2. Format as FAT32
# macOS
hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount sdcard.img
# Find device (e.g., /dev/disk4)
diskutil eraseDisk FAT32 SDCARD MBRFormat /dev/diskX
diskutil unmount /dev/diskX
hdiutil detach /dev/diskX

# Linux
mkfs.vfat sdcard.img

# 3. Mount and copy files
mkdir -p /tmp/sdcard
# macOS
hdiutil attach sdcard.img -mountpoint /tmp/sdcard
# Linux
sudo mount -o loop sdcard.img /tmp/sdcard

# 4. Copy kernel and config
cp kernel8.img /tmp/sdcard/
cp config.txt /tmp/sdcard/
echo "kernel_filename=kernel8.img" > /tmp/sdcard/bootloader.cfg

# 5. Unmount
# macOS
hdiutil detach /tmp/sdcard
# Linux
sudo umount /tmp/sdcard

# 6. Boot with SD card
./qemu_boot.sh --sd sdcard.img
```

---

### Debugging with GDB

Run QEMU with GDB server:

```bash
# Terminal 1: Start QEMU with GDB server
qemu-system-aarch64 -M raspi3b -kernel bootloader.bin \
    -serial stdio -nographic -S -s

# -S : Start paused (wait for GDB)
# -s : GDB server on localhost:1234

# Terminal 2: Connect GDB
aarch64-elf-gdb bootloader.elf
(gdb) target remote localhost:1234
(gdb) break main
(gdb) continue
```

**Useful GDB commands**:
```
(gdb) info registers
(gdb) x/10i $pc          # Disassemble at PC
(gdb) x/10x 0x3F201000   # Examine UART registers
(gdb) step
(gdb) continue
```

---

### Automated Testing

Create a test script:

```bash
#!/bin/bash
# test_qemu.sh - Automated QEMU tests

echo "Building bootloader..."
make clean && make || exit 1

echo "Test 1: Boot verification..."
timeout 10 make qemu-test 2>&1 | tee qemu_output.log

echo "Checking for key markers..."
grep -q "Custom Raspberry Pi Bootloader" qemu_output.log && echo "✓ Boot banner found"
grep -q "Detected Raspberry Pi" qemu_output.log && echo "✓ Hardware detection found"
grep -q "Memory allocation working" qemu_output.log && echo "✓ Memory test passed"
grep -q "BCM2837" qemu_output.log && echo "✓ SoC detected correctly"

echo "Test 2: FSA state progression..."
grep "STATE_" qemu_output.log | wc -l
echo "states traversed"

echo "All tests complete"
```

---

### Performance Profiling

Run with performance monitoring:

```bash
# Boot with timing information
make qemu-test 2>&1 | grep -E "time|duration" > timing.log

# Analyze boot time
# Expected output includes "Total boot time: XXX ms"
```

---

### Memory Layout Verification

Check memory regions:

```bash
# Dump memory map
aarch64-elf-objdump -h bootloader.elf

# Check load address (should be 0x80000)
aarch64-elf-objdump -t bootloader.elf | grep _start
```

---

## Platform Comparison

### QEMU vs Real Hardware

| Aspect | QEMU raspi3b | Real Pi 3B | Real Pi 4B | Real Pi 5 |
|--------|--------------|------------|------------|-----------|
| CPU | Emulated A53 | Cortex-A53 | Cortex-A72 | Cortex-A76 |
| Boot time | 50-500ms | 2-5s | 1-3s | 1-2s |
| UART | Full | Full | Full | Full |
| SD Card | Limited | Full | Full | Full |
| Network | None | Full | Full | Full |
| USB | Limited | Full | Full | Full |
| GPIO | Registers | Full I/O | Full I/O | Full I/O |
| Debugging | Easy (GDB) | JTAG/Serial | JTAG/Serial | JTAG/Serial |
| Iteration | Instant | 30s-2min | 30s-2min | 30s-2min |

**Development Workflow**:
1. **Develop in QEMU**: Fast iteration, easy debugging
2. **Test logic in QEMU**: Verify state machines, algorithms
3. **Validate on Pi 3**: Test peripheral integration
4. **Verify on Pi 4/5**: Test newer SoC features

---

## Test Matrix

### Core Functionality Tests

| Test | QEMU | Pi 1 | Pi 2/3 | Pi 4 | Pi 5 |
|------|------|------|--------|------|------|
| Boot sequence | ✓ | ✓ | ✓ | ✓ | ✓ |
| UART I/O | ✓ | ✓ | ✓ | ✓ | ✓ |
| Timer | ✓ | ✓ | ✓ | ✓ | ✓ |
| Interrupts | ✓ | ✓ | ✓ | ✓ | ✓ |
| Memory | ✓ | ✓ | ✓ | ✓ | ✓ |
| Mailbox | ⚠ | ✓ | ✓ | ✓ | ✓ |
| GPIO | ⚠ | ✓ | ✓ | ✓ | ✓ |
| SD Card | ⚠ | ✓ | ✓ | ✓ | ✓ |
| USB | ✗ | ✓ | ✓ | ✓ | ✓ |
| Ethernet | ✗ | ✗ | ✓ | ✓ | ✓ |
| DMA | ✗ | ✓ | ✓ | ✓ | ✓ |
| I2C/SPI | ✗ | ✓ | ✓ | ✓ | ✓ |

✓ = Full support, ⚠ = Partial support, ✗ = Not supported

---

## Continuous Integration

### Example CI Configuration

```yaml
# .github/workflows/qemu-test.yml
name: QEMU Boot Test

on: [push, pull_request]

jobs:
  qemu-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y qemu-system-arm gcc-aarch64-linux-gnu

      - name: Build bootloader
        run: |
          cd bootloader
          make clean
          make

      - name: Test in QEMU
        run: |
          cd bootloader
          timeout 10 make qemu-test 2>&1 | tee qemu_output.log || true

      - name: Verify boot
        run: |
          grep -q "Custom Raspberry Pi Bootloader" bootloader/qemu_output.log
          grep -q "Detected Raspberry Pi" bootloader/qemu_output.log
          grep -q "BCM2837" bootloader/qemu_output.log
```

---

## Summary

### QEMU is Suitable For:
- ✓ Initial development
- ✓ Boot sequence logic testing
- ✓ FSA state machine verification
- ✓ UART communication testing
- ✓ Memory management testing
- ✓ Security logic testing (not real crypto)
- ✓ Rapid iteration during development
- ✓ Automated CI/CD testing

### Hardware Testing Required For:
- Full peripheral validation (GPIO, I2C, SPI, DMA)
- Network boot (Ethernet, Wi-Fi)
- USB device enumeration
- Real-world timing and performance
- Hardware-specific quirks
- Production validation

---

## Additional Resources

- [QEMU Documentation](https://www.qemu.org/docs/master/)
- [QEMU Raspberry Pi Support](https://www.qemu.org/docs/master/system/arm/raspi.html)
- [Raspberry Pi Hardware Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
- [ARM Architecture Reference Manual](https://developer.arm.com/architectures)

---

**Version**: 1.0
**Last Updated**: 2025-10-18
**Author**: ARM Native Bootloader Project
**License**: MIT
