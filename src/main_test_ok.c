/* Test - Timer Only */

#include "uart.h"
#include "timer.h"

void main(void) {
    uart_init();
    timer_init();

    uart_puts("\nMinimal v0.3 TEST\n");
    uart_puts("UART+Timer OK\n");
    
    timer_delay_ms(500);
    uart_puts("Delay OK\n\n");

    while (1) {
        __asm__ volatile("wfi");
    }
}
