/* Hardware Configuration Implementation */

#include <stdint.h>
#include "hardware.h"
#include "mailbox.h"

// Current model - detected at runtime
pi_model_t current_pi_model = PI_MODEL_UNKNOWN;

// Simple string comparison for freestanding environment
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

// Comprehensive model information table
static const pi_model_info_t model_info_table[] = {
    [PI_MODEL_UNKNOWN] = {
        .name = "Unknown",
        .soc_type = 0,
        .cpu_cores = 1,
        .max_memory_mb = 1024,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 1500,
        .default_cpu_freq_mhz = 600,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_1A] = {
        .name = "Raspberry Pi 1 Model A",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 256,
        .max_cpu_freq_mhz = 700,
        .default_cpu_freq_mhz = 700,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_1B] = {
        .name = "Raspberry Pi 1 Model B",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 700,
        .default_cpu_freq_mhz = 700,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_1A_PLUS] = {
        .name = "Raspberry Pi 1 Model A+",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 256,
        .max_cpu_freq_mhz = 700,
        .default_cpu_freq_mhz = 700,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_1B_PLUS] = {
        .name = "Raspberry Pi 1 Model B+",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 700,
        .default_cpu_freq_mhz = 700,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_2B] = {
        .name = "Raspberry Pi 2 Model B",
        .soc_type = 2837,
        .cpu_cores = 4,
        .max_memory_mb = 1024,
        .default_memory_mb = 1024,
        .max_cpu_freq_mhz = 900,
        .default_cpu_freq_mhz = 900,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_ZERO] = {
        .name = "Raspberry Pi Zero",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 1000,
        .default_cpu_freq_mhz = 1000,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 0,
        .has_bluetooth = 0,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_ZERO_W] = {
        .name = "Raspberry Pi Zero W",
        .soc_type = 2835,
        .cpu_cores = 1,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 1000,
        .default_cpu_freq_mhz = 1000,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_ZERO_2_W] = {
        .name = "Raspberry Pi Zero 2 W",
        .soc_type = 2837,
        .cpu_cores = 4,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 1000,
        .default_cpu_freq_mhz = 1000,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_3B] = {
        .name = "Raspberry Pi 3 Model B",
        .soc_type = 2837,
        .cpu_cores = 4,
        .max_memory_mb = 1024,
        .default_memory_mb = 1024,
        .max_cpu_freq_mhz = 1200,
        .default_cpu_freq_mhz = 1200,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_3B_PLUS] = {
        .name = "Raspberry Pi 3 Model B+",
        .soc_type = 2837,
        .cpu_cores = 4,
        .max_memory_mb = 1024,
        .default_memory_mb = 1024,
        .max_cpu_freq_mhz = 1400,
        .default_cpu_freq_mhz = 1400,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_3A_PLUS] = {
        .name = "Raspberry Pi 3 Model A+",
        .soc_type = 2837,
        .cpu_cores = 4,
        .max_memory_mb = 512,
        .default_memory_mb = 512,
        .max_cpu_freq_mhz = 1400,
        .default_cpu_freq_mhz = 1400,
        .uart_baud_default = 115200,
        .has_ethernet = 0,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 0,
        .has_pcie = 0
    },

    [PI_MODEL_4B] = {
        .name = "Raspberry Pi 4 Model B",
        .soc_type = 2711,
        .cpu_cores = 4,
        .max_memory_mb = 8192,
        .default_memory_mb = 4096,
        .max_cpu_freq_mhz = 1500,
        .default_cpu_freq_mhz = 1500,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 1,
        .has_pcie = 0
    },

    [PI_MODEL_400] = {
        .name = "Raspberry Pi 400",
        .soc_type = 2711,
        .cpu_cores = 4,
        .max_memory_mb = 4096,
        .default_memory_mb = 4096,
        .max_cpu_freq_mhz = 1500,
        .default_cpu_freq_mhz = 1500,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 1,
        .has_pcie = 0
    },

    [PI_MODEL_5B] = {
        .name = "Raspberry Pi 5 Model B",
        .soc_type = 2712,
        .cpu_cores = 4,
        .max_memory_mb = 8192,
        .default_memory_mb = 4096,
        .max_cpu_freq_mhz = 2400,
        .default_cpu_freq_mhz = 2400,
        .uart_baud_default = 115200,
        .has_ethernet = 1,
        .has_wifi = 1,
        .has_bluetooth = 1,
        .has_usb3 = 1,
        .has_pcie = 1
    }
};

uint32_t get_uart_base(void) {
    switch (current_pi_model) {
        // BCM2835 models (Pi 1, Zero, Zero W)
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            return UART_BASE_BCM2835;

        // BCM2837 models (Pi 2, 3, Zero 2 W)
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            return UART_BASE_BCM2837;

        // BCM2711 models (Pi 4, 400)
        case PI_MODEL_4B:
        case PI_MODEL_400:
            return UART_BASE_BCM2711;

        // BCM2712 models (Pi 5)
        case PI_MODEL_5B:
            return UART_BASE_BCM2712;

        default:
            return UART_BASE_BCM2711; // Default to Pi 4 addresses
    }
}

