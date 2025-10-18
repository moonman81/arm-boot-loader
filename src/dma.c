/* DMA Controller Implementation for Raspberry Pi */

#include "dma.h"

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void dma_init(void) {
    // DMA is always available, no initialization needed
    // Channels are enabled by default
}

int dma_transfer(uint8_t channel, void *src, void *dst, uint32_t length, uint32_t ti) {
    if (channel > 7) return -1;

    uint32_t dma_base = DMA_BASE + channel * 0x100;

    // Reset DMA channel
    mmio_write(dma_base + DMA_CS, DMA_CS_RESET);
    while (mmio_read(dma_base + DMA_CS) & DMA_CS_RESET);

    // Clear interrupts
    mmio_write(dma_base + DMA_CS, DMA_CS_INT | DMA_CS_END);

    // Set transfer parameters
    mmio_write(dma_base + DMA_SOURCE_AD, (uint32_t)src);
    mmio_write(dma_base + DMA_DEST_AD, (uint32_t)dst);
    mmio_write(dma_base + DMA_TXFR_LEN, length);
    mmio_write(dma_base + DMA_TI, ti);

    // Start transfer
    mmio_write(dma_base + DMA_CS, DMA_CS_ACTIVE);

    // Wait for completion
    while (!(mmio_read(dma_base + DMA_CS) & DMA_CS_END));

    // Check for errors
    if (mmio_read(dma_base + DMA_CS) & DMA_CS_ERROR) {
        return -1;
    }

    return 0;
}

int dma_transfer_async(uint8_t channel, dma_control_block_t *cb) {
    if (channel > 7 || !cb) return -1;

    uint32_t dma_base = DMA_BASE + channel * 0x100;

    // Reset DMA channel
    mmio_write(dma_base + DMA_CS, DMA_CS_RESET);
    while (mmio_read(dma_base + DMA_CS) & DMA_CS_RESET);

    // Clear interrupts
    mmio_write(dma_base + DMA_CS, DMA_CS_INT | DMA_CS_END);

    // Set control block address
    mmio_write(dma_base + DMA_CONBLK_AD, (uint32_t)cb);

    // Start transfer
    mmio_write(dma_base + DMA_CS, DMA_CS_ACTIVE);

    return 0;
}

int dma_wait_transfer(uint8_t channel) {
    if (channel > 7) return -1;

    uint32_t dma_base = DMA_BASE + channel * 0x100;

    // Wait for completion
    while (!(mmio_read(dma_base + DMA_CS) & DMA_CS_END));

    // Check for errors
    if (mmio_read(dma_base + DMA_CS) & DMA_CS_ERROR) {
        return -1;
    }

    return 0;
}

void dma_abort_transfer(uint8_t channel) {
    if (channel > 7) return;

    uint32_t dma_base = DMA_BASE + channel * 0x100;

    // Abort transfer
    mmio_write(dma_base + DMA_CS, DMA_CS_ABORT);
}