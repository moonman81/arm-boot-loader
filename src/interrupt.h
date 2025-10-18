/* Interrupt Controller Header for Raspberry Pi */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

// GIC-400 Distributor registers
#define GICD_BASE 0xFF841000
#define GICD_CTLR       (GICD_BASE + 0x000)
#define GICD_TYPER      (GICD_BASE + 0x004)
#define GICD_IIDR       (GICD_BASE + 0x008)
#define GICD_IGROUPR    (GICD_BASE + 0x080)
#define GICD_ISENABLER  (GICD_BASE + 0x100)
#define GICD_ICENABLER  (GICD_BASE + 0x180)
#define GICD_ISPENDR    (GICD_BASE + 0x200)
#define GICD_ICPENDR    (GICD_BASE + 0x280)
#define GICD_ISACTIVER  (GICD_BASE + 0x300)
#define GICD_ICACTIVER  (GICD_BASE + 0x380)
#define GICD_IPRIORITYR (GICD_BASE + 0x400)
#define GICD_ITARGETSR  (GICD_BASE + 0x800)
#define GICD_ICFGR      (GICD_BASE + 0xC00)
#define GICD_SGIR       (GICD_BASE + 0xF00)

// GIC-400 CPU Interface registers
#define GICC_BASE 0xFF842000
#define GICC_CTLR       (GICC_BASE + 0x000)
#define GICC_PMR        (GICC_BASE + 0x004)
#define GICC_BPR        (GICC_BASE + 0x008)
#define GICC_IAR        (GICC_BASE + 0x00C)
#define GICC_EOIR       (GICC_BASE + 0x010)
#define GICC_RPR        (GICC_BASE + 0x014)
#define GICC_HPPIR      (GICC_BASE + 0x018)
#define GICC_ABPR       (GICC_BASE + 0x01C)
#define GICC_AIAR       (GICC_BASE + 0x020)
#define GICC_AEOIR      (GICC_BASE + 0x024)
#define GICC_AHPPIR     (GICC_BASE + 0x028)

// Interrupt priorities
#define INTERRUPT_PRIORITY_LOW    0xA0
#define INTERRUPT_PRIORITY_NORMAL 0x80
#define INTERRUPT_PRIORITY_HIGH   0x60
#define INTERRUPT_PRIORITY_CRIT   0x40

// Common interrupt numbers
#define INTERRUPT_TIMER 64
#define INTERRUPT_GPIO  113
#define INTERRUPT_SD    62

typedef void (*interrupt_handler_t)(void);

void interrupt_init(void);
void interrupt_enable(uint32_t irq);
void interrupt_disable(uint32_t irq);
void interrupt_register_handler(uint32_t irq, interrupt_handler_t handler);
void interrupt_set_priority(uint32_t irq, uint8_t priority);
void interrupt_set_target(uint32_t irq, uint8_t cpu_mask);

// Default interrupt handlers
void timer_interrupt_handler(void);
void gpio_interrupt_handler(void);

#endif