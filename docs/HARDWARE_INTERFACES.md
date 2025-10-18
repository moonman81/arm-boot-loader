# Hardware Interfaces

This document details all hardware interfaces and BSP (Board Support Package) drivers implemented in the bootloader.

## Core Drivers (Critical)

These drivers are essential for basic bootloader operation.

### UART Controller

Serial communication interface for debugging and logging.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Baud Rate | ✅ 115200 | Configurable serial communication |
| I/O Modes | ✅ Blocking/Non-blocking | Flexible communication patterns |
| String Output | ✅ Full implementation | Formatted string printing |
| Character I/O | ✅ Implemented | Individual character handling |
| Debug Logging | ✅ Integrated | System-wide logging support |

### GPIO Controller

General-purpose input/output pin management.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Pin Modes | ✅ Input/Output | Digital I/O configuration |
| Pull Resistors | ✅ Up/Down/None | Input signal conditioning |
| LED Control | ✅ GPIO_LED_PIN | Status indication |
| Button Input | ✅ GPIO_BTN_PIN | User input handling |
| Pin Toggling | ✅ Implemented | Signal state changes |

### System Timer

High-precision timing and scheduling.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Precision | ✅ Microseconds | High-resolution timing |
| Delay Functions | ✅ Millisecond delays | Timing utilities |
| Counter Access | ✅ Direct access | Raw timer values |
| ARM Timer | ✅ Interrupt support | Periodic interrupts |
| Event Scheduling | ✅ Timer-based events | Scheduled operations |

### Interrupt Controller

GIC-400 interrupt handling and dispatching.

| Feature | Implementation | Details |
|---------|----------------|---------|
| IRQ Registration | ✅ Handler registration | Interrupt callback setup |
| IRQ Dispatching | ✅ Automatic routing | Handler execution |
| Timer Interrupts | ✅ ARM timer support | Periodic timer events |
| GPIO Interrupts | ✅ Pin change detection | External interrupt handling |
| Interrupt Control | ✅ Enable/Disable | Interrupt masking |

## Important Drivers

Critical for boot process functionality.

### Mailbox Interface

Communication with VideoCore GPU firmware.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Firmware Revision | ✅ Query support | Version information |
| Property Channel | ✅ Message passing | GPU communication |
| Hardware Detection | ✅ Model identification | Device capability discovery |

### Clock Manager

Peripheral clock configuration and control.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Clock Initialization | ✅ Basic setup | Clock tree configuration |
| Frequency Management | ✅ Rate control | Clock frequency settings |
| Peripheral Clocks | ✅ Device clocks | Individual peripheral enabling |

### SD Host Controller

Storage access and filesystem support.

| Feature | Implementation | Details |
|---------|----------------|---------|
| SD Card Init | ✅ Detection/initialization | Card presence and setup |
| FAT Filesystem | ✅ Basic support | File reading capabilities |
| File Loading | ✅ Kernel/initrd loading | Binary file access |
| Sector I/O | ✅ Block operations | Low-level storage access |

## Optional Drivers

Extended peripheral support.

### DMA Controller

Direct memory access for efficient data movement.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Memory Transfers | ✅ Basic support | Memory-to-memory operations |
| Peripheral DMA | ⏳ Planned | Device-to-memory transfers |
| Efficiency | ✅ High throughput | Reduced CPU overhead |

### I2C Controller

Inter-integrated circuit communication.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Multi-bus Support | ✅ I2C_BUS_0 | Multiple I2C interfaces |
| Sensor Communication | ⏳ Planned | Environmental sensors |
| EEPROM Access | ⏳ Planned | Non-volatile storage |

### SPI Controller

Serial peripheral interface for high-speed data.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Multi-bus Support | ✅ SPI_BUS_0 | Multiple SPI interfaces |
| High-speed Transfer | ⏳ Planned | Fast data communication |
| Peripheral Control | ⏳ Planned | SPI device management |

### PWM Controller

Pulse-width modulation for analog signals.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Signal Generation | ✅ Basic PWM | Analog signal creation |
| Motor Control | ⏳ Planned | DC motor speed control |
| LED Dimming | ⏳ Planned | Brightness control |

### USB Controller

Universal serial bus peripheral support.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Mass Storage | ✅ Detection | USB drive recognition |
| USB Boot | ✅ Basic support | Boot from USB devices |
| Peripheral Connection | ⏳ Planned | General USB device support |

### Ethernet Controller

Network connectivity and communication.

| Feature | Implementation | Details |
|---------|----------------|---------|
| L2 Networking | ⏳ Planned | Ethernet frame handling |
| L3 Networking | ⏳ Planned | IP protocol support |
| Network Boot | ⏳ Placeholder | PXE/TFTP boot support |

## Hardware Detection

Automatic Raspberry Pi model detection and configuration.

| Feature | Implementation | Details |
|---------|----------------|---------|
| Model Identification | ✅ Auto-detection | Pi 3/4/5 recognition |
| Capability Reporting | ✅ Feature flags | Hardware capability enumeration |
| Model-specific Tuning | ✅ Configuration | Optimized settings per model |
| SoC Information | ✅ BCMxxxx display | Processor identification |

## Driver Architecture

All drivers follow a consistent architecture:

| Layer | Description | Examples |
|-------|-------------|----------|
| Hardware Abstraction | Register access macros | `mmio_write()`, `mmio_read()` |
| Driver Core | Device-specific logic | UART transmit/receive |
| API Layer | Public interfaces | `uart_puts()`, `gpio_set()` |
| Integration | BSP coordination | Initialization ordering |

## Memory Mapping

Hardware peripherals are memory-mapped at standard Raspberry Pi addresses:

| Peripheral | Base Address | Size |
|------------|--------------|------|
| GPIO Controller | 0xFE200000 | 0x1000 |
| UART | 0xFE201000 | 0x1000 |
| System Timer | 0xFE003000 | 0x1000 |
| ARM Timer | 0xFE00B000 | 0x1000 |
| Interrupt Controller | 0xFF841000 | 0x1000 |
| Mailbox | 0xFE00B000 | 0x1000 |
| DMA Controller | 0xFE007000 | 0x1000 |
| I2C | 0xFE804000 | 0x1000 |
| SPI | 0xFE204000 | 0x1000 |
| PWM | 0xFE20C000 | 0x1000 |
| USB | 0xFE980000 | 0x1000 |
| Ethernet | 0xFE300000 | 0x1000 |

## Testing and Validation

See [Testing & Validation](./TESTING_VALIDATION.md) for detailed testing framework information.

All drivers include comprehensive test suites:

| Test Type | Coverage | Implementation |
|-----------|----------|----------------|
| Basic Functionality | ✅ All drivers | GPIO, timer, UART testing |
| Integration Tests | ✅ Core drivers | Inter-driver communication |
| Error Handling | ✅ Critical paths | Failure mode testing |
| Performance | ✅ Timing-critical | Latency and throughput measurement |

The BSP provides complete hardware abstraction for Raspberry Pi 3, 4, and 5 models with consistent APIs and comprehensive testing.