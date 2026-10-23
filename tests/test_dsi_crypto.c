/*
 * tests/test_dsi_crypto.c
 *
 * Host known-answer tests for the DSi per-console NAND crypto
 * (src/common/dsi_crypto.c). The vectors are taken from a real retail NAND
 * dump and its no$gba/Unlaunch footer, so a pass proves the key derivation and
 * the hardware AES-CTR convention are byte-exact:
 *
 *   suite 17a  key derivation (Console ID -> AES-128 normal key)
 *   suite 17b  SHA1(CID) base counter
 *   suite 17c  MBR block 31 decrypt (0x1FE == 0x55AA)
 *   suite 17d  FAT16 VBR decrypt (OEM "TWL")
 *   suite 17e  round-trip / alignment / NULL safety
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/dsi_crypto.h"

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

/* The console under test (from its dump footer). */
static const char *CONSOLE_ID_HEX = "2081082556282008";
static const char *CID_HEX = "7DA087132A3257373136354D00011500";

static void test_key(void)
{
    u8 console_id[8];
    u8 key[16];

    from_hex(CONSOLE_ID_HEX, console_id, 8u);
    dsi_nand_key(console_id, key);
    check_hex(key, 16u, "be5ebb9c4cf15128026f3fa5fc8e87de");

    dsi_nand_key(NULL, key);
    dsi_nand_key(console_id, NULL);
}

static void test_iv(void)
{
    u8 cid[16];
    u8 iv[16];

    from_hex(CID_HEX, cid, 16u);
    dsi_nand_iv(cid, iv);
    /* SHA1(CID)[0:16]. */
    check_hex(iv, 16u, "076fe79935fb4b473fcdf7ff5ecd6490");

    dsi_nand_iv(NULL, iv);
    dsi_nand_iv(cid, NULL);
}

static void test_mbr_block(void)
{
    u8 console_id[8], cid[16], key[16];
    u8 buf[16];

    from_hex(CONSOLE_ID_HEX, console_id, 8u);
    from_hex(CID_HEX, cid, 16u);
    dsi_nand_key(console_id, key);

    /* Encrypted MBR bytes 0x1F0..0x1FF (counter block 31). */
    from_hex("dcd330b1efd06a7e9e5acf668a69495a", buf, 16u);
    dsi_nand_crypt(key, cid, 0x1F0u, buf, 16u);
    check_hex(buf, 16u, "000000000000000000000000000055aa");
}

static void test_vbr(void)
{
    u8 console_id[8], cid[16], key[16];
    u8 buf[32];
    u8 orig[32];
    const char *ct =
        "a3ed3858b33a6d80b2fec6636b0b996c"
        "4a91245e46911c6bbc034da4bd3723f8";
    const char *pt =
        "e9000054574c202020202000022001000200020000f834002000100077080000";

    from_hex(CONSOLE_ID_HEX, console_id, 8u);
    from_hex(CID_HEX, cid, 16u);
    dsi_nand_key(console_id, key);
    from_hex(ct, buf, 32u);
    memcpy(orig, buf, sizeof(orig));

    /* The FAT16 VBR lives at LBA 0x877 (byte 0x10EE00). */
    dsi_nand_crypt(key, cid, 0x877u * 512u, buf, 32u);
    check_hex(buf, 32u, pt);
    assert(buf[0] == 0xE9u);                 /* jmp near */
    assert(memcmp(buf + 3, "TWL", 3) == 0);  /* OEM id */

    /* CTR is symmetric: re-running restores the ciphertext exactly. */
    dsi_nand_crypt(key, cid, 0x877u * 512u, buf, 32u);
    assert(memcmp(buf, orig, sizeof(orig)) == 0);
}

static void test_safety(void)
{
    u8 console_id[8], cid[16], key[16];
    u8 buf[32];

    memset(buf, 0x5Au, sizeof(buf));
    memset(console_id, 0, sizeof(console_id));
    memset(cid, 0, sizeof(cid));
    dsi_nand_key(console_id, key);

    /* Misaligned length / address and NULL arguments are no-ops (buf intact). */
    dsi_nand_crypt(key, cid, 0u, buf, 17u);
    assert(buf[0] == 0x5Au);
    dsi_nand_crypt(key, cid, 1u, buf, 16u);
    assert(buf[0] == 0x5Au);
    dsi_nand_crypt(key, cid, 0u, buf, 0u);
    dsi_nand_crypt(NULL, cid, 0u, buf, 16u);
    dsi_nand_crypt(key, NULL, 0u, buf, 16u);
    dsi_nand_crypt(key, cid, 0u, NULL, 16u);
    assert(buf[0] == 0x5Au);
}

int main(void)
{
    test_key();
    test_iv();
    test_mbr_block();
    test_vbr();
    test_safety();

    puts("test_dsi_crypto: all assertions passed");
    return 0;
}
