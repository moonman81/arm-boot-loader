/* Hardware Abstraction Layer - Dependency Inversion */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>

// Abstract hardware interfaces for dependency inversion

// UART interface
typedef struct {
    void (*init)(void);
    void (*putc)(char c);
    void (*puts)(const char *s);
} uart_ops_t;

// GPIO interface
typedef struct {
    void (*init)(void);
    void (*set)(uint32_t pin, uint32_t value);
    uint32_t (*read)(uint32_t pin);
} gpio_ops_t;

// Timer interface
typedef struct {
    void (*init)(void);
    uint64_t (*get_counter)(void);
} timer_ops_t;

// SD interface
typedef struct {
    int (*init)(void);
    int (*load_file)(const char *filename, unsigned long addr);
} sd_ops_t;

// Hardware abstraction layer
typedef struct {
    uart_ops_t *uart;
    gpio_ops_t *gpio;
    timer_ops_t *timer;
    sd_ops_t *sd;
} hal_t;

// Global HAL instance
extern hal_t hal;

#endif