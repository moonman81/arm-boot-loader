/* Clock Manager Implementation for Raspberry Pi */

#include "clock.h"
#include "mailbox.h"

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

void clock_init(void) {
    // Clock manager is always available, no initialization needed
    // Individual clocks are configured as needed
}

int clock_configure_gp(uint8_t gp_clock, uint8_t source, uint32_t divisor) {
    if (gp_clock > 2) return -1;

    uint32_t ctl_reg, div_reg;

    switch (gp_clock) {
        case CLK_GP0:
            ctl_reg = CM_GP0CTL;
            div_reg = CM_GP0DIV;
            break;
        case CLK_GP1:
            ctl_reg = CM_GP1CTL;
            div_reg = CM_GP1DIV;
            break;
        case CLK_GP2:
            ctl_reg = CM_GP2CTL;
            div_reg = CM_GP2DIV;
            break;
        default:
            return -1;
    }

    // Disable clock
    mmio_write(ctl_reg, CM_CTL_KILL);
    while (mmio_read(ctl_reg) & CM_CTL_BUSY) {
        // Wait for clock to stop
    }

    // Set divisor (integer part in bits 23:12, fractional in bits 11:0)
    uint32_t div_value = (divisor << 12);  // Integer divisor
    mmio_write(div_reg, div_value);

    // Enable clock with new source
    uint32_t ctl_value = CM_CTL_ENAB | (source << 0) | CM_CTL_MASH_1;
    mmio_write(ctl_reg, ctl_value);

    return 0;
}

int clock_enable_gp(uint8_t gp_clock) {
    if (gp_clock > 2) return -1;

    uint32_t ctl_reg;
    switch (gp_clock) {
        case CLK_GP0: ctl_reg = CM_GP0CTL; break;
        case CLK_GP1: ctl_reg = CM_GP1CTL; break;
        case CLK_GP2: ctl_reg = CM_GP2CTL; break;
        default: return -1;
    }

    uint32_t ctl_value = mmio_read(ctl_reg);
    ctl_value |= CM_CTL_ENAB;
    mmio_write(ctl_reg, ctl_value);

    return 0;
}

int clock_disable_gp(uint8_t gp_clock) {
    if (gp_clock > 2) return -1;

    uint32_t ctl_reg;
    switch (gp_clock) {
        case CLK_GP0: ctl_reg = CM_GP0CTL; break;
        case CLK_GP1: ctl_reg = CM_GP1CTL; break;
        case CLK_GP2: ctl_reg = CM_GP2CTL; break;
        default: return -1;
    }

    uint32_t ctl_value = mmio_read(ctl_reg);
    ctl_value &= ~CM_CTL_ENAB;
    mmio_write(ctl_reg, ctl_value);

    return 0;
}

uint32_t clock_get_rate(uint32_t clock_id) {
    return mailbox_get_clock_rate(clock_id);
}

uint32_t clock_get_max_rate(uint32_t clock_id) {
    return mailbox_get_max_clock_rate(clock_id);
}

int clock_set_rate(uint32_t clock_id, uint32_t rate) {
    return mailbox_set_clock_rate(clock_id, rate);
}