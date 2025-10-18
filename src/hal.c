/* Hardware Abstraction Layer Implementation */

#include "hal.h"
#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include "sd.h"

// UART operations
static void hal_uart_init(void) { uart_init(); }
static void hal_uart_putc(char c) { uart_putc(c); }
static void hal_uart_puts(const char *s) { uart_puts(s); }

// GPIO operations
static void hal_gpio_init(void) { gpio_init(); }
static void hal_gpio_set(uint32_t pin, uint32_t value) { if (value) gpio_set(pin); } // Simplified
static uint32_t hal_gpio_read(uint32_t pin) { return gpio_read(pin); }

// Timer operations
static void hal_timer_init(void) { timer_init(); }
static uint64_t hal_timer_get_counter(void) { return timer_get_counter(); }

// SD operations
static int hal_sd_init(void) { return sd_init(); }
static int hal_sd_load_file(const char *filename, unsigned long addr) { return sd_load_file(filename, addr); }

// Operation tables
static uart_ops_t uart_ops = {
    .init = hal_uart_init,
    .putc = hal_uart_putc,
    .puts = hal_uart_puts
};

static gpio_ops_t gpio_ops = {
    .init = hal_gpio_init,
    .set = hal_gpio_set,
    .read = hal_gpio_read
};

static timer_ops_t timer_ops = {
    .init = hal_timer_init,
    .get_counter = hal_timer_get_counter
};

static sd_ops_t sd_ops = {
    .init = hal_sd_init,
    .load_file = hal_sd_load_file
};

// Global HAL instance
hal_t hal = {
    .uart = &uart_ops,
    .gpio = &gpio_ops,
    .timer = &timer_ops,
    .sd = &sd_ops
};