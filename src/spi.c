/* SPI Controller Implementation for Raspberry Pi */

#include "spi.h"

#define NULL ((void*)0)

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

static uint32_t get_spi_base(uint8_t bus) {
    switch (bus) {
        case SPI_BUS_0: return SPI0_BASE;
        case SPI_BUS_1: return SPI1_BASE;
        case SPI_BUS_2: return SPI2_BASE;
        default: return SPI0_BASE;
    }
}

void spi_init(uint8_t bus) {
    uint32_t base = get_spi_base(bus);

    // Clear FIFOs and disable SPI
    mmio_write(base + SPI_CS, SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX);

    // Set default clock divider (250MHz / 256 = ~1MHz)
    mmio_write(base + SPI_CLK, 256);
}

void spi_set_mode(uint8_t bus, uint8_t mode) {
    uint32_t base = get_spi_base(bus);
    uint32_t cs = mmio_read(base + SPI_CS);

    // Clear CPOL and CPHA
    cs &= ~(SPI_CS_CPOL | SPI_CS_CPHA);

    // Set new mode
    if (mode & 0x01) cs |= SPI_CS_CPHA;
    if (mode & 0x02) cs |= SPI_CS_CPOL;

    mmio_write(base + SPI_CS, cs);
}

void spi_set_clock(uint8_t bus, uint32_t divider) {
    uint32_t base = get_spi_base(bus);
    mmio_write(base + SPI_CLK, divider);
}

int spi_transfer(uint8_t bus, uint8_t cs, const uint8_t *tx_data, uint8_t *rx_data, uint32_t len) {
    uint32_t base = get_spi_base(bus);

    // Set chip select
    uint32_t cs_reg = mmio_read(base + SPI_CS);
    cs_reg &= ~SPI_CS_CS;
    cs_reg |= (cs & 0x03);
    mmio_write(base + SPI_CS, cs_reg);

    // Clear FIFOs
    mmio_write(base + SPI_CS, cs_reg | SPI_CS_CLEAR_TX | SPI_CS_CLEAR_RX);

    // Set transfer active
    mmio_write(base + SPI_CS, cs_reg | SPI_CS_TA);

    uint32_t tx_count = 0;
    uint32_t rx_count = 0;

    while (tx_count < len || rx_count < len) {
        // Send data
        if (tx_count < len && (mmio_read(base + SPI_CS) & SPI_CS_TXD)) {
            uint8_t data = tx_data ? tx_data[tx_count] : 0xFF;
            mmio_write(base + SPI_FIFO, data);
            tx_count++;
        }

        // Receive data
        if (rx_count < len && (mmio_read(base + SPI_CS) & SPI_CS_RXD)) {
            uint8_t data = mmio_read(base + SPI_FIFO) & 0xFF;
            if (rx_data) rx_data[rx_count] = data;
            rx_count++;
        }
    }

    // Wait for transfer complete
    while (!(mmio_read(base + SPI_CS) & SPI_CS_DONE));

    // Clear transfer active
    mmio_write(base + SPI_CS, cs_reg);

    return 0;
}

int spi_write(uint8_t bus, uint8_t cs, const uint8_t *data, uint32_t len) {
    return spi_transfer(bus, cs, data, NULL, len);
}

int spi_read(uint8_t bus, uint8_t cs, uint8_t *data, uint32_t len) {
    return spi_transfer(bus, cs, NULL, data, len);
}