/* Comprehensive Unit Test Framework Implementation */

#include "test_framework.h"
#include "uart.h"
#include "timer.h"
#include "log.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Global test state
static test_state_t test_state;
static uint64_t test_start_time;
static uint64_t suite_start_time;

// String functions
static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

static int memcmp(const void *m1, const void *m2, uint32_t n) {
    const unsigned char *p1 = m1;
    const unsigned char *p2 = m2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

// Initialize test framework
void test_init(void) {
    test_state.total_tests = 0;
    test_state.passed_tests = 0;
    test_state.failed_tests = 0;
    test_state.skipped_tests = 0;
    test_state.current_test_name = NULL;
    test_state.current_suite_name = NULL;
    test_state.current_test_failed = 0;

    uart_puts("\r\n");
    uart_puts("=====================================\r\n");
    uart_puts("  Unit Test Framework Initialized\r\n");
    uart_puts("=====================================\r\n");
    uart_puts("\r\n");
}

// Begin test suite
void test_suite_begin(const char *suite_name) {
    test_state.current_suite_name = suite_name;
    suite_start_time = timer_get_counter();

    uart_puts("\r\n");
    uart_puts("\x1B[1m");  // Bold
    uart_puts("[SUITE] ");
    uart_puts(suite_name);
    uart_puts("\x1B[0m");  // Reset
    uart_puts("\r\n");
    uart_puts("-----------------------------------\r\n");
}

// End test suite
void test_suite_end(void) {
    uint64_t suite_duration = timer_get_counter() - suite_start_time;
    uint32_t suite_duration_ms = suite_duration / 1000;

    uart_puts("-----------------------------------\r\n");
    uart_puts("Suite completed in ");

    char buf[16];
    int pos = 0;
    if (suite_duration_ms == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        uint32_t val = suite_duration_ms;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(" ms\r\n");

    test_state.current_suite_name = NULL;
}

// Begin individual test
void test_begin(const char *test_name) {
    test_state.current_test_name = test_name;
    test_state.current_test_failed = 0;
    test_state.total_tests++;
    test_start_time = timer_get_counter();

    uart_puts("  [TEST] ");
    uart_puts(test_name);
    uart_puts(" ... ");
}

// End individual test
void test_end(void) {
    uint64_t test_duration = timer_get_counter() - test_start_time;
    uint32_t test_duration_us = test_duration;

    if (!test_state.current_test_failed) {
        test_state.passed_tests++;
        uart_puts("\x1B[32m");  // Green
        uart_puts("PASS");
        uart_puts("\x1B[0m");   // Reset

        uart_puts(" (");
        char buf[16];
        int pos = 0;
        if (test_duration_us == 0) buf[pos++] = '0';
        else {
            char temp[16];
            int i = 0;
            uint32_t val = test_duration_us;
            while (val > 0) {
                temp[i++] = '0' + (val % 10);
                val /= 10;
            }
            while (i > 0) buf[pos++] = temp[--i];
        }
        buf[pos] = '\0';
        uart_puts(buf);
        uart_puts(" us)\r\n");
    }

    test_state.current_test_name = NULL;
}

// Skip test
void test_skip(const char *reason) {
    test_state.skipped_tests++;
    test_state.total_tests--;  // Don't count as run

    uart_puts("\x1B[33m");  // Yellow
    uart_puts("SKIP");
    uart_puts("\x1B[0m");   // Reset
    uart_puts(" - ");
    uart_puts(reason);
    uart_puts("\r\n");

    test_state.current_test_name = NULL;
}

// Fail test with message
void test_fail(const char *file, int line, const char *message) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);

    uart_puts(": ");
    uart_puts(message);
    uart_puts("\r\n");
}

// Fail with equality check
void test_fail_equal(const char *file, int line, const char *expected_expr, const char *actual_expr,
                     uint32_t expected, uint32_t actual) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(": ");

    uart_puts(expected_expr);
    uart_puts(" == ");
    uart_puts(actual_expr);
    uart_puts("\r\n");

    uart_puts("    Expected: 0x");
    for (int j = 28; j >= 0; j -= 4) {
        uart_putc("0123456789abcdef"[expected >> j & 0xF]);
    }
    uart_puts("\r\n");

    uart_puts("    Actual:   0x");
    for (int j = 28; j >= 0; j -= 4) {
        uart_putc("0123456789abcdef"[actual >> j & 0xF]);
    }
    uart_puts("\r\n");
}

