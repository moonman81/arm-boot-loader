/* Config.txt Parser */

#include "config.h"
#include "sd.h"
#include "uart.h"
#include "hardware.h"
#include "mailbox.h"

// Forward declare if needed
// typedef struct fat32_dir_entry fat32_dir_entry_t; // Already in sd.h

#define NULL ((void*)0)

// Custom string functions for freestanding environment
static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

static char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return NULL;
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

static char *strncpy(char *dest, const char *src, uint32_t n) {
    char *d = dest;
    while (n-- && *src) *d++ = *src++;
    while (n--) *d++ = '\0';
    return dest;
}

static unsigned long strtoul(const char *str, char **endptr, int base) {
    unsigned long result = 0;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char*)str;
    return result;
}

static int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}

// Simple config storage (key-value pairs)
#define MAX_CONFIG_ENTRIES 64
#define MAX_KEY_LEN 32
#define MAX_VALUE_LEN 128
#define MAX_CONFIG_SIZE 65536  // 64KB max for config file

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} config_entry_t;

static config_entry_t config_entries[MAX_CONFIG_ENTRIES];
static int num_entries = 0;

// Config values - hyper-configurable bootloader
char kernel_filename[256] = "kernel8.img";
char initrd_filename[256] = "initramfs";
unsigned long kernel_addr = 0x80000;
unsigned long initrd_addr = 0x1000000;
int enable_usb = 0;
int enable_nvme = 0;

// Boot source configuration
char bootcode_source[32] = "sd";  // sd, usb, network
char startelf_source[32] = "sd";  // sd, usb, network
char kernel_source[32] = "sd";    // sd, usb, network, module

// State machine configuration
int enable_alternative_boot = 1;  // Allow alternative boot paths
int enable_modular_boot = 0;      // Enable modular component loading
int enable_network_boot = 0;      // Enable network boot attempts
int enable_usb_boot = 0;          // Enable USB boot attempts

// Timeout configuration overrides
int custom_timeouts = 0;          // Use custom timeouts from config
unsigned int timeout_bootcode_loading = 5000;
unsigned int timeout_kernel_loading = 10000;

// Model-specific configuration
unsigned long pi3_kernel_addr = 0x80000;
unsigned long pi3_initrd_addr = 0x1000000;
unsigned long pi4_kernel_addr = 0x80000;
unsigned long pi4_initrd_addr = 0x1000000;
unsigned long pi5_kernel_addr = 0x80000;
unsigned long pi5_initrd_addr = 0x1000000;
int pi3_uart_baud = 115200;
int pi4_uart_baud = 115200;
int pi5_uart_baud = 115200;

static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_comment(char c) {
    return c == '#';
}

static int validate_filename_value(const char *value) {
    if (!value) return 0;
    int len = 0;
    while (*value) {
        if (*value == '/' || *value == '\\' || *value == ':' || *value == '*' || *value == '?' || *value == '"' || *value == '<' || *value == '>' || *value == '|') {
            return 0; // Invalid characters
        }
        if (++len > 255) return 0; // Too long
        value++;
    }
    return len > 0; // Must not be empty
}

static int validate_ip_value(const char *value) {
    if (!value || !*value) return 0;

    int dots = 0;
    int num = 0;
    int parts = 0;

    for (int i = 0; value[i]; i++) {
        char c = value[i];
        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
            if (num > 255) return 0;
        } else if (c == '.') {
            if (parts >= 3) return 0; // Too many parts
            parts++;
            num = 0;
            dots++;
        } else {
            return 0; // Invalid character
        }
    }

    return (dots == 3 && parts == 3);
}

static int validate_mac_value(const char *value) {
    if (!value || strlen(value) != 17) return 0; // XX:XX:XX:XX:XX:XX format

    for (int i = 0; i < 17; i++) {
        char c = value[i];
        if (i % 3 == 2) {
            if (c != ':') return 0;
        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                return 0;
            }
        }
    }

    return 1;
}

static void trim_whitespace(char *str) {
    // Trim leading whitespace
    char *start = str;
    while (*start && is_whitespace(*start)) start++;

    // Trim trailing whitespace
    char *end = start + strlen(start) - 1;
    while (end > start && is_whitespace(*end)) {
        *end = '\0';
        end--;
    }

    // Move string if needed
    if (start != str) {
        char *p = str;
        while (*start) *p++ = *start++;
        *p = '\0';
    }
}

