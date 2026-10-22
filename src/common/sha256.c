/*
 * src/common/sha256.c
 *
 * Clean-room SHA-256 + HMAC-SHA-256. See include/common/sha256.h.
 *
 * FIPS 180-4 (SHA-256) and RFC 2104 (HMAC). Implemented from the algorithm
 * description only; no third-party code was copied. Everything is freestanding
 * (no libc, no allocation) so it links into the ARM9/ARM7 images and the host
 * test harness unchanged.
 */
#include "common/sha256.h"

/* ---- Round constants (first 32 bits of the fractional parts of the cube
 *      roots of the first 64 primes). ---- */
static const u32 K[64] = {
    0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u,
    0x3956C25Bu, 0x59F111F1u, 0x923F82A4u, 0xAB1C5ED5u,
    0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u,
    0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u,
    0xE49B69C1u, 0xEFBE4786u, 0x0FC19DC6u, 0x240CA1CCu,
    0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
    0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u,
    0xC6E00BF3u, 0xD5A79147u, 0x06CA6351u, 0x14292967u,
    0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u,
    0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u,
    0xA2BFE8A1u, 0xA81A664Bu, 0xC24B8B70u, 0xC76C51A3u,
    0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
    0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u,
    0x391C0CB3u, 0x4ED8AA4Au, 0x5B9CCA4Fu, 0x682E6FF3u,
    0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
    0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u,
};

/* Initial hash values H0..H7 (fractional parts of the square roots of the
 * first eight primes). */
static const u32 H0[8] = {
    0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
    0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u,
};

