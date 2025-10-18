/* I2C Controller Header for Raspberry Pi */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// I2C Controller registers (Pi 4)
#define I2C0_BASE 0xFE804000
#define I2C1_BASE 0xFE805000
#define I2C2_BASE 0xFE806000

#define I2C_C       0x00  // Control
#define I2C_S       0x04  // Status
#define I2C_DLEN    0x08  // Data Length
#define I2C_A       0x0C  // Slave Address
#define I2C_FIFO    0x10  // Data FIFO
#define I2C_DIV     0x14  // Clock Divider
#define I2C_DEL     0x18  // Data Delay
#define I2C_CLKT    0x1C  // Clock Stretch Timeout

// Control register bits
#define I2C_C_I2CEN    (1 << 15)
#define I2C_C_INTR     (1 << 10)
#define I2C_C_INTT     (1 << 9)
#define I2C_C_INTD     (1 << 8)
#define I2C_C_ST       (1 << 7)
#define I2C_C_CLEAR    (1 << 4)
#define I2C_C_READ     (1 << 0)

// Status register bits
#define I2C_S_CLKT     (1 << 9)
#define I2C_S_ERR      (1 << 8)
#define I2C_S_RXF      (1 << 7)
#define I2C_S_TXE      (1 << 6)
#define I2C_S_RXD      (1 << 5)
#define I2C_S_TXD      (1 << 4)
#define I2C_S_RXR      (1 << 3)
#define I2C_S_TXW      (1 << 2)
#define I2C_S_DONE     (1 << 1)
#define I2C_S_TA       (1 << 0)

// I2C bus numbers
#define I2C_BUS_0 0
#define I2C_BUS_1 1
#define I2C_BUS_2 2

// I2C transfer flags
#define I2C_READ  1
#define I2C_WRITE 0

void i2c_init(uint8_t bus);
int i2c_write(uint8_t bus, uint8_t addr, const uint8_t *data, uint32_t len);
int i2c_read(uint8_t bus, uint8_t addr, uint8_t *data, uint32_t len);
int i2c_write_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t value);
int i2c_read_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *value);

#endif