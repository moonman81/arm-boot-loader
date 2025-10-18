/* Minimal GPIO Header */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

// GPIO functions
#define GPIO_FUNC_INPUT  0
#define GPIO_FUNC_OUTPUT 1

// LED pin (Pi 3 uses GPIO 47 for ACT LED, but varies by model)
#define GPIO_LED_PIN 47

void gpio_init(void);
void gpio_set_function(uint8_t pin, uint8_t function);
void gpio_set(uint8_t pin);
void gpio_clear(uint8_t pin);
void gpio_toggle(uint8_t pin);

#endif
