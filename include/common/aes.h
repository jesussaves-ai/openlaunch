/*
 * include/common/aes.h
 *
 * Clean-room AES-128 (FIPS-197) with ECB block and CTR modes.
 *
 * This is the foundation for the DSi per-console NAND crypto: the eMMC's MBR
 * and both FAT partitions are AES-128-CTR encrypted with a key derived from
 * the console/CPU id and the eMMC CID (GBATEK "DSi SD/MMC Internal NAND
 * Layout"; see docs/hardware_notes.md 34.12). It is implemented from the
 * FIPS-197 description only and is freestanding (no libc, no allocation), so
 * it links into the ARM9/ARM7 images and the offline host tool unchanged.
 */
#ifndef DSI_COMMON_AES_H
#define DSI_COMMON_AES_H

#include "types.h"

#define AES_BLOCK_SIZE    16u
#define AES128_KEY_SIZE   16u
#define AES128_ROUND_KEYS (11u * AES_BLOCK_SIZE)   /* 176 bytes */

/* Expanded AES-128 key. Treat as opaque; keep it on the stack. */
typedef struct {
    u8 round_key[AES128_ROUND_KEYS];
} aes128_ctx_t;

/* Expand a 16-byte key into the 11 round keys. */
void aes128_init(aes128_ctx_t *ctx, const u8 key[AES128_KEY_SIZE]);

/* Encrypt / decrypt exactly one 16-byte block. */
void aes128_encrypt_block(const aes128_ctx_t *ctx,
                          const u8 in[AES_BLOCK_SIZE],
                          u8 out[AES_BLOCK_SIZE]);
void aes128_decrypt_block(const aes128_ctx_t *ctx,
                          const u8 in[AES_BLOCK_SIZE],
                          u8 out[AES_BLOCK_SIZE]);

/*
 * AES-128-CTR keystream XOR. `counter` is the initial 128-bit big-endian
 * counter block; it is incremented (mod 2^128) after every 16 bytes and updated
 * in place so a caller can continue a stream. XORs `len` bytes of `buf` in
 * place (encrypt and decrypt are the same operation).
 *
 * The DSi builds `counter` from SHA1(CID) + (sector address / 0x10) at the
 * higher layer; this primitive only supplies generic RFC 3686-style CTR mode.
 */
void aes128_ctr(const aes128_ctx_t *ctx, u8 counter[AES_BLOCK_SIZE],
                u8 *buf, u32 len);

#endif /* DSI_COMMON_AES_H */
