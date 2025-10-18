/* Minimal UART Implementation for QEMU raspi3b */

#include <stdint.h>
#include "uart.h"

// Hardcoded for QEMU raspi3b (BCM2837)
#define UART_BASE 0x3F201000

#define UART_DR     (UART_BASE + 0x00)
#define UART_FR     (UART_BASE + 0x18)
#define UART_IBRD   (UART_BASE + 0x24)
#define UART_FBRD   (UART_BASE + 0x28)
#define UART_LCRH   (UART_BASE + 0x2C)
#define UART_CR     (UART_BASE + 0x30)

#define UART_FR_TXFF (1 << 5)  // TX FIFO full

static inline void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static inline uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void uart_init(void) {
    // Disable UART
    mmio_write(UART_CR, 0);

    // Set baud rate to 115200
    mmio_write(UART_IBRD, 26);
    mmio_write(UART_FBRD, 3);

    // 8n1, enable FIFOs
    mmio_write(UART_LCRH, (3 << 5) | (1 << 4));

    // Enable UART, TX, RX
    mmio_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(char c) {
    // Wait for TX FIFO not full
    while (mmio_read(UART_FR) & UART_FR_TXFF);

    // Write character
    mmio_write(UART_DR, (uint32_t)c);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

// Stubs for compatibility
char uart_getc(void) { return 0; }
int uart_available(void) { return 0; }
int uart_getc_timeout(char *c, uint32_t timeout_ms) { (void)c; (void)timeout_ms; return -1; }
void uart_gets(char *buffer, uint32_t max_len) { (void)buffer; (void)max_len; }
void uart_flush_rx(void) { }
