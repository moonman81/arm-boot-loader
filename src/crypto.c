/* Cryptographic Functions Implementation */

#include "crypto.h"

// Custom memory functions for freestanding environment
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

// SHA-256 constants (first 32 bits of fractional parts of cube roots of first 64 primes)
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// SHA-256 initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// Rotate right
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

// SHA-256 functions
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

// Transform block
static void sha256_transform(sha256_context_t *ctx, const uint8_t *block) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    int i;

    // Prepare message schedule
    for (i = 0; i < 16; i++) {
        W[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }

    for (i = 16; i < 64; i++) {
        W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
    }

    // Initialize working variables
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    // Main loop
    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

// Initialize SHA-256 context
void sha256_init(sha256_context_t *ctx) {
    ctx->state[0] = H0[0];
    ctx->state[1] = H0[1];
    ctx->state[2] = H0[2];
    ctx->state[3] = H0[3];
    ctx->state[4] = H0[4];
    ctx->state[5] = H0[5];
    ctx->state[6] = H0[6];
    ctx->state[7] = H0[7];
    ctx->bitcount = 0;
    ctx->buffer_len = 0;
}

// Update SHA-256 with data
void sha256_update(sha256_context_t *ctx, const uint8_t *data, uint32_t length) {
    uint32_t i;

    for (i = 0; i < length; i++) {
        ctx->buffer[ctx->buffer_len++] = data[i];

        if (ctx->buffer_len == SHA256_BLOCK_SIZE) {
            sha256_transform(ctx, ctx->buffer);
            ctx->bitcount += 512;
            ctx->buffer_len = 0;
        }
    }
}

// Finalize SHA-256 and produce digest
void sha256_final(sha256_context_t *ctx, uint8_t *digest) {
    uint32_t i;
    uint64_t bitcount;

    // Pad message
    i = ctx->buffer_len;

    // Append the '1' bit (plus zero padding)
    ctx->buffer[i++] = 0x80;

    // Pad with zeros until 56 bytes
    if (ctx->buffer_len < 56) {
        while (i < 56) {
            ctx->buffer[i++] = 0x00;
        }
    } else {
        while (i < 64) {
            ctx->buffer[i++] = 0x00;
        }
        sha256_transform(ctx, ctx->buffer);
        memset(ctx->buffer, 0, 56);
    }

    // Append length in bits as 64-bit big-endian
    bitcount = ctx->bitcount + (ctx->buffer_len * 8);
    ctx->buffer[63] = bitcount & 0xff;
    ctx->buffer[62] = (bitcount >> 8) & 0xff;
    ctx->buffer[61] = (bitcount >> 16) & 0xff;
    ctx->buffer[60] = (bitcount >> 24) & 0xff;
    ctx->buffer[59] = (bitcount >> 32) & 0xff;
    ctx->buffer[58] = (bitcount >> 40) & 0xff;
    ctx->buffer[57] = (bitcount >> 48) & 0xff;
    ctx->buffer[56] = (bitcount >> 56) & 0xff;

    sha256_transform(ctx, ctx->buffer);

    // Produce final hash value (big-endian)
    for (i = 0; i < 8; i++) {
        digest[i * 4] = (ctx->state[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
        digest[i * 4 + 3] = ctx->state[i] & 0xff;
    }

    // Securely clear context
    crypto_secure_zero(ctx, sizeof(sha256_context_t));
}

// One-shot SHA-256 hash
void sha256_hash(const uint8_t *data, uint32_t length, uint8_t *digest) {
    sha256_context_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, length);
    sha256_final(&ctx, digest);
}

// HMAC-SHA256
void hmac_sha256(const uint8_t *key, uint32_t key_len,
                 const uint8_t *data, uint32_t data_len,
                 uint8_t *digest) {
    sha256_context_t ctx;
    uint8_t k_pad[SHA256_BLOCK_SIZE];
    uint8_t tk[SHA256_DIGEST_SIZE];
    uint32_t i;

    // If key is longer than block size, hash it first
    if (key_len > SHA256_BLOCK_SIZE) {
        sha256_hash(key, key_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_SIZE;
    }

    // Prepare key with padding
    memset(k_pad, 0, SHA256_BLOCK_SIZE);
    memcpy(k_pad, key, key_len);

    // Inner hash: H((K XOR ipad) || message)
    for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
        k_pad[i] ^= 0x36; // ipad
    }

    sha256_init(&ctx);
    sha256_update(&ctx, k_pad, SHA256_BLOCK_SIZE);
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, digest);

    // Outer hash: H((K XOR opad) || inner_hash)
    memset(k_pad, 0, SHA256_BLOCK_SIZE);
    memcpy(k_pad, key, key_len);

    for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
        k_pad[i] ^= 0x5c; // opad
    }

    sha256_init(&ctx);
    sha256_update(&ctx, k_pad, SHA256_BLOCK_SIZE);
    sha256_update(&ctx, digest, SHA256_DIGEST_SIZE);
    sha256_final(&ctx, digest);

    // Securely clear sensitive data
    crypto_secure_zero(k_pad, SHA256_BLOCK_SIZE);
    crypto_secure_zero(tk, SHA256_DIGEST_SIZE);
}

