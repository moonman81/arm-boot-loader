/* SPI Controller Header for Raspberry Pi */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>

// SPI Controller registers (Pi 4)
#define SPI0_BASE 0xFE204000
#define SPI1_BASE 0xFE204800
#define SPI2_BASE 0xFE205000

#define SPI_CS   0x00  // Control and Status
#define SPI_FIFO 0x04  // TX and RX FIFO
#define SPI_CLK  0x08  // Clock Divider
#define SPI_DLEN 0x0C  // Data Length
#define SPI_LTOH 0x10  // LOSSI mode TOH
#define SPI_DC   0x14  // DMA Controls

// Control and Status register bits
#define SPI_CS_LEN_LONG  (1 << 25)
#define SPI_CS_DMA_LEN   (1 << 24)
#define SPI_CS_CSPOL2    (1 << 23)
#define SPI_CS_CSPOL1    (1 << 22)
#define SPI_CS_CSPOL0    (1 << 21)
#define SPI_CS_RXF       (1 << 20)
#define SPI_CS_RXR       (1 << 19)
#define SPI_CS_TXD       (1 << 18)
#define SPI_CS_RXD       (1 << 17)
#define SPI_CS_DONE      (1 << 16)
#define SPI_CS_LEN       (1 << 13)
#define SPI_CS_REN       (1 << 12)
#define SPI_CS_ADCS      (1 << 11)
#define SPI_CS_INTR      (1 << 10)
#define SPI_CS_INTD      (1 << 9)
#define SPI_CS_DMAEN     (1 << 8)
#define SPI_CS_TA        (1 << 7)
#define SPI_CS_CSPOL     (1 << 6)
#define SPI_CS_CLEAR_TX  (1 << 5)
#define SPI_CS_CLEAR_RX  (1 << 4)
#define SPI_CS_CPOL      (1 << 3)
#define SPI_CS_CPHA      (1 << 2)
#define SPI_CS_CS        (1 << 0)

// SPI bus numbers
#define SPI_BUS_0 0
#define SPI_BUS_1 1
#define SPI_BUS_2 2

// SPI chip select
#define SPI_CS_0 0
#define SPI_CS_1 1
#define SPI_CS_2 2

// SPI modes
#define SPI_MODE_0 0  // CPOL=0, CPHA=0
#define SPI_MODE_1 1  // CPOL=0, CPHA=1
#define SPI_MODE_2 2  // CPOL=1, CPHA=0
#define SPI_MODE_3 3  // CPOL=1, CPHA=1

void spi_init(uint8_t bus);
void spi_set_mode(uint8_t bus, uint8_t mode);
void spi_set_clock(uint8_t bus, uint32_t divider);
int spi_transfer(uint8_t bus, uint8_t cs, const uint8_t *tx_data, uint8_t *rx_data, uint32_t len);
int spi_write(uint8_t bus, uint8_t cs, const uint8_t *data, uint32_t len);
int spi_read(uint8_t bus, uint8_t cs, uint8_t *data, uint32_t len);

#endif