/*
 * include/common/sha1.h
 *
 * Clean-room SHA-1 (FIPS 180-4).
 *
 * The DSi derives the NAND AES-CTR initial counter from SHA1(eMMC CID)
 * (GBATEK "DSi SD/MMC Internal NAND Layout"), so the offline installer and the
 * on-console path both need SHA-1. Freestanding (no libc, no allocation) and
 * zero-dependency so it links into every image and the host tool.
 */
#ifndef DSI_COMMON_SHA1_H
#define DSI_COMMON_SHA1_H

#include "types.h"

#define SHA1_BLOCK_SIZE  64u
#define SHA1_DIGEST_SIZE 20u

/* Incremental SHA-1 state. Treat as opaque; keep it on the stack. */
typedef struct {
    u32 state[5];
    u64 bitlen;                       /* total message length in bits    */
    u8  buffer[SHA1_BLOCK_SIZE];
    u32 buffer_len;                   /* bytes pending in `buffer`       */
} sha1_ctx_t;

void sha1_init(sha1_ctx_t *ctx);
void sha1_update(sha1_ctx_t *ctx, const void *data, u32 len);
/* Finalise and write SHA1_DIGEST_SIZE bytes to `out`. */
void sha1_final(sha1_ctx_t *ctx, u8 out[SHA1_DIGEST_SIZE]);

/* Convenience one-shot over a contiguous buffer. */
void sha1(const void *data, u32 len, u8 out[SHA1_DIGEST_SIZE]);

#endif /* DSI_COMMON_SHA1_H */
