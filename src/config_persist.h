/* Persistent Configuration System */

#ifndef CONFIG_PERSIST_H
#define CONFIG_PERSIST_H

#include <stdint.h>

// Configuration storage location
#define CONFIG_MAGIC            0x42544346  // "BTCF" - BootConfig
#define CONFIG_VERSION          1
#define CONFIG_SD_SECTOR        2048        // Sector 2048 (1MB offset)
#define CONFIG_BACKUP_SECTOR    2049        // Backup copy

// Boot configuration structure (persisted)
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t crc32;

    // Boot menu settings
    uint32_t menu_timeout_sec;
    uint32_t default_boot_device;    // 0=SD, 1=Network, 2=USB
    uint8_t  auto_boot_enabled;

    // Network settings
    uint8_t  dhcp_enabled;
    uint8_t  static_ip[4];
    uint8_t  static_netmask[4];
    uint8_t  static_gateway[4];
    uint8_t  static_dns[4];
    uint8_t  mac_address[6];
    char     tftp_server[64];
    char     tftp_filename[64];

    // Logging settings
    uint8_t  log_level;               // 0-5 (NONE to TRACE)
    uint32_t log_targets;             // Bitmask (UART, SD, Memory)

    // Watchdog settings
    uint8_t  watchdog_enabled;
    uint32_t watchdog_timeout_ms;

    // Boot behavior
    uint8_t  enable_secure_boot;
    uint8_t  enable_a_b_boot;
    uint8_t  current_boot_slot;       // 0=A, 1=B
    uint32_t boot_counter_a;
    uint32_t boot_counter_b;
    uint32_t boot_success_a;
    uint32_t boot_success_b;

    // Reserved for future use
    uint8_t  reserved[128];
} boot_config_t;

// Initialize configuration system
int config_persist_init(void);

// Load configuration from storage
int config_persist_load(boot_config_t *config);

// Save configuration to storage
int config_persist_save(const boot_config_t *config);

// Reset to factory defaults
int config_persist_reset(void);

// Get current configuration
const boot_config_t *config_persist_get(void);

// Update configuration fields
int config_persist_set_menu_timeout(uint32_t seconds);
int config_persist_set_default_boot(uint32_t device);
int config_persist_set_log_level(uint8_t level);
int config_persist_set_watchdog(uint8_t enabled, uint32_t timeout_ms);
int config_persist_set_network(uint8_t dhcp, const uint8_t *ip, const uint8_t *netmask, const uint8_t *gateway);

// A/B boot slot management
int config_persist_get_boot_slot(void);
int config_persist_mark_boot_success(void);
int config_persist_mark_boot_failure(void);

// Validate configuration
int config_persist_validate(const boot_config_t *config);

// Calculate CRC32
uint32_t config_calc_crc32(const void *data, uint32_t length);

#endif
