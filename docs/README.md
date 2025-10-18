# Custom Raspberry Pi Bootloader

A minimal, reliable bootloader for Raspberry Pi devices that boots Linux kernels and initrd images without using U-Boot or RedBoot.

## Features

- **Minimal Size**: < 1KB binary
- **Reliable**: FSA-based state machine with error recovery
- **Custom**: No external dependencies
- **ARM64 Support**: Built for Raspberry Pi 4/5
- **UART Debugging**: Serial output for troubleshooting

## Building

### Prerequisites
- macOS with Homebrew
- ARM64 cross-compiler toolchain

### Setup
```bash
brew install aarch64-elf-gcc
make
```

### Output
- `bootloader.bin`: The bootloader binary
- `bootloader.elf`: ELF file for debugging

## Usage

1. Copy `bootloader.bin` to the boot partition of an SD card
2. Place `kernel8.img` and `initramfs` (optional) in the boot partition
3. Create `config.txt` with kernel parameters
4. Boot the Raspberry Pi

## Architecture

### FSA States
1. Bootcode Loading
2. Bootcode Execution
3. Start.elf Loading
4. Start.elf Execution
5. Kernel Loading
6. Kernel Execution
7. Success

### Components
- `start.S`: ARM64 assembly entry point
- `main.c`: FSA state machine implementation
- `uart.c`: UART driver for debugging
- `sd.c`: SD card file loading
- `config.c`: Configuration parsing
- `gpio.c/h`: GPIO controller driver
- `timer.c/h`: System timer driver
- `interrupt.c/h`: GIC-400 interrupt controller
- `mailbox.c/h`: VideoCore mailbox interface
- `clock.c/h`: Clock manager driver
- `memory.c/h`: Memory manager
- `dma.c/h`: DMA controller driver
- `i2c.c/h`: I2C controller driver
- `spi.c/h`: SPI controller driver
- `pwm.c/h`: PWM controller driver
- `usb.c/h`: USB controller driver
- `ethernet.c/h`: Ethernet controller driver
- `dtb.c/h`: Device tree service
- `test_drivers.c`: Driver test suite

## Testing

Run the comprehensive test suite:
```bash
python3 test_bootloader.py
```

Run FSA verification:
```bash
python3 verify_fsa.py
```

## Board Support Package (BSP)

This bootloader includes a complete BSP for Raspberry Pi 4/5 with the following drivers:

### Core Drivers (Critical)
- **GPIO Controller**: Pin control, LED control, button input
- **System Timer**: High-precision timing, delays, scheduling
- **Interrupt Controller**: GIC-400 interrupt handling
- **UART**: Serial communication for debugging

### Important Drivers
- **Mailbox Interface**: Communication with VideoCore firmware
- **Clock Manager**: Peripheral clock configuration
- **SD Host Controller**: Storage access and filesystem support

### Optional Drivers
- **DMA Controller**: Efficient data movement
- **I2C Controller**: Peripheral communication (sensors, EEPROM)
- **SPI Controller**: High-speed data transfer
- **PWM Controller**: Analog signal generation
- **USB Controller**: Peripheral connections
- **Ethernet Controller**: Network connectivity

### System Services
- **Memory Manager**: Heap allocation/deallocation
- **Interrupt Service**: IRQ dispatching and registration
- **Timer Service**: Timer-based events
- **Device Tree Service**: Hardware discovery and configuration
- **Firmware Interface**: VideoCore property access

All drivers are implemented according to the formal N3 ontology specifications in `../rpi_bsp_requirements.n3`.

## Development Status

- ✅ Phase 1: Minimal boot (completed)
- ✅ Phase 2: Enhanced features (SD loading, config parsing, BSP)
- 🔄 Phase 3: Reliability and security (ongoing)
- ⏳ Phase 4: Optimization

## Mathematical Foundation

This bootloader is designed based on formal mathematical models:
- Finite State Automata (FSA) for boot process
- Kripke semantics for modal logic
- Tarski model theory for truth conditions
- Grothendieck category theory for configurations
- Scott domain theory for state ordering
- Type theory for safe transitions
- Homotopy theory for equivalent paths

See the ontologies in the parent directory for formal specifications.

## License

This project is open-source and provided as-is for educational purposes.