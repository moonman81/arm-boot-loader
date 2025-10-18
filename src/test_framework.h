/* Comprehensive Unit Test Framework */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>

// Test result structure
typedef struct {
    const char *test_name;
    int passed;
    int failed;
    int skipped;
    int total;
    uint64_t duration_us;
} test_result_t;

// Test suite structure
typedef struct {
    const char *suite_name;
    int (*setup)(void);
    int (*teardown)(void);
    void (*tests)(void);
    test_result_t result;
} test_suite_t;

// Global test state
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
    const char *current_test_name;
    const char *current_suite_name;
    int current_test_failed;
} test_state_t;

// Test framework API
void test_init(void);
void test_suite_begin(const char *suite_name);
void test_suite_end(void);
void test_begin(const char *test_name);
void test_end(void);
void test_skip(const char *reason);

// Assertion macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            test_fail(__FILE__, __LINE__, #condition); \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            test_fail_equal(__FILE__, __LINE__, #expected, #actual, (expected), (actual)); \
        } \
    } while (0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            test_fail_not_equal(__FILE__, __LINE__, #expected, #actual, (expected)); \
        } \
    } while (0)

#define TEST_ASSERT_NULL(pointer) \
    do { \
        if ((pointer) != NULL) { \
            test_fail(__FILE__, __LINE__, #pointer " should be NULL"); \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(pointer) \
    do { \
        if ((pointer) == NULL) { \
            test_fail(__FILE__, __LINE__, #pointer " should not be NULL"); \
        } \
    } while (0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    do { \
        if (!test_strings_equal((expected), (actual))) { \
            test_fail_string(__FILE__, __LINE__, (expected), (actual)); \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, length) \
    do { \
        if (!test_memory_equal((expected), (actual), (length))) { \
            test_fail_memory(__FILE__, __LINE__, (expected), (actual), (length)); \
        } \
    } while (0)

#define TEST_ASSERT_IN_RANGE(value, min, max) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            test_fail_range(__FILE__, __LINE__, #value, (value), (min), (max)); \
        } \
    } while (0)

// Failure handlers
void test_fail(const char *file, int line, const char *message);
void test_fail_equal(const char *file, int line, const char *expected_expr, const char *actual_expr,
                     uint32_t expected, uint32_t actual);
void test_fail_not_equal(const char *file, int line, const char *expected_expr, const char *actual_expr,
                         uint32_t value);
void test_fail_string(const char *file, int line, const char *expected, const char *actual);
void test_fail_memory(const char *file, int line, const void *expected, const void *actual, uint32_t length);
void test_fail_range(const char *file, int line, const char *expr, uint32_t value, uint32_t min, uint32_t max);

// Helper functions
int test_strings_equal(const char *s1, const char *s2);
int test_memory_equal(const void *m1, const void *m2, uint32_t length);

// Print test results
void test_print_results(void);
void test_print_summary(void);

// Get test statistics
const test_state_t *test_get_state(void);

#endif
