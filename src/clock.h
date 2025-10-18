/* Clock Manager Header for Raspberry Pi */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

// Clock Manager registers (Pi 4)
#define CM_BASE 0xFE101000

// General Purpose clocks
#define CM_GP0CTL (CM_BASE + 0x070)
#define CM_GP0DIV (CM_BASE + 0x074)
#define CM_GP1CTL (CM_BASE + 0x078)
#define CM_GP1DIV (CM_BASE + 0x07C)
#define CM_GP2CTL (CM_BASE + 0x080)
#define CM_GP2DIV (CM_BASE + 0x084)

// PWM clock
#define CM_PWMCTL (CM_BASE + 0x0A0)
#define CM_PWMDIV (CM_BASE + 0x0A4)

// UART clock
#define CM_UARTCTL (CM_BASE + 0x0F0)
#define CM_UARTDIV (CM_BASE + 0x0F4)

// Clock control bits
#define CM_CTL_ENAB    (1 << 4)
#define CM_CTL_KILL    (1 << 5)
#define CM_CTL_BUSY    (1 << 7)
#define CM_CTL_FLIP    (1 << 8)
#define CM_CTL_MASH_1  (1 << 9)
#define CM_CTL_MASH_2  (1 << 10)
#define CM_CTL_MASH_3  (1 << 11)

// Clock sources
#define CM_SRC_GND     0
#define CM_SRC_OSC     1  // 19.2MHz oscillator
#define CM_SRC_TEST    2
#define CM_SRC_PLLD    6  // 500MHz PLLD
#define CM_SRC_PLLC    5  // 1000MHz PLLC
#define CM_SRC_PLLB    4  // 1000MHz PLLB
#define CM_SRC_AUX     3  // 200MHz auxiliary oscillator

// Common clock IDs (for mailbox interface)
#define CLK_ID_EMMC    1
#define CLK_ID_UART    2
#define CLK_ID_ARM     3
#define CLK_ID_CORE    4
#define CLK_ID_V3D     5
#define CLK_ID_H264    6
#define CLK_ID_ISP     7
#define CLK_ID_SDRAM   8
#define CLK_ID_PIXEL   9
#define CLK_ID_PWM     10

// GPIO clock IDs
#define CLK_GP0        0
#define CLK_GP1        1
#define CLK_GP2        2

void clock_init(void);
int clock_configure_gp(uint8_t gp_clock, uint8_t source, uint32_t divisor);
int clock_enable_gp(uint8_t gp_clock);
int clock_disable_gp(uint8_t gp_clock);
uint32_t clock_get_rate(uint32_t clock_id);
uint32_t clock_get_max_rate(uint32_t clock_id);
int clock_set_rate(uint32_t clock_id, uint32_t rate);

#endif