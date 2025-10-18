/* PWM Controller Header for Raspberry Pi */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>

// PWM Controller registers (Pi 4)
#define PWM_BASE 0xFE20C000

#define PWM_CTL  0x00  // Control
#define PWM_STA  0x04  // Status
#define PWM_DMAC 0x08  // DMA Configuration
#define PWM_RNG1 0x10  // Channel 1 Range
#define PWM_DAT1 0x14  // Channel 1 Data
#define PWM_FIF1 0x18  // FIFO Input
#define PWM_RNG2 0x20  // Channel 2 Range
#define PWM_DAT2 0x24  // Channel 2 Data

// Control register bits
#define PWM_CTL_MSEN2 (1 << 15)  // Channel 2 M/S mode
#define PWM_CTL_USEF2 (1 << 13)  // Channel 2 FIFO
#define PWM_CTL_POLA2 (1 << 12)  // Channel 2 Polarity
#define PWM_CTL_SBIT2 (1 << 11)  // Channel 2 Silence bit
#define PWM_CTL_RPTL2 (1 << 10)  // Channel 2 Repeat last
#define PWM_CTL_MODE2 (1 << 9)   // Channel 2 Mode
#define PWM_CTL_PWEN2 (1 << 8)   // Channel 2 Enable
#define PWM_CTL_MSEN1 (1 << 7)   // Channel 1 M/S mode
#define PWM_CTL_CLRF1 (1 << 6)   // Clear FIFO
#define PWM_CTL_USEF1 (1 << 5)   // Channel 1 FIFO
#define PWM_CTL_POLA1 (1 << 4)   // Channel 1 Polarity
#define PWM_CTL_SBIT1 (1 << 3)   // Channel 1 Silence bit
#define PWM_CTL_RPTL1 (1 << 2)   // Channel 1 Repeat last
#define PWM_CTL_MODE1 (1 << 1)   // Channel 1 Mode
#define PWM_CTL_PWEN1 (1 << 0)   // Channel 1 Enable

// Status register bits
#define PWM_STA_STA4 (1 << 12)  // Channel 4 State
#define PWM_STA_STA3 (1 << 11)  // Channel 3 State
#define PWM_STA_STA2 (1 << 10)  // Channel 2 State
#define PWM_STA_STA1 (1 << 9)   // Channel 1 State
#define PWM_STA_BERR (1 << 8)   // Bus Error
#define PWM_STA_GAPO4 (1 << 7)  // Channel 4 Gap occurred
#define PWM_STA_GAPO3 (1 << 6)  // Channel 3 Gap occurred
#define PWM_STA_GAPO2 (1 << 5)  // Channel 2 Gap occurred
#define PWM_STA_GAPO1 (1 << 4)  // Channel 1 Gap occurred
#define PWM_STA_RERR1 (1 << 3)  // FIFO Read Error
#define PWM_STA_WERR1 (1 << 2)  // FIFO Write Error
#define PWM_STA_EMPT1 (1 << 1)  // FIFO Empty
#define PWM_STA_FULL1 (1 << 0)  // FIFO Full

// PWM channels
#define PWM_CHANNEL_1 1
#define PWM_CHANNEL_2 2

// PWM modes
#define PWM_MODE_PWM 0
#define PWM_MODE_SERIALIZER 1

void pwm_init(void);
void pwm_set_mode(uint8_t channel, uint8_t mode);
void pwm_set_range(uint8_t channel, uint32_t range);
void pwm_set_data(uint8_t channel, uint32_t data);
void pwm_enable(uint8_t channel);
void pwm_disable(uint8_t channel);
void pwm_set_polarity(uint8_t channel, uint8_t polarity);

#endif