static int parse_line(char *line) {
    if (!line || !*line) return 0;

    // Skip comments and empty lines
    if (is_comment(*line)) return 0;

    char *equals = strchr(line, '=');
    if (!equals) return 0; // Not a key=value line

    *equals = '\0';
    char *key = line;
    char *value = equals + 1;

    trim_whitespace(key);
    trim_whitespace(value);

    if (!*key || !*value) return 0;

    // Store in our config - hyper-configurable options
    if (strcmp(key, "kernel") == 0) {
        if (validate_filename_value(value)) {
            strncpy(kernel_filename, value, sizeof(kernel_filename) - 1);
            kernel_filename[sizeof(kernel_filename) - 1] = '\0';
        }
    } else if (strcmp(key, "initramfs") == 0) {
        if (validate_filename_value(value)) {
            strncpy(initrd_filename, value, sizeof(initrd_filename) - 1);
            initrd_filename[sizeof(initrd_filename) - 1] = '\0';
        }
    } else if (strcmp(key, "kernel_address") == 0) {
        kernel_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "initramfs_address") == 0) {
        initrd_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "enable_usb") == 0) {
        enable_usb = atoi(value);
    } else if (strcmp(key, "enable_nvme") == 0) {
        enable_nvme = atoi(value);
    }
    // Boot source configuration
    else if (strcmp(key, "bootcode_source") == 0) {
        if (strcmp(value, "sd") == 0 || strcmp(value, "usb") == 0 || strcmp(value, "network") == 0 || strcmp(value, "pxe") == 0) {
            strncpy(bootcode_source, value, sizeof(bootcode_source) - 1);
        }
    } else if (strcmp(key, "startelf_source") == 0) {
        if (strcmp(value, "sd") == 0 || strcmp(value, "usb") == 0 || strcmp(value, "network") == 0 || strcmp(value, "pxe") == 0) {
            strncpy(startelf_source, value, sizeof(startelf_source) - 1);
        }
    } else if (strcmp(key, "kernel_source") == 0) {
        if (strcmp(value, "sd") == 0 || strcmp(value, "usb") == 0 || strcmp(value, "network") == 0 || strcmp(value, "pxe") == 0 || strcmp(value, "module") == 0) {
            strncpy(kernel_source, value, sizeof(kernel_source) - 1);
        }
    }
    // Advanced boot options
    else if (strcmp(key, "enable_alternative_boot") == 0) {
        enable_alternative_boot = atoi(value);
    } else if (strcmp(key, "enable_modular_boot") == 0) {
        enable_modular_boot = atoi(value);
    } else if (strcmp(key, "enable_network_boot") == 0) {
        enable_network_boot = atoi(value);
    } else if (strcmp(key, "enable_usb_boot") == 0) {
        enable_usb_boot = atoi(value);
    }
    // Custom timeouts
    else if (strcmp(key, "timeout_bootcode_loading") == 0) {
        timeout_bootcode_loading = atoi(value);
        custom_timeouts = 1;
    } else if (strcmp(key, "timeout_kernel_loading") == 0) {
        timeout_kernel_loading = atoi(value);
        custom_timeouts = 1;
    }
    // Model-specific configuration
    else if (strcmp(key, "pi3_kernel_address") == 0) {
        pi3_kernel_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi3_initramfs_address") == 0) {
        pi3_initrd_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi4_kernel_address") == 0) {
        pi4_kernel_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi4_initramfs_address") == 0) {
        pi4_initrd_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi5_kernel_address") == 0) {
        pi5_kernel_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi5_initramfs_address") == 0) {
        pi5_initrd_addr = strtoul(value, NULL, 16);
    } else if (strcmp(key, "pi3_uart_baud") == 0) {
        pi3_uart_baud = atoi(value);
    } else if (strcmp(key, "pi4_uart_baud") == 0) {
        pi4_uart_baud = atoi(value);
    } else if (strcmp(key, "pi5_uart_baud") == 0) {
        pi5_uart_baud = atoi(value);
    }
    // Network configuration with validation
    else if (strcmp(key, "network_server") == 0) {
        if (validate_ip_value(value)) {
            // Store in config_entries for network module to use
        }
    } else if (strcmp(key, "network_boot_protocol") == 0) {
        if (strcmp(value, "dhcp") == 0 || strcmp(value, "bootp") == 0) {
            // Valid protocol
        }
    } else if (strcmp(key, "network_boot_filename") == 0) {
        if (validate_filename_value(value)) {
            // Valid filename
        }
    } else if (strcmp(key, "network_timeout_dhcp") == 0) {
        int timeout = atoi(value);
        if (timeout > 0 && timeout <= 30000) { // 30 seconds max
            // Valid timeout
        }
    } else if (strcmp(key, "network_timeout_tftp") == 0) {
        int timeout = atoi(value);
        if (timeout > 0 && timeout <= 60000) { // 60 seconds max
            // Valid timeout
        }
    } else if (strcmp(key, "network_mac_address") == 0) {
        if (validate_mac_value(value)) {
            // Valid MAC address
        }
    }

    // Store in entries array if space
    if (num_entries < MAX_CONFIG_ENTRIES) {
        strncpy(config_entries[num_entries].key, key, MAX_KEY_LEN - 1);
        strncpy(config_entries[num_entries].value, value, MAX_VALUE_LEN - 1);
        num_entries++;
    }

    return 1;
}

