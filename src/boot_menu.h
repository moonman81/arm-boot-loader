/* Interactive Boot Menu System */

#ifndef BOOT_MENU_H
#define BOOT_MENU_H

#include <stdint.h>

// Menu item structure
typedef struct {
    const char *title;
    const char *description;
    int (*action)(void *context);
    void *context;
    int enabled;
} menu_item_t;

// Menu configuration
typedef struct {
    const char *title;
    const char *header;
    menu_item_t *items;
    int item_count;
    int default_selection;
    int timeout_seconds;  // 0 = no timeout
} menu_config_t;

// Initialize boot menu system
void boot_menu_init(void);

// Display and run menu
int boot_menu_run(menu_config_t *menu);

// Built-in menu actions
int boot_menu_action_sd_boot(void *context);
int boot_menu_action_network_boot(void *context);
int boot_menu_action_usb_boot(void *context);
int boot_menu_action_show_info(void *context);
int boot_menu_action_config(void *context);
int boot_menu_action_shell(void *context);

// Helper functions
void boot_menu_clear_screen(void);
void boot_menu_draw_box(int x, int y, int width, int height);
void boot_menu_print_centered(const char *text, int y);

#endif
