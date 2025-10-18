/* Interactive Boot Menu Implementation */

#include "boot_menu.h"
#include "uart.h"
#include "timer.h"
#include "sd.h"
#include "network.h"
#include "usb.h"
#include "mailbox.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// ANSI escape codes
#define ANSI_CLEAR      "\x1B[2J\x1B[H"
#define ANSI_BOLD       "\x1B[1m"
#define ANSI_RESET      "\x1B[0m"
#define ANSI_REVERSE    "\x1B[7m"
#define ANSI_UP         "\x1B[A"
#define ANSI_DOWN       "\x1B[B"

// Custom string functions
static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

static void print_spaces(int count) {
    for (int i = 0; i < count; i++) uart_putc(' ');
}

// Initialize boot menu
void boot_menu_init(void) {
    // Already initialized by uart_init
}

// Clear screen
void boot_menu_clear_screen(void) {
    uart_puts(ANSI_CLEAR);
}

// Print centered text
void boot_menu_print_centered(const char *text, int y) {
    int screen_width = 80;
    int text_len = strlen(text);
    int padding = (screen_width - text_len) / 2;

    print_spaces(padding);
    uart_puts(text);
    uart_putc('\n');
}

// Draw simple box
void boot_menu_draw_box(int x, int y, int width, int height) {
    // Top border
    uart_putc('+');
    for (int i = 0; i < width - 2; i++) uart_putc('-');
    uart_puts("+\n");

    // Sides
    for (int i = 0; i < height - 2; i++) {
        uart_putc('|');
        print_spaces(width - 2);
        uart_puts("|\n");
    }

    // Bottom border
    uart_putc('+');
    for (int i = 0; i < width - 2; i++) uart_putc('-');
    uart_puts("+\n");
}

// Run interactive menu
int boot_menu_run(menu_config_t *menu) {
    if (!menu || !menu->items) return -1;

    int selected = menu->default_selection;
    if (selected >= menu->item_count) selected = 0;

    boot_menu_clear_screen();

    while (1) {
        // Display header
        uart_puts(ANSI_BOLD);
        boot_menu_print_centered(menu->title, 0);
        uart_puts(ANSI_RESET);
        uart_putc('\n');

        if (menu->header) {
            uart_puts(menu->header);
            uart_putc('\n');
        }

        uart_putc('\n');

        // Display menu items
        for (int i = 0; i < menu->item_count; i++) {
            if (!menu->items[i].enabled) continue;

            if (i == selected) {
                uart_puts(ANSI_REVERSE);
                uart_puts(" > ");
            } else {
                uart_puts("   ");
            }

            uart_puts(menu->items[i].title);

            if (i == selected) {
                uart_puts(ANSI_RESET);
            }

            uart_putc('\n');

            if (menu->items[i].description && i == selected) {
                uart_puts("     ");
                uart_puts(menu->items[i].description);
                uart_putc('\n');
            }
        }

        uart_putc('\n');
        uart_puts("Use arrow keys to navigate, Enter to select\n");

        if (menu->timeout_seconds > 0) {
            uart_puts("Auto-boot in ");
            // Print timeout (would need number formatting)
            uart_puts(" seconds...\n");
        }

        // Get user input
        char c = uart_getc();

        // Clear screen for redraw
        boot_menu_clear_screen();

        switch (c) {
            case '\r':  // Enter
            case '\n':
                if (menu->items[selected].action) {
                    int result = menu->items[selected].action(menu->items[selected].context);
                    if (result == 0) {
                        return selected; // Action successful
                    }
                    // Action failed, redisplay menu
                    uart_puts("\nAction failed. Press any key to continue...\n");
                    uart_getc();
                    boot_menu_clear_screen();
                }
                break;

            case 0x1B:  // ESC sequence
                {
                    char seq[2];
                    if (uart_getc_timeout(&seq[0], 100) == 0 &&
                        uart_getc_timeout(&seq[1], 100) == 0) {
                        if (seq[0] == '[') {
                            if (seq[1] == 'A') {  // Up arrow
                                do {
                                    selected = (selected - 1 + menu->item_count) % menu->item_count;
                                } while (!menu->items[selected].enabled);
                            } else if (seq[1] == 'B') {  // Down arrow
                                do {
                                    selected = (selected + 1) % menu->item_count;
                                } while (!menu->items[selected].enabled);
                            }
                        }
                    }
                }
                break;

            case 'k':  // Vi-style up
            case 'K':
                do {
                    selected = (selected - 1 + menu->item_count) % menu->item_count;
                } while (!menu->items[selected].enabled);
                break;

            case 'j':  // Vi-style down
            case 'J':
                do {
                    selected = (selected + 1) % menu->item_count;
                } while (!menu->items[selected].enabled);
                break;

            case '1'...'9':  // Direct number selection
                {
                    int num = c - '0';
                    if (num > 0 && num <= menu->item_count && menu->items[num - 1].enabled) {
                        selected = num - 1;
                        if (menu->items[selected].action) {
                            return menu->items[selected].action(menu->items[selected].context);
                        }
                    }
                }
                break;
        }
    }

    return -1;
}

