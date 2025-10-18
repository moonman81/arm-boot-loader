/* Watchdog Timer Implementation for Raspberry Pi */

#include "watchdog.h"
#include "timer.h"
#include "interrupt.h"
#include "log.h"

// BCM2711 PM Watchdog registers
#define PM_BASE         0xFE100000  // Power Management base (Pi 4/5)
#define PM_RSTC         (PM_BASE + 0x1C)  // Reset Control
#define PM_RSTS         (PM_BASE + 0x20)  // Reset Status
#define PM_WDOG         (PM_BASE + 0x24)  // Watchdog

// PM_RSTC register bits
#define PM_PASSWORD     0x5A000000   // PM password
#define PM_RSTC_WRCFG_CLR    0xFFFFFFCF
#define PM_RSTC_WRCFG_SET    0x00000030
#define PM_RSTC_WRCFG_FULL_RESET  0x00000020
#define PM_RSTC_RESET   0x00000102

// PM_WDOG register
#define PM_WDOG_TIME_SET     0x000FFFFF
#define PM_WDOG_TICKS_PER_MS 65  // Approximate ticks per millisecond

// MMIO helpers
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// Watchdog state
static struct {
    int enabled;
    watchdog_mode_t mode;
    uint32_t timeout_ms;
    uint64_t last_kick_time;
    uint32_t kick_count;
    uint32_t timeout_count;
    watchdog_callback_t callback;
} wdt_state = {
    .enabled = 0,
    .mode = WDT_MODE_DISABLED,
    .timeout_ms = 0,
    .last_kick_time = 0,
    .kick_count = 0,
    .timeout_count = 0,
    .callback = NULL
};

// Convert milliseconds to watchdog ticks
static uint32_t ms_to_ticks(uint32_t ms) {
    return (ms * PM_WDOG_TICKS_PER_MS) & PM_WDOG_TIME_SET;
}

// Watchdog interrupt handler (if interrupt mode)
static void watchdog_irq_handler(void *context) {
    (void)context;

    wdt_state.timeout_count++;
    log_error("WATCHDOG", "Watchdog timeout occurred!");

    if (wdt_state.callback) {
        wdt_state.callback();
    }

    // In interrupt mode, we kick the watchdog to prevent reset
    watchdog_kick();
}

// Initialize watchdog
int watchdog_init(watchdog_mode_t mode, uint32_t timeout_ms) {
    if (timeout_ms == 0 || timeout_ms > 60000) {
        return -1; // Invalid timeout
    }

    wdt_state.mode = mode;
    wdt_state.timeout_ms = timeout_ms;
    wdt_state.enabled = 0;
    wdt_state.kick_count = 0;
    wdt_state.timeout_count = 0;

    // Configure watchdog timeout
    uint32_t wdog_ticks = ms_to_ticks(timeout_ms);
    mmio_write(PM_WDOG, PM_PASSWORD | wdog_ticks);

    // Configure reset control based on mode
    uint32_t rstc = mmio_read(PM_RSTC);
    rstc &= PM_RSTC_WRCFG_CLR;

    if (mode == WDT_MODE_RESET) {
        rstc |= PM_RSTC_WRCFG_FULL_RESET;
    }

    mmio_write(PM_RSTC, PM_PASSWORD | rstc);

    log_info("WATCHDOG", "Watchdog initialized");

    return 0;
}

// Start watchdog
int watchdog_start(void) {
    if (wdt_state.timeout_ms == 0) {
        return -1; // Not initialized
    }

    // Set timeout value
    uint32_t wdog_ticks = ms_to_ticks(wdt_state.timeout_ms);
    mmio_write(PM_WDOG, PM_PASSWORD | wdog_ticks);

    // Enable watchdog
    uint32_t rstc = mmio_read(PM_RSTC);
    rstc &= PM_RSTC_WRCFG_CLR;

    if (wdt_state.mode == WDT_MODE_RESET) {
        rstc |= PM_RSTC_WRCFG_FULL_RESET;
    } else {
        rstc |= PM_RSTC_WRCFG_SET;
    }

    mmio_write(PM_RSTC, PM_PASSWORD | rstc);

    wdt_state.enabled = 1;
    wdt_state.last_kick_time = timer_get_counter();

    log_info("WATCHDOG", "Watchdog started");

    return 0;
}

// Stop watchdog
int watchdog_stop(void) {
    uint32_t rstc = mmio_read(PM_RSTC);
    rstc &= PM_RSTC_WRCFG_CLR;
    mmio_write(PM_RSTC, PM_PASSWORD | rstc);

    wdt_state.enabled = 0;

    log_info("WATCHDOG", "Watchdog stopped");

    return 0;
}

// Kick watchdog
void watchdog_kick(void) {
    if (!wdt_state.enabled) return;

    // Reset watchdog timer
    uint32_t wdog_ticks = ms_to_ticks(wdt_state.timeout_ms);
    mmio_write(PM_WDOG, PM_PASSWORD | wdog_ticks);

    wdt_state.last_kick_time = timer_get_counter();
    wdt_state.kick_count++;

    log_trace("WATCHDOG", "Watchdog kicked");
}

// Check if enabled
int watchdog_is_enabled(void) {
    return wdt_state.enabled;
}

// Get remaining time
uint32_t watchdog_get_remaining_ms(void) {
    if (!wdt_state.enabled) return 0;

    uint64_t current_time = timer_get_counter();
    uint64_t elapsed_us = current_time - wdt_state.last_kick_time;
    uint32_t elapsed_ms = elapsed_us / 1000;

    if (elapsed_ms >= wdt_state.timeout_ms) {
        return 0;
    }

    return wdt_state.timeout_ms - elapsed_ms;
}

// Set callback
void watchdog_set_callback(watchdog_callback_t callback) {
    wdt_state.callback = callback;
}

// Get status
void watchdog_get_status(watchdog_status_t *status) {
    if (!status) return;

    status->enabled = wdt_state.enabled;
    status->mode = wdt_state.mode;
    status->timeout_ms = wdt_state.timeout_ms;
    status->remaining_ms = watchdog_get_remaining_ms();
    status->kick_count = wdt_state.kick_count;
    status->timeout_count = wdt_state.timeout_count;
}
