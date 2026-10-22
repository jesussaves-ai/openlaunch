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

    /* Glyphs occupy columns 1..5 only (bits 6..2), so their ink can never
     * reach the 8-pixel cell edge and visually merge with a neighbour: bits
     * 7 (col 0), 1 (col 6) and 0 (col 7) must stay clear. */
    for (g = 0u; g < FONT8X8_GLYPH_COUNT; g++) {
        u32 row;
        for (row = 0u; row < FONT8X8_GLYPH_BYTES; row++) {
            u8 bits = font8x8_basic[g][row];
            assert((bits & 0x81u) == 0u);   /* columns 0 and 7 blank */
            assert((bits & 0x02u) == 0u);   /* column 6 blank       */
        }
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
/* Menu-line spacing / character-overlap regression                    */
/* ------------------------------------------------------------------ */

/*
 * Render `line` at row `row` into a zeroed BG map using the same path as
 * console_puts(). This is the fixed-width contract the menu relies on.
 */
static void render_row(u16 *map, const char *line, int row)
{
    u32 i;

    for (i = 0u; i < (u32)(CONSOLE_MAP_COLS * CONSOLE_ROWS); i++) {
        map[i] = 0u;
    }
    console_render_line((vu16 *)map, CONSOLE_MAP_COLS, 0, row,
                        CONSOLE_PAL_WHITE, line, CONSOLE_COLS, CONSOLE_ROWS);
}

/*
 * Every menu string must occupy one tile per character with no shared /
 * skipped cells, and a redraw must fully replace the previous line (so a
 * moving cursor cannot leave stale characters behind).
 */
static void test_menu_line_spacing(void)
{
    static const char *const labels[6] = {
        "[1] Re-run IPC & HW Test",
        "[2] Inspect SCFG / MBK",
        "[3] Probe SD / TMIO RO",
        "[4] LZ77 Decompress Verify",
        "[5] Reboot / Halt System",
        "[6] Browse SD Card (.nds)",
    };
    u16 map[CONSOLE_MAP_COLS * CONSOLE_ROWS];
    u32 i;

    for (i = 0u; i < 6u; i++) {
        char line[CONSOLE_COLS + 1];
        u32 n;
        u32 col;

        n = (u32)console_format(line, sizeof(line), "%c%s",
                                (i == 1u) ? '>' : ' ', labels[i]);
        assert(n < CONSOLE_COLS);

        render_row(map, line, 2);

        for (col = 0u; col < n; col++) {
            u16 e = map[2u * CONSOLE_MAP_COLS + col];
            u8 c = (u8)line[col];
            u8 tile = (c >= FONT8X8_FIRST_CHAR && c <= FONT8X8_LAST_CHAR)
                          ? (u8)(c - FONT8X8_FIRST_CHAR)
                          : (u8)('?' - FONT8X8_FIRST_CHAR);

            assert((e & 0x03FFu) == tile);
            assert((e & 0xF000u) == (u16)(CONSOLE_PAL_WHITE << 12));
        }
        /* Nothing is written past the end of the string. */
        assert(map[2u * CONSOLE_MAP_COLS + n] == 0u);
    }

    /* Redrawing a shorter line must clear the tail of the longer one. */
    render_row(map, ">[2] Inspect SCFG / MBK", 2);
    render_row(map, " [3] Probe SD", 2);
    assert(map[2u * CONSOLE_MAP_COLS + 12u] != 0u);
    assert(map[2u * CONSOLE_MAP_COLS + 13u] == 0u);
    assert(map[2u * CONSOLE_MAP_COLS + 22u] == 0u);

    /* Out-of-range rows are dropped, never wrapped to the next row. */
    render_row(map, "SHOULD NOT WRAP", (int)CONSOLE_ROWS);
    for (i = 0u; i < (u32)(CONSOLE_MAP_COLS * CONSOLE_ROWS); i++) {
        assert(map[i] == 0u);
    }

    /* Overlong strings clip at the visible width instead of wrapping. */
    render_row(map, "1234567890123456789012345678901234567890", 3);
    assert(map[3u * CONSOLE_MAP_COLS + (CONSOLE_COLS - 1u)] != 0u);
    assert(map[4u * CONSOLE_MAP_COLS + 0u] == 0u);
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
    test_menu_line_spacing();
    test_format_hex();
    test_format();
    test_format_truncation();

    puts("test_console: all assertions passed");
    return 0;
}
