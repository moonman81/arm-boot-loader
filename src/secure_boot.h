/* Secure Boot Chain of Trust */

#ifndef SECURE_BOOT_H
#define SECURE_BOOT_H

#include <stdint.h>

// Signature algorithms
typedef enum {
    SIG_ALG_RSA2048_SHA256,
    SIG_ALG_RSA4096_SHA256,
    SIG_ALG_ECDSA_P256_SHA256,
    SIG_ALG_ED25519
} signature_algorithm_t;

// Boot stage verification
typedef enum {
    BOOT_STAGE_BOOTLOADER,
    BOOT_STAGE_DTB,
    BOOT_STAGE_KERNEL,
    BOOT_STAGE_INITRD,
    BOOT_STAGE_MAX
} boot_stage_t;

// Signature block structure
#define SIGNATURE_MAGIC     0x53494742  // "SIGB"
#define MAX_SIGNATURE_SIZE  512

typedef struct {
    uint32_t magic;
    uint32_t algorithm;
    uint32_t key_id;
    uint32_t signature_length;
    uint8_t  signature[MAX_SIGNATURE_SIZE];
    uint8_t  public_key_hash[32];  // SHA-256 of public key
} signature_block_t;

// Public key structure
#define PUBLIC_KEY_MAGIC    0x5055424B  // "PUBK"

typedef struct {
    uint32_t magic;
    uint32_t algorithm;
    uint32_t key_id;
    uint32_t modulus_length;     // For RSA
    uint32_t exponent_length;    // For RSA
    uint8_t  key_data[512];      // Public key material
} public_key_t;

// Chain of trust state
typedef struct {
    uint8_t  bootloader_verified;
    uint8_t  dtb_verified;
    uint8_t  kernel_verified;
    uint8_t  initrd_verified;
    uint32_t last_verified_stage;
    uint8_t  root_of_trust_hash[32];
} chain_of_trust_t;

// Initialize secure boot
int secure_boot_init(void);

// Load and verify root public key
int secure_boot_load_root_key(const public_key_t *key);

// Verify signature on data
int secure_boot_verify_signature(
    const void *data,
    uint32_t data_length,
    const signature_block_t *signature,
    const public_key_t *public_key
);

// Verify boot stage
int secure_boot_verify_stage(
    boot_stage_t stage,
    const void *data,
    uint32_t data_length,
    const signature_block_t *signature
);

// Get chain of trust state
const chain_of_trust_t *secure_boot_get_chain_state(void);

// Check if secure boot is enabled
int secure_boot_is_enabled(void);

// Lock secure boot (prevent disabling)
void secure_boot_lock(void);

// RSA-2048 signature verification
int secure_boot_verify_rsa2048(
    const void *data,
    uint32_t data_length,
    const uint8_t *signature,
    const uint8_t *public_key_n,  // Modulus
    const uint8_t *public_key_e   // Exponent
);

// Hash verification (for testing)
int secure_boot_verify_hash(
    const void *data,
    uint32_t data_length,
    const uint8_t *expected_hash
);

// Generate SHA-256 hash of data
void secure_boot_hash_sha256(const void *data, uint32_t length, uint8_t *hash);

#endif
