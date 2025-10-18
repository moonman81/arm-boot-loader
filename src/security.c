/* Security and Trust Implementation for Bootloader */

#include "uart.h"
#include "crypto.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

// External symbols for bootloader boundaries
extern char _start;
extern char _end;

// Custom string/memory functions for freestanding environment
static void *memcpy(void *dest, const void *src, uint32_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}

static int memcmp(const void *s1, const void *s2, uint32_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

// Security constants
#define TPM_BASE 0xFE002000  // TPM base address (placeholder)
#define SECURITY_MEASUREMENT_SIZE 32
#define BOOT_POLICY_SIZE 256

// Security state structures
typedef struct {
    uint8_t measurement[SECURITY_MEASUREMENT_SIZE];
    uint8_t pcr_values[24][SECURITY_MEASUREMENT_SIZE];  // 24 PCRs
    uint32_t measurement_count;
} security_context_t;

typedef struct {
    uint8_t policy_data[BOOT_POLICY_SIZE];
    uint32_t policy_size;
    uint8_t signature[64];  // ECDSA signature
} boot_policy_t;

// Global security context
static security_context_t security_ctx;
static boot_policy_t boot_policy;

// Real SHA-256 hash function
static void security_hash(const void *data, uint32_t length, uint8_t *hash) {
    sha256_hash((const uint8_t *)data, length, hash);
}

// Extend PCR with measurement
static void security_extend_pcr(uint8_t pcr_index, const uint8_t *measurement) {
    if (pcr_index >= 24) return;

    // PCR extend: PCR[new] = SHA256(PCR[old] || measurement)
    uint8_t combined[SECURITY_MEASUREMENT_SIZE * 2];
    memcpy(combined, security_ctx.pcr_values[pcr_index], SECURITY_MEASUREMENT_SIZE);
    memcpy(combined + SECURITY_MEASUREMENT_SIZE, measurement, SECURITY_MEASUREMENT_SIZE);

    security_hash(combined, sizeof(combined), security_ctx.pcr_values[pcr_index]);
}

// External symbols for bootloader boundaries
extern char _start;
extern char _end;

// Measure component
static void security_measure_component(const char *component_name, const void *data, uint32_t length) {
    uint8_t measurement[SECURITY_MEASUREMENT_SIZE];

    uart_puts("Measuring component: ");
    uart_puts(component_name);
    uart_puts("\n");

    security_hash(data, length, measurement);
    security_extend_pcr(0, measurement);  // PCR 0 for boot components

    security_ctx.measurement_count++;
}

// Verify measurement against expected value
static int security_verify_measurement(const uint8_t *expected_measurement) {
    return memcmp(security_ctx.measurement, expected_measurement, SECURITY_MEASUREMENT_SIZE) == 0;
}

// Initialize security context
int security_init(void) {
    memset(&security_ctx, 0, sizeof(security_context_t));
    memset(&boot_policy, 0, sizeof(boot_policy_t));

    // Initialize PCRs to zero
    for (int i = 0; i < 24; i++) {
        memset(security_ctx.pcr_values[i], 0, SECURITY_MEASUREMENT_SIZE);
    }

    return 0;
}

// Perform security attestation
int security_attest(void) {
    uart_puts("Performing security attestation...\n");

    // Measure bootloader code
    uint32_t bootloader_size = (uint32_t)(&_end - &_start);
    security_measure_component("bootloader", (void *)&_start, bootloader_size);

    // Measure hardware configuration
    // In a real implementation, would measure CPU, memory, etc.

    return 0;
}

// Perform firmware measurement
int security_measure_firmware(void) {
    uart_puts("Measuring firmware integrity...\n");

    // Measure various firmware components
    security_measure_component("interrupt_vector", (void *)0x00000000, 0x1000);
    security_measure_component("exception_handlers", (void *)0x00000800, 0x1800);

    // Measure device tree if available
    // security_measure_component("device_tree", dtb_addr, dtb_size);

    return 0;
}

// Validate boot policy
int security_validate_boot_policy(void) {
    uart_puts("Validating boot policy...\n");

    // In a real implementation, would:
    // 1. Load boot policy from secure storage
    // 2. Verify policy signature
    // 3. Check policy against current measurements
    // 4. Ensure policy allows current boot configuration

    // For bootloader, simulate policy validation
    if (boot_policy.policy_size == 0) {
        // Create default policy
        boot_policy.policy_size = 32;
        memset(boot_policy.policy_data, 0xFF, boot_policy.policy_size);
    }

    return 0;
}

// Initialize trusted execution environment
int security_init_trusted_execution(void) {
    uart_puts("Initializing trusted execution environment...\n");

    // In a real implementation, would:
    // 1. Set up secure memory regions
    // 2. Initialize trust zones
    // 3. Configure secure interrupts
    // 4. Establish secure communication channels

    return 0;
}

// Get current security state
security_context_t* security_get_context(void) {
    return &security_ctx;
}

// Verify component integrity
int security_verify_component(const char *component_name, const void *data, uint32_t length, const uint8_t *expected_hash) {
    uint8_t computed_hash[SECURITY_MEASUREMENT_SIZE];

    security_hash(data, length, computed_hash);

    if (memcmp(computed_hash, expected_hash, SECURITY_MEASUREMENT_SIZE) != 0) {
        uart_puts("Security verification failed for: ");
        uart_puts(component_name);
        uart_puts("\n");
        return -1;
    }

    return 0;
}