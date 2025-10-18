/* Interrupt Controller Implementation for Raspberry Pi */

#include "interrupt.h"
#include "timer.h"
#include "gpio.h"
#include "hardware.h"

#define NULL ((void*)0)

// Interrupt handler table
#define MAX_INTERRUPTS 256
static interrupt_handler_t interrupt_handlers[MAX_INTERRUPTS];

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void interrupt_init(void) {
    // Initialize handler table
    for (int i = 0; i < MAX_INTERRUPTS; i++) {
        interrupt_handlers[i] = NULL;
    }

    // Disable all interrupts in distributor
    for (int i = 0; i < 32; i++) {
        mmio_write(GICD_ICENABLER + i * 4, 0xFFFFFFFF);
        mmio_write(GICD_ICPENDR + i * 4, 0xFFFFFFFF);
        mmio_write(GICD_ICACTIVER + i * 4, 0xFFFFFFFF);
    }

    // Set all interrupts to group 0 (secure)
    for (int i = 0; i < 32; i++) {
        mmio_write(GICD_IGROUPR + i * 4, 0x00000000);
    }

    // Set priority mask to allow all priorities
    mmio_write(GICC_PMR, 0xFF);

    // Enable CPU interface
    mmio_write(GICC_CTLR, 0x01);

    // Enable distributor
    mmio_write(GICD_CTLR, 0x01);
}

void interrupt_enable(uint32_t irq) {
    if (irq >= MAX_INTERRUPTS) return;

    uint32_t reg_index = irq / 32;
    uint32_t bit_index = irq % 32;

    mmio_write(GICD_ISENABLER + reg_index * 4, 1 << bit_index);
}

void interrupt_disable(uint32_t irq) {
    if (irq >= MAX_INTERRUPTS) return;

    uint32_t reg_index = irq / 32;
    uint32_t bit_index = irq % 32;

    mmio_write(GICD_ICENABLER + reg_index * 4, 1 << bit_index);
}

void interrupt_register_handler(uint32_t irq, interrupt_handler_t handler) {
    if (irq >= MAX_INTERRUPTS) return;
    interrupt_handlers[irq] = handler;
}

void interrupt_set_priority(uint32_t irq, uint8_t priority) {
    if (irq >= MAX_INTERRUPTS) return;

    uint32_t reg_index = irq / 4;
    uint32_t byte_index = irq % 4;

    uint32_t reg_addr = GICD_IPRIORITYR + reg_index * 4;
    uint32_t reg_value = mmio_read(reg_addr);

    // Clear the byte for this interrupt
    reg_value &= ~(0xFF << (byte_index * 8));
    // Set the priority
    reg_value |= (priority << (byte_index * 8));

    mmio_write(reg_addr, reg_value);
}

void interrupt_set_target(uint32_t irq, uint8_t cpu_mask) {
    if (irq >= MAX_INTERRUPTS) return;

    uint32_t reg_index = irq / 4;
    uint32_t byte_index = irq % 4;

    uint32_t reg_addr = GICD_ITARGETSR + reg_index * 4;
    uint32_t reg_value = mmio_read(reg_addr);

    // Clear the byte for this interrupt
    reg_value &= ~(0xFF << (byte_index * 8));
    // Set the target CPUs
    reg_value |= (cpu_mask << (byte_index * 8));

    mmio_write(reg_addr, reg_value);
}

// Default interrupt handlers
void timer_interrupt_handler(void) {
    // Clear the ARM timer interrupt
    uint32_t arm_timer_base = get_arm_timer_base();
    mmio_write(arm_timer_base + ARM_TIMER_IRQCLR_OFFSET, 0);

    // Handle timer interrupt (e.g., schedule tasks, update counters)
    // For now, just acknowledge
}

void gpio_interrupt_handler(void) {
    uint32_t gpio_base = get_gpio_base();
    // Clear GPIO interrupt events
    for (int i = 0; i < 2; i++) {
        uint32_t events = mmio_read(gpio_base + GPIO_GPEDS0_OFFSET + i * 4);
        if (events) {
            mmio_write(gpio_base + GPIO_GPEDS0_OFFSET + i * 4, events);
        }
    }

    // Handle GPIO interrupt (e.g., button press, pin change)
    // For now, just acknowledge
}

// Interrupt handler dispatcher (called from assembly)
void interrupt_handler(void) {
    // Read the interrupt acknowledge register
    uint32_t iar = mmio_read(GICC_IAR);
    uint32_t irq = iar & 0x3FF;

    if (irq < MAX_INTERRUPTS && interrupt_handlers[irq]) {
        interrupt_handlers[irq]();
    }

    // Write end of interrupt
    mmio_write(GICC_EOIR, iar);
}