/* UART Header for Raspberry Pi */

#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);

// Enhanced input functions
char uart_getc(void);                          // Blocking character receive
int uart_available(void);                       // Check if character available (non-blocking)
int uart_getc_timeout(char *c, uint32_t timeout_ms);  // Receive with timeout
void uart_gets(char *buffer, uint32_t max_len); // Receive string (until newline)
void uart_flush_rx(void);                       // Flush receive buffer

#endif