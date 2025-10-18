/* Centralized Logging System Implementation */

#include "log.h"
#include "uart.h"
#include "timer.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Log configuration
static struct {
    log_level_t level;
    uint32_t targets;
    uint32_t entry_count;
} log_config = {
    .level = LOG_LEVEL_INFO,
    .targets = LOG_TARGET_UART,
    .entry_count = 0
};

// Memory log buffer
#define LOG_BUFFER_SIZE 256
static log_entry_t log_buffer[LOG_BUFFER_SIZE];
static uint32_t log_buffer_head = 0;

// Level names
static const char *level_names[] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

// Level colors (ANSI)
static const char *level_colors[] = {
    "\x1B[0m",      // NONE - reset
    "\x1B[31m",     // ERROR - red
    "\x1B[33m",     // WARN - yellow
    "\x1B[32m",     // INFO - green
    "\x1B[36m",     // DEBUG - cyan
    "\x1B[90m"      // TRACE - gray
};

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

static char *strncpy(char *dest, const char *src, uint32_t n) {
    char *d = dest;
    while (n-- && (*d++ = *src++));
    while (n--) *d++ = '\0';
    return dest;
}

// Initialize logging
void log_init(log_level_t level, uint32_t targets) {
    log_config.level = level;
    log_config.targets = targets;
    log_config.entry_count = 0;
    log_buffer_head = 0;

    log_info("LOG", "Logging system initialized");
}

// Set log level
void log_set_level(log_level_t level) {
    log_config.level = level;
}

// Get log level
log_level_t log_get_level(void) {
    return log_config.level;
}

// Set targets
void log_set_targets(uint32_t targets) {
    log_config.targets = targets;
}

// Format timestamp (simple - milliseconds)
static void format_timestamp(uint64_t timestamp, char *buffer, uint32_t size) {
    uint64_t ms = timestamp / 1000;
    uint32_t seconds = ms / 1000;
    uint32_t millis = ms % 1000;

    // Simple conversion (would be better with snprintf)
    buffer[0] = '[';
    int pos = 1;

    // Convert seconds
    if (seconds == 0) {
        buffer[pos++] = '0';
    } else {
        char temp[16];
        int i = 0;
        uint32_t s = seconds;
        while (s > 0) {
            temp[i++] = '0' + (s % 10);
            s /= 10;
        }
        while (i > 0) {
            buffer[pos++] = temp[--i];
        }
    }

    buffer[pos++] = '.';

    // Convert milliseconds (3 digits)
    buffer[pos++] = '0' + (millis / 100);
    buffer[pos++] = '0' + ((millis / 10) % 10);
    buffer[pos++] = '0' + (millis % 10);
    buffer[pos++] = ']';
    buffer[pos] = '\0';
}

// Core log function
static void log_write(log_level_t level, const char *subsystem, const char *message) {
    if (level > log_config.level) return;
    if (!subsystem || !message) return;

    uint64_t timestamp = timer_get_counter();

    // Write to UART
    if (log_config.targets & LOG_TARGET_UART) {
        char ts_buf[32];
        format_timestamp(timestamp, ts_buf, sizeof(ts_buf));

        uart_puts(level_colors[level]);
        uart_puts(ts_buf);
        uart_putc(' ');
        uart_puts(level_names[level]);
        uart_puts(": [");
        uart_puts(subsystem);
        uart_puts("] ");
        uart_puts(message);
        uart_puts("\x1B[0m"); // Reset color
        uart_putc('\n');
    }

    // Write to memory buffer
    if (log_config.targets & LOG_TARGET_MEMORY) {
        log_entry_t *entry = &log_buffer[log_buffer_head];
        entry->timestamp = timestamp;
        entry->level = level;
        entry->subsystem = subsystem;
        strncpy(entry->message, message, sizeof(entry->message) - 1);
        entry->message[sizeof(entry->message) - 1] = '\0';

        log_buffer_head = (log_buffer_head + 1) % LOG_BUFFER_SIZE;
        if (log_config.entry_count < LOG_BUFFER_SIZE) {
            log_config.entry_count++;
        }
    }
}

// Log level functions
void log_error(const char *subsystem, const char *message) {
    log_write(LOG_LEVEL_ERROR, subsystem, message);
}

void log_warn(const char *subsystem, const char *message) {
    log_write(LOG_LEVEL_WARN, subsystem, message);
}

void log_info(const char *subsystem, const char *message) {
    log_write(LOG_LEVEL_INFO, subsystem, message);
}

void log_debug(const char *subsystem, const char *message) {
    log_write(LOG_LEVEL_DEBUG, subsystem, message);
}

void log_trace(const char *subsystem, const char *message) {
    log_write(LOG_LEVEL_TRACE, subsystem, message);
}

// Hexdump utility
void log_hexdump(log_level_t level, const char *subsystem, const void *data, uint32_t length) {
    if (level > log_config.level) return;
    if (!data) return;

    const uint8_t *bytes = (const uint8_t *)data;
    char hex_buffer[80];
    char ascii_buffer[17];

    for (uint32_t i = 0; i < length; i += 16) {
        int hex_pos = 0;
        int ascii_pos = 0;

        // Offset
        hex_pos += 8; // Space for offset

        for (uint32_t j = 0; j < 16 && (i + j) < length; j++) {
            uint8_t byte = bytes[i + j];

            // Hex representation
            hex_buffer[hex_pos++] = "0123456789abcdef"[byte >> 4];
            hex_buffer[hex_pos++] = "0123456789abcdef"[byte & 0x0F];
            hex_buffer[hex_pos++] = ' ';

            // ASCII representation
            ascii_buffer[ascii_pos++] = (byte >= 32 && byte < 127) ? byte : '.';
        }

        hex_buffer[hex_pos] = '\0';
        ascii_buffer[ascii_pos] = '\0';

        // Output line
        if (log_config.targets & LOG_TARGET_UART) {
            uart_puts(hex_buffer);
            uart_puts(" | ");
            uart_puts(ascii_buffer);
            uart_putc('\n');
        }
    }
}

// Memory log access
int log_get_entry_count(void) {
    return log_config.entry_count;
}

const log_entry_t *log_get_entry(int index) {
    if (index < 0 || index >= log_config.entry_count) return NULL;

    uint32_t actual_index;
    if (log_config.entry_count < LOG_BUFFER_SIZE) {
        actual_index = index;
    } else {
        actual_index = (log_buffer_head + index) % LOG_BUFFER_SIZE;
    }

    return &log_buffer[actual_index];
}

void log_dump_memory(void) {
    uart_puts("\n=== Log Memory Dump ===\n");
    uart_puts("Total entries: ");
    // Print count
    uart_putc('\n');

    for (int i = 0; i < log_config.entry_count; i++) {
        const log_entry_t *entry = log_get_entry(i);
        if (entry) {
            char ts_buf[32];
            format_timestamp(entry->timestamp, ts_buf, sizeof(ts_buf));

            uart_puts(ts_buf);
            uart_putc(' ');
            uart_puts(level_names[entry->level]);
            uart_puts(": [");
            uart_puts(entry->subsystem);
            uart_puts("] ");
            uart_puts(entry->message);
            uart_putc('\n');
        }
    }

    uart_puts("======================\n\n");
}

void log_clear_memory(void) {
    log_buffer_head = 0;
    log_config.entry_count = 0;
}
