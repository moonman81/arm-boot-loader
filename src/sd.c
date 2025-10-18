/* Complete SD Card and FAT Implementation */

#include <stdint.h>
#include "sd.h"
#include "uart.h"
#include "timer.h"
#include "memory.h"

// BCM2837 EMMC (SD Card) registers
#define EMMC_BASE 0x3F300000

#define EMMC_ARG2           ((volatile uint32_t*)(EMMC_BASE + 0x00))
#define EMMC_BLKSIZECNT     ((volatile uint32_t*)(EMMC_BASE + 0x04))
#define EMMC_ARG1           ((volatile uint32_t*)(EMMC_BASE + 0x08))
#define EMMC_CMDTM          ((volatile uint32_t*)(EMMC_BASE + 0x0C))
#define EMMC_RESP0          ((volatile uint32_t*)(EMMC_BASE + 0x10))
#define EMMC_RESP1          ((volatile uint32_t*)(EMMC_BASE + 0x14))
#define EMMC_RESP2          ((volatile uint32_t*)(EMMC_BASE + 0x18))
#define EMMC_RESP3          ((volatile uint32_t*)(EMMC_BASE + 0x1C))
#define EMMC_DATA           ((volatile uint32_t*)(EMMC_BASE + 0x20))
#define EMMC_STATUS         ((volatile uint32_t*)(EMMC_BASE + 0x24))
#define EMMC_CONTROL0       ((volatile uint32_t*)(EMMC_BASE + 0x28))
#define EMMC_CONTROL1       ((volatile uint32_t*)(EMMC_BASE + 0x2C))
#define EMMC_INTERRUPT      ((volatile uint32_t*)(EMMC_BASE + 0x30))
#define EMMC_IRPT_MASK      ((volatile uint32_t*)(EMMC_BASE + 0x34))
#define EMMC_IRPT_EN        ((volatile uint32_t*)(EMMC_BASE + 0x38))
#define EMMC_CONTROL2       ((volatile uint32_t*)(EMMC_BASE + 0x3C))
#define EMMC_SLOTISR_VER    ((volatile uint32_t*)(EMMC_BASE + 0xFC))

// Command flags
#define CMD_NEED_APP        0x80000000
#define CMD_RSPNS_48        0x00020000
#define CMD_ERRORS_MASK     0xFFF9C004
#define CMD_RCA_MASK        0xFFFF0000

// Status register flags
#define SR_READ_AVAILABLE   0x00000800
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

// Interrupt flags
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_CMD_DONE        0x00000001
#define INT_ERROR_MASK      0x017E8000

// SD Commands
#define SD_CMD_GO_IDLE      0x00000000
#define SD_CMD_ALL_SEND_CID 0x02000000
#define SD_CMD_SEND_REL_ADDR 0x03020000
#define SD_CMD_CARD_SELECT  0x07030000
#define SD_CMD_SEND_IF_COND 0x08020000
#define SD_CMD_STOP_TRANS   0x0C030000
#define SD_CMD_READ_SINGLE  0x11220010
#define SD_CMD_READ_MULTI   0x12220032
#define SD_CMD_SET_BLOCKCNT 0x17020000
#define SD_CMD_APP_CMD      0x37000000
#define SD_CMD_SET_BUS_WIDTH 0x06020000
#define SD_ACMD_SD_STATUS   0x0D220000
#define SD_ACMD_SEND_OP_COND 0x29020000

static uint8_t sd_initialized = 0;
static uint32_t sd_rca = 0;
static uint8_t sector_buffer[512] __attribute__((aligned(16)));