// Fail with not-equal check
void test_fail_not_equal(const char *file, int line, const char *expected_expr, const char *actual_expr,
                         uint32_t value) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(": ");

    uart_puts(expected_expr);
    uart_puts(" != ");
    uart_puts(actual_expr);
    uart_puts(" (both are 0x");
    for (int j = 28; j >= 0; j -= 4) {
        uart_putc("0123456789abcdef"[value >> j & 0xF]);
    }
    uart_puts(")\r\n");
}

// Fail with string comparison
void test_fail_string(const char *file, int line, const char *expected, const char *actual) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(": String mismatch\r\n");

    uart_puts("    Expected: \"");
    uart_puts(expected);
    uart_puts("\"\r\n");

    uart_puts("    Actual:   \"");
    uart_puts(actual);
    uart_puts("\"\r\n");
}

// Fail with memory comparison
void test_fail_memory(const char *file, int line, const void *expected, const void *actual, uint32_t length) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(": Memory mismatch (");

    pos = 0;
    if (length == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        uint32_t val = length;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(" bytes)\r\n");
}

// Fail with range check
void test_fail_range(const char *file, int line, const char *expr, uint32_t value, uint32_t min, uint32_t max) {
    test_state.current_test_failed = 1;
    test_state.failed_tests++;

    uart_puts("\x1B[31m");  // Red
    uart_puts("FAIL");
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("    ");
    uart_puts(file);
    uart_puts(":");

    char buf[16];
    int pos = 0;
    if (line == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        int val = line;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(": ");

    uart_puts(expr);
    uart_puts(" out of range\r\n");

    uart_puts("    Value: ");
    pos = 0;
    if (value == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        uint32_t val = value;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(", Range: [");

    pos = 0;
    if (min == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        uint32_t val = min;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts(", ");

    pos = 0;
    if (max == 0) buf[pos++] = '0';
    else {
        char temp[16];
        int i = 0;
        uint32_t val = max;
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
        while (i > 0) buf[pos++] = temp[--i];
    }
    buf[pos] = '\0';
    uart_puts(buf);
    uart_puts("]\r\n");
}

// Helper: Compare strings
int test_strings_equal(const char *s1, const char *s2) {
    if (!s1 && !s2) return 1;
    if (!s1 || !s2) return 0;
    return strcmp(s1, s2) == 0;
}

// Helper: Compare memory
int test_memory_equal(const void *m1, const void *m2, uint32_t length) {
    if (!m1 && !m2) return 1;
    if (!m1 || !m2) return 0;
    return memcmp(m1, m2, length) == 0;
}

// Print test results
void test_print_results(void) {
    uart_puts("\r\n");
    uart_puts("=====================================\r\n");
    uart_puts("  Test Results\r\n");
    uart_puts("=====================================\r\n");

    uart_puts("Total Tests:  ");
    char buf[16];
    int pos = 0;
    uint32_t val = test_state.total_tests;
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

    uart_puts("Passed:       ");
    pos = 0;
    val = test_state.passed_tests;
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
    uart_puts("\x1B[32m");  // Green
    uart_puts(buf);
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("Failed:       ");
    pos = 0;
    val = test_state.failed_tests;
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
    if (test_state.failed_tests > 0) {
        uart_puts("\x1B[31m");  // Red
    }
    uart_puts(buf);
    uart_puts("\x1B[0m");   // Reset
    uart_puts("\r\n");

    uart_puts("Skipped:      ");
    pos = 0;
    val = test_state.skipped_tests;
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

    uart_puts("=====================================\r\n");

    if (test_state.failed_tests == 0) {
        uart_puts("\x1B[32m");  // Green
        uart_puts("ALL TESTS PASSED\r\n");
        uart_puts("\x1B[0m");   // Reset
    } else {
        uart_puts("\x1B[31m");  // Red
        uart_puts("SOME TESTS FAILED\r\n");
        uart_puts("\x1B[0m");   // Reset
    }

    uart_puts("\r\n");
}

// Print summary
void test_print_summary(void) {
    uart_puts("Tests: ");

    char buf[16];
    int pos = 0;
    uint32_t val = test_state.passed_tests;
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
    uart_puts(" passed, ");

    pos = 0;
    val = test_state.failed_tests;
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
    uart_puts(" failed");

    if (test_state.skipped_tests > 0) {
        uart_puts(", ");
        pos = 0;
        val = test_state.skipped_tests;
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
        uart_puts(" skipped");
    }

    uart_puts("\r\n");
}

// Get test state
const test_state_t *test_get_state(void) {
    return &test_state;
}
