/* I2C Controller Implementation for Raspberry Pi */

#include "i2c.h"

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

static uint32_t get_i2c_base(uint8_t bus) {
    switch (bus) {
        case I2C_BUS_0: return I2C0_BASE;
        case I2C_BUS_1: return I2C1_BASE;
        case I2C_BUS_2: return I2C2_BASE;
        default: return I2C0_BASE;
    }
}

void i2c_init(uint8_t bus) {
    uint32_t base = get_i2c_base(bus);

    // Disable I2C
    mmio_write(base + I2C_C, 0);

    // Set clock divider for 100kHz (assuming 250MHz core clock)
    // CDIV = core_clock / (2 * SCL_freq) = 250000000 / (2 * 100000) = 1250
    mmio_write(base + I2C_DIV, 1250);

    // Clear FIFOs
    mmio_write(base + I2C_C, I2C_C_CLEAR);

    // Enable I2C
    mmio_write(base + I2C_C, I2C_C_I2CEN);
}

int i2c_write(uint8_t bus, uint8_t addr, const uint8_t *data, uint32_t len) {
    uint32_t base = get_i2c_base(bus);

    // Set slave address
    mmio_write(base + I2C_A, addr);

    // Set data length
    mmio_write(base + I2C_DLEN, len);

    // Clear status
    mmio_write(base + I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);

    // Start transfer
    mmio_write(base + I2C_C, I2C_C_I2CEN | I2C_C_ST);

    // Write data
    for (uint32_t i = 0; i < len; i++) {
        // Wait for TXD
        while (!(mmio_read(base + I2C_S) & I2C_S_TXD));

        mmio_write(base + I2C_FIFO, data[i]);
    }

    // Wait for transfer complete
    while (!(mmio_read(base + I2C_S) & I2C_S_DONE));

    // Check for errors
    if (mmio_read(base + I2C_S) & I2C_S_ERR) {
        return -1;
    }

    return 0;
}

int i2c_read(uint8_t bus, uint8_t addr, uint8_t *data, uint32_t len) {
    uint32_t base = get_i2c_base(bus);

    // Set slave address
    mmio_write(base + I2C_A, addr);

    // Set data length
    mmio_write(base + I2C_DLEN, len);

    // Clear status
    mmio_write(base + I2C_S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);

    // Start transfer with read bit
    mmio_write(base + I2C_C, I2C_C_I2CEN | I2C_C_ST | I2C_C_READ);

    // Read data
    for (uint32_t i = 0; i < len; i++) {
        // Wait for RXD
        while (!(mmio_read(base + I2C_S) & I2C_S_RXD));

        data[i] = mmio_read(base + I2C_FIFO) & 0xFF;
    }

    // Wait for transfer complete
    while (!(mmio_read(base + I2C_S) & I2C_S_DONE));

    // Check for errors
    if (mmio_read(base + I2C_S) & I2C_S_ERR) {
        return -1;
    }

    return 0;
}

int i2c_write_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write(bus, addr, data, 2);
}

int i2c_read_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *value) {
    int ret = i2c_write(bus, addr, &reg, 1);
    if (ret != 0) return ret;

    return i2c_read(bus, addr, value, 1);
}