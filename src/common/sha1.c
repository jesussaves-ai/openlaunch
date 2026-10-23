/*
 * src/common/sha1.c
 *
 * Clean-room SHA-1 (FIPS 180-4). See include/common/sha1.h.
 *
 * Implemented from the FIPS 180-4 description only; no third-party code. The
 * whole state lives in the caller's sha1_ctx_t (92 bytes) on the stack, with no
 * libc dependency. Validated by the FIPS 180-4 / RFC 3174 known-answer vectors
 * in tests/test_crypto.c.
 */
#include "common/sha1.h"

static u32 rol32(u32 x, u32 n)
{
    return (x << n) | (x >> (32u - n));
}

static u32 load_be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
           ((u32)p[2] << 8) | (u32)p[3];
}

static void store_be32(u8 *p, u32 v)
{
    p[0] = (u8)(v >> 24);
    p[1] = (u8)(v >> 16);
    p[2] = (u8)(v >> 8);
    p[3] = (u8)v;
}

static void store_be64(u8 *p, u64 v)
{
    store_be32(p, (u32)(v >> 32));
    store_be32(p + 4, (u32)v);
}

/* Process one 64-byte block, updating the chaining state. */
static void sha1_block(sha1_ctx_t *ctx, const u8 *p)
{
    u32 w[80];
    u32 a, b, c, d, e;
    u32 i;

    for (i = 0u; i < 16u; i++) {
        w[i] = load_be32(p + 4u * i);
    }
    for (i = 16u; i < 80u; i++) {
        w[i] = rol32(w[i - 3u] ^ w[i - 8u] ^ w[i - 14u] ^ w[i - 16u], 1u);
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (i = 0u; i < 80u; i++) {
        u32 f, k, t;

        if (i < 20u) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        } else if (i < 40u) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60u) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }

        t = rol32(a, 5u) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol32(b, 30u);
        b = a;
        a = t;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

void sha1_init(sha1_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xEFCDAB89u;
    ctx->state[2] = 0x98BADCFEu;
    ctx->state[3] = 0x10325476u;
    ctx->state[4] = 0xC3D2E1F0u;
    ctx->bitlen = 0u;
    ctx->buffer_len = 0u;
}

void sha1_update(sha1_ctx_t *ctx, const void *data, u32 len)
{
    const u8 *p = (const u8 *)data;

    if (ctx == NULL || (data == NULL && len != 0u)) {
        return;
    }
    ctx->bitlen += (u64)len * 8u;

    while (len != 0u) {
        u32 space = SHA1_BLOCK_SIZE - ctx->buffer_len;
        u32 n = (len < space) ? len : space;

        for (u32 i = 0u; i < n; i++) {
            ctx->buffer[ctx->buffer_len + i] = p[i];
        }
        ctx->buffer_len += n;
        p += n;
        len -= n;

        if (ctx->buffer_len == SHA1_BLOCK_SIZE) {
            sha1_block(ctx, ctx->buffer);
            ctx->buffer_len = 0u;
        }
    }
}

void sha1_final(sha1_ctx_t *ctx, u8 out[SHA1_DIGEST_SIZE])
{
    u8 *b;
    u32 i;
    u64 bits;

    if (ctx == NULL || out == NULL) {
        return;
    }
    b = ctx->buffer;
    bits = ctx->bitlen;
    i = ctx->buffer_len;

    b[i++] = 0x80u;
    if (i > 56u) {
        while (i < SHA1_BLOCK_SIZE) {
            b[i++] = 0u;
        }
        sha1_block(ctx, b);
        i = 0u;
    }
    while (i < 56u) {
        b[i++] = 0u;
    }
    store_be64(b + 56u, bits);
    sha1_block(ctx, b);

    for (i = 0u; i < 5u; i++) {
        store_be32(out + 4u * i, ctx->state[i]);
    }
}

void sha1(const void *data, u32 len, u8 out[SHA1_DIGEST_SIZE])
{
    sha1_ctx_t ctx;

    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, out);
}
