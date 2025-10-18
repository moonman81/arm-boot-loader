/* Hardware Configuration for Different Raspberry Pi Models */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include "pi_model.h"

// Peripheral base addresses for different Raspberry Pi models
// BCM2835 (Pi 1, Zero): 0x20xxxxxx
// BCM2836/BCM2837 (Pi 2, 3, Zero 2): 0x3Fxxxxxx
// BCM2711/BCM2712 (Pi 4, 5, 400): 0xFExxxxxx

// UART base addresses
#define UART_BASE_BCM2835 0x20201000  // Pi 1, Zero, Zero W
#define UART_BASE_BCM2837 0x3F201000  // Pi 2, 3, 3+, 3A+, Zero 2 W
#define UART_BASE_QEMU_VIRT 0x09000000  // QEMU virt machine UART
#define UART_BASE_BCM2711 0xFE201000  // Pi 4, 400
#define UART_BASE_BCM2712 0xFE201000  // Pi 5 (same as BCM2711)

// GPIO base addresses
#define GPIO_BASE_BCM2835 0x20200000  // Pi 1, Zero, Zero W
#define GPIO_BASE_BCM2837 0x3F200000  // Pi 2, 3, 3+, 3A+, Zero 2 W
#define GPIO_BASE_BCM2711 0xFE200000  // Pi 4, 400
#define GPIO_BASE_BCM2712 0xFE200000  // Pi 5 (same as BCM2711)

// System Timer base addresses
#define TIMER_BASE_BCM2835 0x20003000  // Pi 1, Zero, Zero W
#define TIMER_BASE_BCM2837 0x3F003000  // Pi 2, 3, 3+, 3A+, Zero 2 W
#define TIMER_BASE_BCM2711 0xFE003000  // Pi 4, 400
#define TIMER_BASE_BCM2712 0xFE003000  // Pi 5 (same as BCM2711)

// EMMC/SD base addresses
#define EMMC_BASE_BCM2835 0x20300000  // Pi 1, Zero, Zero W
#define EMMC_BASE_BCM2837 0x3F300000  // Pi 2, 3, 3+, 3A+, Zero 2 W
#define EMMC_BASE_BCM2711 0xFE340000  // Pi 4, 400
#define EMMC_BASE_BCM2712 0xFE340000  // Pi 5 (same as BCM2711)

// ARM Timer base addresses
#define ARM_TIMER_BASE_BCM2835 0x2000B000  // Pi 1, Zero, Zero W
#define ARM_TIMER_BASE_BCM2837 0x3F00B000  // Pi 2, 3, 3+, 3A+, Zero 2 W
#define ARM_TIMER_BASE_BCM2711 0xFE00B000  // Pi 4, 400
#define ARM_TIMER_BASE_BCM2712 0xFE00B000  // Pi 5 (same as BCM2711)

// Current model
extern pi_model_t current_pi_model;

// Get base addresses based on model
uint32_t get_uart_base(void);
uint32_t get_gpio_base(void);
uint32_t get_timer_base(void);
uint32_t get_emmc_base(void);
uint32_t get_arm_timer_base(void);

// Detect and set current Pi model
void hardware_detect_model(void);

// Model-specific information and tuning
typedef struct {
    const char *name;
    uint32_t soc_type;        // BCM2835, BCM2837, BCM2711, BCM2712
    uint32_t cpu_cores;
    uint32_t max_memory_mb;
    uint32_t default_memory_mb;
    uint32_t max_cpu_freq_mhz;
    uint32_t default_cpu_freq_mhz;
    uint32_t uart_baud_default;
    uint8_t has_ethernet;
    uint8_t has_wifi;
    uint8_t has_bluetooth;
    uint8_t has_usb3;
    uint8_t has_pcie;
} pi_model_info_t;

// Get comprehensive model information
const pi_model_info_t* hardware_get_model_info(void);

// Model-specific tuning functions
void hardware_apply_model_tuning(void);
uint32_t hardware_get_optimal_memory_size(void);
uint32_t hardware_get_optimal_cpu_frequency(void);
uint32_t hardware_get_recommended_uart_baud(void);

// Model-specific quirks and workarounds
void hardware_apply_model_quirks(void);
int hardware_has_quirk(const char *quirk_name);

#endif