# Custom Raspberry Pi Bootloader Configuration Guide

This guide provides comprehensive instructions for configuring, building, and deploying the custom Raspberry Pi bootloader.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building the Bootloader](#building-the-bootloader)
3. [Configuration Options](#configuration-options)
4. [Deployment](#deployment)
5. [Hardware Setup](#hardware-setup)
6. [Testing and Validation](#testing-and-validation)
7. [Troubleshooting](#troubleshooting)
8. [Advanced Configuration](#advanced-configuration)

## Prerequisites

### Software Requirements
- **Operating System**: macOS (with Homebrew) or Linux
- **Cross-Compiler**: ARM64 GCC toolchain
- **Build Tools**: Make, Git
- **Testing Tools**: QEMU (optional, for emulation)

### Hardware Requirements
- **Raspberry Pi**: Model 4 or 5 (primary targets)
- **Storage**: MicroSD card (formatted as FAT32)
- **Serial Console**: USB-to-TTL adapter (for debugging)
- **Power Supply**: Official Raspberry Pi power supply

### Installation
```bash
# Install ARM64 toolchain (macOS)
brew install aarch64-elf-gcc

# Clone repository (if applicable)
git clone <repository-url>
cd bootloader
```

## Building the Bootloader

### Basic Build
```bash
# Clean previous build
make clean

# Build bootloader
make

# Verify build
ls -la bootloader.bin
```

Expected output:
```
-rw-r--r--  1 user  staff  1030 Dec 31 12:00 bootloader.bin
```

### Build Options
```bash
# Check toolchain
make check-toolchain

# Run static analysis
make static-analysis

# Run all validations
make validate

# Clean build
make clean
```

### Build Configuration
Edit `Makefile` for custom settings:

```makefile
# Change cross-compiler prefix
CROSS_COMPILE ?= aarch64-elf-

# Modify compiler flags
CFLAGS = -Wall -Wextra -O2 -ffreestanding -nostdlib -nostartfiles
```

## Configuration Options

### UART Configuration
The bootloader uses UART for debugging output. Configure in `uart.c`:

```c
#define UART_BASE 0xFE201000  // Pi 4
// #define UART_BASE 0xFE201400  // Pi 5 (if needed)

#define UART_BAUD_RATE 115200
```

### Memory Layout
Configure memory addresses in `linker.ld`:

```ld
. = 0x80000;  /* Bootloader load address */
KERNEL_LOAD_ADDR = 0x80000;  /* Kernel load address */
INITRD_LOAD_ADDR = 0x1000000; /* Initrd load address */
```

### Boot States
Modify FSA states in `main.c`:

```c
typedef enum {
    STATE_BOOTCODE_LOADING,
    STATE_BOOTCODE_EXEC,
    STATE_STARTELF_LOADING,  // Can be disabled for minimal boot
    STATE_STARTELF_EXEC,
    STATE_KERNEL_LOADING,
    STATE_KERNEL_EXEC,
    STATE_SUCCESS,
    STATE_FAILURE
} boot_state_t;
```

### File Paths
Configure expected file locations in `main.c`:

```c
#define KERNEL_FILENAME "kernel8.img"
#define INITRD_FILENAME "initramfs"
#define CONFIG_FILENAME "config.txt"
```

## Deployment

### Prepare SD Card
1. **Format SD Card**:
   ```bash
   # macOS
   diskutil list
   diskutil eraseDisk FAT32 RASPI MBRFormat /dev/diskX
   ```

2. **Copy Files**:
   ```bash
   # Mount SD card (adjust path)
   cp bootloader.bin /Volumes/RASPI/
   cp kernel8.img /Volumes/RASPI/
   cp initramfs /Volumes/RASPI/  # Optional
   cp config.txt /Volumes/RASPI/  # Optional
   ```

### Bootloader Placement
- Place `bootloader.bin` in the root of the boot partition
- Ensure it's the first file loaded by the Pi's ROM

### Kernel Preparation
- Compile Linux kernel for ARM64:
  ```bash
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
  make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image
  ```

- Rename kernel image:
  ```bash
  cp arch/arm64/boot/Image kernel8.img
  ```

## Hardware Setup

### Raspberry Pi 4 Setup
1. **Insert SD Card** with bootloader
2. **Connect Serial Console**:
   - GPIO 14 (TX) → RX on USB-TTL
   - GPIO 15 (RX) → TX on USB-TTL
   - Ground → Ground
3. **Power On** the Raspberry Pi
4. **Monitor Output**:
   ```bash
   screen /dev/tty.usbserial 115200
   ```

### Serial Output
Expected boot sequence output:
```
Custom Raspberry Pi Bootloader v1.0
Loading bootcode...
Executing bootcode...
Loading start.elf...
Executing start.elf...
Loading kernel...
Executing kernel...
Handing over to kernel...
```

### LED Indicators
- **Green LED**: Steady = success, Blinking = loading
- **Red LED**: Steady = error, Off = power issue

## Testing and Validation

### Static Testing
```bash
# Run all validations
make validate

# Individual tests
python3 static_analysis.py
python3 validate_ontology.py
python3 code_metrics.py
python3 security_analysis.py
```

### Emulation Testing (QEMU)
```bash
# Build for QEMU
make

# Run in emulator
make qemu-test
```

### Hardware Testing
1. **Minimal Test**: Boot without kernel
2. **Kernel Test**: Boot with kernel only
3. **Full Test**: Boot with kernel + initrd + config

### Validation Checklist
- [ ] Bootloader builds without errors
- [ ] Static analysis passes
- [ ] Ontology validation passes
- [ ] Serial output appears
- [ ] Kernel handover successful
- [ ] Linux boots to prompt

## Troubleshooting

### Build Issues
**Problem**: `aarch64-elf-gcc: command not found`
**Solution**: Install toolchain with `brew install aarch64-elf-gcc`

**Problem**: Linker errors
**Solution**: Check `linker.ld` syntax and symbol definitions

### Boot Issues
**Problem**: No serial output
**Solution**:
- Check UART connections
- Verify baud rate (115200)
- Ensure bootloader.bin is in boot partition root

**Problem**: Kernel not found
**Solution**:
- Verify kernel8.img exists
- Check file system format (FAT32)
- Ensure correct filename

**Problem**: Boot hangs
**Solution**:
- Check LED status
- Review serial logs
- Verify hardware connections

### Debug Tips
1. **Enable Verbose Output**: Modify `uart_puts` calls in `main.c`
2. **Add Debug Points**: Insert `uart_puts("DEBUG: step X\n");` at key points
3. **Test Components**: Comment out sections to isolate issues

## Advanced Configuration

### Custom Device Tree
Create `custom.dtb` and modify bootloader to load it:

```c
// In main.c
#define DTB_FILENAME "custom.dtb"
#define DTB_LOAD_ADDR 0x1000

// Load DTB before kernel handover
sd_load_file(DTB_FILENAME, DTB_LOAD_ADDR);
```

### Secure Boot
For secure boot support (future implementation):

1. Generate key pair
2. Sign bootloader and kernel
3. Enable signature verification in bootloader

### Multi-Core Support
Enable SMP in kernel handover:

```c
// Set CPU masks for multi-core
regs.x0 |= (1 << 0);  // CPU 0
regs.x0 |= (1 << 1);  // CPU 1 (if available)
```

### Network Boot
Future extension for PXE boot:

1. Implement Ethernet driver
2. Add DHCP client
3. Load files via TFTP

### Performance Optimization
- **Size Reduction**: Strip debug symbols
- **Speed Optimization**: Use faster memory access patterns
- **Power Management**: Implement sleep modes

## Configuration Examples

### Minimal Configuration
```
# Files on SD card
bootloader.bin
kernel8.img
```

### Full Configuration
```
# Files on SD card
bootloader.bin
kernel8.img
initramfs
config.txt
custom.dtb
```

### config.txt Example
```ini
# Kernel parameters
kernel=kernel8.img
initramfs initramfs followkernel

# Hardware settings
gpu_mem=128
hdmi_mode=1
dtparam=audio=on

# Boot options
boot_delay=0
disable_splash=1
```

## Support and Resources

### Documentation
- `README.md`: Project overview
- `VALIDATION_REPORT.md`: Testing results
- Ontologies: Formal specifications

### Community
- GitHub Issues: Report bugs
- Discussions: Share configurations

### Development
- `Makefile`: Build customization
- Source files: Well-commented for modification

This configuration guide ensures successful deployment and operation of the custom bootloader across different Raspberry Pi models and use cases.