uint32_t get_gpio_base(void) {
    switch (current_pi_model) {
        // BCM2835 models
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            return GPIO_BASE_BCM2835;

        // BCM2837 models
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            return GPIO_BASE_BCM2837;

        // BCM2711 models
        case PI_MODEL_4B:
        case PI_MODEL_400:
            return GPIO_BASE_BCM2711;

        // BCM2712 models
        case PI_MODEL_5B:
            return GPIO_BASE_BCM2712;

        default:
            return GPIO_BASE_BCM2711;
    }
}

uint32_t get_timer_base(void) {
    switch (current_pi_model) {
        // BCM2835 models
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            return TIMER_BASE_BCM2835;

        // BCM2837 models
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            return TIMER_BASE_BCM2837;

        // BCM2711 models
        case PI_MODEL_4B:
        case PI_MODEL_400:
            return TIMER_BASE_BCM2711;

        // BCM2712 models
        case PI_MODEL_5B:
            return TIMER_BASE_BCM2712;

        default:
            return TIMER_BASE_BCM2711;
    }
}

uint32_t get_emmc_base(void) {
    switch (current_pi_model) {
        // BCM2835 models
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            return EMMC_BASE_BCM2835;

        // BCM2837 models
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            return EMMC_BASE_BCM2837;

        // BCM2711 models
        case PI_MODEL_4B:
        case PI_MODEL_400:
            return EMMC_BASE_BCM2711;

        // BCM2712 models
        case PI_MODEL_5B:
            return EMMC_BASE_BCM2712;

        default:
            return EMMC_BASE_BCM2711;
    }
}

uint32_t get_arm_timer_base(void) {
    switch (current_pi_model) {
        // BCM2835 models
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            return ARM_TIMER_BASE_BCM2835;

        // BCM2837 models
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            return ARM_TIMER_BASE_BCM2837;

        // BCM2711 models
        case PI_MODEL_4B:
        case PI_MODEL_400:
            return ARM_TIMER_BASE_BCM2711;

        // BCM2712 models
        case PI_MODEL_5B:
            return ARM_TIMER_BASE_BCM2712;

        default:
            return ARM_TIMER_BASE_BCM2711;
    }
}

void hardware_detect_model(void) {
    current_pi_model = pi_get_model();
}

const pi_model_info_t* hardware_get_model_info(void) {
    if (current_pi_model >= PI_MODEL_MAX) {
        return &model_info_table[PI_MODEL_UNKNOWN];
    }
    return &model_info_table[current_pi_model];
}

void hardware_apply_model_tuning(void) {
    const pi_model_info_t *info = hardware_get_model_info();

    // Apply model-specific tuning
    // This could include:
    // - Setting optimal CPU frequency
    // - Configuring memory layout
    // - Setting UART baud rate
    // - Enabling/disabling features based on capabilities

    // For now, just log the model information
    // In a real implementation, this would configure the hardware
}

uint32_t hardware_get_optimal_memory_size(void) {
    const pi_model_info_t *info = hardware_get_model_info();
    return info->default_memory_mb * 1024 * 1024; // Convert to bytes
}

uint32_t hardware_get_optimal_cpu_frequency(void) {
    const pi_model_info_t *info = hardware_get_model_info();
    return info->default_cpu_freq_mhz * 1000; // Convert to Hz
}

uint32_t hardware_get_recommended_uart_baud(void) {
    const pi_model_info_t *info = hardware_get_model_info();
    return info->uart_baud_default;
}

void hardware_apply_model_quirks(void) {
    const pi_model_info_t *info = hardware_get_model_info();

    // Apply model-specific workarounds and quirks
    switch (current_pi_model) {
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
            // BCM2835-specific quirks
            // - Limited USB bandwidth
            // - Single core
            // - Lower memory bandwidth
            break;

        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            // BCM2837-specific quirks
            // - USB issues with certain peripherals
            // - Thermal management differences
            break;

        case PI_MODEL_4B:
        case PI_MODEL_400:
            // BCM2711-specific quirks
            // - PCIe support
            // - USB 3.0 support
            // - Different power management
            break;

        case PI_MODEL_5B:
            // BCM2712-specific quirks
            // - PCIe Gen 3
            // - Higher performance
            // - Different memory controller
            break;

        default:
            break;
    }
}

int hardware_has_quirk(const char *quirk_name) {
    // Check for specific hardware quirks
    if (strcmp(quirk_name, "single_core") == 0) {
        return hardware_get_model_info()->cpu_cores == 1;
    }
    if (strcmp(quirk_name, "limited_usb") == 0) {
        return current_pi_model <= PI_MODEL_1B_PLUS;
    }
    if (strcmp(quirk_name, "has_pcie") == 0) {
        return hardware_get_model_info()->has_pcie;
    }
    if (strcmp(quirk_name, "has_usb3") == 0) {
        return hardware_get_model_info()->has_usb3;
    }
    if (strcmp(quirk_name, "has_wifi") == 0) {
        return hardware_get_model_info()->has_wifi;
    }
    if (strcmp(quirk_name, "has_bluetooth") == 0) {
        return hardware_get_model_info()->has_bluetooth;
    }

    return 0; // No quirk found
}