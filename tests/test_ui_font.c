/*
 * tests/test_ui_font.c
 *
 * Host-side unit tests for the graphical bitmap-font / banner-icon renderer
 * (src/arm9/ui_font.c). Only the pure half is exercised: the 4bpp/8bpp tile
 * decoders, the icon de-tiler, the 4bpp packer, the 16x16 glyph expander, the
 * palette reader, the UTF-16 title sanitizer and the bounded 16x16 line
 * renderer. The MMIO entry points are never called.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arm9/ui_font.h"

/* ---- 4bpp / 8bpp tile decoders ---------------------------------------- */

static void test_decode_tile4(void)
{
    u8 tile[UI_FONT_TILE_BYTES];
    u8 px[UI_FONT_TILE_PIXELS];
    u32 i;

    memset(tile, 0, sizeof(tile));
    /* Row 0, first byte: low nibble = pixel 0, high nibble = pixel 1. */
    tile[0] = 0x21u;
    ui_font_decode_tile4(tile, px);
    assert(px[0] == 0x1u);
    assert(px[1] == 0x2u);
    assert(px[2] == 0x0u);

    /* A byte per (row, pixel-pair): byte index = row*4 + col/2. */
    memset(tile, 0, sizeof(tile));
    for (i = 0u; i < UI_FONT_TILE_BYTES; i++) {
        tile[i] = (u8)(i & 0xFFu);
    }
    ui_font_decode_tile4(tile, px);
    assert(px[0] == (tile[0] & 0x0Fu));
    assert(px[1] == (tile[0] >> 4));
    assert(px[8] == (tile[4] & 0x0Fu));   /* row 1, col 0 */
    assert(px[63] == (tile[31] >> 4));    /* row 7, col 7 */
}

static void test_decode_tile8(void)
{
    u8 tile[64];
    u8 px[UI_FONT_TILE_PIXELS];
    u32 i;

    for (i = 0u; i < 64u; i++) {
        tile[i] = (u8)(i * 3u + 1u);
    }
    ui_font_decode_tile8(tile, px);
    for (i = 0u; i < 64u; i++) {
        assert(px[i] == tile[i]);
    }
}

/* ---- 32x32 icon de-tiler ---------------------------------------------- */

static void test_decode_icon4(void)
{
    u8 icon[0x200];
    u8 px[UI_FONT_ICON_PIXELS];
    u32 t;
    u32 py;
    u32 px_x;

    /*
     * Fill tile t with the byte t | (t << 4) so BOTH nibbles decode to t.
     * (A plain byte value would leave the high nibble zero.)
     */
    for (t = 0u; t < UI_FONT_ICON_TILES; t++) {
        u32 i;

        for (i = 0u; i < 32u; i++) {
            icon[t * 32u + i] = (u8)(t | (t << 4));
        }
    }
    ui_font_decode_icon(icon, UI_FONT_BPP_4, px);

    /* Pixel (y, x) belongs to tile (y/8, x/8) = ty*4 + tx. */
    for (py = 0u; py < UI_FONT_ICON_DIM; py++) {
        for (px_x = 0u; px_x < UI_FONT_ICON_DIM; px_x++) {
            u32 expect = (py / 8u) * 4u + (px_x / 8u);
            assert(px[py * UI_FONT_ICON_DIM + px_x] == expect);
        }
    }
}

static void test_decode_icon8(void)
{
    u8 icon[0x400];
    u8 px[UI_FONT_ICON_PIXELS];
    u32 i;

    for (i = 0u; i < sizeof(icon); i++) {
        icon[i] = (u8)(i & 0xFFu);
    }
    ui_font_decode_icon(icon, UI_FONT_BPP_8, px);

    /* Tile 0 covers pixels (0..7, 0..7); 8bpp is a 1:1 copy per tile. */
    for (i = 0u; i < 64u; i++) {
        u32 y = i / 8u;
        u32 x = i % 8u;
        assert(px[y * UI_FONT_ICON_DIM + x] == icon[i]);
    }
    /* Tile 5 (ty=1, tx=1) starts at byte 5*64; its pixel (0,0) is at (8,8). */
    assert(px[8u * UI_FONT_ICON_DIM + 8u] == icon[5u * 64u]);
}

/* ---- 4bpp packer round-trip ------------------------------------------- */

static void test_pack_roundtrip(void)
{
    u8 idx[UI_FONT_TILE_PIXELS];
    u8 tile[UI_FONT_TILE_BYTES];
    u8 back[UI_FONT_TILE_PIXELS];
    u32 i;

    for (i = 0u; i < UI_FONT_TILE_PIXELS; i++) {
        idx[i] = (u8)(i & 0x0Fu);
    }
    ui_font_pack_tile4(idx, tile);
    ui_font_decode_tile4(tile, back);
    for (i = 0u; i < UI_FONT_TILE_PIXELS; i++) {
        assert(back[i] == idx[i]);
    }

    /* The high nibble of every tile byte is pixel 2k+1. */
    assert((tile[0] & 0x0Fu) == idx[0]);
    assert((tile[0] >> 4) == idx[1]);
}

