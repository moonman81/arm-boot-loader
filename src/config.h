/* Config Header */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

void config_parse(void);
void config_apply_model_settings(void);
const char *config_get(const char *key);

// Configurable values
extern char kernel_filename[256];
extern char initrd_filename[256];
extern unsigned long kernel_addr;
extern unsigned long initrd_addr;
extern int enable_usb;
extern int enable_nvme;

#endif