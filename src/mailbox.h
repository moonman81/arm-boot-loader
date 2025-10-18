/* Minimal Mailbox Header */

#ifndef MAILBOX_H
#define MAILBOX_H

#include <stdint.h>

// Property tags
#define PROP_TAG_GET_FIRMWARE_REV  0x00000001
#define PROP_TAG_GET_BOARD_MODEL   0x00010001
#define PROP_TAG_GET_BOARD_REV     0x00010002
#define PROP_TAG_GET_ARM_MEMORY    0x00010005

void mailbox_init(void);
uint32_t mailbox_get_firmware_revision(void);
uint32_t mailbox_get_board_model(void);
uint32_t mailbox_get_board_revision(void);

#endif
