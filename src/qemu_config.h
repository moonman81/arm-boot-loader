/* QEMU-Specific Configuration Header */

#ifndef QEMU_CONFIG_H
#define QEMU_CONFIG_H

#include <stdint.h>

/* QEMU emulation detection and configuration */

/* QEMU-specific peripheral base addresses for raspi3b */
/* QEMU emulates Raspberry Pi 3B (BCM2837) with base 0x3F000000 */
#define QEMU_PERIPHERAL_BASE    0x3F000000
#define QEMU_UART_BASE          0x3F201000
#define QEMU_GPIO_BASE          0x3F200000
#define QEMU_TIMER_BASE         0x3F003000
#define QEMU_INTERRUPT_BASE     0x3F00B200
#define QEMU_MAILBOX_BASE       0x3F00B880
#define QEMU_EMMC_BASE          0x3F300000

/* QEMU limitations and workarounds */
#define QEMU_EMULATION_ENABLED  1

/* Features available in QEMU */
#define QEMU_HAS_UART           1
#define QEMU_HAS_TIMER          1
#define QEMU_HAS_INTERRUPT      1
#define QEMU_HAS_MAILBOX        1
#define QEMU_HAS_GPIO           1    /* Limited emulation */
#define QEMU_HAS_SD             1    /* Via -drive parameter */

/* Features NOT available or limited in QEMU */
#define QEMU_HAS_ETHERNET       0    /* Not emulated in raspi3b */
#define QEMU_HAS_USB            0    /* Limited USB emulation */
#define QEMU_HAS_PWM            0    /* Not emulated */
#define QEMU_HAS_I2C            0    /* Not emulated */
#define QEMU_HAS_SPI            0    /* Not emulated */
#define QEMU_HAS_DMA            0    /* Limited emulation */
#define QEMU_HAS_WATCHDOG       0    /* Not emulated */

/* QEMU boot configuration */
#define QEMU_BOOT_MODE_SD       1    /* Boot from emulated SD card */
#define QEMU_BOOT_MODE_KERNEL   2    /* Direct kernel loading with -kernel */

/* Current boot mode (default to direct kernel) */
#ifndef QEMU_BOOT_MODE
#define QEMU_BOOT_MODE          QEMU_BOOT_MODE_KERNEL
#endif

/* QEMU detection at runtime */
/*
 * QEMU can be detected by checking mailbox responses
 * Real hardware returns specific firmware revision codes
 * QEMU often returns 0 or simplified values
 */
static inline int is_running_in_qemu(void) {
    /* Simple heuristic: QEMU mailbox returns different values */
    /* This is a placeholder - actual detection would check mailbox responses */
    return 0;  /* Assume real hardware by default */
}

/* QEMU-safe peripheral initialization */
/* These macros can be used to skip unsupported peripherals in QEMU */
#define QEMU_SKIP_IF_NOT_AVAILABLE(feature) \
    do { \
        if (!QEMU_HAS_##feature) { \
            uart_puts("[QEMU] Skipping " #feature " (not emulated)\n"); \
            return 0; \
        } \
    } while (0)

/* QEMU verbose boot messages */
#define QEMU_VERBOSE_BOOT       1

#if QEMU_VERBOSE_BOOT
#define QEMU_LOG(msg) uart_puts("[QEMU] " msg "\n")
#else
#define QEMU_LOG(msg)
#endif

/* QEMU memory layout (matches raspi3b) */
#define QEMU_MEMORY_BASE        0x00000000
#define QEMU_MEMORY_SIZE        (1024 * 1024 * 1024)  /* 1GB default */
#define QEMU_KERNEL_BASE        0x00080000             /* 512KB offset */
#define QEMU_DTB_BASE           0x02000000             /* 32MB offset */
#define QEMU_INITRD_BASE        0x02600000             /* 38MB offset */

/* QEMU testing configuration */
typedef struct {
    uint32_t machine_type;      /* raspi3b = 3 */
    uint32_t memory_mb;          /* Memory size in MB */
    uint8_t  enable_serial;      /* Enable serial console */
    uint8_t  enable_sd;          /* Enable SD card emulation */
    uint8_t  nographic;          /* Headless mode */
    uint8_t  debug_mode;         /* Enable QEMU debug output */
} qemu_config_t;

/* Default QEMU configuration */
static const qemu_config_t default_qemu_config = {
    .machine_type = 3,           /* raspi3b */
    .memory_mb = 1024,           /* 1GB */
    .enable_serial = 1,
    .enable_sd = 0,              /* Direct kernel mode by default */
    .nographic = 1,              /* Headless */
    .debug_mode = 0
};

/* QEMU command line generation hints */
/*
 * Basic QEMU boot:
 *   qemu-system-aarch64 -M raspi3b -kernel bootloader.bin -serial stdio -nographic
 *
 * With SD card:
 *   qemu-system-aarch64 -M raspi3b -kernel bootloader.bin \
 *                       -drive file=sdcard.img,if=sd,format=raw \
 *                       -serial stdio -nographic
 *
 * With DTB:
 *   qemu-system-aarch64 -M raspi3b -kernel bootloader.bin \
 *                       -dtb bcm2710-rpi-3-b.dtb \
 *                       -serial stdio -nographic
 *
 * With debug:
 *   qemu-system-aarch64 -M raspi3b -kernel bootloader.bin \
 *                       -serial stdio -nographic \
 *                       -d int,cpu_reset -D qemu.log
 */

#endif /* QEMU_CONFIG_H */