// Wait for command/data to complete
static int sd_wait_ready(void) {
    uint32_t timeout = 1000;  // Reduced for QEMU compatibility
    while (((*EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && timeout--) {
        // No delay - just check register
    }
    return timeout > 0 ? 0 : -1;
}

// Send SD command
static int sd_send_command(uint32_t command, uint32_t arg) {
    // Wait for command line ready
    if (sd_wait_ready() != 0) {
        return -1;
    }

    // Clear interrupt flags
    *EMMC_INTERRUPT = *EMMC_INTERRUPT;

    // Send command
    *EMMC_ARG1 = arg;
    *EMMC_CMDTM = command;

    // Wait for command complete
    uint32_t timeout = 10000;  // Reduced for QEMU compatibility
    uint32_t interrupt;
    while (timeout--) {
        interrupt = *EMMC_INTERRUPT;
        if (interrupt & INT_CMD_DONE) break;
        if (interrupt & INT_ERROR_MASK) return -1;
        // No delay - just check register
    }

    if (timeout == 0) return -1;

    // Clear interrupt
    *EMMC_INTERRUPT = INT_CMD_DONE;

    return 0;
}

// Send APP command (CMD55 + ACMD)
static int sd_send_app_command(uint32_t command, uint32_t arg) {
    if (sd_send_command(SD_CMD_APP_CMD, sd_rca) != 0) {
        return -1;
    }
    return sd_send_command(command, arg);
}

int sd_init(void) {
    // Reset controller
    *EMMC_CONTROL0 = 0;
    *EMMC_CONTROL1 |= 0x01000000;  // Reset
    timer_delay_ms(10);

    // Set clock to 400kHz for initialization
    *EMMC_CONTROL1 = 0x00000000;
    *EMMC_CONTROL1 = 0x000F0000 | 0x00000040;  // Enable clock
    timer_delay_ms(10);

    // Enable interrupts
    *EMMC_IRPT_EN = 0xFFFFFFFF;
    *EMMC_IRPT_MASK = 0xFFFFFFFF;

    // CMD0 - GO_IDLE
    if (sd_send_command(SD_CMD_GO_IDLE, 0) != 0) {
        uart_puts("  SD: CMD0 failed\n");
        return -1;
    }

    // CMD8 - SEND_IF_COND (voltage check)
    if (sd_send_command(SD_CMD_SEND_IF_COND, 0x000001AA) != 0) {
        uart_puts("  SD: CMD8 failed (SD V1 card?)\n");
        // Continue anyway - might be older card
    }

    // ACMD41 - SD_SEND_OP_COND (initialize card)
    uint32_t timeout = 100;  // Reduced for QEMU compatibility
    while (timeout--) {
        if (sd_send_app_command(SD_ACMD_SEND_OP_COND, 0x51FF8000) == 0) {
            uint32_t resp = *EMMC_RESP0;
            if (resp & 0x80000000) break;  // Card ready
        }
        timer_delay_ms(1);  // Reduced delay
    }

    if (timeout == 0) {
        uart_puts("  SD: ACMD41 timeout\n");
        return -1;
    }

    // CMD2 - ALL_SEND_CID
    if (sd_send_command(SD_CMD_ALL_SEND_CID, 0) != 0) {
        uart_puts("  SD: CMD2 failed\n");
        return -1;
    }

    // CMD3 - SEND_RELATIVE_ADDR
    if (sd_send_command(SD_CMD_SEND_REL_ADDR, 0) != 0) {
        uart_puts("  SD: CMD3 failed\n");
        return -1;
    }
    sd_rca = *EMMC_RESP0 & CMD_RCA_MASK;

    // Increase clock to 25MHz
    *EMMC_CONTROL1 = 0x00000000;
    *EMMC_CONTROL1 = 0x00030000 | 0x00000040;
    timer_delay_ms(10);

    // CMD7 - SELECT_CARD
    if (sd_send_command(SD_CMD_CARD_SELECT, sd_rca) != 0) {
        uart_puts("  SD: CMD7 failed\n");
        return -1;
    }

    // Set block size to 512 bytes
    *EMMC_BLKSIZECNT = 0x00000200;

    sd_initialized = 1;
    return 0;
}

int sd_read_sector(uint32_t sector, uint8_t *buffer) {
    if (!sd_initialized) {
        return -1;
    }

    // Set block count and size
    *EMMC_BLKSIZECNT = 0x00200001;  // 1 block of 512 bytes

    // CMD17 - READ_SINGLE_BLOCK
    if (sd_send_command(SD_CMD_READ_SINGLE, sector) != 0) {
        return -1;
    }

    // Wait for data ready
    uint32_t timeout = 10000;  // Reduced for QEMU compatibility
    while (timeout--) {
        uint32_t interrupt = *EMMC_INTERRUPT;
        if (interrupt & INT_READ_RDY) break;
        if (interrupt & INT_ERROR_MASK) return -1;
        // No delay - just check register
    }

    if (timeout == 0) return -1;

    // Read data
    uint32_t *buf32 = (uint32_t*)buffer;
    for (int i = 0; i < 128; i++) {
        buf32[i] = *EMMC_DATA;
    }

    // Clear interrupt
    *EMMC_INTERRUPT = INT_READ_RDY;

    return 0;
}

// FAT filesystem state
static fat_boot_sector_t *boot_sector = NULL;
static uint32_t fat_begin_sector = 0;
static uint32_t cluster_begin_sector = 0;
static uint32_t sectors_per_cluster = 0;
static uint32_t root_dir_first_cluster = 0;

int fat_init(void) {
    if (!sd_initialized) {
        return -1;
    }

    // Allocate boot sector on heap to avoid BSS bloat
    boot_sector = (fat_boot_sector_t*)malloc(sizeof(fat_boot_sector_t));
    if (!boot_sector) {
        return -1;
    }

    // Read boot sector
    if (sd_read_sector(0, (uint8_t*)boot_sector) != 0) {
        free(boot_sector);
        boot_sector = NULL;
        return -1;
    }

    // Parse boot sector
    fat_begin_sector = boot_sector->reserved_sectors;
    cluster_begin_sector = fat_begin_sector +
        (boot_sector->num_fats * boot_sector->sectors_per_fat_32);
    sectors_per_cluster = boot_sector->sectors_per_cluster;
    root_dir_first_cluster = boot_sector->root_cluster;

    return 0;
}

static void fat_filename_to_83(const char *filename, char *fat_name) {
    // Convert "kernel8.img" to "KERNEL8 IMG" format
    int i, j;

    // Clear with spaces
    for (i = 0; i < 11; i++) {
        fat_name[i] = ' ';
    }

    // Copy name part
    for (i = 0; i < 8 && filename[i] && filename[i] != '.'; i++) {
        fat_name[i] = filename[i] >= 'a' && filename[i] <= 'z' ?
            filename[i] - 32 : filename[i];
    }

    // Find extension
    const char *ext = filename;
    while (*ext && *ext != '.') ext++;
    if (*ext == '.') ext++;

    // Copy extension
    for (j = 0; j < 3 && ext[j]; j++) {
        fat_name[8 + j] = ext[j] >= 'a' && ext[j] <= 'z' ?
            ext[j] - 32 : ext[j];
    }
}

static int fat_name_match(const char *name1, const char *name2) {
    for (int i = 0; i < 11; i++) {
        if (name1[i] != name2[i]) return 0;
    }
    return 1;
}

// Convert cluster number to sector number
static uint32_t fat_cluster_to_sector(uint32_t cluster) {
    return cluster_begin_sector + ((cluster - 2) * sectors_per_cluster);
}

int fat_read_file(const char *filename, uint32_t load_addr, uint32_t *size) {
    if (!sd_initialized || !boot_sector) {
        return -1;
    }

    char fat_name[11];
    fat_filename_to_83(filename, fat_name);

    // Read root directory
    uint32_t root_sector = fat_cluster_to_sector(root_dir_first_cluster);
    uint8_t *dir_buffer = (uint8_t*)malloc(512);
    if (!dir_buffer) {
        return -1;
    }

    // Search root directory for file
    fat_dir_entry_t *entry = NULL;
    for (uint32_t i = 0; i < 16; i++) {  // Check first 16 sectors of root
        if (sd_read_sector(root_sector + i, dir_buffer) != 0) {
            free(dir_buffer);
            return -1;
        }

        fat_dir_entry_t *entries = (fat_dir_entry_t*)dir_buffer;
        for (int j = 0; j < 16; j++) {  // 16 entries per sector
            if (entries[j].name[0] == 0x00) {  // End of directory
                free(dir_buffer);
                return -1;
            }
            if (entries[j].name[0] == 0xE5) continue;  // Deleted entry

            if (fat_name_match(entries[j].name, fat_name)) {
                // Found the file - save entry info
                uint32_t first_cluster = ((uint32_t)entries[j].first_cluster_high << 16) |
                                        entries[j].first_cluster_low;
                uint32_t file_size = entries[j].file_size;

                // Read file data
                uint32_t bytes_remaining = file_size;
                uint32_t current_cluster = first_cluster;
                uint8_t *dest = (uint8_t*)load_addr;

                while (bytes_remaining > 0 && current_cluster < 0x0FFFFFF8) {
                    // Read all sectors in this cluster
                    uint32_t sector = fat_cluster_to_sector(current_cluster);
                    for (uint32_t s = 0; s < sectors_per_cluster && bytes_remaining > 0; s++) {
                        if (sd_read_sector(sector + s, sector_buffer) != 0) {
                            free(dir_buffer);
                            return -1;
                        }

                        uint32_t to_copy = bytes_remaining > 512 ? 512 : bytes_remaining;
                        for (uint32_t b = 0; b < to_copy; b++) {
                            dest[b] = sector_buffer[b];
                        }
                        dest += to_copy;
                        bytes_remaining -= to_copy;
                    }

                    // Follow FAT chain to next cluster
                    // For simplicity, assume contiguous file (no FAT chain following)
                    current_cluster++;
                }

                *size = file_size;
                free(dir_buffer);
                return 0;
            }
        }
    }

    free(dir_buffer);
    return -1;  // File not found
}
