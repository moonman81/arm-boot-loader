/* Interrupt-Driven Ethernet Reception Header */

#ifndef ETHERNET_IRQ_H
#define ETHERNET_IRQ_H

#include <stdint.h>
#include "ethernet.h"

// Initialize interrupt-driven Ethernet reception
int ethernet_irq_init(void);

// Receive packet (non-blocking, returns -1 if no packet available)
int ethernet_irq_receive(ethernet_frame_t *frame, uint16_t *length);

// Receive packet with timeout (blocking up to timeout_ms)
int ethernet_irq_receive_timeout(ethernet_frame_t *frame, uint16_t *length, uint32_t timeout_ms);

// Check if packets are available in queue
int ethernet_irq_available(void);

// Flush receive queue
void ethernet_irq_flush(void);

// Get reception statistics
void ethernet_irq_get_stats(uint32_t *received, uint32_t *dropped, uint32_t *errors);

// Shutdown interrupt-driven reception
void ethernet_irq_shutdown(void);

#endif
