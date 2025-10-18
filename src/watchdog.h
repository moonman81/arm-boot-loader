/* Watchdog Timer for System Safety */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

// Watchdog timeout values (in milliseconds)
#define WDT_TIMEOUT_1S      1000
#define WDT_TIMEOUT_5S      5000
#define WDT_TIMEOUT_10S     10000
#define WDT_TIMEOUT_30S     30000
#define WDT_TIMEOUT_60S     60000

// Watchdog modes
typedef enum {
    WDT_MODE_DISABLED = 0,
    WDT_MODE_RESET = 1,      // Reset system on timeout
    WDT_MODE_INTERRUPT = 2    // Generate interrupt on timeout
} watchdog_mode_t;

// Watchdog callback for interrupt mode
typedef void (*watchdog_callback_t)(void);

// Initialize watchdog timer
int watchdog_init(watchdog_mode_t mode, uint32_t timeout_ms);

// Start watchdog
int watchdog_start(void);

// Stop watchdog
int watchdog_stop(void);

// Pet/kick the watchdog (reset counter)
void watchdog_kick(void);

// Check if watchdog is enabled
int watchdog_is_enabled(void);

// Get remaining time before timeout
uint32_t watchdog_get_remaining_ms(void);

// Set callback for interrupt mode
void watchdog_set_callback(watchdog_callback_t callback);

// Get watchdog status
typedef struct {
    int enabled;
    watchdog_mode_t mode;
    uint32_t timeout_ms;
    uint32_t remaining_ms;
    uint32_t kick_count;
    uint32_t timeout_count;
} watchdog_status_t;

void watchdog_get_status(watchdog_status_t *status);

#endif
