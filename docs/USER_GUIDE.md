# User Guide: ARM Native Bootloader

This guide provides practical instructions for using and troubleshooting the ARM Native Bootloader on Raspberry Pi devices.

## Quick Start

### Basic Setup

1. **Compile the bootloader:**
   ```bash
   cd bootloader
   make
   ```

2. **Flash to SD card:**
   ```bash
   # Replace /dev/sdX with your SD card device
   sudo dd if=bootloader.bin of=/dev/sdX bs=4M
   ```

3. **Boot your Raspberry Pi:**
   - Insert the SD card
   - Connect UART for debugging (optional but recommended)
   - Power on the device

### Expected Output

When booting successfully, you should see:
```
Custom Raspberry Pi Bootloader v1.0
Complete BSP Initialized
Detected Raspberry Pi: Raspberry Pi 4 Model B
SoC: BCM2711, 4 cores, 8192MB RAM
Capabilities: ETH WiFi BT USB3 PCIe
Power-on detected
Early hardware initialization...
Selecting bootcode source...
Loading bootcode...
Validating bootcode...
Executing bootcode...
...
Loading kernel...
Validating kernel...
Executing kernel...
Handing over to kernel...
Boot successful!
Total boot time: 1250 ms
```

## Configuration

### config.txt Options

The bootloader reads `config.txt` from the SD card root. Key options:

| Option | Description | Example |
|--------|-------------|---------|
| `kernel` | Kernel filename | `kernel=kernel8.img` |
| `initramfs` | Initrd filename | `initramfs initrd.img` |
| `device_tree` | DTB filename | `device_tree=bcm2711-rpi-4-b.dtb` |
| `bootcode_source` | Boot source for bootcode | `bootcode_source=sd` |
| `startelf_source` | Boot source for start.elf | `startelf_source=sd` |
| `kernel_source` | Boot source for kernel | `kernel_source=sd` |

### Boot Source Configuration

Configure alternative boot sources:

```ini
# Enable USB boot
enable_usb_boot=1
kernel_source=usb

# Enable network boot
enable_network_boot=1
kernel_source=network

# Enable modular boot
enable_modular_boot=1
kernel_source=module
```

## Boot Sources

### SD Card Boot (Default)

- **Requirements:** FAT32 formatted SD card with boot files
- **Files needed:** `bootcode.bin`, `start.elf`, `kernel8.img`, `config.txt`
- **Configuration:** `kernel_source=sd`

### USB Mass Storage Boot

- **Requirements:** USB drive with boot files, `enable_usb_boot=1`
- **Configuration:** `kernel_source=usb`
- **Expected output:** "USB mass storage device found"

### Network Boot

- **Requirements:** Ethernet connection, DHCP server, TFTP server
- **Configuration:** `kernel_source=network`, `enable_network_boot=1`
- **Note:** Currently placeholder implementation

## Troubleshooting

### Common Issues

#### Bootloader Doesn't Start

**Symptoms:** No UART output, LED doesn't blink

**Solutions:**
1. Check SD card formatting (must be FAT32)
2. Verify `bootloader.bin` is in the correct location
3. Ensure proper power supply (5V, 3A minimum)
4. Check UART connection (115200 baud, 8N1)

#### "SD init failed" Error

**Symptoms:** Boot stops at "Loading bootcode..." with retry messages

**Solutions:**
1. Check SD card insertion
2. Try a different SD card
3. Format SD card as FAT32
4. Check for SD card corruption

#### "Kernel load failed" Error

**Symptoms:** Boot progresses but fails at kernel loading

**Solutions:**
1. Verify `kernel8.img` exists on SD card
2. Check filename in `config.txt` matches actual file
3. Ensure kernel is compatible with Raspberry Pi model
4. Check SD card space and file integrity

#### "Security attestation failed" Error

**Symptoms:** Boot fails during security checks

**Solutions:**
1. This indicates security features are enabled but not properly configured
2. Check if secure boot is required for your use case
3. Disable security features if not needed: `enable_security=0`

### Debug Output

#### Enabling Verbose Logging

The bootloader provides detailed UART output. To capture logs:

