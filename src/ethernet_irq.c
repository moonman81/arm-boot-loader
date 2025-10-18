/* Interrupt-Driven Ethernet Reception for Raspberry Pi */

#include "ethernet.h"
#include "interrupt.h"
#include "timer.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Custom memory functions
static void *memcpy(void *dest, const void *src, uint32_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}

// Ethernet GENET interrupt definitions
#define ETH_IRQ_STATUS       (ETH_BASE + 0x200)  // Interrupt Status
#define ETH_IRQ_ENABLE       (ETH_BASE + 0x204)  // Interrupt Enable
#define ETH_IRQ_CLEAR        (ETH_BASE + 0x208)  // Interrupt Clear

// Interrupt bits
#define ETH_IRQ_RX_DONE      (1 << 0)   // RX packet received
#define ETH_IRQ_TX_DONE      (1 << 1)   // TX packet sent
#define ETH_IRQ_RX_ERROR     (1 << 2)   // RX error
#define ETH_IRQ_TX_ERROR     (1 << 3)   // TX error
#define ETH_IRQ_LINK_CHANGE  (1 << 4)   // Link status changed
#define ETH_IRQ_RX_OVERFLOW  (1 << 5)   // RX FIFO overflow

// GIC interrupt number for Ethernet (platform-specific)
#define ETH_GIC_IRQ          157        // Raspberry Pi 4/5

// RX packet queue
#define RX_QUEUE_SIZE 16

typedef struct {
    ethernet_frame_t frame;
    uint16_t length;
    uint64_t timestamp;
    int valid;
} rx_packet_entry_t;

