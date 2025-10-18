#!/bin/bash
# QEMU Boot Script for ARM Native Bootloader
# Boots the bootloader in QEMU raspi3b emulation without GPU

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  QEMU ARM Virtual Machine Boot Test${NC}"
echo -e "${GREEN}======================================${NC}"
echo ""

# Configuration
BOOTLOADER_BIN="bootloader.bin"
QEMU_MACHINE="virt"
MEMORY_SIZE="1G"
SERIAL_OUTPUT="stdio"
KERNEL_IMAGE=""
DTB_FILE=""
SD_CARD_IMAGE=""
EXTRA_ARGS=""

# Parse command line arguments
BOOT_MODE="direct"  # direct, sd, or kernel

while [[ $# -gt 0 ]]; do
    case $1 in
        --bootloader)
            BOOTLOADER_BIN="$2"
            shift 2
            ;;
        --kernel)
            KERNEL_IMAGE="$2"
            BOOT_MODE="kernel"
            shift 2
            ;;
        --dtb)
            DTB_FILE="$2"
            shift 2
            ;;
        --sd)
            SD_CARD_IMAGE="$2"
            BOOT_MODE="sd"
            shift 2
            ;;
        --memory)
            MEMORY_SIZE="$2"
            shift 2
            ;;
        --debug)
            EXTRA_ARGS="$EXTRA_ARGS -d int,cpu_reset -D qemu.log"
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --bootloader FILE   Bootloader binary (default: bootloader.bin)"
            echo "  --kernel FILE       Kernel image to load"
            echo "  --dtb FILE          Device tree blob"
            echo "  --sd FILE           SD card image"
            echo "  --memory SIZE       Memory size (default: 1G)"
            echo "  --debug             Enable QEMU debug output"
            echo "  --help              Show this help message"
            echo ""
            echo "Boot modes:"
            echo "  1. Direct bootloader (default)"
            echo "     $0"
            echo ""
            echo "  2. Bootloader with SD card"
            echo "     $0 --sd sdcard.img"
            echo ""
            echo "  3. Direct kernel boot (bypass bootloader)"
            echo "     $0 --kernel kernel8.img --dtb bcm2710-rpi-3-b.dtb"
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Unknown option $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check if QEMU is installed
if ! command -v qemu-system-aarch64 &> /dev/null; then
    echo -e "${RED}Error: qemu-system-aarch64 not found${NC}"
    echo "Please install QEMU:"
    echo "  macOS: brew install qemu"
    echo "  Linux: sudo apt-get install qemu-system-arm"
    exit 1
fi

# Check QEMU version
QEMU_VERSION=$(qemu-system-aarch64 --version | head -1)
echo -e "${GREEN}QEMU Version:${NC} $QEMU_VERSION"

# Check if bootloader exists
if [ "$BOOT_MODE" = "direct" ] || [ "$BOOT_MODE" = "sd" ]; then
    if [ ! -f "$BOOTLOADER_BIN" ]; then
        echo -e "${RED}Error: Bootloader binary not found: $BOOTLOADER_BIN${NC}"
        echo "Build it first with: make"
        exit 1
    fi
    BOOTLOADER_SIZE=$(stat -f%z "$BOOTLOADER_BIN" 2>/dev/null || stat -c%s "$BOOTLOADER_BIN" 2>/dev/null)
    echo -e "${GREEN}Bootloader:${NC} $BOOTLOADER_BIN ($BOOTLOADER_SIZE bytes)"

    # Also check for ELF file
    BOOTLOADER_ELF="${BOOTLOADER_BIN%.bin}.elf"
    if [ -f "$BOOTLOADER_ELF" ]; then
        echo -e "${GREEN}ELF file:${NC} $BOOTLOADER_ELF available"
    fi
fi

# Build QEMU command based on boot mode
QEMU_CMD="qemu-system-aarch64 -M $QEMU_MACHINE -m $MEMORY_SIZE"

