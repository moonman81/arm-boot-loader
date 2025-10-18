/* Cryptographic Functions for Bootloader */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>

// SHA-256 constants
#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

// SHA-256 context
typedef struct {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[SHA256_BLOCK_SIZE];
    uint32_t buffer_len;
} sha256_context_t;

// SHA-256 functions
void sha256_init(sha256_context_t *ctx);
void sha256_update(sha256_context_t *ctx, const uint8_t *data, uint32_t length);
void sha256_final(sha256_context_t *ctx, uint8_t *digest);
void sha256_hash(const uint8_t *data, uint32_t length, uint8_t *digest);

// HMAC-SHA256 for authenticated hashing
void hmac_sha256(const uint8_t *key, uint32_t key_len,
                 const uint8_t *data, uint32_t data_len,
                 uint8_t *digest);

// PBKDF2 for key derivation
void pbkdf2_sha256(const uint8_t *password, uint32_t password_len,
                   const uint8_t *salt, uint32_t salt_len,
                   uint32_t iterations,
                   uint8_t *derived_key, uint32_t key_len);

// Utility functions
void crypto_secure_zero(void *ptr, uint32_t size);
int crypto_constant_time_compare(const void *a, const void *b, uint32_t size);

#endif