static struct {
    rx_packet_entry_t queue[RX_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
    volatile uint32_t dropped;
    volatile uint32_t errors;
    volatile uint32_t interrupts;
} rx_queue;

// Statistics
static struct {
    uint32_t packets_received;
    uint32_t packets_dropped;
    uint32_t rx_errors;
    uint32_t rx_overflows;
    uint32_t link_changes;
} eth_stats;

// MMIO helpers
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// Ethernet interrupt handler
static void ethernet_irq_handler(void *context) {
    (void)context; // Unused

    uint32_t irq_status = mmio_read(ETH_IRQ_STATUS);
    rx_queue.interrupts++;

    // Handle RX packet
    if (irq_status & ETH_IRQ_RX_DONE) {
        ethernet_frame_t rx_frame;
        uint16_t rx_len;

        // Read packet from hardware (polling the actual FIFO)
        if (ethernet_receive_frame(&rx_frame, &rx_len) == 0) {
            // Add to queue if space available
            if (rx_queue.count < RX_QUEUE_SIZE) {
                uint32_t tail_idx = rx_queue.tail;

                memcpy(&rx_queue.queue[tail_idx].frame, &rx_frame, sizeof(ethernet_frame_t));
                rx_queue.queue[tail_idx].length = rx_len;
                rx_queue.queue[tail_idx].timestamp = timer_get_counter();
                rx_queue.queue[tail_idx].valid = 1;

                rx_queue.tail = (rx_queue.tail + 1) % RX_QUEUE_SIZE;
                rx_queue.count++;
                eth_stats.packets_received++;
            } else {
                // Queue full, drop packet
                rx_queue.dropped++;
                eth_stats.packets_dropped++;
            }
        }

        // Clear interrupt
        mmio_write(ETH_IRQ_CLEAR, ETH_IRQ_RX_DONE);
    }

    // Handle RX error
    if (irq_status & ETH_IRQ_RX_ERROR) {
        rx_queue.errors++;
        eth_stats.rx_errors++;
        mmio_write(ETH_IRQ_CLEAR, ETH_IRQ_RX_ERROR);
    }

    // Handle RX overflow
    if (irq_status & ETH_IRQ_RX_OVERFLOW) {
        eth_stats.rx_overflows++;
        mmio_write(ETH_IRQ_CLEAR, ETH_IRQ_RX_OVERFLOW);
    }

    // Handle link change
    if (irq_status & ETH_IRQ_LINK_CHANGE) {
        eth_stats.link_changes++;
        mmio_write(ETH_IRQ_CLEAR, ETH_IRQ_LINK_CHANGE);
    }

    // Handle TX done (optional)
    if (irq_status & ETH_IRQ_TX_DONE) {
        mmio_write(ETH_IRQ_CLEAR, ETH_IRQ_TX_DONE);
    }

    // Clear any remaining interrupts
    mmio_write(ETH_IRQ_CLEAR, irq_status);
}

// Initialize interrupt-driven Ethernet reception
int ethernet_irq_init(void) {
    // Initialize queue
    memset(&rx_queue, 0, sizeof(rx_queue));
    memset(&eth_stats, 0, sizeof(eth_stats));

    // Register interrupt handler
    if (interrupt_register_handler(ETH_GIC_IRQ, ethernet_irq_handler, NULL) < 0) {
        return -1;
    }

    // Configure interrupt priority (lower number = higher priority)
    interrupt_set_priority(ETH_GIC_IRQ, 0x40);

    // Enable interrupt in GIC
    interrupt_enable(ETH_GIC_IRQ);

    // Enable Ethernet interrupts in controller
    uint32_t irq_enable = ETH_IRQ_RX_DONE | ETH_IRQ_RX_ERROR |
                         ETH_IRQ_RX_OVERFLOW | ETH_IRQ_LINK_CHANGE;
    mmio_write(ETH_IRQ_ENABLE, irq_enable);

    return 0;
}

// Dequeue a received packet (non-blocking)
int ethernet_irq_receive(ethernet_frame_t *frame, uint16_t *length) {
    if (!frame || !length) return -1;

    // Disable interrupts briefly to access queue
    interrupt_disable(ETH_GIC_IRQ);

    if (rx_queue.count == 0) {
        interrupt_enable(ETH_GIC_IRQ);
        return -1; // No packets
    }

    // Dequeue packet
    uint32_t head_idx = rx_queue.head;

    if (rx_queue.queue[head_idx].valid) {
        memcpy(frame, &rx_queue.queue[head_idx].frame, sizeof(ethernet_frame_t));
        *length = rx_queue.queue[head_idx].length;

        rx_queue.queue[head_idx].valid = 0;
        rx_queue.head = (rx_queue.head + 1) % RX_QUEUE_SIZE;
        rx_queue.count--;

        interrupt_enable(ETH_GIC_IRQ);
        return 0;
    }

    interrupt_enable(ETH_GIC_IRQ);
    return -1;
}

// Dequeue with timeout
int ethernet_irq_receive_timeout(ethernet_frame_t *frame, uint16_t *length, uint32_t timeout_ms) {
    uint64_t start = timer_get_counter();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;

    while ((timer_get_counter() - start) < timeout_us) {
        if (ethernet_irq_receive(frame, length) == 0) {
            return 0; // Success
        }
        // Small delay to avoid busy-waiting
        timer_delay_ms(1);
    }

    return -1; // Timeout
}

// Check if packets are available
int ethernet_irq_available(void) {
    return rx_queue.count;
}

// Flush receive queue
void ethernet_irq_flush(void) {
    interrupt_disable(ETH_GIC_IRQ);

    memset(&rx_queue, 0, sizeof(rx_queue));

    interrupt_enable(ETH_GIC_IRQ);
}

// Get statistics
void ethernet_irq_get_stats(uint32_t *received, uint32_t *dropped, uint32_t *errors) {
    if (received) *received = eth_stats.packets_received;
    if (dropped) *dropped = eth_stats.packets_dropped;
    if (errors) *errors = eth_stats.rx_errors;
}

// Shutdown interrupt-driven reception
void ethernet_irq_shutdown(void) {
    // Disable Ethernet interrupts
    mmio_write(ETH_IRQ_ENABLE, 0);

    // Disable GIC interrupt
    interrupt_disable(ETH_GIC_IRQ);

    // Unregister handler
    interrupt_unregister_handler(ETH_GIC_IRQ);

    // Flush queue
    ethernet_irq_flush();
}
