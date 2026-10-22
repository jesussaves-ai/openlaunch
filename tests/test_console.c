/*
 * tests/test_console.c
 *
 * Host-side unit tests for the console tile/glyph packer, the map-entry
 * packer, the bounded formatters and the font table bounds.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "arm9/console.h"
#include "arm9/font8x8.h"

/* ------------------------------------------------------------------ */
/* 1bpp -> 4bpp glyph packing                                          */
/* ------------------------------------------------------------------ */

static void test_glyph_pack(void)
{
    u8 glyph[8];
    u8 tile[CONSOLE_TILE_BYTES];

    memset(glyph, 0, sizeof(glyph));
    memset(tile, 0xEE, sizeof(tile));

    /* Pixel (col 0, row 0): bit 7 -> low nibble of tile byte 0. */
    glyph[0] = 0x80u;
    console_glyph_to_tile(glyph, tile);
    assert(tile[0] == 0x01u);
    assert(tile[1] == 0x00u);

    /* Pixel (col 1, row 0): bit 6 -> high nibble of tile byte 0. */
    glyph[0] = 0x40u;
    console_glyph_to_tile(glyph, tile);
    assert(tile[0] == 0x10u);

    /* Full row 0 -> four 0x11 bytes. */
    glyph[0] = 0xFFu;
    console_glyph_to_tile(glyph, tile);
    assert(tile[0] == 0x11u && tile[1] == 0x11u);
    assert(tile[2] == 0x11u && tile[3] == 0x11u);
    assert(tile[4] == 0x00u);

    /* Row 7 maps to tile bytes 28..31. */
    memset(glyph, 0, sizeof(glyph));
    glyph[7] = 0x80u;                 /* col 0 -> tile[28] low nibble */
    console_glyph_to_tile(glyph, tile);
    assert(tile[28] == 0x01u);

    glyph[7] = 0x01u;                 /* col 7 -> tile[31] high nibble */
    console_glyph_to_tile(glyph, tile);
    assert(tile[31] == 0x10u);
    assert(tile[28] == 0x00u);
}

/* Every glyph must contain only pixel values 0/1 in each nibble. */
static void test_font_pixels(void)
{
    u32 g;

    for (g = 0u; g < FONT8X8_GLYPH_COUNT; g++) {
        u8 tile[CONSOLE_TILE_BYTES];
        u32 i;

        console_glyph_to_tile(font8x8_basic[g], tile);
        for (i = 0u; i < CONSOLE_TILE_BYTES; i++) {
            assert((tile[i] & 0x0Fu) <= 1u);
            assert((tile[i] & 0xF0u) <= 0x10u);
        }
    }

    /* The eighth scanline of every glyph is blank. */
    for (g = 0u; g < FONT8X8_GLYPH_COUNT; g++) {
        assert(font8x8_basic[g][7] == 0u);
    }
}

static void test_font_bounds(void)
{
    assert(FONT8X8_GLYPH_COUNT == 96u);
    assert(font8x8_glyph('A') == font8x8_basic['A' - FONT8X8_FIRST_CHAR]);
    /* Out-of-range characters fall back to '?'. */
    assert(font8x8_glyph(0x00u) == font8x8_basic['?' - FONT8X8_FIRST_CHAR]);
    assert(font8x8_glyph(0xFFu) == font8x8_basic['?' - FONT8X8_FIRST_CHAR]);
}

/* ------------------------------------------------------------------ */
/* BG map entry packing                                                */
/* ------------------------------------------------------------------ */

static void test_map_entry(void)
{
    assert(console_map_entry(0u, 0u) == 0x0000u);
    assert(console_map_entry(5u, 3u) == (u16)(5u | (3u << 12)));
    assert(console_map_entry(0x3FFu, 0xFu) == 0xF3FFu);
}

/* ------------------------------------------------------------------ */
/* Bounded formatters                                                  */
/* ------------------------------------------------------------------ */

static void test_format_hex(void)
{
    char buf[32];

    assert(console_format_hex32(buf, sizeof(buf), 0xDEADBEEFu) == 8);
    assert(strcmp(buf, "DEADBEEF") == 0);

    assert(console_format_hex32(buf, sizeof(buf), 0x00000000u) == 8);
    assert(strcmp(buf, "00000000") == 0);
}

static void test_format(void)
{
    char buf[64];

    assert(console_format(buf, sizeof(buf), "%d", -1234) == 5);
    assert(strcmp(buf, "-1234") == 0);

    assert(console_format(buf, sizeof(buf), "%u", 4294967295u) == 10);
    assert(strcmp(buf, "4294967295") == 0);

    assert(console_format(buf, sizeof(buf), "%08X", 0x1234u) == 8);
    assert(strcmp(buf, "00001234") == 0);

    assert(console_format(buf, sizeof(buf), "%-5d|", 7) == 6);
    assert(strcmp(buf, "7    |") == 0);

    assert(console_format(buf, sizeof(buf), "%s %c", "hi", '!') == 4);
    assert(strcmp(buf, "hi !") == 0);

    assert(console_format(buf, sizeof(buf), "100%%") == 4);
    assert(strcmp(buf, "100%") == 0);

    /* Unknown conversion is emitted literally. */
    assert(console_format(buf, sizeof(buf), "%q") == 2);
    assert(strcmp(buf, "%q") == 0);

    /* NULL string is substituted. */
    assert(console_format(buf, sizeof(buf), "%s", (const char *)NULL) == 6);
    assert(strcmp(buf, "(null)") == 0);
}

static void test_format_truncation(void)
{
    char small[5];
    int n;

    n = console_format(small, sizeof(small), "%s", "abcdefgh");
    assert(n == 8);                 /* full logical length reported */
    assert(strlen(small) == 4);     /* but only 4 fit plus the NUL   */
    assert(strcmp(small, "abcd") == 0);

    /* A one-byte buffer holds only the terminator. */
    {
        char one[1];
        n = console_format(one, sizeof(one), "xyz");
        assert(n == 3);
        assert(one[0] == '\0');
    }
}

int main(void)
{
    test_glyph_pack();
    test_font_pixels();
    test_font_bounds();
    test_map_entry();
    test_format_hex();
    test_format();
    test_format_truncation();

    puts("test_console: all assertions passed");
    return 0;
}
