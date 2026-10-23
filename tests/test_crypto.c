/*
 * tests/test_crypto.c
 *
 * Host-side known-answer tests for the AES-128 (FIPS-197) and SHA-1
 * (FIPS 180-4) core used by the DSi NAND crypto:
 *
 *   suite 16a  AES-128 block encrypt/decrypt (FIPS-197 C.1)
 *   suite 16b  AES-128-CTR (NIST SP 800-38A F.5.1)
 *   suite 16c  SHA-1 (FIPS 180-4 / RFC 3174 vectors, incl. 1,000,000 'a')
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/aes.h"
#include "common/sha1.h"

/* ------------------------------------------------------------------ */
/* Hex helpers                                                         */
/* ------------------------------------------------------------------ */

static u8 hexval(char c)
{
    if (c >= '0' && c <= '9') {
        return (u8)(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return (u8)(c - 'a' + 10);
    }
    return (u8)(c - 'A' + 10);
}

static void from_hex(const char *s, u8 *out, u32 len)
{
    u32 i;

    for (i = 0u; i < len; i++) {
        out[i] = (u8)((hexval(s[2u * i]) << 4) | hexval(s[2u * i + 1u]));
    }
}

static void check_hex(const u8 *data, u32 len, const char *expect)
{
    static const char hx[] = "0123456789abcdef";
    u32 i;

    for (i = 0u; i < len; i++) {
        assert(hx[data[i] >> 4] == expect[2u * i]);
        assert(hx[data[i] & 0x0Fu] == expect[2u * i + 1u]);
    }
    assert(expect[2u * len] == '\0');
}

/* ------------------------------------------------------------------ */
/* 16a  AES-128 block                                                  */
/* ------------------------------------------------------------------ */

static void test_aes128_block(void)
{
    aes128_ctx_t ctx;
    u8 key[16], pt[16], ct[16], rt[16];

    /* FIPS-197 Appendix C.1. */
    from_hex("000102030405060708090a0b0c0d0e0f", key, 16u);
    from_hex("00112233445566778899aabbccddeeff", pt, 16u);

    aes128_init(&ctx, key);
    aes128_encrypt_block(&ctx, pt, ct);
    check_hex(ct, 16u, "69c4e0d86a7b0430d8cdb78070b4c55a");

    aes128_decrypt_block(&ctx, ct, rt);
    assert(memcmp(rt, pt, 16u) == 0);

    /* A zero key/plaintext must not crash (and must be reversible). */
    memset(key, 0, sizeof(key));
    memset(pt, 0, sizeof(pt));
    aes128_init(&ctx, key);
    aes128_encrypt_block(&ctx, pt, ct);
    aes128_decrypt_block(&ctx, ct, rt);
    assert(memcmp(rt, pt, 16u) == 0);

    /* NULL safety. */
    aes128_init(NULL, key);
    aes128_encrypt_block(NULL, pt, ct);
    aes128_decrypt_block(&ctx, NULL, ct);
    aes128_decrypt_block(&ctx, pt, NULL);
    aes128_ctr(&ctx, NULL, pt, 16u);
    aes128_ctr(NULL, key, pt, 16u);
    aes128_ctr(&ctx, key, NULL, 16u);
}

/* ------------------------------------------------------------------ */
/* 16b  AES-128-CTR                                                    */
/* ------------------------------------------------------------------ */

static void test_aes128_ctr(void)
{
    aes128_ctx_t ctx;
    u8 key[16], ctr[16], buf[64];

    /* NIST SP 800-38A F.5.1 (CTR-AES128.Encrypt), 4 blocks. */
    static const char *const KEY = "2b7e151628aed2a6abf7158809cf4f3c";
    static const char *const IV  = "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";
    static const char *const PT  =
        "6bc1bee22e409f96e93d7e117393172a"
        "ae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52ef"
        "f69f2445df4f9b17ad2b417be66c3710";
    static const char *const CT  =
        "874d6191b620e3261bef6864990db6ce"
        "9806f66b7970fdff8617187bb9fffdff"
        "5ae4df3edbd5d35e5b4f09020db03eab"
        "1e031dda2fbe03d1792170a0f3009cee";

    from_hex(KEY, key, 16u);
    from_hex(IV, ctr, 16u);
    from_hex(PT, buf, 64u);

    aes128_init(&ctx, key);
    aes128_ctr(&ctx, ctr, buf, 64u);
    check_hex(buf, 64u, CT);

    /* CTR is its own inverse: re-running with the original IV recovers PT. */
    from_hex(IV, ctr, 16u);
    aes128_ctr(&ctx, ctr, buf, 64u);
    check_hex(buf, 64u, PT);

    /* A partial (non-block-aligned) run only touches `len` bytes. */
    from_hex(IV, ctr, 16u);
    memset(buf, 0xAA, sizeof(buf));
    aes128_ctr(&ctx, ctr, buf, 20u);
    assert(buf[20] == 0xAA && buf[63] == 0xAA);
}

/* ------------------------------------------------------------------ */
/* 16c  SHA-1                                                          */
/* ------------------------------------------------------------------ */

static void test_sha1(void)
{
    u8 d[SHA1_DIGEST_SIZE];
    sha1_ctx_t c;

    sha1("", 0u, d);
    check_hex(d, SHA1_DIGEST_SIZE,
              "da39a3ee5e6b4b0d3255bfef95601890afd80709");

    sha1("abc", 3u, d);
    check_hex(d, SHA1_DIGEST_SIZE,
              "a9993e364706816aba3e25717850c26c9cd0d89d");

    {
        const char *s =
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

        sha1(s, (u32)strlen(s), d);
        check_hex(d, SHA1_DIGEST_SIZE,
                  "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    }

    /* Incremental updates must match the one-shot result. */
    sha1_init(&c);
    sha1_update(&c, "ab", 2u);
    sha1_update(&c, "", 0u);
    sha1_update(&c, "c", 1u);
    sha1_final(&c, d);
    check_hex(d, SHA1_DIGEST_SIZE,
              "a9993e364706816aba3e25717850c26c9cd0d89d");

    /* One million 'a' (FIPS 180-4 / RFC 3174 long vector). */
    {
        static u8 a[1000];
        u32 i;

        memset(a, 'a', sizeof(a));
        sha1_init(&c);
        for (i = 0u; i < 1000u; i++) {
            sha1_update(&c, a, sizeof(a));
        }
        sha1_final(&c, d);
        check_hex(d, SHA1_DIGEST_SIZE,
                  "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
    }

    /* NULL safety. */
    sha1_init(NULL);
    sha1_update(NULL, "x", 1u);
    sha1_final(NULL, d);
    sha1(NULL, 0u, d);
}

int main(void)
{
    test_aes128_block();
    test_aes128_ctr();
    test_sha1();

    puts("test_crypto: all assertions passed");
    return 0;
}
