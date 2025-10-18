/* DMA Controller Header for Raspberry Pi */

#ifndef DMA_H
#define DMA_H

#include <stdint.h>

// DMA Controller registers (Pi 4)
#define DMA_BASE 0xFE007000
#define DMA_CS        (DMA_BASE + 0x00)  // Control and Status
#define DMA_CONBLK_AD (DMA_BASE + 0x04)  // Control Block Address
#define DMA_TI        (DMA_BASE + 0x08)  // Transfer Information
#define DMA_SOURCE_AD (DMA_BASE + 0x0C)  // Source Address
#define DMA_DEST_AD   (DMA_BASE + 0x10)  // Destination Address
#define DMA_TXFR_LEN  (DMA_BASE + 0x14)  // Transfer Length
#define DMA_STRIDE    (DMA_BASE + 0x18)  // 2D Stride
#define DMA_NEXTCONBK (DMA_BASE + 0x1C)  // Next Control Block Address
#define DMA_DEBUG     (DMA_BASE + 0x20)  // Debug

// DMA Control and Status bits
#define DMA_CS_RESET    (1 << 31)
#define DMA_CS_ABORT    (1 << 30)
#define DMA_CS_DISDEBUG (1 << 29)
#define DMA_CS_WAIT_FOR_OUTSTANDING_WRITES (1 << 28)
#define DMA_CS_PANIC_PRIORITY(x) ((x) << 20)
#define DMA_CS_PRIORITY(x) ((x) << 16)
#define DMA_CS_ERROR    (1 << 8)
#define DMA_CS_WAITING_FOR_OUTSTANDING_WRITES (1 << 6)
#define DMA_CS_DREQ_STOPS_DMA (1 << 5)
#define DMA_CS_PAUSED   (1 << 4)
#define DMA_CS_DREQ     (1 << 3)
#define DMA_CS_INT      (1 << 2)
#define DMA_CS_END      (1 << 1)
#define DMA_CS_ACTIVE   (1 << 0)

// DMA Transfer Information bits
#define DMA_TI_NO_WIDE_BURSTS (1 << 26)
#define DMA_TI_WAITS(x) ((x) << 21)
#define DMA_TI_PERMAP(x) ((x) << 16)
#define DMA_TI_BURST_LENGTH(x) ((x) << 12)
#define DMA_TI_SRC_IGNORE (1 << 11)
#define DMA_TI_SRC_DREQ (1 << 10)
#define DMA_TI_SRC_WIDTH (1 << 9)
#define DMA_TI_SRC_INC (1 << 8)
#define DMA_TI_DEST_IGNORE (1 << 7)
#define DMA_TI_DEST_DREQ (1 << 6)
#define DMA_TI_DEST_WIDTH (1 << 5)
#define DMA_TI_DEST_INC (1 << 4)
#define DMA_TI_WAIT_RESP (1 << 3)
#define DMA_TI_TDMODE (1 << 1)
#define DMA_TI_INTEN (1 << 0)

// DMA Control Block structure
typedef struct {
    uint32_t ti;        // Transfer information
    uint32_t source_ad; // Source address
    uint32_t dest_ad;   // Destination address
    uint32_t txfr_len;  // Transfer length
    uint32_t stride;    // 2D stride
    uint32_t nextconbk; // Next control block
    uint32_t reserved[2];
} dma_control_block_t;

// DMA channel numbers
#define DMA_CHANNEL_0 0
#define DMA_CHANNEL_1 1
#define DMA_CHANNEL_2 2
#define DMA_CHANNEL_3 3
#define DMA_CHANNEL_4 4
#define DMA_CHANNEL_5 5
#define DMA_CHANNEL_6 6
#define DMA_CHANNEL_7 7

// DMA transfer types
#define DMA_MEM_TO_MEM 0
#define DMA_MEM_TO_PERIPH 1
#define DMA_PERIPH_TO_MEM 2

void dma_init(void);
int dma_transfer(uint8_t channel, void *src, void *dst, uint32_t length, uint32_t ti);
int dma_transfer_async(uint8_t channel, dma_control_block_t *cb);
int dma_wait_transfer(uint8_t channel);
void dma_abort_transfer(uint8_t channel);

#endif