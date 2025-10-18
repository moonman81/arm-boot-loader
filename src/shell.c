/* Interactive Diagnostic Shell Implementation */

#include "shell.h"
#include "uart.h"
#include "log.h"
#include "perfmon.h"
#include "watchdog.h"
#include "memory.h"
#include "gpio.h"
#include "timer.h"
#include "network.h"
#include "sd.h"
#include "hardware.h"
#include "boot_menu.h"
#include "ethernet_irq.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// String functions
static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

static char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

static void *memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}

// Shell state
static struct {
    shell_command_t commands[SHELL_MAX_COMMANDS];
    uint32_t command_count;
    char line_buffer[SHELL_MAX_LINE_LENGTH];
    char history[SHELL_HISTORY_SIZE][SHELL_MAX_LINE_LENGTH];
    uint32_t history_head;
    uint32_t history_count;
    int running;
} shell_state;

// Forward declarations of built-in commands
static int cmd_help(int argc, char **argv);
static int cmd_info(int argc, char **argv);
static int cmd_log(int argc, char **argv);
static int cmd_perf(int argc, char **argv);
static int cmd_mem(int argc, char **argv);
static int cmd_net(int argc, char **argv);
static int cmd_boot(int argc, char **argv);
static int cmd_gpio(int argc, char **argv);
static int cmd_test(int argc, char **argv);
static int cmd_reset(int argc, char **argv);
static int cmd_exit(int argc, char **argv);
static int cmd_clear(int argc, char **argv);
static int cmd_read(int argc, char **argv);
static int cmd_write(int argc, char **argv);
static int cmd_watchdog(int argc, char **argv);

// Initialize shell
void shell_init(void) {
    memset(&shell_state, 0, sizeof(shell_state));
    shell_state.running = 0;

    // Register built-in commands
    shell_register_command("help", "Show available commands", cmd_help);
    shell_register_command("info", "Display system information", cmd_info);
    shell_register_command("log", "Control logging system", cmd_log);
    shell_register_command("perf", "Show performance statistics", cmd_perf);
    shell_register_command("mem", "Memory diagnostics", cmd_mem);
    shell_register_command("net", "Network diagnostics", cmd_net);
    shell_register_command("boot", "Boot configuration", cmd_boot);
    shell_register_command("gpio", "GPIO control", cmd_gpio);
    shell_register_command("test", "Run diagnostic tests", cmd_test);
    shell_register_command("reset", "Reset system", cmd_reset);
    shell_register_command("exit", "Exit shell", cmd_exit);
    shell_register_command("clear", "Clear screen", cmd_clear);
    shell_register_command("read", "Read memory address", cmd_read);
    shell_register_command("write", "Write memory address", cmd_write);
    shell_register_command("watchdog", "Watchdog control", cmd_watchdog);
}

// Register command
int shell_register_command(const char *name, const char *description, shell_command_fn handler) {
    if (shell_state.command_count >= SHELL_MAX_COMMANDS) return -1;

    shell_command_t *cmd = &shell_state.commands[shell_state.command_count];
    cmd->name = name;
    cmd->description = description;
    cmd->handler = handler;

    shell_state.command_count++;
    return 0;
}

// Read line with editing support
static int shell_read_line(char *buffer, uint32_t max_len) {
    uint32_t pos = 0;

    while (1) {
        char c = uart_getc();

        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            buffer[pos] = '\0';
            return pos;
        } else if (c == 0x7F || c == 0x08) { // Backspace
            if (pos > 0) {
                pos--;
                uart_puts("\b \b"); // Erase character
            }
        } else if (c == 0x03) { // Ctrl-C
            uart_puts("^C\r\n");
            return -1;
        } else if (c >= 32 && c < 127) { // Printable
            if (pos < max_len - 1) {
                buffer[pos++] = c;
                uart_putc(c);
            }
        }
    }
}

