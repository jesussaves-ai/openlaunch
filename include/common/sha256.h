/*
 * include/common/sha256.h
 *
 * Clean-room SHA-256 and HMAC-SHA-256 (FIPS 180-4 / RFC 2104).
 *
 * The installer uses these to fingerprint the NAND image it is about to write
 * and to authenticate the signed installer package, so the implementation is
 * deliberately small, freestanding and zero-allocation: the whole state lives
 * in the caller's sha256_ctx_t / hmac_ctx_t on the stack (112 / 224 bytes).
 *
 * There are no libc dependencies (no memcpy/memset), which keeps the module
 * usable from the earliest boot stages and linkable into both CPU images.
 */
#ifndef DSI_COMMON_SHA256_H
#define DSI_COMMON_SHA256_H

#include "types.h"

#define SHA256_BLOCK_SIZE 64u
#define SHA256_DIGEST_SIZE 32u
#define SHA256_HEX_SIZE (SHA256_DIGEST_SIZE * 2u + 1u)   /* 65, incl. NUL */

/* Incremental SHA-256 state. Treat as opaque; keep it on the stack. */
typedef struct {
    u32 state[8];
    u64 bitlen;                       /* total message length in bits    */
    u8  buffer[SHA256_BLOCK_SIZE];
    u32 buffer_len;                   /* bytes pending in `buffer`       */
} sha256_ctx_t;

/* HMAC-SHA-256 incremental state (inner + outer SHA-256 contexts). */
typedef struct {
    sha256_ctx_t inner;
    sha256_ctx_t outer;
} hmac_sha256_ctx_t;

/* ---- One-shot SHA-256 ------------------------------------------------- */

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const void *data, u32 len);
/* Finalise and write SHA256_DIGEST_SIZE bytes to `out`. */
void sha256_final(sha256_ctx_t *ctx, u8 out[SHA256_DIGEST_SIZE]);

/* Convenience one-shot over a contiguous buffer. */
void sha256(const void *data, u32 len, u8 out[SHA256_DIGEST_SIZE]);

/* ---- HMAC-SHA-256 ----------------------------------------------------- */

void hmac_sha256_init(hmac_sha256_ctx_t *ctx, const u8 *key, u32 key_len);
void hmac_sha256_update(hmac_sha256_ctx_t *ctx, const void *data, u32 len);
void hmac_sha256_final(hmac_sha256_ctx_t *ctx, u8 out[SHA256_DIGEST_SIZE]);

void hmac_sha256(const u8 *key, u32 key_len,
                 const void *data, u32 data_len,
                 u8 out[SHA256_DIGEST_SIZE]);

/* ---- Helpers ---------------------------------------------------------- */

/* Constant-time comparison of two 32-byte digests (no early exit). */
bool sha256_digest_equal(const u8 a[SHA256_DIGEST_SIZE],
                         const u8 b[SHA256_DIGEST_SIZE]);

/* Lower-case hex of a 32-byte digest into `out` (SHA256_HEX_SIZE bytes). */
void sha256_hex(const u8 digest[SHA256_DIGEST_SIZE],
                char out[SHA256_HEX_SIZE]);

#endif /* DSI_COMMON_SHA256_H */