void config_parse(void) {
    uint8_t *config_buffer = (uint8_t*)0x200000; // Temporary buffer at 2MB
    fat32_dir_entry_t entry;

    uart_puts("Loading config.txt...\n");

    // Try to find and load config.txt
    if (fat_find_file("config.txt", &entry) == 0) {
        // Check config file size bounds
        if (entry.file_size > MAX_CONFIG_SIZE) {
            uart_puts("Config file too large\n");
            return;
        }
        int32_t size = fat_read_file(&entry, config_buffer, MAX_CONFIG_SIZE);
        if (size > 0) {
            // File loaded, now parse it
            char *line_start = (char*)config_buffer;
            char *buffer_end = (char*)config_buffer + size;

            while (line_start < buffer_end) {
                char *line_end = strchr(line_start, '\n');
                if (!line_end) line_end = buffer_end;

                *line_end = '\0';

                parse_line(line_start);

                line_start = line_end + 1;
            }

            uart_puts("Config parsed successfully\n");
        } else {
            uart_puts("Failed to read config.txt\n");
        }
    } else {
        uart_puts("No config.txt found, using defaults\n");
    }
}

// Apply model-specific configuration
void config_apply_model_settings(void) {
    const pi_model_info_t *model_info = hardware_get_model_info();

    // Apply model-specific memory and performance settings
    uint32_t optimal_memory = hardware_get_optimal_memory_size();
    uint32_t optimal_cpu_freq = hardware_get_optimal_cpu_frequency();

    // Set kernel load address based on model capabilities
    // Higher-end models can use higher addresses
    if (model_info->soc_type >= 2711) {  // Pi 4 and newer
        if (kernel_addr == 0x80000) {  // If not set in config
            kernel_addr = 0x200000;  // Higher address for better memory layout
        }
        if (initrd_addr == 0x1000000) {  // If not set in config
            initrd_addr = 0x2000000;  // Higher address
        }
    }

    // Apply config file overrides if they exist
    pi_model_t model = pi_get_model();
    switch (model) {
        case PI_MODEL_1A:
        case PI_MODEL_1B:
        case PI_MODEL_1A_PLUS:
        case PI_MODEL_1B_PLUS:
        case PI_MODEL_ZERO:
        case PI_MODEL_ZERO_W:
        case PI_MODEL_2B:
        case PI_MODEL_3B:
        case PI_MODEL_3B_PLUS:
        case PI_MODEL_3A_PLUS:
        case PI_MODEL_ZERO_2_W:
            // Use pi3_ config for older models
            if (pi3_kernel_addr != 0x80000) kernel_addr = pi3_kernel_addr;
            if (pi3_initrd_addr != 0x1000000) initrd_addr = pi3_initrd_addr;
            break;
        case PI_MODEL_4B:
        case PI_MODEL_400:
            if (pi4_kernel_addr != 0x80000) kernel_addr = pi4_kernel_addr;
            if (pi4_initrd_addr != 0x1000000) initrd_addr = pi4_initrd_addr;
            break;
        case PI_MODEL_5B:
            if (pi5_kernel_addr != 0x80000) kernel_addr = pi5_kernel_addr;
            if (pi5_initrd_addr != 0x1000000) initrd_addr = pi5_initrd_addr;
            break;
        default:
            // Use default values
            break;
    }

    // Log applied settings
    uart_puts("Applied model-specific configuration\n");
}

// Get config value by key
const char *config_get(const char *key) {
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(config_entries[i].key, key) == 0) {
            return config_entries[i].value;
        }
    }
    return NULL;
}