// Built-in actions
int boot_menu_action_sd_boot(void *context) {
    uart_puts("\nBooting from SD card...\n");

    // Initialize SD
    if (sd_init() < 0) {
        uart_puts("SD card initialization failed\n");
        return -1;
    }

    // Load kernel
    extern char kernel_filename[];
    extern unsigned long kernel_addr;

    if (sd_load_file(kernel_filename, kernel_addr) < 0) {
        uart_puts("Failed to load kernel\n");
        return -1;
    }

    uart_puts("Kernel loaded successfully\n");
    return 0;
}

int boot_menu_action_network_boot(void *context) {
    uart_puts("\nBooting from network...\n");

    network_config_t config;

    // Initialize network
    if (network_init() < 0) {
        uart_puts("Network initialization failed\n");
        return -1;
    }

    // Get IP via DHCP
    uart_puts("Requesting IP address via DHCP...\n");
    if (network_dhcp_discover(&config) < 0) {
        uart_puts("DHCP failed\n");
        return -1;
    }

    uart_puts("IP address acquired: ");
    // Print IP (would need formatting)
    uart_putc('\n');

    // Download kernel via TFTP
    uart_puts("Downloading kernel via TFTP...\n");
    uint32_t server_ip = network_ip_to_int(config.boot_server);
    uint8_t *kernel_buf = (uint8_t *)0x80000;

    int size = network_tftp_download(config.boot_filename, server_ip, kernel_buf, 0x1000000);
    if (size < 0) {
        uart_puts("TFTP download failed\n");
        return -1;
    }

    uart_puts("Kernel downloaded successfully\n");
    return 0;
}

int boot_menu_action_usb_boot(void *context) {
    uart_puts("\nBooting from USB...\n");

    if (usb_boot_init() < 0) {
        uart_puts("No USB mass storage device found\n");
        return -1;
    }

    uart_puts("USB device found\n");
    return 0;
}

int boot_menu_action_show_info(void *context) {
    boot_menu_clear_screen();
    uart_puts("System Information\n");
    uart_puts("==================\n\n");

    // Get board info via mailbox
    uint32_t board_model = mailbox_get_board_model();
    uint32_t board_revision = mailbox_get_board_revision();
    uint32_t serial_high = 0, serial_low = 0;
    mailbox_get_board_serial(&serial_high, &serial_low);

    uart_puts("Board Model: 0x");
    // Print hex (would need formatting)
    uart_putc('\n');

    uart_puts("Board Revision: 0x");
    uart_putc('\n');

    uart_puts("Serial Number: ");
    uart_putc('\n');

    // Get memory info
    uint32_t arm_mem_base, arm_mem_size;
    mailbox_get_arm_memory(&arm_mem_base, &arm_mem_size);

    uart_puts("ARM Memory: ");
    // Print size
    uart_puts(" MB\n");

    uart_puts("\nPress any key to return...\n");
    uart_getc();

    return -1; // Return to menu
}

int boot_menu_action_config(void *context) {
    boot_menu_clear_screen();
    uart_puts("Configuration Menu\n");
    uart_puts("==================\n\n");

    uart_puts("1. View current configuration\n");
    uart_puts("2. Edit boot order\n");
    uart_puts("3. Network settings\n");
    uart_puts("4. Back to main menu\n\n");

    uart_puts("Select option: ");
    char choice = uart_getc();
    uart_putc('\n');

    switch (choice) {
        case '1':
            uart_puts("\nCurrent Configuration:\n");
            extern char kernel_filename[];
            uart_puts("Kernel: ");
            uart_puts(kernel_filename);
            uart_putc('\n');
            break;

        case '4':
            return -1; // Back to menu

        default:
            uart_puts("Not implemented yet\n");
            break;
    }

    uart_puts("\nPress any key to return...\n");
    uart_getc();

    return -1; // Return to menu
}

int boot_menu_action_shell(void *context) {
    boot_menu_clear_screen();
    uart_puts("Bootloader Shell\n");
    uart_puts("================\n\n");
    uart_puts("Type 'help' for commands, 'exit' to return to menu\n\n");

    char cmd_buffer[128];

    while (1) {
        uart_puts("boot> ");
        uart_gets(cmd_buffer, sizeof(cmd_buffer));

        if (cmd_buffer[0] == '\0') continue;

        // Simple command parsing
        if (strcmp(cmd_buffer, "exit") == 0 || strcmp(cmd_buffer, "quit") == 0) {
            break;
        } else if (strcmp(cmd_buffer, "help") == 0) {
            uart_puts("Available commands:\n");
            uart_puts("  help    - Show this help\n");
            uart_puts("  info    - Show system information\n");
            uart_puts("  boot    - Boot with current settings\n");
            uart_puts("  reboot  - Reboot system\n");
            uart_puts("  exit    - Return to menu\n");
        } else if (strcmp(cmd_buffer, "info") == 0) {
            boot_menu_action_show_info(NULL);
        } else if (strcmp(cmd_buffer, "boot") == 0) {
            return boot_menu_action_sd_boot(NULL);
        } else {
            uart_puts("Unknown command: ");
            uart_puts(cmd_buffer);
            uart_putc('\n');
        }
    }

    return -1; // Return to menu
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) { s1++; s2++; }
    return *s1 - *s2;
}
