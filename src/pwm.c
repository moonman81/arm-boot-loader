/* PWM Controller Implementation for Raspberry Pi */

#include "pwm.h"

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void pwm_init(void) {
    // Disable PWM channels
    mmio_write(PWM_BASE + PWM_CTL, 0);

    // Clear status
    mmio_write(PWM_BASE + PWM_STA, PWM_STA_BERR | PWM_STA_GAPO1 | PWM_STA_GAPO2 | PWM_STA_RERR1 | PWM_STA_WERR1);
}

void pwm_set_mode(uint8_t channel, uint8_t mode) {
    uint32_t ctl = mmio_read(PWM_BASE + PWM_CTL);

    if (channel == PWM_CHANNEL_1) {
        if (mode == PWM_MODE_SERIALIZER) {
            ctl |= PWM_CTL_MODE1;
        } else {
            ctl &= ~PWM_CTL_MODE1;
        }
    } else if (channel == PWM_CHANNEL_2) {
        if (mode == PWM_MODE_SERIALIZER) {
            ctl |= PWM_CTL_MODE2;
        } else {
            ctl &= ~PWM_CTL_MODE2;
        }
    }

    mmio_write(PWM_BASE + PWM_CTL, ctl);
}

void pwm_set_range(uint8_t channel, uint32_t range) {
    if (channel == PWM_CHANNEL_1) {
        mmio_write(PWM_BASE + PWM_RNG1, range);
    } else if (channel == PWM_CHANNEL_2) {
        mmio_write(PWM_BASE + PWM_RNG2, range);
    }
}

void pwm_set_data(uint8_t channel, uint32_t data) {
    if (channel == PWM_CHANNEL_1) {
        mmio_write(PWM_BASE + PWM_DAT1, data);
    } else if (channel == PWM_CHANNEL_2) {
        mmio_write(PWM_BASE + PWM_DAT2, data);
    }
}

void pwm_enable(uint8_t channel) {
    uint32_t ctl = mmio_read(PWM_BASE + PWM_CTL);

    if (channel == PWM_CHANNEL_1) {
        ctl |= PWM_CTL_PWEN1;
    } else if (channel == PWM_CHANNEL_2) {
        ctl |= PWM_CTL_PWEN2;
    }

    mmio_write(PWM_BASE + PWM_CTL, ctl);
}

void pwm_disable(uint8_t channel) {
    uint32_t ctl = mmio_read(PWM_BASE + PWM_CTL);

    if (channel == PWM_CHANNEL_1) {
        ctl &= ~PWM_CTL_PWEN1;
    } else if (channel == PWM_CHANNEL_2) {
        ctl &= ~PWM_CTL_PWEN2;
    }

    mmio_write(PWM_BASE + PWM_CTL, ctl);
}

void pwm_set_polarity(uint8_t channel, uint8_t polarity) {
    uint32_t ctl = mmio_read(PWM_BASE + PWM_CTL);

    if (channel == PWM_CHANNEL_1) {
        if (polarity) {
            ctl |= PWM_CTL_POLA1;
        } else {
            ctl &= ~PWM_CTL_POLA1;
        }
    } else if (channel == PWM_CHANNEL_2) {
        if (polarity) {
            ctl |= PWM_CTL_POLA2;
        } else {
            ctl &= ~PWM_CTL_POLA2;
        }
    }

    mmio_write(PWM_BASE + PWM_CTL, ctl);
}