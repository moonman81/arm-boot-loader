/* Centralized Logging System */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>

// Log levels
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level_t;

// Log output targets
typedef enum {
    LOG_TARGET_UART = (1 << 0),
    LOG_TARGET_MEMORY = (1 << 1),
    LOG_TARGET_SD = (1 << 2)
} log_target_t;

// Log entry structure
typedef struct {
    uint64_t timestamp;
    log_level_t level;
    const char *subsystem;
    char message[256];
} log_entry_t;

// Initialize logging system
void log_init(log_level_t level, uint32_t targets);

// Set log level
void log_set_level(log_level_t level);

// Get current log level
log_level_t log_get_level(void);

// Set log targets
void log_set_targets(uint32_t targets);

// Core logging functions
void log_error(const char *subsystem, const char *message);
void log_warn(const char *subsystem, const char *message);
void log_info(const char *subsystem, const char *message);
void log_debug(const char *subsystem, const char *message);
void log_trace(const char *subsystem, const char *message);

// Formatted logging (with simple formatting)
void log_errorf(const char *subsystem, const char *fmt, ...);
void log_warnf(const char *subsystem, const char *fmt, ...);
void log_infof(const char *subsystem, const char *fmt, ...);
void log_debugf(const char *subsystem, const char *fmt, ...);
void log_tracef(const char *subsystem, const char *fmt, ...);

// Hexdump utility
void log_hexdump(log_level_t level, const char *subsystem, const void *data, uint32_t length);

// Memory log functions
int log_get_entry_count(void);
const log_entry_t *log_get_entry(int index);
void log_dump_memory(void);
void log_clear_memory(void);

// File log functions (if SD target enabled)
int log_flush_to_file(const char *filename);

// Convenience macros
#define LOG_ERROR(msg) log_error(__func__, msg)
#define LOG_WARN(msg) log_warn(__func__, msg)
#define LOG_INFO(msg) log_info(__func__, msg)
#define LOG_DEBUG(msg) log_debug(__func__, msg)
#define LOG_TRACE(msg) log_trace(__func__, msg)

#endif
