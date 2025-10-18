/* Ethernet Controller Implementation for Raspberry Pi */

#include "ethernet.h"
#include "memory.h"

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// Local memcpy for freestanding environment
static void *memcpy(void *dest, const void *src, uint32_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

// DMA ring buffers
#define TX_RING_SIZE 8
#define RX_RING_SIZE 8

static dma_desc_t tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static dma_desc_t rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
static uint8_t tx_buffers[TX_RING_SIZE][2048] __attribute__((aligned(16)));
static uint8_t rx_buffers[RX_RING_SIZE][2048] __attribute__((aligned(16)));

static uint32_t tx_head = 0;
static uint32_t tx_tail = 0;
static uint32_t rx_head = 0;

void ethernet_init(void) {
    // Reset the controller
    mmio_write(ETH_RBUF_FLUSH_CTRL, 0xFFFFFFFF);
    mmio_write(ETH_TBUF_FLUSH_CTRL, 0xFFFFFFFF);

    // Set maximum frame length
    mmio_write(ETH_UMAC_MAX_FRAME_LEN, 1518);

    // Enable padding and CRC
    uint32_t cmd = mmio_read(ETH_UMAC_CMD);
    cmd |= ETH_CMD_PAD_EN | ETH_CMD_CRC_EN;
    mmio_write(ETH_UMAC_CMD, cmd);

    // Initialize DMA rings
    for (int i = 0; i < TX_RING_SIZE; i++) {
        tx_ring[i].addr = (uint32_t)&tx_buffers[i];
        tx_ring[i].length = 0; // Empty
    }

    for (int i = 0; i < RX_RING_SIZE; i++) {
        rx_ring[i].addr = (uint32_t)&rx_buffers[i];
        rx_ring[i].length = 2048 | (1 << 31); // Max length, empty
    }

    // Set up DMA rings
    mmio_write(ETH_DMA_TX_RING0_ADDR, (uint32_t)tx_ring);
    mmio_write(ETH_DMA_TX_RING0_SIZE, TX_RING_SIZE);
    mmio_write(ETH_DMA_TX_RING0_CTRL, RING_CTRL_EN | RING_CTRL_INTR_EN);

    mmio_write(ETH_DMA_RX_RING0_ADDR, (uint32_t)rx_ring);
    mmio_write(ETH_DMA_RX_RING0_SIZE, RX_RING_SIZE);
    mmio_write(ETH_DMA_RX_RING0_CTRL, RING_CTRL_EN | RING_CTRL_INTR_EN);

    // Enable DMA
    mmio_write(ETH_DMA_CTRL, DMA_TX_EN | DMA_RX_EN | DMA_TX_RING_EN | DMA_RX_RING_EN);
}

void ethernet_set_mac_address(const uint8_t *mac) {
    uint32_t mac_low = (mac[0] << 24) | (mac[1] << 16) | (mac[2] << 8) | mac[3];
    uint32_t mac_high = (mac[4] << 8) | mac[5];

    mmio_write(ETH_UMAC_MAC0, mac_low);
    mmio_write(ETH_UMAC_MAC1, mac_high);
}

int ethernet_send_frame(const ethernet_frame_t *frame, uint16_t length) {
    // Check if TX ring has space
    uint32_t status = mmio_read(ETH_DMA_TX_RING0_STATUS);
    if (status & DMA_TX_RING_FULL) {
        return -1; // Ring full
    }

    // Copy frame to TX buffer
    uint32_t buffer_idx = tx_tail;
    uint8_t *buffer = tx_buffers[buffer_idx];
    memcpy(buffer, frame, length);

    // Set up descriptor
    tx_ring[buffer_idx].length = length | (1 << 31); // Length with ownership bit

    // Update tail pointer
    tx_tail = (tx_tail + 1) % TX_RING_SIZE;

    // Notify DMA of new frame
    mmio_write(ETH_DMA_TX_RING0_STATUS, 1); // Ring doorbell

    return 0;
}

int ethernet_receive_frame(ethernet_frame_t *frame, uint16_t *length) {
    // Check if RX ring has data
    uint32_t status = mmio_read(ETH_DMA_RX_RING0_STATUS);
    if (status & DMA_RX_RING_EMPTY) {
        return -1; // No data
    }

    // Get frame from RX buffer
    uint32_t buffer_idx = rx_head;
    uint8_t *buffer = rx_buffers[buffer_idx];
    uint32_t desc_length = rx_ring[buffer_idx].length;

    if (!(desc_length & (1 << 31))) { // Check ownership
        uint16_t frame_length = desc_length & 0x7FFF; // Mask out flags
        if (frame_length <= sizeof(ethernet_frame_t)) {
            memcpy(frame, buffer, frame_length);
            *length = frame_length;

            // Mark buffer as empty and update head
            rx_ring[buffer_idx].length = 2048 | (1 << 31);
            rx_head = (rx_head + 1) % RX_RING_SIZE;

            return 0;
        }
    }

    return -1; // Invalid frame or no data
}

uint32_t ethernet_get_status(void) {
    // Basic status - in a real implementation, check link status, etc.
    return 0; // Placeholder
}

int ethernet_enable(void) {
    uint32_t cmd = mmio_read(ETH_UMAC_CMD);
    cmd |= ETH_CMD_TX_EN | ETH_CMD_RX_EN;
    mmio_write(ETH_UMAC_CMD, cmd);

    return 0;
}

int ethernet_disable(void) {
    uint32_t cmd = mmio_read(ETH_UMAC_CMD);
    cmd &= ~(ETH_CMD_TX_EN | ETH_CMD_RX_EN);
    mmio_write(ETH_UMAC_CMD, cmd);

    return 0;
}