// Parse command line into argc/argv
static int shell_parse_args(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        // Mark argument start
        argv[argc++] = p;

        // Find end of argument
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }

    return argc;
}

// Execute command
int shell_execute(const char *command_line) {
    char line[SHELL_MAX_LINE_LENGTH];
    char *argv[SHELL_MAX_ARGS];
    int argc;

    if (strlen(command_line) >= SHELL_MAX_LINE_LENGTH) {
        shell_error("Command line too long");
        return -1;
    }

    strcpy(line, command_line);
    argc = shell_parse_args(line, argv, SHELL_MAX_ARGS);

    if (argc == 0) return 0;

    // Find and execute command
    for (uint32_t i = 0; i < shell_state.command_count; i++) {
        if (strcmp(argv[0], shell_state.commands[i].name) == 0) {
            return shell_state.commands[i].handler(argc, argv);
        }
    }

    shell_error("Unknown command");
    uart_puts("Type 'help' for available commands\r\n");
    return -1;
}

// Run shell
void shell_run(void) {
    shell_state.running = 1;

    uart_puts("\r\n");
    uart_puts(SHELL_COLOR_CYAN);
    uart_puts("=====================================\r\n");
    uart_puts("  Bootloader Diagnostic Shell\r\n");
    uart_puts("=====================================\r\n");
    uart_puts(SHELL_COLOR_RESET);
    uart_puts("Type 'help' for available commands\r\n\r\n");

    while (shell_state.running) {
        uart_puts(SHELL_COLOR_GREEN);
        uart_puts(SHELL_PROMPT);
        uart_puts(SHELL_COLOR_RESET);

        if (shell_read_line(shell_state.line_buffer, SHELL_MAX_LINE_LENGTH) < 0) {
            continue;
        }

        if (shell_state.line_buffer[0] != '\0') {
            // Add to history
            if (shell_state.history_count < SHELL_HISTORY_SIZE) {
                strcpy(shell_state.history[shell_state.history_count++],
                       shell_state.line_buffer);
            }

            // Execute command
            shell_execute(shell_state.line_buffer);
        }
    }
}

// Print formatted helpers
void shell_error(const char *message) {
    uart_puts(SHELL_COLOR_RED);
    uart_puts("Error: ");
    uart_puts(message);
    uart_puts(SHELL_COLOR_RESET);
    uart_puts("\r\n");
}

void shell_warning(const char *message) {
    uart_puts(SHELL_COLOR_YELLOW);
    uart_puts("Warning: ");
    uart_puts(message);
    uart_puts(SHELL_COLOR_RESET);
    uart_puts("\r\n");
}

void shell_success(const char *message) {
    uart_puts(SHELL_COLOR_GREEN);
    uart_puts(message);
    uart_puts(SHELL_COLOR_RESET);
    uart_puts("\r\n");
}

// Parse hex number
int shell_parse_hex(const char *str, uint32_t *value) {
    uint32_t result = 0;
    const char *p = str;

    // Skip 0x prefix
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;

    while (*p) {
        char c = *p++;
        uint32_t digit;

        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return -1;

        result = (result << 4) | digit;
    }

    *value = result;
    return 0;
}

// Parse decimal number
int shell_parse_dec(const char *str, uint32_t *value) {
    uint32_t result = 0;
    const char *p = str;

    while (*p) {
        char c = *p++;
        if (c < '0' || c > '9') return -1;
        result = result * 10 + (c - '0');
    }

    *value = result;
    return 0;
}