```bash
# Using screen
screen /dev/ttyUSB0 115200

# Using minicom
minicom -b 115200 -o -D /dev/ttyUSB0

# Using Python
python3 -c "
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
while True:
    print(ser.readline().decode('utf-8'), end='')
"
```

#### Interpreting Boot States

Each boot state is logged. Common successful sequence:
1. `Power-on detected`
2. `Early hardware initialization...`
3. `Selecting bootcode source...`
4. `Loading bootcode...`
5. `Executing bootcode...`
6. `Loading kernel...`
7. `Executing kernel...`
8. `Handing over to kernel...`
9. `Boot successful!`

### Hardware-Specific Issues

#### Raspberry Pi 4 Issues

- **USB boot problems:** Ensure USB device is connected before power-on
- **Ethernet issues:** Check cable and network configuration
- **Memory detection:** Pi 4 should show 4GB or 8GB RAM

#### Raspberry Pi 3 Issues

- **WiFi/Bluetooth:** May not be detected on older models
- **USB 3.0:** Not available on Pi 3B/B+
- **Performance:** Expect slower boot times

### Recovery Procedures

#### Emergency Boot

If normal boot fails, the bootloader will attempt recovery:

1. **Automatic retry:** Failed operations retry up to 3 times
2. **State rollback:** Returns to previous valid state
3. **Failsafe mode:** Attempts alternative boot sources

#### Manual Recovery

1. **Safe mode boot:** Create `config.txt` with minimal options:
   ```
   kernel=kernel8.img
   bootcode_source=sd
   startelf_source=sd
   kernel_source=sd
   enable_security=0
   ```

2. **USB recovery:** Boot from USB drive with known-good files

3. **Network recovery:** Use network boot if available

## Performance Tuning

### Optimizing Boot Time

1. **Disable unused features:**
   ```
   enable_network_boot=0
   enable_usb_boot=0
   enable_security=0
   ```

2. **Use faster SD card:** Class 10 or UHS-I recommended

3. **Minimize config.txt:** Remove unused options

### Monitoring Performance

Boot time is logged at completion:
```
Total boot time: 1250 ms
```

Typical boot times:
- **SD card boot:** 1000-2000ms
- **USB boot:** 1500-2500ms
- **Network boot:** 3000-5000ms (when implemented)

## Advanced Usage

### Custom Configuration

#### Model-Specific Tuning

The bootloader auto-detects Pi model and applies optimizations:

```c
// In main.c - automatic model detection
const pi_model_info_t *model_info = hardware_get_model_info();
hardware_apply_model_tuning();
hardware_apply_model_quirks();
```

#### Custom Boot Scripts

For complex boot scenarios, modify `config.txt` with conditional logic:

```ini
# Model-specific kernels
[pi4]
kernel=kernel8.img
device_tree=bcm2711-rpi-4-b.dtb

[pi3]
kernel=kernel7.img
device_tree=bcm2710-rpi-3-b.dtb
```

### Development and Testing

#### Running Tests

```bash
cd bootloader
make static-analysis    # Code quality checks
make ontology-validation # Specification compliance
make validate-docs      # Documentation accuracy
make generate-docs      # Update documentation
```

#### QEMU Testing

Test without hardware:

```bash
make qemu-test
```

#### Hardware Testing

```bash
make hw-test-full
```

## Support and Resources

### Getting Help

1. **Check UART logs:** Most issues are logged to serial console
2. **Verify configuration:** Ensure `config.txt` is correct
3. **Test with minimal config:** Simplify to isolate issues
4. **Check hardware:** Verify Pi model and peripherals

### Documentation Links

- [Boot Process](./BOOT_PROCESS.md) - Detailed FSA state machine
- [Hardware Interfaces](./HARDWARE_INTERFACES.md) - BSP driver documentation
- [Testing & Validation](./TESTING_VALIDATION.md) - Validation procedures
- [Mathematical Verification](./MATHEMATICAL_VERIFICATION.md) - Formal verification

### Community Resources

- GitHub Issues: Report bugs and request features
- Raspberry Pi Forums: Hardware-specific discussions
- Bootloader Wiki: Advanced configuration examples

---

**Note:** This bootloader is a research platform implementing advanced formal verification. For production use, consider the official Raspberry Pi bootloader.