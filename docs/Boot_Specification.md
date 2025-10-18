# Standardized Boot Specification for Raspberry Pi ARM Devices

This specification standardizes the boot process for Raspberry Pi devices, developed with community input.

## Version
1.0 - Initial release

## Scope
This specification covers the boot sequence, configuration, and firmware requirements for Raspberry Pi models 3, 4, and 5.

## Boot Sequence Standard

### Stage Definitions
1. **ROM Stage**: Hardware-initialized code that loads bootcode.bin
2. **Bootloader Stage**: bootcode.bin initializes SDRAM and loads start.elf
3. **Firmware Stage**: start.elf configures hardware and loads kernel
4. **Kernel Stage**: Linux kernel initializes OS

### Required Files
- bootcode.bin: Bootloader binary
- start.elf: GPU firmware
- config.txt: Boot configuration
- kernel.img/kernel8.img: Linux kernel

### Configuration Schema
config.txt must follow this schema:

```ini
# Basic configuration
kernel=kernel.img
ramfsfile=initramfs.gz
initramfs initrd.img followkernel

# Hardware settings
gpu_mem=128
hdmi_mode=1
dtparam=audio=on

# Boot options
boot_delay=0
disable_splash=1
```

### Secure Boot Extension
For secure boot capable devices:

- OTP bits must be programmed with key hash
- All firmware signed with ECDSA
- Boot failure on signature verification error

## Community Input
This specification is open for community contributions via GitHub issues and pull requests.

## References
- Raspberry Pi Firmware: https://github.com/raspberrypi/firmware
- Bootloader Source: https://github.com/raspberrypi/rpi-bootloader