/*
 * include/common/lz77.h
 *
 * Nintendo LZ77 ("type 0x10"/"0x11") and run-length ("type 0x30") stream
 * decompression.
 *
 * @ref The ARM9 crt0 in the reference image (asm/decompiled, address 0x02200800)
 * issues BIOS SWI #0x11 to expand the compressed ARM9 payload from ROM into
 * VRAM before jumping into it. See docs/hardware_notes.md §ARM9 image layout.
 *
 * This is a freestanding reimplementation of that BIOS routine so the loader
 * can expand payloads without depending on firmware. Both decoders are fully
 * bounded: no input byte is read past `src_size` and no output byte is written
 * past `dst_size`. They return the number of bytes produced, or 0 if the
 * stream is malformed or truncated.
 */
#ifndef DSI_COMMON_LZ77_H
#define DSI_COMMON_LZ77_H

#include "types.h"

/* Header bytes recognised by nds_decompress(). */
#define NDS_LZ77_TYPE10   0x10u   /* 8-bit frames, 2-byte back-refs   */
#define NDS_LZ77_TYPE11   0x11u   /* 8-bit frames, 3/4-byte back-refs */
#define NDS_RLE_TYPE30    0x30u   /* RLE: flags + literal/run pairs   */

/*
 * Expand an LZ77 (0x10/0x11) stream.
 *
 * @param src       input stream; first byte is the type marker, next three
 *                  the little-endian 24-bit uncompressed size.
 * @param src_size  number of readable bytes at `src`.
 * @param dst       destination buffer.
 * @param dst_size  capacity of `dst` in bytes.
 * @return bytes written, or 0 on malformed / truncated / overflowing input.
 */
u32 nds_lz77_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size);

/*
 * Expand a run-length (0x30) stream.
 * @return bytes written, or 0 on malformed / truncated input.
 */
u32 nds_rle_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size);

/*
 * Dispatch on the leading type byte using the header's own length.
 * @return bytes written, or 0 on malformed or unsupported input.
 */
u32 nds_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size);

#endif /* DSI_COMMON_LZ77_H */
