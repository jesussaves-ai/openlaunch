/*
 * tests/test_arm9.c
 *
 * Host-side unit tests for the ARM9 modules that contain hardware-independent
 * logic: the LZ77/RLE decoders, the segment loader, the MPU descriptor
 * decoders and the VRAM bank packer.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "common/lz77.h"
#include "arm9/boot.h"
#include "arm9/mpu.h"
#include "arm9/video.h"

/* ------------------------------------------------------------------ */
/* LZ77 / RLE                                                          */
/* ------------------------------------------------------------------ */

static void test_lz77(void)
{
    /* "ABCD" literals then a length-4/distance-4 back-reference => "ABCDABCD" */
    static const u8 stream[] = {
        NDS_LZ77_TYPE10, 0x08, 0x00, 0x00,
        0x08, 'A', 'B', 'C', 'D', 0x10, 0x03,
    };
    u8 out[16];

    memset(out, 0, sizeof(out));
    assert(nds_lz77_decompress(stream, sizeof(stream), out, sizeof(out)) == 8u);
    assert(memcmp(out, "ABCDABCD", 8) == 0);

    /* A back-reference that reaches before the start must be rejected. */
    {
        static const u8 bad[] = {
            NDS_LZ77_TYPE10, 0x04, 0x00, 0x00, 0x80, 0x10, 0x00,
        };
        u8 small[4];
        assert(nds_lz77_decompress(bad, sizeof(bad), small, sizeof(small)) == 0u);
    }

    /* Truncated stream: header claims 8 bytes but data ends early. */
    {
        static const u8 cut[] = { NDS_LZ77_TYPE10, 0x08, 0x00, 0x00, 0x08, 'A' };
        u8 small[8];
        assert(nds_lz77_decompress(cut, sizeof(cut), small, sizeof(small)) == 0u);
    }

    /* Wrong type byte. */
    {
        static const u8 wrong[] = { 0x20, 0x04, 0x00, 0x00 };
        u8 buf[4];
        assert(nds_lz77_decompress(wrong, sizeof(wrong), buf, sizeof(buf)) == 0u);
    }
}

static void test_rle(void)
{
    /* 'X' then a run of five 'Y' => "XYYYYY" */
    static const u8 stream[] = {
        NDS_RLE_TYPE30, 0x06, 0x00, 0x00,
        0x40, 'X', 0x02, 'Y',
    };
    u8 out[8];

    memset(out, 0, sizeof(out));
    assert(nds_rle_decompress(stream, sizeof(stream), out, sizeof(out)) == 6u);
    assert(out[0] == 'X');
    assert(memcmp(out + 1, "YYYYY", 5) == 0);

    assert(nds_decompress(stream, sizeof(stream), out, sizeof(out)) == 6u);
}

/* ------------------------------------------------------------------ */
/* Segment loader                                                      */
/* ------------------------------------------------------------------ */

static void test_segment_loader(void)
{
    u8 src[16] = "HELLO WORLD";
    u8 dst[16];

    /* Verbatim copy. */
    arm9_segment_t plain = {
        (uintptr_t)src, sizeof(src), (uintptr_t)dst, 11u, false
    };
    memset(dst, 0, sizeof(dst));
    assert(arm9_load_segments(&plain, 1u) == 1u);
    assert(memcmp(dst, src, 11) == 0);

    /* Compressed copy ("ABCDABCD"). */
    {
        static const u8 stream[] = {
            NDS_LZ77_TYPE10, 0x08, 0x00, 0x00,
            0x08, 'A', 'B', 'C', 'D', 0x10, 0x03,
        };
        u8 out[8];
        arm9_segment_t lz = {
            (uintptr_t)stream, sizeof(stream), (uintptr_t)out, 8u, true
        };
        memset(out, 0, sizeof(out));
        assert(arm9_load_segments(&lz, 1u) == 1u);
        assert(memcmp(out, "ABCDABCD", 8) == 0);
    }

    /* A malformed compressed segment reports partial progress (0). */
    {
        static const u8 stream[] = {
            NDS_LZ77_TYPE10, 0x04, 0x00, 0x00, 0x80, 0x10, 0x00,
        };
        u8 out[64];
        arm9_segment_t lz = {
            (uintptr_t)stream, sizeof(stream), (uintptr_t)out, 0x04u, true
        };
        assert(arm9_load_segments(&lz, 1u) == 0u);
    }
}

/* ------------------------------------------------------------------ */
/* MPU descriptor decoding                                             */
/* ------------------------------------------------------------------ */

static void test_mpu(void)
{
    /* Reference region 0: base 0, 128 MiB, enabled. */
    assert(arm9_mpu_region_base(0x00000035u) == 0x00000000u);
    assert(arm9_mpu_region_size(0x00000035u) == 0x08000000u);  /* 128 MiB */
    assert(arm9_mpu_region_enabled(0x00000035u));

    /* Reference region 1: Main RAM, 4 MiB. */
    assert(arm9_mpu_region_base(0x0200002Bu) == 0x02000000u);
    assert(arm9_mpu_region_size(0x0200002Bu) == 0x00400000u);  /* 4 MiB */

    /* Region 2 (VRAM): 16 MiB. */
    assert(arm9_mpu_region_size(0x0600002Fu) == 0x01000000u);  /* 16 MiB */

    /* Region 4 (Shared WRAM): 16 KiB. */
    assert(arm9_mpu_region_size(0x0300001Bu) == 0x00004000u);

    /* A disabled region has bit 0 clear. */
    assert(!arm9_mpu_region_enabled(0x0100002Cu));
}

/* ------------------------------------------------------------------ */
/* VRAM bank packing                                                   */
/* ------------------------------------------------------------------ */

static void test_vram(void)
{
    /* The reference boot store: A=0x81, B=0x89, C=0x84, D=0x8A. */
    assert(arm9_vram_bank_word(0x81u, 0x89u, 0x84u, 0x8Au) == 0x8A848981u);
    assert(arm9_vram_bank_word(0u, 0u, 0u, 0u) == 0u);
    assert(arm9_vram_bank_word(0xFFu, 0u, 0u, 0u) == 0x000000FFu);
}

int main(void)
{
    test_lz77();
    test_rle();
    test_segment_loader();
    test_mpu();
    test_vram();

    puts("test_arm9: all assertions passed");
    return 0;
}