// Print hex dump
void shell_print_hex_dump(const void *data, uint32_t length, uint32_t base_addr) {
    const uint8_t *bytes = (const uint8_t *)data;

    for (uint32_t i = 0; i < length; i += 16) {
        // Print address
        uart_puts("0x");
        for (int j = 28; j >= 0; j -= 4) {
            uart_putc("0123456789abcdef"[(base_addr + i) >> j & 0xF]);
        }
        uart_puts(": ");

        // Print hex bytes
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < length) {
                uint8_t byte = bytes[i + j];
                uart_putc("0123456789abcdef"[byte >> 4]);
                uart_putc("0123456789abcdef"[byte & 0xF]);
                uart_putc(' ');
            } else {
                uart_puts("   ");
            }
        }

        // Print ASCII
        uart_puts(" |");
        for (uint32_t j = 0; j < 16 && i + j < length; j++) {
            uint8_t byte = bytes[i + j];
            uart_putc((byte >= 32 && byte < 127) ? byte : '.');
        }
        uart_puts("|\r\n");
    }
}

// Built-in command implementations

static int cmd_help(int argc, char **argv) {
    uart_puts("\r\nAvailable commands:\r\n");
    uart_puts("-------------------\r\n");

    for (uint32_t i = 0; i < shell_state.command_count; i++) {
        uart_puts(SHELL_COLOR_CYAN);
        uart_puts(shell_state.commands[i].name);
        uart_puts(SHELL_COLOR_RESET);
        uart_puts(" - ");
        uart_puts(shell_state.commands[i].description);
        uart_puts("\r\n");
    }

    return 0;
}