// PBKDF2-SHA256
void pbkdf2_sha256(const uint8_t *password, uint32_t password_len,
                   const uint8_t *salt, uint32_t salt_len,
                   uint32_t iterations,
                   uint8_t *derived_key, uint32_t key_len) {
    uint8_t U[SHA256_DIGEST_SIZE];
    uint8_t T[SHA256_DIGEST_SIZE];
    uint8_t salt_block[256]; // Max salt + block number
    uint32_t block_count = (key_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE;
    uint32_t i, j, k;

    for (i = 1; i <= block_count; i++) {
        // Prepare salt || INT(i)
        memcpy(salt_block, salt, salt_len);
        salt_block[salt_len] = (i >> 24) & 0xff;
        salt_block[salt_len + 1] = (i >> 16) & 0xff;
        salt_block[salt_len + 2] = (i >> 8) & 0xff;
        salt_block[salt_len + 3] = i & 0xff;

        // U_1 = PRF(password, salt || INT(i))
        hmac_sha256(password, password_len, salt_block, salt_len + 4, U);
        memcpy(T, U, SHA256_DIGEST_SIZE);

        // U_j = PRF(password, U_{j-1})
        for (j = 1; j < iterations; j++) {
            hmac_sha256(password, password_len, U, SHA256_DIGEST_SIZE, U);

            // T = T XOR U_j
            for (k = 0; k < SHA256_DIGEST_SIZE; k++) {
                T[k] ^= U[k];
            }
        }

        // Copy T to output
        uint32_t offset = (i - 1) * SHA256_DIGEST_SIZE;
        uint32_t copy_len = (key_len - offset > SHA256_DIGEST_SIZE) ?
                            SHA256_DIGEST_SIZE : (key_len - offset);
        memcpy(derived_key + offset, T, copy_len);
    }

    // Securely clear sensitive data
    crypto_secure_zero(U, SHA256_DIGEST_SIZE);
    crypto_secure_zero(T, SHA256_DIGEST_SIZE);
    crypto_secure_zero(salt_block, 256);
}

// Secure memory zeroing (prevents compiler optimization)
void crypto_secure_zero(void *ptr, uint32_t size) {
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (size--) {
        *p++ = 0;
    }
}

// Constant-time comparison (prevents timing attacks)
int crypto_constant_time_compare(const void *a, const void *b, uint32_t size) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t result = 0;
    uint32_t i;

    for (i = 0; i < size; i++) {
        result |= pa[i] ^ pb[i];
    }

    return result == 0 ? 0 : -1;
}
