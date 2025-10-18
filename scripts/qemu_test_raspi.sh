#!/bin/bash
# Simple QEMU test for Raspberry Pi bootloader
# This script uses the raspi3b machine which matches the bootloader's hardware

set -e

echo "======================================"
echo "  QEMU Raspberry Pi 3B Boot Test"
echo "======================================"
echo ""

# Check if bootloader exists
if [ ! -f "bootloader.bin" ]; then
    echo "Error: bootloader.bin not found"
    echo "Build it first with: make"
    exit 1
fi

BOOTLOADER_SIZE=$(stat -f%z "bootloader.bin" 2>/dev/null || stat -c%s "bootloader.bin" 2>/dev/null)
echo "Bootloader: bootloader.bin ($BOOTLOADER_SIZE bytes)"
echo ""

# Check if QEMU is installed
if ! command -v qemu-system-aarch64 &> /dev/null; then
    echo "Error: qemu-system-aarch64 not found"
    echo "Install with: brew install qemu (macOS)"
    exit 1
fi

QEMU_VERSION=$(qemu-system-aarch64 --version | head -1)
echo "QEMU: $QEMU_VERSION"
echo ""

echo "Configuration:"
echo "  Machine:  raspi3b (Raspberry Pi 3 Model B)"
echo "  CPU:      ARM Cortex-A53 (4 cores)"
echo "  Memory:   1GB"
echo "  Serial:   UART0 -> stdio"
echo "  Graphics: Disabled (headless)"
echo ""

echo "Starting QEMU..."
echo "Press Ctrl-A then X to exit"
echo ""
echo "======================================"
echo ""

# Run QEMU with correct settings for Raspberry Pi 3
exec qemu-system-aarch64 \
    -M raspi3b \
    -m 1G \
    -kernel bootloader.bin \
    -serial stdio \
    -nographic \
    -monitor none