case $BOOT_MODE in
    direct)
        echo -e "${YELLOW}Boot Mode:${NC} Direct bootloader"
        # Use -bios to load bootloader at address 0
        QEMU_CMD="$QEMU_CMD -cpu cortex-a72 -bios $BOOTLOADER_BIN"
        ;;
    sd)
        echo -e "${YELLOW}Boot Mode:${NC} Bootloader with SD card"
        if [ ! -f "$SD_CARD_IMAGE" ]; then
            echo -e "${RED}Error: SD card image not found: $SD_CARD_IMAGE${NC}"
            exit 1
        fi
        SD_SIZE=$(stat -f%z "$SD_CARD_IMAGE" 2>/dev/null || stat -c%s "$SD_CARD_IMAGE" 2>/dev/null)
        echo -e "${GREEN}SD Card:${NC} $SD_CARD_IMAGE ($SD_SIZE bytes)"
        # For SD card boot, we still need to load the bootloader
        BOOTLOADER_ELF="${BOOTLOADER_BIN%.bin}.elf"
        if [ -f "$BOOTLOADER_ELF" ]; then
            QEMU_CMD="$QEMU_CMD -kernel $BOOTLOADER_ELF -drive file=$SD_CARD_IMAGE,if=sd,format=raw"
        else
            QEMU_CMD="$QEMU_CMD -device loader,file=$BOOTLOADER_BIN,addr=0x80000 -drive file=$SD_CARD_IMAGE,if=sd,format=raw"
        fi
        ;;
    kernel)
        echo -e "${YELLOW}Boot Mode:${NC} Direct kernel (bypass bootloader)"
        if [ ! -f "$KERNEL_IMAGE" ]; then
            echo -e "${RED}Error: Kernel image not found: $KERNEL_IMAGE${NC}"
            exit 1
        fi
        KERNEL_SIZE=$(stat -f%z "$KERNEL_IMAGE" 2>/dev/null || stat -c%s "$KERNEL_IMAGE" 2>/dev/null)
        echo -e "${GREEN}Kernel:${NC} $KERNEL_IMAGE ($KERNEL_SIZE bytes)"
        QEMU_CMD="$QEMU_CMD -kernel $KERNEL_IMAGE"
        if [ -n "$DTB_FILE" ]; then
            if [ ! -f "$DTB_FILE" ]; then
                echo -e "${RED}Error: DTB file not found: $DTB_FILE${NC}"
                exit 1
            fi
            echo -e "${GREEN}DTB:${NC} $DTB_FILE"
            QEMU_CMD="$QEMU_CMD -dtb $DTB_FILE"
        fi
        ;;
esac

# Add common options
QEMU_CMD="$QEMU_CMD -serial $SERIAL_OUTPUT -d int,cpu_reset,guest_errors -D qemu_debug.log"

# Add extra arguments
if [ -n "$EXTRA_ARGS" ]; then
    # When debug mode is enabled, disable the monitor to avoid stdio conflict
    QEMU_CMD="$QEMU_CMD -monitor none $EXTRA_ARGS"
else
    # Normal mode: allow monitor (though it won't interfere without debug flags)
    QEMU_CMD="$QEMU_CMD"
fi

echo ""
echo -e "${GREEN}Configuration:${NC}"
echo "  Machine:  $QEMU_MACHINE"
echo "  Memory:   $MEMORY_SIZE"
echo "  Serial:   $SERIAL_OUTPUT"
echo "  Graphics: Default (SDL/window)"
echo "  Debug:    Enabled (qemu_debug.log)"
echo ""
echo -e "${YELLOW}QEMU Command:${NC}"
echo "  $QEMU_CMD"
echo ""
echo -e "${GREEN}Starting QEMU...${NC}"
echo -e "${YELLOW}Press Ctrl-A then X to exit QEMU${NC}"
echo ""
echo "========================================"
echo ""

# Run QEMU
echo "Running: $QEMU_CMD"
echo "Bootloader output should appear below (press Ctrl-C to stop):"
echo ""

# Run QEMU in foreground so we can see output
$QEMU_CMD

echo ""
echo "QEMU test completed"
