/* Minimal Mailbox Implementation for QEMU raspi3b */

#include <stdint.h>
#include "mailbox.h"

// BCM2837 Mailbox (QEMU raspi3b)
#define MAILBOX_BASE   0x3F00B880
#define MAILBOX_READ   (MAILBOX_BASE + 0x00)
#define MAILBOX_STATUS (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE  (MAILBOX_BASE + 0x20)

#define MAILBOX_FULL   0x80000000
#define MAILBOX_EMPTY  0x40000000
#define MAILBOX_CH_PROP 8

#define PROP_RESPONSE_SUCCESS 0x80000000

static inline void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static inline uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// Property message buffer (must be 16-byte aligned)
typedef struct {
    uint32_t size;
    uint32_t code;
    uint32_t tags[32];
} __attribute__((aligned(16))) mailbox_msg_t;

static mailbox_msg_t mailbox_buffer;

void mailbox_init(void) {
    // Mailbox is always available
}

static int mailbox_call(uint32_t tag, uint32_t *response) {
    mailbox_buffer.size = sizeof(mailbox_buffer);
    mailbox_buffer.code = 0;
    mailbox_buffer.tags[0] = tag;        // Tag
    mailbox_buffer.tags[1] = 4;          // Value buffer size
    mailbox_buffer.tags[2] = 0;          // Request
    mailbox_buffer.tags[3] = 0;          // Value
    mailbox_buffer.tags[4] = 0;          // End tag

    // Wait for mailbox ready with short timeout (QEMU has limited support)
    int timeout = 1000;
    while ((mmio_read(MAILBOX_STATUS) & MAILBOX_FULL) && timeout--);
    if (timeout <= 0) return -1;

    // Send message
    uint32_t addr = (uint32_t)(uintptr_t)&mailbox_buffer;
    mmio_write(MAILBOX_WRITE, (addr & ~0xF) | MAILBOX_CH_PROP);

    // Wait for response with short timeout (QEMU may not respond)
    timeout = 1000;
    while ((mmio_read(MAILBOX_STATUS) & MAILBOX_EMPTY) && timeout--);
    if (timeout <= 0) return -1;

    // Read response
    uint32_t result = mmio_read(MAILBOX_READ);
    (void)result;  // Unused

    if (mailbox_buffer.code == PROP_RESPONSE_SUCCESS) {
        *response = mailbox_buffer.tags[3];
        return 0;
    }

    return -1;
}

uint32_t mailbox_get_firmware_revision(void) {
    uint32_t rev = 0;
    if (mailbox_call(PROP_TAG_GET_FIRMWARE_REV, &rev) == 0) {
        return rev;
    }
    return 0;
}

uint32_t mailbox_get_board_model(void) {
    uint32_t model = 0;
    if (mailbox_call(PROP_TAG_GET_BOARD_MODEL, &model) == 0) {
        return model;
    }
    return 0;
}

uint32_t mailbox_get_board_revision(void) {
    uint32_t rev = 0;
    if (mailbox_call(PROP_TAG_GET_BOARD_REV, &rev) == 0) {
        return rev;
    }
    return 0;
}
