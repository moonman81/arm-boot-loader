/* Boot Performance Monitoring Implementation */

#include "perfmon.h"
#include "timer.h"
#include "uart.h"
#include "log.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Checkpoint names
static const char *checkpoint_names[] = {
    "Boot Start",
    "Hardware Init",
    "Driver Init",
    "Config Load",
    "Filesystem Mount",
    "Kernel Load Start",
    "Kernel Load End",
    "Kernel Verify",
    "DTB Load",
    "Initrd Load",
    "Network Init",
    "DHCP Complete",
    "TFTP Start",
    "TFTP End",
    "Security Check",
    "Boot Complete"
};

// Performance records
static perf_record_t records[PERF_MAX_CHECKPOINTS];
static int record_count = 0;
static uint64_t boot_start_time = 0;

// Initialize performance monitoring
void perfmon_init(void) {
    record_count = 0;
    boot_start_time = timer_get_counter();

    // Record first checkpoint
    perfmon_checkpoint(PERF_BOOT_START, 0);

    log_info("PERFMON", "Performance monitoring initialized");
}

// Record checkpoint
void perfmon_checkpoint(perf_checkpoint_t type, uint32_t data) {
    if (record_count >= PERF_MAX_CHECKPOINTS) return;

    perf_record_t *record = &records[record_count];
    record->type = type;
    record->name = checkpoint_names[type];
    record->timestamp = timer_get_counter();
    record->data = data;

    // Calculate delta from previous checkpoint
    if (record_count > 0) {
        uint64_t prev_ts = records[record_count - 1].timestamp;
        record->delta_us = record->timestamp - prev_ts;
    } else {
        record->delta_us = 0;
    }

    record_count++;

    log_debug("PERFMON", record->name);
}

// Get checkpoint count
int perfmon_get_count(void) {
    return record_count;
}

// Get specific checkpoint
const perf_record_t *perfmon_get_checkpoint(int index) {
    if (index < 0 || index >= record_count) return NULL;
    return &records[index];
}

// Calculate statistics
void perfmon_calc_stats(perf_stats_t *stats) {
    if (!stats || record_count == 0) return;

    // Initialize
    stats->total_boot_time_us = 0;
    stats->hardware_init_us = 0;
    stats->driver_init_us = 0;
    stats->filesystem_time_us = 0;
    stats->kernel_load_time_us = 0;
    stats->network_time_us = 0;
    stats->kernel_size_bytes = 0;
    stats->network_transfer_bytes = 0;
    stats->kernel_load_speed_mbps = 0.0f;

    // Find relevant checkpoints
    uint64_t kernel_load_start_ts = 0;
    uint64_t kernel_load_end_ts = 0;

    for (int i = 0; i < record_count; i++) {
        const perf_record_t *rec = &records[i];

        switch (rec->type) {
            case PERF_HARDWARE_INIT:
                stats->hardware_init_us = rec->delta_us;
                break;

            case PERF_DRIVER_INIT:
                stats->driver_init_us = rec->delta_us;
                break;

            case PERF_FILESYSTEM_MOUNT:
                stats->filesystem_time_us = rec->delta_us;
                break;

            case PERF_KERNEL_LOAD_START:
                kernel_load_start_ts = rec->timestamp;
                break;

            case PERF_KERNEL_LOAD_END:
                kernel_load_end_ts = rec->timestamp;
                stats->kernel_size_bytes = rec->data;
                break;

            case PERF_DHCP_COMPLETE:
            case PERF_TFTP_END:
                stats->network_time_us += rec->delta_us;
                if (rec->type == PERF_TFTP_END) {
                    stats->network_transfer_bytes = rec->data;
                }
                break;

            case PERF_BOOT_COMPLETE:
                stats->total_boot_time_us = rec->timestamp - boot_start_time;
                break;

            default:
                break;
        }
    }

    // Calculate kernel load time
    if (kernel_load_end_ts > kernel_load_start_ts) {
        stats->kernel_load_time_us = kernel_load_end_ts - kernel_load_start_ts;

        // Calculate speed if we have size
        if (stats->kernel_size_bytes > 0 && stats->kernel_load_time_us > 0) {
            // bytes / microseconds * 8 = megabits per second
            stats->kernel_load_speed_mbps =
                ((float)stats->kernel_size_bytes * 8.0f) /
                (float)stats->kernel_load_time_us;
        }
    }
}

// Print performance report
void perfmon_print_report(void) {
    uart_puts("\n");
    uart_puts("=====================================\n");
    uart_puts("   Boot Performance Report\n");
    uart_puts("=====================================\n\n");

    uart_puts("Checkpoint                   Time (ms)  Delta (ms)\n");
    uart_puts("--------------------------------------------------\n");

    for (int i = 0; i < record_count; i++) {
        const perf_record_t *rec = &records[i];

        // Print name (pad to 25 chars)
        uart_puts(rec->name);
        int name_len = 0;
        const char *p = rec->name;
        while (*p++) name_len++;

        for (int j = name_len; j < 25; j++) uart_putc(' ');

        // Print absolute time
        uint64_t ms = (rec->timestamp - boot_start_time) / 1000;
        // Would need proper number formatting

        uart_puts("  ");

        // Print delta
        uint64_t delta_ms = rec->delta_us / 1000;
        // Print delta

        uart_putc('\n');

        // Print data if present
        if (rec->data > 0) {
            uart_puts("  -> Data: ");
            // Print data
            uart_puts(" bytes\n");
        }
    }

    uart_puts("--------------------------------------------------\n\n");
}

// Print summary
void perfmon_print_summary(void) {
    perf_stats_t stats;
    perfmon_calc_stats(&stats);

    uart_puts("\n");
    uart_puts("Performance Summary:\n");
    uart_puts("-------------------\n");

    uart_puts("Total Boot Time:    ");
    // Print total time in ms
    uart_puts(" ms\n");

    uart_puts("Hardware Init:      ");
    // Print hardware init time
    uart_puts(" ms\n");

    uart_puts("Driver Init:        ");
    uart_puts(" ms\n");

    uart_puts("Filesystem:         ");
    uart_puts(" ms\n");

    uart_puts("Kernel Load:        ");
    uart_puts(" ms\n");

    if (stats.kernel_size_bytes > 0) {
        uart_puts("Kernel Size:        ");
        // Print size in KB
        uart_puts(" KB\n");

        uart_puts("Load Speed:         ");
        // Print speed in MB/s
        uart_puts(" MB/s\n");
    }

    if (stats.network_time_us > 0) {
        uart_puts("Network Time:       ");
        uart_puts(" ms\n");
    }

    uart_puts("\n");
}

// Reset monitoring
void perfmon_reset(void) {
    record_count = 0;
    boot_start_time = timer_get_counter();
}

// Get total boot time
uint64_t perfmon_get_total_time_us(void) {
    if (record_count == 0) return 0;
    return records[record_count - 1].timestamp - boot_start_time;
}

uint64_t perfmon_get_total_time_ms(void) {
    return perfmon_get_total_time_us() / 1000;
}
