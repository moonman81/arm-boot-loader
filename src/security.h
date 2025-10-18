/* Security and Trust Header for Bootloader */

#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>

// Security constants
#define SECURITY_MEASUREMENT_SIZE 32
#define BOOT_POLICY_SIZE 256

// Security context structure
typedef struct {
    uint8_t measurement[SECURITY_MEASUREMENT_SIZE];
    uint8_t pcr_values[24][SECURITY_MEASUREMENT_SIZE];  // 24 PCRs
    uint32_t measurement_count;
} security_context_t;

// Boot policy structure
typedef struct {
    uint8_t policy_data[BOOT_POLICY_SIZE];
    uint32_t policy_size;
    uint8_t signature[64];  // ECDSA signature
} boot_policy_t;

// Security functions
int security_init(void);
int security_attest(void);
int security_measure_firmware(void);
int security_validate_boot_policy(void);
int security_init_trusted_execution(void);
security_context_t* security_get_context(void);
int security_verify_component(const char *component_name, const void *data, uint32_t length, const uint8_t *expected_hash);

#endif