/* ---- 16x16 glyph expansion -------------------------------------------- */

static void test_expand_glyph16(void)
{
    u8 glyph[FONT8X8_GLYPH_BYTES];
    u8 tiles[4][UI_FONT_TILE_BYTES];
    u32 i;
    u32 t;

    /* A single ink pixel at (row 0, col 0) scales to top-left's 2x2 block. */
    memset(glyph, 0, sizeof(glyph));
    glyph[0] = 0x80u;
    ui_font_expand_glyph16(glyph, tiles);
    assert(tiles[0][0] == 0x11u);   /* row 0: pixels 0 and 1 set */
    assert(tiles[0][4] == 0x11u);   /* row 1: pixels 0 and 1 set */
    for (i = 1u; i < sizeof(tiles[0]); i++) {
        if (i != 4u) {
            assert(tiles[0][i] == 0x00u);
        }
    }
    for (t = 1u; t < 4u; t++) {
        for (i = 0u; i < UI_FONT_TILE_BYTES; i++) {
            assert(tiles[t][i] == 0x00u);
        }
    }

    /* A single ink pixel at (row 7, col 7) lands in the bottom-right tile. */
    memset(glyph, 0, sizeof(glyph));
    glyph[7] = 0x01u;
    ui_font_expand_glyph16(glyph, tiles);
    assert(tiles[3][6u * 4u + 3u] == 0x11u);   /* row 6: pixels 6 and 7 */
    assert(tiles[3][7u * 4u + 3u] == 0x11u);   /* row 7: pixels 6 and 7 */
    for (t = 0u; t < 3u; t++) {
        for (i = 0u; i < UI_FONT_TILE_BYTES; i++) {
            assert(tiles[t][i] == 0x00u);
        }
    }
}

/* ---- Palette reader ---------------------------------------------------- */

static void test_read_palette(void)
{
    u8 raw[8];
    u16 out[4];
    u32 n;

    /* 3 little-endian BGR555 entries; the 16th bit must be masked off. */
    raw[0] = 0x1Fu; raw[1] = 0x80u;   /* 0x801F -> 0x001F */
    raw[2] = 0xE0u; raw[3] = 0x7Fu;   /* 0x7FE0 */
    raw[4] = 0x00u; raw[5] = 0x00u;   /* 0x0000 */
    raw[6] = 0xFFu; raw[7] = 0xFFu;

    n = ui_font_read_palette(raw, 4u, out, 4u);
    assert(n == 4u);
    assert(out[0] == 0x001Fu);
    assert(out[1] == 0x7FE0u);
    assert(out[2] == 0x0000u);

    /* Capacity clamping. */
    n = ui_font_read_palette(raw, 8u, out, 2u);
    assert(n == 2u);

    /* NULL safety. */
    assert(ui_font_read_palette(NULL, 4u, out, 4u) == 0u);
    assert(ui_font_read_palette(raw, 4u, NULL, 4u) == 0u);
}

/* ---- UTF-16 title sanitizer ------------------------------------------- */

static void put_utf16(u8 *dst, const unsigned short *s, u32 n)
{
    u32 i;

    for (i = 0u; i < n; i++) {
        dst[i * 2u] = (u8)(s[i] & 0xFFu);
        dst[i * 2u + 1u] = (u8)(s[i] >> 8);
    }
}

static void test_sanitize_utf16(void)
{
    u8 raw[64];
    char out[64];
    const unsigned short simple[] = { 'H', 'i', 0x0000u };
    const unsigned short multi[] = { 'L', '1', 0x000Au, 'L', '2', 0x0000u };
    const unsigned short crlf[] = { 'A', 0x000Du, 0x000Au, 'B', 0x0000u };
    const unsigned short weird[] = { 'A', 0x2603u, 'B', 0x0000u }; /* snowman */
    u32 n;

    memset(raw, 0, sizeof(raw));
    put_utf16(raw, simple, 3u);
    n = ui_font_sanitize_utf16(raw, 16u, out, sizeof(out));
    assert(n == 2u && strcmp(out, "Hi") == 0);

    put_utf16(raw, multi, 6u);
    n = ui_font_sanitize_utf16(raw, 16u, out, sizeof(out));
    assert(n == 7u && strcmp(out, "L1 / L2") == 0);

    put_utf16(raw, crlf, 5u);
    n = ui_font_sanitize_utf16(raw, 16u, out, sizeof(out));
    assert(strcmp(out, "A / B") == 0);

    put_utf16(raw, weird, 4u);
    n = ui_font_sanitize_utf16(raw, 16u, out, sizeof(out));
    assert(strcmp(out, "A?B") == 0);

    /* A leading line break must not emit a dangling separator. */
    {
        const unsigned short lead[] = { 0x000Au, 'X', 0x0000u };
        put_utf16(raw, lead, 3u);
        (void)ui_font_sanitize_utf16(raw, 16u, out, sizeof(out));
        assert(strcmp(out, "X") == 0);
    }

    /* Truncation: capacity 3 holds "Hi" + NUL only. */
    put_utf16(raw, multi, 6u);
    n = ui_font_sanitize_utf16(raw, 16u, out, 3u);
    assert(n <= 2u && out[2] == '\0');

    /* NULL safety. */
    assert(ui_font_sanitize_utf16(NULL, 16u, out, sizeof(out)) == 0u);
    assert(ui_font_sanitize_utf16(raw, 16u, NULL, sizeof(out)) == 0u);
}

