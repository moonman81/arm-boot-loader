/* Boot Performance Monitoring */

#ifndef PERFMON_H
#define PERFMON_H

#include <stdint.h>

// Performance checkpoint types
typedef enum {
    PERF_BOOT_START,
    PERF_HARDWARE_INIT,
    PERF_DRIVER_INIT,
    PERF_CONFIG_LOAD,
    PERF_FILESYSTEM_MOUNT,
    PERF_KERNEL_LOAD_START,
    PERF_KERNEL_LOAD_END,
    PERF_KERNEL_VERIFY,
    PERF_DTB_LOAD,
    PERF_INITRD_LOAD,
    PERF_NETWORK_INIT,
    PERF_DHCP_COMPLETE,
    PERF_TFTP_START,
    PERF_TFTP_END,
    PERF_SECURITY_CHECK,
    PERF_BOOT_COMPLETE,
    PERF_MAX_CHECKPOINTS
} perf_checkpoint_t;

// Checkpoint data
typedef struct {
    perf_checkpoint_t type;
    const char *name;
    uint64_t timestamp;
    uint64_t delta_us;       // Time since previous checkpoint
    uint32_t data;           // Optional data (e.g., bytes transferred)
} perf_record_t;

// Performance statistics
typedef struct {
    uint64_t total_boot_time_us;
    uint64_t hardware_init_us;
    uint64_t driver_init_us;
    uint64_t filesystem_time_us;
    uint64_t kernel_load_time_us;
    uint64_t network_time_us;
    uint32_t kernel_size_bytes;
    uint32_t network_transfer_bytes;
    float kernel_load_speed_mbps;
} perf_stats_t;

// Initialize performance monitoring
void perfmon_init(void);

// Record checkpoint
void perfmon_checkpoint(perf_checkpoint_t type, uint32_t data);

// Get checkpoint count
int perfmon_get_count(void);

// Get specific checkpoint
const perf_record_t *perfmon_get_checkpoint(int index);

// Calculate statistics
void perfmon_calc_stats(perf_stats_t *stats);

// Print performance report
void perfmon_print_report(void);

// Print summary
void perfmon_print_summary(void);

// Reset monitoring
void perfmon_reset(void);

// Get total boot time
uint64_t perfmon_get_total_time_us(void);
uint64_t perfmon_get_total_time_ms(void);

#endif