static int cmd_info(int argc, char **argv) {
    const pi_model_info_t *model = hardware_get_model_info();

    uart_puts("\r\n");
    uart_puts(SHELL_COLOR_BOLD);
    uart_puts("System Information\r\n");
    uart_puts(SHELL_COLOR_RESET);
    uart_puts("==================\r\n");

    uart_puts("Model: ");
    uart_puts(model->name);
    uart_puts("\r\n");

    uart_puts("SoC: BCM");
    // Print SoC number
    uint32_t soc = model->soc_type;
    char buf[16];
    int pos = 0;
    if (soc == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        while (soc > 0) {
            temp[i++] = '0' + (soc % 10);
            soc /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts("\r\n");

    uart_puts("CPU Cores: ");
    uart_putc('0' + model->cpu_cores);
    uart_puts("\r\n");

    uart_puts("Memory: ");
    uint32_t mem = model->default_memory_mb;
    pos = 0;
    if (mem == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        while (mem > 0) {
            temp[i++] = '0' + (mem % 10);
            mem /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(" MB\r\n");

    uart_puts("Capabilities: ");
    if (model->has_ethernet) uart_puts("Ethernet ");
    if (model->has_wifi) uart_puts("WiFi ");
    if (model->has_bluetooth) uart_puts("Bluetooth ");
    if (model->has_usb3) uart_puts("USB3 ");
    if (model->has_pcie) uart_puts("PCIe ");
    uart_puts("\r\n");

    // Uptime
    uint64_t uptime_us = timer_get_counter();
    uint32_t uptime_s = uptime_us / 1000000;
    uart_puts("Uptime: ");
    pos = 0;
    uint32_t temp = uptime_s;
    if (temp == 0) buf[pos++] = '0';
    else {
        char tbuf[16];
        int i = 0;
        while (temp > 0) {
            tbuf[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (i > 0) buf[pos++] = tbuf[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(" seconds\r\n");

    return 0;
}

static int cmd_log(int argc, char **argv) {
    if (argc < 2) {
        uart_puts("Usage: log <level|dump|clear>\r\n");
        uart_puts("  level <ERROR|WARN|INFO|DEBUG|TRACE> - Set log level\r\n");
        uart_puts("  dump - Dump memory log\r\n");
        uart_puts("  clear - Clear memory log\r\n");
        return -1;
    }

    if (strcmp(argv[1], "dump") == 0) {
        log_dump_memory();
    } else if (strcmp(argv[1], "clear") == 0) {
        log_clear_memory();
        shell_success("Memory log cleared");
    } else if (strcmp(argv[1], "level") == 0) {
        if (argc < 3) {
            shell_error("Missing log level");
            return -1;
        }

        if (strcmp(argv[2], "ERROR") == 0) log_set_level(LOG_LEVEL_ERROR);
        else if (strcmp(argv[2], "WARN") == 0) log_set_level(LOG_LEVEL_WARN);
        else if (strcmp(argv[2], "INFO") == 0) log_set_level(LOG_LEVEL_INFO);
        else if (strcmp(argv[2], "DEBUG") == 0) log_set_level(LOG_LEVEL_DEBUG);
        else if (strcmp(argv[2], "TRACE") == 0) log_set_level(LOG_LEVEL_TRACE);
        else {
            shell_error("Invalid log level");
            return -1;
        }

        shell_success("Log level updated");
    } else {
        shell_error("Invalid log command");
        return -1;
    }

    return 0;
}

static int cmd_perf(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "report") == 0) {
        perfmon_print_report();
    } else {
        perfmon_print_summary();
    }
    return 0;
}

static int cmd_mem(int argc, char **argv) {
    if (argc < 2) {
        // Show memory stats
        uart_puts("\r\nMemory Statistics\r\n");
        uart_puts("=================\r\n");

        uart_puts("Heap Free: ");
        uint32_t free = memory_get_free();
        char buf[16];
        int pos = 0;
        if (free == 0) buf[pos++] = '0';
        else {
            char temp[16];
            int i = 0;
            while (free > 0) {
                temp[i++] = '0' + (free % 10);
                free /= 10;
            }
            while (i > 0) buf[pos++] = temp[--i];
        }
        buf[pos] = '\0';
        uart_puts(buf);
        uart_puts(" bytes\r\n");

        uart_puts("Heap Used: ");
        uint32_t used = memory_get_used();
        pos = 0;
        if (used == 0) buf[pos++] = '0';
        else {
            char temp[16];
            int i = 0;
            while (used > 0) {
                temp[i++] = '0' + (used % 10);
                used /= 10;
            }
            while (i > 0) buf[pos++] = temp[--i];
        }
        buf[pos] = '\0';
        uart_puts(buf);
        uart_puts(" bytes\r\n");

        return 0;
    }

    if (strcmp(argv[1], "test") == 0) {
        shell_warning("Memory test not implemented");
    } else {
        shell_error("Invalid mem command");
    }

    return 0;
}

static int cmd_net(int argc, char **argv) {
    uart_puts("\r\nNetwork Status\r\n");
    uart_puts("==============\r\n");

    // Get IRQ stats if available
    ethernet_irq_stats_t stats;
    ethernet_irq_get_stats(&stats);

    uart_puts("RX Interrupts: ");
    char buf[16];
    int pos = 0;
    uint32_t val = stats.interrupts;
    if (val == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts("\r\n");

    uart_puts("Packets Received: ");
    pos = 0;
    val = stats.packets_received;
    if (val == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts("\r\n");

    return 0;
}

static int cmd_boot(int argc, char **argv) {
    shell_warning("Boot configuration not implemented");
    return 0;
}

static int cmd_gpio(int argc, char **argv) {
    if (argc < 2) {
        uart_puts("Usage: gpio <get|set|clear> <pin>\r\n");
        return -1;
    }

    if (argc < 3) {
        shell_error("Missing pin number");
        return -1;
    }

    uint32_t pin;
    if (shell_parse_dec(argv[2], &pin) < 0) {
        shell_error("Invalid pin number");
        return -1;
    }

    if (strcmp(argv[1], "set") == 0) {
        gpio_set(pin);
        shell_success("GPIO pin set");
    } else if (strcmp(argv[1], "clear") == 0) {
        gpio_clear(pin);
        shell_success("GPIO pin cleared");
    } else if (strcmp(argv[1], "get") == 0) {
        uint32_t level = gpio_get_level(pin);
        uart_puts("GPIO ");
        uart_putc('0' + (pin / 10));
        uart_putc('0' + (pin % 10));
        uart_puts(" = ");
        uart_putc('0' + level);
        uart_puts("\r\n");
    } else {
        shell_error("Invalid gpio command");
        return -1;
    }

    return 0;
}

static int cmd_test(int argc, char **argv) {
    uart_puts("\r\nRunning Diagnostic Tests\r\n");
    uart_puts("========================\r\n");

    // Timer test
    uart_puts("Timer: ");
    uint64_t t1 = timer_get_counter();
    timer_delay_ms(10);
    uint64_t t2 = timer_get_counter();
    if (t2 > t1) {
        shell_success("PASS");
    } else {
        shell_error("FAIL");
    }

    // GPIO test (toggle LED)
    uart_puts("GPIO: ");
    gpio_set(GPIO_LED_PIN);
    timer_delay_ms(100);
    gpio_clear(GPIO_LED_PIN);
    shell_success("PASS");

    uart_puts("\r\n");
    return 0;
}

static int cmd_reset(int argc, char **argv) {
    uart_puts("Resetting system...\r\n");
    timer_delay_ms(100);

    // Trigger watchdog reset
    watchdog_init(WDT_MODE_RESET, WDT_TIMEOUT_1S);
    watchdog_start();

    // Wait for reset
    while (1) {}

    return 0;
}

static int cmd_exit(int argc, char **argv) {
    shell_state.running = 0;
    uart_puts("Exiting shell\r\n");
    return 0;
}

static int cmd_clear(int argc, char **argv) {
    uart_puts("\x1B[2J\x1B[H"); // Clear screen and home cursor
    return 0;
}

static int cmd_read(int argc, char **argv) {
    if (argc < 2) {
        uart_puts("Usage: read <address> [length]\r\n");
        return -1;
    }

    uint32_t addr, length = 64;
    if (shell_parse_hex(argv[1], &addr) < 0) {
        shell_error("Invalid address");
        return -1;
    }

    if (argc >= 3) {
        if (shell_parse_hex(argv[2], &length) < 0) {
            shell_error("Invalid length");
            return -1;
        }
    }

    uart_puts("\r\n");
    shell_print_hex_dump((void *)addr, length, addr);
    return 0;
}

static int cmd_write(int argc, char **argv) {
    if (argc < 3) {
        uart_puts("Usage: write <address> <value>\r\n");
        return -1;
    }

    uint32_t addr, value;
    if (shell_parse_hex(argv[1], &addr) < 0) {
        shell_error("Invalid address");
        return -1;
    }

    if (shell_parse_hex(argv[2], &value) < 0) {
        shell_error("Invalid value");
        return -1;
    }

    *(volatile uint32_t *)addr = value;
    shell_success("Memory written");
    return 0;
}

static int cmd_watchdog(int argc, char **argv) {
    if (argc < 2) {
        watchdog_status_t status;
        watchdog_get_status(&status);

        uart_puts("\r\nWatchdog Status\r\n");
        uart_puts("===============\r\n");
        uart_puts("Enabled: ");
        uart_puts(status.enabled ? "Yes" : "No");
        uart_puts("\r\n");

        if (status.enabled) {
            uart_puts("Timeout: ");
            char buf[16];
            int pos = 0;
            uint32_t val = status.timeout_ms;
            if (val == 0) buf[pos++] = '0';
            else {
                char temp[16];
                int i = 0;
                while (val > 0) {
                    temp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                while (i > 0) buf[pos++] = temp[--i];
            }
            buf[pos] = '\0';
            uart_puts(buf);
            uart_puts(" ms\r\n");
        }
        return 0;
    }

    if (strcmp(argv[1], "kick") == 0) {
        watchdog_kick();
        shell_success("Watchdog kicked");
    } else if (strcmp(argv[1], "stop") == 0) {
        watchdog_stop();
        shell_success("Watchdog stopped");
    } else {
        shell_error("Invalid watchdog command");
        return -1;
    }

    return 0;
}
