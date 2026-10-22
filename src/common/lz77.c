/*
 * src/common/lz77.c
 *
 * Freestanding Nintendo LZ77 / RLE decoders. See include/common/lz77.h.
 *
 * The reference ARM9 crt0 delegates this to BIOS SWI #0x11; reproducing it in
 * plain C removes the firmware dependency and makes the format testable on the
 * host. The reference stub and its literal pool are documented in
 * docs/hardware_notes.md §ARM9 image layout.
 *
 * Every read is guarded by `src_size` and every write by `dst_size`, so a
 * corrupted header cannot walk the loader off the end of a buffer.
 */
#include "common/lz77.h"

static u32 read_le24(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16);
}

/* Emit one byte, refusing to grow past the destination capacity. */
static bool emit(u8 *dst, u32 dst_size, u32 *out, u8 value)
{
    if (*out >= dst_size) {
        return false;
    }
    dst[*out] = value;
    (*out)++;
    return true;
}

/* Fetch one input byte, refusing to read past the end of the stream. */
static bool next_byte(const u8 *src, u32 src_size, u32 *in, u8 *out)
{
    if (*in >= src_size) {
        return false;
    }
    *out = src[*in];
    (*in)++;
    return true;
}

u32 nds_lz77_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size)
{
    u32 expected;
    u32 out = 0;
    u32 in = 4;

    if (src_size < 4u) {
        return 0;
    }
    if (src[0] != NDS_LZ77_TYPE10 && src[0] != NDS_LZ77_TYPE11) {
        return 0;
    }
    expected = read_le24(src + 1);
    if (expected > dst_size) {
        return 0;
    }

    while (out < expected) {
        u8 flags;
        u32 frames;

        if (!next_byte(src, src_size, &in, &flags)) {
            return 0;
        }

        for (frames = 0; frames < 8u && out < expected; frames++) {
            if ((flags & (u8)(0x80u >> frames)) == 0u) {
                u8 value;
                if (!next_byte(src, src_size, &in, &value)) {
                    return 0;
                }
                if (!emit(dst, dst_size, &out, value)) {
                    return 0;
                }
            } else {
                u32 length;
                u32 disp;
                u8 b1;
                u8 b2;

                if (!next_byte(src, src_size, &in, &b1) ||
                    !next_byte(src, src_size, &in, &b2)) {
                    return 0;
                }

                length = (u32)(b1 >> 4) + 3u;
                disp = ((u32)(b1 & 0x0Fu) << 8) | (u32)b2;

                if (src[0] == NDS_LZ77_TYPE11) {
                    if (length == 3u) {
                        u8 b3;
                        if (!next_byte(src, src_size, &in, &b3)) {
                            return 0;
                        }
                        length = (((u32)b1 & 0x0Fu) << 12) |
                                 ((u32)b2 << 4) | ((u32)b3 >> 4);
                        length += 0x11u;
                        disp = ((disp & 0x0Fu) << 8) | ((u32)b3 & 0x0Fu);
                    } else if (length == 4u) {
                        u8 b3;
                        u8 b4;
                        if (!next_byte(src, src_size, &in, &b3) ||
                            !next_byte(src, src_size, &in, &b4)) {
                            return 0;
                        }
                        length = (((u32)b1 & 0x0Fu) << 18) |
                                 ((u32)b2 << 10) |
                                 ((u32)b3 << 2) |
                                 ((u32)b4 >> 6);
                        length += 0x211u;
                        disp = (disp << 8) | (u32)b4;
                    }
                }

                /* A back-reference may not reach before the output start. */
                if (disp + 1u > out) {
                    return 0;
                }
                while (length != 0u) {
                    if (!emit(dst, dst_size, &out, dst[out - disp - 1u])) {
                        return 0;
                    }
                    length--;
                }
            }
        }
    }
    return out;
}

u32 nds_rle_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size)
{
    u32 expected;
    u32 out = 0;
    u32 in = 4;

    if (src_size < 4u || src[0] != NDS_RLE_TYPE30) {
        return 0;
    }
    expected = read_le24(src + 1);
    if (expected > dst_size) {
        return 0;
    }

    while (out < expected) {
        u8 flags;
        u32 bit;

        if (!next_byte(src, src_size, &in, &flags)) {
            return 0;
        }

        for (bit = 0; bit < 8u && out < expected; bit++) {
            if ((flags & (u8)(0x80u >> bit)) == 0u) {
                u8 value;
                if (!next_byte(src, src_size, &in, &value)) {
                    return 0;
                }
                if (!emit(dst, dst_size, &out, value)) {
                    return 0;
                }
            } else {
                u8 len_raw;
                u8 value;
                u32 length;

                if (!next_byte(src, src_size, &in, &len_raw) ||
                    !next_byte(src, src_size, &in, &value)) {
                    return 0;
                }
                length = (u32)(len_raw & 0x7Fu) + 3u;
                while (length != 0u) {
                    if (!emit(dst, dst_size, &out, value)) {
                        return 0;
                    }
                    length--;
                }
            }
        }
    }
    return out;
}

u32 nds_decompress(const u8 *src, u32 src_size, u8 *dst, u32 dst_size)
{
    if (src_size < 1u) {
        return 0;
    }
    switch (src[0]) {
    case NDS_LZ77_TYPE10:
    case NDS_LZ77_TYPE11:
        return nds_lz77_decompress(src, src_size, dst, dst_size);
    case NDS_RLE_TYPE30:
        return nds_rle_decompress(src, src_size, dst, dst_size);
    default:
        return 0;
    }
}
