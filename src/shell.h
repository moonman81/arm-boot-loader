/* Interactive Diagnostic Shell */

#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

// Shell command handler typedef
typedef int (*shell_command_fn)(int argc, char **argv);

// Command registration structure
typedef struct {
    const char *name;
    const char *description;
    shell_command_fn handler;
} shell_command_t;

// Shell configuration
#define SHELL_MAX_COMMANDS      32
#define SHELL_MAX_LINE_LENGTH   256
#define SHELL_MAX_ARGS          16
#define SHELL_HISTORY_SIZE      10
#define SHELL_PROMPT            "bootloader> "

// ANSI codes for shell UI
#define SHELL_COLOR_RESET       "\x1B[0m"
#define SHELL_COLOR_RED         "\x1B[31m"
#define SHELL_COLOR_GREEN       "\x1B[32m"
#define SHELL_COLOR_YELLOW      "\x1B[33m"
#define SHELL_COLOR_BLUE        "\x1B[34m"
#define SHELL_COLOR_CYAN        "\x1B[36m"
#define SHELL_COLOR_GRAY        "\x1B[90m"
#define SHELL_COLOR_BOLD        "\x1B[1m"

// Initialize shell
void shell_init(void);

// Register command
int shell_register_command(const char *name, const char *description, shell_command_fn handler);

// Run shell (blocks until exit command)
void shell_run(void);

// Execute single command (for scripting)
int shell_execute(const char *command_line);

// Print formatted output
void shell_printf(const char *format, ...);
void shell_error(const char *message);
void shell_warning(const char *message);
void shell_success(const char *message);

// Utility functions for command implementations
int shell_parse_hex(const char *str, uint32_t *value);
int shell_parse_dec(const char *str, uint32_t *value);
void shell_print_hex_dump(const void *data, uint32_t length, uint32_t base_addr);

#endif