/* ---- Bounded 16x16 line renderer -------------------------------------- */

static u16 g_map[CONSOLE_MAP_COLS * 32];

static void test_render_line16_clip(void)
{
    u16 base = (u16)(UI_FONT_BIG_TILE_BASE +
                     (u16)('A' - FONT8X8_FIRST_CHAR) * 4u);
    u32 i;

    memset(g_map, 0, sizeof(g_map));

    /* x = 30 leaves exactly one 2-column cell: only 'A' fits. */
    ui_font_render_line16((vu16 *)g_map, CONSOLE_MAP_COLS, 30, 0, 1u, "ABC",
                          CONSOLE_COLS, CONSOLE_ROWS,
                          UI_FONT_BIG_TILE_BASE);
    assert(g_map[0 * CONSOLE_MAP_COLS + 30] ==
           console_map_entry((u16)(base + 0u), 1u));
    assert(g_map[0 * CONSOLE_MAP_COLS + 31] ==
           console_map_entry((u16)(base + 1u), 1u));
    assert(g_map[1 * CONSOLE_MAP_COLS + 30] ==
           console_map_entry((u16)(base + 2u), 1u));
    assert(g_map[1 * CONSOLE_MAP_COLS + 31] ==
           console_map_entry((u16)(base + 3u), 1u));
    /* 'B' would start at column 32: nothing outside the visible area. */
    for (i = 0u; i < CONSOLE_MAP_COLS * 32u; i++) {
        if (i != 0u * CONSOLE_MAP_COLS + 30u && i != 0u * CONSOLE_MAP_COLS + 31u &&
            i != 1u * CONSOLE_MAP_COLS + 30u && i != 1u * CONSOLE_MAP_COLS + 31u) {
            assert(g_map[i] == 0u);
        }
    }
}

static void test_render_line16_last_row(void)
{
    u16 base = (u16)(UI_FONT_BIG_TILE_BASE +
                     (u16)('A' - FONT8X8_FIRST_CHAR) * 4u);
    int y = (int)CONSOLE_ROWS - 1;

    memset(g_map, 0, sizeof(g_map));
    ui_font_render_line16((vu16 *)g_map, CONSOLE_MAP_COLS, 0, y, 0u, "A",
                          CONSOLE_COLS, CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
    assert(g_map[(u32)y * CONSOLE_MAP_COLS + 0u] ==
           console_map_entry((u16)(base + 0u), 0u));
    assert(g_map[(u32)y * CONSOLE_MAP_COLS + 1u] ==
           console_map_entry((u16)(base + 1u), 0u));
    /* The bottom row is the last visible one: no second row is written. */
    assert(g_map[(u32)(y + 1) * CONSOLE_MAP_COLS + 0u] == 0u);
}

static void test_render_line16_offscreen(void)
{
    /* Negative x: a single off-screen character writes nothing. */
    memset(g_map, 0, sizeof(g_map));
    ui_font_render_line16((vu16 *)g_map, CONSOLE_MAP_COLS, -2, 0, 0u, "A",
                          CONSOLE_COLS, CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
    for (u32 i = 0u; i < CONSOLE_MAP_COLS * 32u; i++) {
        assert(g_map[i] == 0u);
    }

    /* y outside the visible rows: nothing is written. */
    ui_font_render_line16((vu16 *)g_map, CONSOLE_MAP_COLS, 0,
                          (int)CONSOLE_ROWS, 0u, "A", CONSOLE_COLS,
                          CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
    for (u32 i = 0u; i < CONSOLE_MAP_COLS * 32u; i++) {
        assert(g_map[i] == 0u);
    }

    /* NULL safety. */
    ui_font_render_line16(NULL, CONSOLE_MAP_COLS, 0, 0, 0u, "A",
                          CONSOLE_COLS, CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
    ui_font_render_line16((vu16 *)g_map, CONSOLE_MAP_COLS, 0, 0, 0u, NULL,
                          CONSOLE_COLS, CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
}

int main(void)
{
    test_decode_tile4();
    test_decode_tile8();
    test_decode_icon4();
    test_decode_icon8();
    test_pack_roundtrip();
    test_expand_glyph16();
    test_read_palette();
    test_sanitize_utf16();
    test_render_line16_clip();
    test_render_line16_last_row();
    test_render_line16_offscreen();

    puts("test_ui_font: all assertions passed");
    return 0;
}
