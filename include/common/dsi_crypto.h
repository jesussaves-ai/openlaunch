/*
 * include/common/dsi_crypto.h
 *
 * DSi (TWL) per-console NAND crypto: key derivation and AES-CTR.
 *
 * The internal eMMC's MBR and both FAT partitions are AES-128-CTR encrypted
 * with a console-unique key. This module reproduces the exact scheme used by
 * the hardware (validated against a real retail NAND dump):
 *
 *   KeyX  = f(CPU/Console ID at [0x4004D00]/[0x4004D04])   (see dsi_nand_key)
 *   KeyY  = 0AB9DC76 BD4DC4D3 202DDD1D E1A00005            (constant)
 *   Key   = ((KeyX XOR KeyY) + C2) ROL 42                  (hardware keygen)
 *   CTR   = SHA1(eMMC CID)[0:16] + (byte_address / 0x10)   (128-bit, LE)
 *
 * The AES core is the DSi hardware "little-endian" variant, so the counter and
 * the keystream bytes are reversed relative to standard AES; AES-128 itself
 * lives in src/common/aes.c.
 *
 * Sources: GBATEK "DSi SD/MMC Internal NAND Layout" / "DSi AES I/O Ports" and
 * 3dbrew "AES Registers" (hardware key generator). See docs/hardware_notes.md.
 */
#ifndef DSI_COMMON_DSI_CRYPTO_H
#define DSI_COMMON_DSI_CRYPTO_H

#include "types.h"

#define DSI_CID_SIZE        16u   /* eMMC Card ID                         */
#define DSI_CONSOLE_ID_SIZE 8u    /* CPU/Console ID ([0x4004D00..0x4004D07]) */
#define DSI_NAND_KEY_SIZE   16u   /* AES-128 normal key                   */

/*
 * Derive the per-console NAND AES-128 normal key from the 8-byte CPU/Console
 * ID (as stored in the no$gba/Unlaunch footer at 0xFF820, or read from
 * [0x4004D00]). Pure; safe to call with any 8-byte buffer.
 */
void dsi_nand_key(const u8 console_id[DSI_CONSOLE_ID_SIZE],
                  u8 key[DSI_NAND_KEY_SIZE]);

/* SHA1(CID)[0:16]: the base counter (before the address term). */
void dsi_nand_iv(const u8 cid[DSI_CID_SIZE], u8 iv[16]);

/*
 * Encrypt or decrypt `len` bytes of eMMC data located at byte address
 * `byte_addr`, in place (AES-CTR is symmetric). `len` must be a non-zero
 * multiple of 16 and `byte_addr` must be 16-byte aligned.
 */
void dsi_nand_crypt(const u8 key[DSI_NAND_KEY_SIZE],
                    const u8 cid[DSI_CID_SIZE],
                    u32 byte_addr, u8 *buf, u32 len);

#endif /* DSI_COMMON_DSI_CRYPTO_H */
