/* Minimal GPIO Implementation for QEMU raspi3b */

#include <stdint.h>
#include "gpio.h"

// BCM2837 GPIO (QEMU raspi3b)
#define GPIO_BASE 0x3F200000

#define GPIO_GPFSEL0 (GPIO_BASE + 0x00)  // Function Select 0
#define GPIO_GPFSEL1 (GPIO_BASE + 0x04)  // Function Select 1
#define GPIO_GPFSEL2 (GPIO_BASE + 0x08)  // Function Select 2
#define GPIO_GPFSEL3 (GPIO_BASE + 0x0C)  // Function Select 3
#define GPIO_GPFSEL4 (GPIO_BASE + 0x10)  // Function Select 4
#define GPIO_GPFSEL5 (GPIO_BASE + 0x14)  // Function Select 5
#define GPIO_GPSET0  (GPIO_BASE + 0x1C)  // Pin Output Set 0
#define GPIO_GPSET1  (GPIO_BASE + 0x20)  // Pin Output Set 1
#define GPIO_GPCLR0  (GPIO_BASE + 0x28)  // Pin Output Clear 0
#define GPIO_GPCLR1  (GPIO_BASE + 0x2C)  // Pin Output Clear 1

static inline void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static inline uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void gpio_init(void) {
    // GPIO is always available, no initialization needed
}

void gpio_set_function(uint8_t pin, uint8_t function) {
    if (pin > 53) return;  // Invalid pin

    // Calculate which GPFSEL register and which 3-bit field
    uint8_t reg_index = pin / 10;
    uint8_t bit_index = (pin % 10) * 3;

    uint32_t reg_addr = GPIO_GPFSEL0 + (reg_index * 4);
    uint32_t reg_value = mmio_read(reg_addr);

    // Clear the 3 bits for this pin
    reg_value &= ~(7 << bit_index);
    // Set the function (input=0, output=1, alt0-5=4,5,6,7,3,2)
    reg_value |= (function << bit_index);

    mmio_write(reg_addr, reg_value);
}

void gpio_set(uint8_t pin) {
    if (pin > 53) return;

    if (pin < 32) {
        mmio_write(GPIO_GPSET0, 1 << pin);
    } else {
        mmio_write(GPIO_GPSET1, 1 << (pin - 32));
    }
}

void gpio_clear(uint8_t pin) {
    if (pin > 53) return;

    if (pin < 32) {
        mmio_write(GPIO_GPCLR0, 1 << pin);
    } else {
        mmio_write(GPIO_GPCLR1, 1 << (pin - 32));
    }
}

void gpio_toggle(uint8_t pin) {
    // Simple toggle: always set (for LED blink demo)
    // In real implementation, would read current state
    static uint8_t state = 0;
    state = !state;
    if (state) {
        gpio_set(pin);
    } else {
        gpio_clear(pin);
    }
}
