/* Memory Diagnostics and Testing */

#ifndef MEMTEST_H
#define MEMTEST_H

#include <stdint.h>

// Test patterns
#define PATTERN_0x00000000  0x00000000
#define PATTERN_0xFFFFFFFF  0xFFFFFFFF
#define PATTERN_0xAAAAAAAA  0xAAAAAAAA
#define PATTERN_0x55555555  0x55555555
#define PATTERN_WALKING_1   0x00000001
#define PATTERN_WALKING_0   0xFFFFFFFE

// Test result structure
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t errors_found;
    uint32_t last_error_address;
    uint32_t last_error_expected;
    uint32_t last_error_actual;
} memtest_result_t;

// Initialize memory test subsystem
void memtest_init(void);

// Quick memory test (basic patterns)
int memtest_quick(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Comprehensive memory test (all patterns + walking bits)
int memtest_comprehensive(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Test specific pattern
int memtest_pattern(uint32_t start_addr, uint32_t length, uint32_t pattern, memtest_result_t *result);

// Walking ones test
int memtest_walking_ones(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Walking zeros test
int memtest_walking_zeros(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Address line test
int memtest_address_lines(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Data line test
int memtest_data_lines(uint32_t start_addr, memtest_result_t *result);

// Random pattern test
int memtest_random(uint32_t start_addr, uint32_t length, uint32_t iterations, memtest_result_t *result);

// Marching test (1s and 0s)
int memtest_marching(uint32_t start_addr, uint32_t length, memtest_result_t *result);

// Print test results
void memtest_print_results(const memtest_result_t *result);

// Get safe test range (avoid bootloader code)
void memtest_get_safe_range(uint32_t *start_addr, uint32_t *length);

#endif