static u32 rotr32(u32 x, u32 n)
{
    return (x >> n) | (x << (32u - n));
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

static void sha256_zero(void *p, u32 len)
{
    u8 *b = (u8 *)p;
    u32 i;

    for (i = 0u; i < len; i++) {
        b[i] = 0u;
    }
}

static void sha256_copy(u8 *dst, const u8 *src, u32 len)
{
    u32 i;

    for (i = 0u; i < len; i++) {
        dst[i] = src[i];
    }
}

/* Process one 64-byte block in place. */
static void sha256_block(sha256_ctx_t *ctx, const u8 *block)
{
    u32 w[64];
    u32 a, b, c, d, e, f, g, h;
    u32 i;

    for (i = 0u; i < 16u; i++) {
        w[i] = load_be32(block + i * 4u);
    }
    for (i = 16u; i < 64u; i++) {
        u32 s0 = rotr32(w[i - 15u], 7u) ^ rotr32(w[i - 15u], 18u) ^
                 (w[i - 15u] >> 3);
        u32 s1 = rotr32(w[i - 2u], 17u) ^ rotr32(w[i - 2u], 19u) ^
                 (w[i - 2u] >> 10);

        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0u; i < 64u; i++) {
        u32 S1 = rotr32(e, 6u) ^ rotr32(e, 11u) ^ rotr32(e, 25u);
        u32 ch = (e & f) ^ ((~e) & g);
        u32 t1 = h + S1 + ch + K[i] + w[i];
        u32 S0 = rotr32(a, 2u) ^ rotr32(a, 13u) ^ rotr32(a, 22u);
        u32 maj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_init(sha256_ctx_t *ctx)
{
    u32 i;

    if (ctx == NULL) {
        return;
    }
    for (i = 0u; i < 8u; i++) {
        ctx->state[i] = H0[i];
    }
    ctx->bitlen = 0u;
    ctx->buffer_len = 0u;
    sha256_zero(ctx->buffer, sizeof(ctx->buffer));
}

void sha256_update(sha256_ctx_t *ctx, const void *data, u32 len)
{
    const u8 *p = (const u8 *)data;

    if (ctx == NULL || (data == NULL && len != 0u)) {
        return;
    }

    ctx->bitlen += (u64)len * 8u;

    /* Top up a pending partial block first. */
    if (ctx->buffer_len != 0u) {
        u32 need = SHA256_BLOCK_SIZE - ctx->buffer_len;

        if (len < need) {
            sha256_copy(ctx->buffer + ctx->buffer_len, p, len);
            ctx->buffer_len += len;
            return;
        }
        sha256_copy(ctx->buffer + ctx->buffer_len, p, need);
        sha256_block(ctx, ctx->buffer);
        ctx->buffer_len = 0u;
        p += need;
        len -= need;
    }

    while (len >= SHA256_BLOCK_SIZE) {
        sha256_block(ctx, p);
        p += SHA256_BLOCK_SIZE;
        len -= SHA256_BLOCK_SIZE;
    }

    if (len != 0u) {
        sha256_copy(ctx->buffer, p, len);
        ctx->buffer_len = len;
    }
}

void sha256_final(sha256_ctx_t *ctx, u8 out[SHA256_DIGEST_SIZE])
{
    u64 bitlen;
    u32 i;

    if (ctx == NULL || out == NULL) {
        return;
    }

    bitlen = ctx->bitlen;

    /* Append the 0x80 padding byte, then zero-fill to 56 mod 64. */
    ctx->buffer[ctx->buffer_len++] = 0x80u;
    if (ctx->buffer_len > 56u) {
        while (ctx->buffer_len < SHA256_BLOCK_SIZE) {
            ctx->buffer[ctx->buffer_len++] = 0u;
        }
        sha256_block(ctx, ctx->buffer);
        ctx->buffer_len = 0u;
    }
    while (ctx->buffer_len < 56u) {
        ctx->buffer[ctx->buffer_len++] = 0u;
    }

    /* 64-bit big-endian message length in bits. */
    for (i = 0u; i < 8u; i++) {
        ctx->buffer[56u + i] = (u8)(bitlen >> (56u - i * 8u));
    }
    sha256_block(ctx, ctx->buffer);

    for (i = 0u; i < 8u; i++) {
        store_be32(out + i * 4u, ctx->state[i]);
    }
}

void sha256(const void *data, u32 len, u8 out[SHA256_DIGEST_SIZE])
{
    sha256_ctx_t ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

/* ---- HMAC-SHA-256 ----------------------------------------------------- */

void hmac_sha256_init(hmac_sha256_ctx_t *ctx, const u8 *key, u32 key_len)
{
    u8 k[SHA256_BLOCK_SIZE];
    u8 pad[SHA256_BLOCK_SIZE];
    u32 i;

    if (ctx == NULL) {
        return;
    }

    sha256_zero(k, sizeof(k));
    if (key != NULL && key_len != 0u) {
        if (key_len > SHA256_BLOCK_SIZE) {
            /* Keys longer than the block size are hashed first. */
            sha256(key, key_len, k);
        } else {
            sha256_copy(k, key, key_len);
        }
    }

    /* inner = H((K ^ ipad) || message) */
    for (i = 0u; i < SHA256_BLOCK_SIZE; i++) {
        pad[i] = (u8)(k[i] ^ 0x36u);
    }
    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, pad, SHA256_BLOCK_SIZE);

    /* outer = H((K ^ opad) || inner) */
    for (i = 0u; i < SHA256_BLOCK_SIZE; i++) {
        pad[i] = (u8)(k[i] ^ 0x5Cu);
    }
    sha256_init(&ctx->outer);
    sha256_update(&ctx->outer, pad, SHA256_BLOCK_SIZE);

    /* Scrub the key material. */
    sha256_zero(k, sizeof(k));
    sha256_zero(pad, sizeof(pad));
}

void hmac_sha256_update(hmac_sha256_ctx_t *ctx, const void *data, u32 len)
{
    if (ctx == NULL) {
        return;
    }
    sha256_update(&ctx->inner, data, len);
}

void hmac_sha256_final(hmac_sha256_ctx_t *ctx, u8 out[SHA256_DIGEST_SIZE])
{
    u8 inner_digest[SHA256_DIGEST_SIZE];

    if (ctx == NULL || out == NULL) {
        return;
    }

    sha256_final(&ctx->inner, inner_digest);
    sha256_update(&ctx->outer, inner_digest, SHA256_DIGEST_SIZE);
    sha256_final(&ctx->outer, out);

    sha256_zero(inner_digest, sizeof(inner_digest));
}

void hmac_sha256(const u8 *key, u32 key_len,
                 const void *data, u32 data_len,
                 u8 out[SHA256_DIGEST_SIZE])
{
    hmac_sha256_ctx_t ctx;

    hmac_sha256_init(&ctx, key, key_len);
    hmac_sha256_update(&ctx, data, data_len);
    hmac_sha256_final(&ctx, out);
}

/* ---- Helpers ---------------------------------------------------------- */

bool sha256_digest_equal(const u8 a[SHA256_DIGEST_SIZE],
                         const u8 b[SHA256_DIGEST_SIZE])
{
    u8 diff = 0u;
    u32 i;

    if (a == NULL || b == NULL) {
        return false;
    }
    for (i = 0u; i < SHA256_DIGEST_SIZE; i++) {
        diff = (u8)(diff | (a[i] ^ b[i]));
    }
    return diff == 0u;
}

void sha256_hex(const u8 digest[SHA256_DIGEST_SIZE], char out[SHA256_HEX_SIZE])
{
    static const char hex[] = "0123456789abcdef";
    u32 i;

    if (digest == NULL || out == NULL) {
        return;
    }
    for (i = 0u; i < SHA256_DIGEST_SIZE; i++) {
        out[i * 2u] = hex[digest[i] >> 4];
        out[i * 2u + 1u] = hex[digest[i] & 0x0Fu];
    }
    out[SHA256_DIGEST_SIZE * 2u] = '\0';
}
