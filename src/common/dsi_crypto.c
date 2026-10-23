/*
 * src/common/dsi_crypto.c
 *
 * DSi per-console NAND key derivation + AES-CTR. See include/common/dsi_crypto.h.
 *
 * The key generator constant and the round function come from GBATEK
 * ("DSi AES I/O Ports": Key = ((Key_X XOR Key_Y) + Const) ROL 42) and 3dbrew
 * ("AES Registers": DSi hardware key generator, big-endian 128-bit arithmetic).
 * The derivation here was validated against a real retail dump: the derived key
 * decrypts the MBR (0x55AA, FAT16 partition at LBA 0x877) and that partition's
 * FAT16 VBR ("TWL" OEM id). See tests/test_dsi_crypto.c.
 *
 * Freestanding: no libc, no allocation.
 */
#include "common/dsi_crypto.h"
#include "common/aes.h"
#include "common/sha1.h"
#include "common/mem.h"

/* KeyY (constant), as four little-endian words. */
static const u32 DSI_KEYY[4] = {
    0x0AB9DC76u, 0xBD4DC4D3u, 0x202DDD1Du, 0xE1A00005u,
};

/* Key-generator constant C2 = FFFEFB4E295902582A680F5F1A4F3E79, as four
 * little-endian 32-bit limbs (limb 0 = least significant). */
static const u32 DSI_KEYGEN_C2[4] = {
    0x1A4F3E79u, 0x2A680F5Fu, 0x29590258u, 0xFFFEFB4Eu,
};

/* Rotate the 128-bit little-endian limb array left by 42 bits. */
static void dsi_rotl42(u32 w[4])
{
    u32 a = w[0], b = w[1], c = w[2], d = w[3];

    w[0] = (c >> 22) | (d << 10);
    w[1] = (d >> 22) | (a << 10);
    w[2] = (a >> 22) | (b << 10);
    w[3] = (b >> 22) | (c << 10);
}

void dsi_nand_key(const u8 console_id[DSI_CONSOLE_ID_SIZE],
                  u8 key[DSI_NAND_KEY_SIZE])
{
    u32 kx[4];
    u32 w[4];
    u32 id0, id1;
    u32 carry;
    u32 i;

    if (console_id == NULL || key == NULL) {
        return;
    }

    id0 = (u32)console_id[0] | ((u32)console_id[1] << 8) |
          ((u32)console_id[2] << 16) | ((u32)console_id[3] << 24);
    id1 = (u32)console_id[4] | ((u32)console_id[5] << 8) |
          ((u32)console_id[6] << 16) | ((u32)console_id[7] << 24);

    /* KeyX built from the console ID (GBATEK). */
    kx[0] = id0;
    kx[1] = id0 ^ 0x24EE6906u;
    kx[2] = id1 ^ 0xE65B601Du;
    kx[3] = id1;

    /* w = KeyX XOR KeyY. */
    for (i = 0u; i < 4u; i++) {
        w[i] = kx[i] ^ DSI_KEYY[i];
    }

    /* w += C2 (128-bit little-endian, wraparound). */
    carry = 0u;
    for (i = 0u; i < 4u; i++) {
        u64 t = (u64)w[i] + (u64)DSI_KEYGEN_C2[i] + (u64)carry;

        w[i] = (u32)t;
        carry = (u32)(t >> 32);
    }

    dsi_rotl42(w);

    /* Output the 128-bit value big-endian (the state "big-endian" form). */
    for (i = 0u; i < 4u; i++) {
        u32 lim = w[3u - i];

        key[i * 4u + 0u] = (u8)(lim >> 24);
        key[i * 4u + 1u] = (u8)(lim >> 16);
        key[i * 4u + 2u] = (u8)(lim >> 8);
        key[i * 4u + 3u] = (u8)lim;
    }
}

void dsi_nand_iv(const u8 cid[DSI_CID_SIZE], u8 iv[16])
{
    u8 digest[SHA1_DIGEST_SIZE];

    if (cid == NULL || iv == NULL) {
        return;
    }
    sha1(cid, DSI_CID_SIZE, digest);
    dsi_memcpy(iv, digest, 16u);
}

/* Add a small 32-bit delta to the 128-bit little-endian counter in place. */
static void ctr_add_le(u8 ctr[16], u32 delta)
{
    u32 carry = delta;
    u32 i;

    for (i = 0u; i < 16u && carry != 0u; i++) {
        u32 t = (u32)ctr[i] + (carry & 0xFFu);

        ctr[i] = (u8)t;
        carry = (carry >> 8) + (t >> 8);
    }
}

void dsi_nand_crypt(const u8 key[DSI_NAND_KEY_SIZE],
                    const u8 cid[DSI_CID_SIZE],
                    u32 byte_addr, u8 *buf, u32 len)
{
    aes128_ctx_t ctx;
    u8 base[16];
    u32 block;
    u32 count;

    if (key == NULL || cid == NULL || buf == NULL || len == 0u ||
        (len & 15u) != 0u || (byte_addr & 15u) != 0u) {
        return;
    }

    aes128_init(&ctx, key);
    dsi_nand_iv(cid, base);

    count = len / 16u;
    for (block = 0u; block < count; block++) {
        u8 ctr[16];
        u8 rev[16];
        u8 ks[16];
        u32 i;
        u8 *p = buf + block * 16u;

        /* Counter for this block: base + (byte_addr/16 + block), LE. */
        dsi_memcpy(ctr, base, 16u);
        ctr_add_le(ctr, (byte_addr >> 4) + block);

        /* The hardware AES core is little-endian: reverse the counter in,
         * reverse the keystream out. */
        for (i = 0u; i < 16u; i++) {
            rev[i] = ctr[15u - i];
        }
        aes128_encrypt_block(&ctx, rev, ks);
        for (i = 0u; i < 16u; i++) {
            p[i] ^= ks[15u - i];
        }
    }
}
