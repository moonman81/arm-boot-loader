/* Memory Diagnostics Implementation */

#include "memtest.h"
#include "uart.h"
#include "log.h"
#include "timer.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Safe test range (avoid bootloader at 0x00-0x100000)
#define SAFE_TEST_START     0x10000000  // 256MB
#define SAFE_TEST_LENGTH    0x10000000  // 256MB

// Simple PRNG for random tests
static uint32_t prng_state = 0x12345678;

static uint32_t prng_next(void) {
    // XorShift32
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    return prng_state;
}

static void prng_seed(uint32_t seed) {
    prng_state = seed;
}

// Initialize memory test
void memtest_init(void) {
    prng_seed(timer_get_counter() & 0xFFFFFFFF);
    log_info("MEMTEST", "Memory test subsystem initialized");
}

// Get safe test range
void memtest_get_safe_range(uint32_t *start_addr, uint32_t *length) {
    if (start_addr) *start_addr = SAFE_TEST_START;
    if (length) *length = SAFE_TEST_LENGTH;
}

// Record error
static void record_error(memtest_result_t *result, uint32_t address, uint32_t expected, uint32_t actual) {
    result->errors_found++;
    result->last_error_address = address;
    result->last_error_expected = expected;
    result->last_error_actual = actual;
}

// Test specific pattern
int memtest_pattern(uint32_t start_addr, uint32_t length, uint32_t pattern, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    uint32_t words = length / 4;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Testing pattern...");

    // Write pattern
    for (uint32_t i = 0; i < words; i++) {
        memory[i] = pattern;
    }

    // Verify pattern
    for (uint32_t i = 0; i < words; i++) {
        uint32_t read_val = memory[i];
        if (read_val != pattern) {
            record_error(result, start_addr + (i * 4), pattern, read_val);
            errors++;
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Quick memory test
int memtest_quick(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    int total_errors = 0;

    log_info("MEMTEST", "Running quick memory test");

    // Test basic patterns
    total_errors += memtest_pattern(start_addr, length, PATTERN_0x00000000, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0xFFFFFFFF, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0xAAAAAAAA, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0x55555555, result);

    if (total_errors == 0) {
        log_info("MEMTEST", "Quick test PASSED");
    } else {
        log_error("MEMTEST", "Quick test FAILED");
    }

    return total_errors;
}

// Walking ones test
int memtest_walking_ones(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    uint32_t words = length / 4;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running walking ones test");

    // Test each bit position
    for (int bit = 0; bit < 32; bit++) {
        uint32_t pattern = 1U << bit;

        // Write pattern
        for (uint32_t i = 0; i < words; i++) {
            memory[i] = pattern;
        }

        // Verify pattern
        for (uint32_t i = 0; i < words; i++) {
            uint32_t read_val = memory[i];
            if (read_val != pattern) {
                record_error(result, start_addr + (i * 4), pattern, read_val);
                errors++;
            }
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Walking zeros test
int memtest_walking_zeros(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    uint32_t words = length / 4;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running walking zeros test");

    // Test each bit position
    for (int bit = 0; bit < 32; bit++) {
        uint32_t pattern = ~(1U << bit);

        // Write pattern
        for (uint32_t i = 0; i < words; i++) {
            memory[i] = pattern;
        }

        // Verify pattern
        for (uint32_t i = 0; i < words; i++) {
            uint32_t read_val = memory[i];
            if (read_val != pattern) {
                record_error(result, start_addr + (i * 4), pattern, read_val);
                errors++;
            }
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Address line test
int memtest_address_lines(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running address line test");

    // Write address to each power-of-2 offset
    for (uint32_t offset = 4; offset < length; offset <<= 1) {
        uint32_t addr = start_addr + offset;
        *(volatile uint32_t *)addr = addr;
    }

    // Verify addresses
    for (uint32_t offset = 4; offset < length; offset <<= 1) {
        uint32_t addr = start_addr + offset;
        uint32_t read_val = *(volatile uint32_t *)addr;
        if (read_val != addr) {
            record_error(result, addr, addr, read_val);
            errors++;
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Data line test
int memtest_data_lines(uint32_t start_addr, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running data line test");

    // Test each data bit
    for (int bit = 0; bit < 32; bit++) {
        uint32_t pattern = 1U << bit;

        memory[0] = pattern;
        uint32_t read_val = memory[0];

        if (read_val != pattern) {
            record_error(result, start_addr, pattern, read_val);
            errors++;
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Random pattern test
int memtest_random(uint32_t start_addr, uint32_t length, uint32_t iterations, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    uint32_t words = length / 4;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running random pattern test");

    for (uint32_t iter = 0; iter < iterations; iter++) {
        // Write random pattern
        for (uint32_t i = 0; i < words; i++) {
            memory[i] = prng_next();
        }

        // Reset PRNG to same seed
        prng_seed(prng_state);

        // Verify pattern
        for (uint32_t i = 0; i < words; i++) {
            uint32_t expected = prng_next();
            uint32_t read_val = memory[i];

            if (read_val != expected) {
                record_error(result, start_addr + (i * 4), expected, read_val);
                errors++;
            }
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Marching test
int memtest_marching(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    volatile uint32_t *memory = (volatile uint32_t *)start_addr;
    uint32_t words = length / 4;
    int errors = 0;

    result->tests_run++;

    log_info("MEMTEST", "Running marching test");

    // March up with 0s
    for (uint32_t i = 0; i < words; i++) {
        memory[i] = 0x00000000;
    }

    for (uint32_t i = 0; i < words; i++) {
        // Verify 0
        if (memory[i] != 0x00000000) {
            record_error(result, start_addr + (i * 4), 0x00000000, memory[i]);
            errors++;
        }

        // Write 1
        memory[i] = 0xFFFFFFFF;

        // Verify 1
        if (memory[i] != 0xFFFFFFFF) {
            record_error(result, start_addr + (i * 4), 0xFFFFFFFF, memory[i]);
            errors++;
        }
    }

    // March down with 1s
    for (int i = words - 1; i >= 0; i--) {
        // Verify 1
        if (memory[i] != 0xFFFFFFFF) {
            record_error(result, start_addr + (i * 4), 0xFFFFFFFF, memory[i]);
            errors++;
        }

        // Write 0
        memory[i] = 0x00000000;

        // Verify 0
        if (memory[i] != 0x00000000) {
            record_error(result, start_addr + (i * 4), 0x00000000, memory[i]);
            errors++;
        }
    }

    if (errors == 0) {
        result->tests_passed++;
    } else {
        result->tests_failed++;
    }

    return errors;
}

// Comprehensive memory test
int memtest_comprehensive(uint32_t start_addr, uint32_t length, memtest_result_t *result) {
    int total_errors = 0;

    log_info("MEMTEST", "Running comprehensive memory test");

    // Run all tests
    total_errors += memtest_data_lines(start_addr, result);
    total_errors += memtest_address_lines(start_addr, length, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0x00000000, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0xFFFFFFFF, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0xAAAAAAAA, result);
    total_errors += memtest_pattern(start_addr, length, PATTERN_0x55555555, result);
    total_errors += memtest_walking_ones(start_addr, length, result);
    total_errors += memtest_walking_zeros(start_addr, length, result);
    total_errors += memtest_marching(start_addr, length, result);
    total_errors += memtest_random(start_addr, length, 3, result);

    if (total_errors == 0) {
        log_info("MEMTEST", "Comprehensive test PASSED");
    } else {
        log_error("MEMTEST", "Comprehensive test FAILED");
    }

    return total_errors;
}

// Print test results
void memtest_print_results(const memtest_result_t *result) {
    uart_puts("\r\n");
    uart_puts("Memory Test Results\r\n");
    uart_puts("===================\r\n");

    uart_puts("Tests Run: ");
    char buf[16];
    int pos = 0;
    uint32_t val = result->tests_run;
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

    uart_puts("Tests Passed: ");
    pos = 0;
    val = result->tests_passed;
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

    uart_puts("Tests Failed: ");
    pos = 0;
    val = result->tests_failed;
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

    uart_puts("Errors Found: ");
    pos = 0;
    val = result->errors_found;
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

    if (result->errors_found > 0) {
        uart_puts("\r\nLast Error:\r\n");
        uart_puts("  Address:  0x");
        for (int j = 28; j >= 0; j -= 4) {
            uart_putc("0123456789abcdef"[result->last_error_address >> j & 0xF]);
        }
        uart_puts("\r\n");

        uart_puts("  Expected: 0x");
        for (int j = 28; j >= 0; j -= 4) {
            uart_putc("0123456789abcdef"[result->last_error_expected >> j & 0xF]);
        }
        uart_puts("\r\n");

        uart_puts("  Actual:   0x");
        for (int j = 28; j >= 0; j -= 4) {
            uart_putc("0123456789abcdef"[result->last_error_actual >> j & 0xF]);
        }
        uart_puts("\r\n");
    }

    uart_puts("\r\n");
}
