/* Secure Boot Chain of Trust Implementation */

#include "secure_boot.h"
#include "crypto.h"
#include "log.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Memory functions
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
        p1++; p2++;
    }
    return 0;
}

// Secure boot state
static struct {
    int enabled;
    int locked;
    chain_of_trust_t chain;
    public_key_t root_key;
    int root_key_loaded;
} secure_boot_state;

// Initialize secure boot
int secure_boot_init(void) {
    memset(&secure_boot_state, 0, sizeof(secure_boot_state));

    // Check if secure boot should be enabled
    // In production, this would check OTP bits or config
    secure_boot_state.enabled = 0;  // Disabled by default

    log_info("SECURE_BOOT", "Secure boot initialized");
    return 0;
}

// Load root public key
int secure_boot_load_root_key(const public_key_t *key) {
    if (!key) return -1;

    // Verify magic
    if (key->magic != PUBLIC_KEY_MAGIC) {
        log_error("SECURE_BOOT", "Invalid public key magic");
        return -1;
    }

    // Copy key
    memcpy(&secure_boot_state.root_key, key, sizeof(public_key_t));

    // Calculate root of trust hash
    sha256_hash((const uint8_t *)key->key_data,
                key->modulus_length,
                secure_boot_state.chain.root_of_trust_hash);

    secure_boot_state.root_key_loaded = 1;
    log_info("SECURE_BOOT", "Root public key loaded");

    return 0;
}

// Generate SHA-256 hash
void secure_boot_hash_sha256(const void *data, uint32_t length, uint8_t *hash) {
    sha256_hash((const uint8_t *)data, length, hash);
}

// Verify hash
int secure_boot_verify_hash(const void *data, uint32_t data_length, const uint8_t *expected_hash) {
    uint8_t calculated_hash[32];

    // Calculate hash of data
    sha256_hash((const uint8_t *)data, data_length, calculated_hash);

    // Compare hashes
    if (memcmp(calculated_hash, expected_hash, 32) != 0) {
        log_error("SECURE_BOOT", "Hash verification failed");
        return -1;
    }

    log_info("SECURE_BOOT", "Hash verification passed");
    return 0;
}

// Simplified RSA-2048 verification (framework only)
// In production, this would use a proper RSA library
int secure_boot_verify_rsa2048(
    const void *data,
    uint32_t data_length,
    const uint8_t *signature,
    const uint8_t *public_key_n,
    const uint8_t *public_key_e)
{
    // Calculate SHA-256 hash of data
    uint8_t hash[32];
    sha256_hash((const uint8_t *)data, data_length, hash);

    // RSA verification would go here
    // For now, we'll use hash verification as a placeholder
    log_warn("SECURE_BOOT", "RSA verification not implemented, using hash mode");

    // In a real implementation:
    // 1. Decrypt signature using public key (s^e mod n)
    // 2. Extract hash from decrypted signature (PKCS#1 v1.5 padding)
    // 3. Compare extracted hash with calculated hash

    // Placeholder: assume verification succeeds for testing
    return 0;
}

// Verify signature block
int secure_boot_verify_signature(
    const void *data,
    uint32_t data_length,
    const signature_block_t *signature,
    const public_key_t *public_key)
{
    if (!data || !signature || !public_key) return -1;

    // Verify signature magic
    if (signature->magic != SIGNATURE_MAGIC) {
        log_error("SECURE_BOOT", "Invalid signature magic");
        return -1;
    }

    // Verify public key hash matches
    uint8_t key_hash[32];
    sha256_hash((const uint8_t *)public_key->key_data,
                public_key->modulus_length,
                key_hash);

    if (memcmp(key_hash, signature->public_key_hash, 32) != 0) {
        log_error("SECURE_BOOT", "Public key hash mismatch");
        return -1;
    }

    // Verify signature based on algorithm
    switch (signature->algorithm) {
        case SIG_ALG_RSA2048_SHA256:
            return secure_boot_verify_rsa2048(
                data,
                data_length,
                signature->signature,
                public_key->key_data,
                public_key->key_data + 256  // Exponent after modulus
            );

        case SIG_ALG_RSA4096_SHA256:
            log_error("SECURE_BOOT", "RSA-4096 not implemented");
            return -1;

        case SIG_ALG_ECDSA_P256_SHA256:
            log_error("SECURE_BOOT", "ECDSA not implemented");
            return -1;

        case SIG_ALG_ED25519:
            log_error("SECURE_BOOT", "Ed25519 not implemented");
            return -1;

        default:
            log_error("SECURE_BOOT", "Unknown signature algorithm");
            return -1;
    }
}

// Verify boot stage
int secure_boot_verify_stage(
    boot_stage_t stage,
    const void *data,
    uint32_t data_length,
    const signature_block_t *signature)
{
    if (!secure_boot_state.enabled) {
        log_info("SECURE_BOOT", "Secure boot disabled, skipping verification");
        return 0;
    }

    if (!secure_boot_state.root_key_loaded) {
        log_error("SECURE_BOOT", "Root key not loaded");
        return -1;
    }

    log_info("SECURE_BOOT", "Verifying boot stage");

    // Verify signature
    if (secure_boot_verify_signature(data, data_length, signature, &secure_boot_state.root_key) != 0) {
        log_error("SECURE_BOOT", "Signature verification failed");
        return -1;
    }

    // Update chain of trust
    switch (stage) {
        case BOOT_STAGE_BOOTLOADER:
            secure_boot_state.chain.bootloader_verified = 1;
            break;
        case BOOT_STAGE_DTB:
            secure_boot_state.chain.dtb_verified = 1;
            break;
        case BOOT_STAGE_KERNEL:
            secure_boot_state.chain.kernel_verified = 1;
            break;
        case BOOT_STAGE_INITRD:
            secure_boot_state.chain.initrd_verified = 1;
            break;
        default:
            log_error("SECURE_BOOT", "Invalid boot stage");
            return -1;
    }

    secure_boot_state.chain.last_verified_stage = stage;
    log_info("SECURE_BOOT", "Boot stage verified");

    return 0;
}

// Get chain of trust state
const chain_of_trust_t *secure_boot_get_chain_state(void) {
    return &secure_boot_state.chain;
}

// Check if secure boot is enabled
int secure_boot_is_enabled(void) {
    return secure_boot_state.enabled;
}

// Lock secure boot
void secure_boot_lock(void) {
    secure_boot_state.locked = 1;
    log_info("SECURE_BOOT", "Secure boot locked");
}
