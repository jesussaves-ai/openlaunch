/*
 * src/arm9/ui_font.c
 *
 * Graphical bitmap-font / NDS banner-icon renderer. See include/arm9/ui_font.h.
 *
 * The pure half (decoders, packers, the bounded 16x16 line renderer) is built
 * for both the host tests and the target; the MMIO half is compiled only for an
 * ARM target so tests/test_ui_font.c can link it without touching hardware.
 *
 * Tile data is written with 16-bit stores because VRAM on the DS only accepts
 * 16/32-bit accesses (mirroring src/arm9/console.c).
 */
#include "arm9/ui_font.h"
#include "arm9/video.h"
#include "common/mem.h"

/* Read one little-endian 16-bit word (UTF-16 code unit / BGR555 colour). */
static u16 ui_le16(const u8 *p)
{
    return dsi_read_le16(p);
}

/* ---- Tile decoders ---------------------------------------------------- */

void ui_font_decode_tile4(const u8 *tile, u8 out[UI_FONT_TILE_PIXELS])
{
    u32 row;
    u32 col;

    if (tile == NULL || out == NULL) {
        return;
    }
    for (row = 0u; row < 8u; row++) {
        for (col = 0u; col < 8u; col++) {
            u8 byte = tile[row * 4u + (col >> 1)];
            out[row * 8u + col] =
                (u8)(((col & 1u) != 0u) ? (byte >> 4) : (byte & 0x0Fu));
        }
    }
}

void ui_font_decode_tile8(const u8 *tile, u8 out[UI_FONT_TILE_PIXELS])
{
    u32 i;

    if (tile == NULL || out == NULL) {
        return;
    }
    for (i = 0u; i < UI_FONT_TILE_PIXELS; i++) {
        out[i] = tile[i];
    }
}

void ui_font_decode_icon(const u8 *icon, u32 bpp,
                         u8 out[UI_FONT_ICON_PIXELS])
{
    u32 i;
    u32 ty;
    u32 tx;

    if (icon == NULL || out == NULL) {
        return;
    }
    for (i = 0u; i < UI_FONT_ICON_PIXELS; i++) {
        out[i] = 0u;
    }

    for (ty = 0u; ty < 4u; ty++) {
        for (tx = 0u; tx < 4u; tx++) {
            u8 tile_idx[UI_FONT_TILE_PIXELS];
            u32 t = ty * 4u + tx;
            u32 py;
            u32 px;

            if (bpp == UI_FONT_BPP_8) {
                ui_font_decode_tile8(icon + t * 64u, tile_idx);
            } else {
                ui_font_decode_tile4(icon + t * 32u, tile_idx);
            }

            for (py = 0u; py < 8u; py++) {
                for (px = 0u; px < 8u; px++) {
                    out[(ty * 8u + py) * UI_FONT_ICON_DIM + (tx * 8u + px)] =
                        tile_idx[py * 8u + px];
                }
            }
        }
    }
}

/* ---- Packers ---------------------------------------------------------- */

void ui_font_pack_tile4(const u8 idx[UI_FONT_TILE_PIXELS],
                        u8 tile[UI_FONT_TILE_BYTES])
{
    u32 row;
    u32 col;

    if (idx == NULL || tile == NULL) {
        return;
    }
    for (row = 0u; row < 8u; row++) {
        for (col = 0u; col < 8u; col += 2u) {
            u8 lo = (u8)(idx[row * 8u + col] & 0x0Fu);
            u8 hi = (u8)(idx[row * 8u + col + 1u] & 0x0Fu);
            tile[row * 4u + (col >> 1)] = (u8)(lo | (u8)(hi << 4));
        }
    }
}

void ui_font_expand_glyph16(const u8 *glyph,
                            u8 tiles[4][UI_FONT_TILE_BYTES])
{
    u32 t;

    if (glyph == NULL || tiles == NULL) {
        return;
    }

    for (t = 0u; t < 4u; t++) {
        u32 tile_row = t >> 1;    /* 0 = top, 1 = bottom */
        u32 tile_col = t & 1u;    /* 0 = left, 1 = right */
        u8 idx[UI_FONT_TILE_PIXELS];
        u32 r;
        u32 c;

        for (r = 0u; r < 8u; r++) {
            for (c = 0u; c < 8u; c++) {
                u32 srow = (tile_row * 8u + r) >> 1;
                u32 scol = (tile_col * 8u + c) >> 1;
                u8 bit = (u8)((glyph[srow] >> (7u - scol)) & 1u);

                idx[r * 8u + c] = (bit != 0u) ? 1u : 0u;
            }
        }
        ui_font_pack_tile4(idx, tiles[t]);
    }
}

u32 ui_font_read_palette(const u8 *src, u32 entries, u16 *out, u32 out_cap)
{
    u32 i;
    u32 n;

    if (src == NULL || out == NULL) {
        return 0u;
    }
    n = (entries < out_cap) ? entries : out_cap;
    for (i = 0u; i < n; i++) {
        out[i] = (u16)(ui_le16(src + i * 2u) & 0x7FFFu);
    }
    return n;
}

/* ---- UTF-16 title sanitizer ------------------------------------------- */

u32 ui_font_sanitize_utf16(const u8 *utf16, u32 code_units, char *dst, u32 cap)
{
    u32 i;
    u32 o = 0u;
    bool pending_sep = false;

    if (dst == NULL || cap == 0u) {
        return 0u;
    }

    for (i = 0u; i < code_units; i++) {
        u16 ch;

        if (utf16 == NULL) {
            break;
        }
        ch = ui_le16(utf16 + i * 2u);
        if (ch == 0x0000u) {
            break;
        }
        if (ch == 0x000Au || ch == 0x000Du) {
            /* Collapse a CRLF pair into one line break. */
            if (ch == 0x000Du && (i + 1u) < code_units &&
                ui_le16(utf16 + (i + 1u) * 2u) == 0x000Au) {
                i++;
            }
            if (o > 0u) {
                pending_sep = true;
            }
            continue;
        }

        if (pending_sep) {
            /* Need three separator bytes plus room for this character. */
            if (o + 4u > cap) {
                break;
            }
            dst[o++] = ' ';
            dst[o++] = '/';
            dst[o++] = ' ';
            pending_sep = false;
        }
        if (o + 1u >= cap) {
            break;
        }
        dst[o++] = (ch >= 0x0020u && ch <= 0x007Eu) ? (char)ch : '?';
    }

    dst[o] = '\0';
    while (o > 0u && dst[o - 1u] == ' ') {
        dst[--o] = '\0';
    }
    return o;
}

/* ---- Bounded 16x16 line renderer (pure) ------------------------------- */

void ui_font_render_line16(vu16 *map, u32 map_cols, int x, int y, u8 pal,
                           const char *str, u32 cols, u32 rows,
                           u16 tile_base)
{
    u32 i;

    if (map == NULL || map_cols == 0u || str == NULL) {
        return;
    }
    if (y < 0 || (u32)y >= rows) {
        return;
    }

    for (i = 0u; str[i] != '\0'; i++) {
        int cx = x + (int)(i * 2u);
        int cy = y;
        u8 c = (u8)str[i];
        u16 tile;

        if (c < FONT8X8_FIRST_CHAR || c > FONT8X8_LAST_CHAR) {
            c = (u8)'?';
        }
        if (cx < 0) {
            continue;
        }
        if ((u32)cx >= cols) {
            break;
        }

        tile = (u16)(tile_base + (u16)(c - FONT8X8_FIRST_CHAR) * 4u);

        if ((u32)cx < map_cols) {
            map[(u32)cy * map_cols + (u32)cx] = console_map_entry(tile + 0u, pal);
        }
        if ((u32)(cx + 1) < cols && (u32)(cx + 1) < map_cols) {
            map[(u32)cy * map_cols + (u32)(cx + 1)] =
                console_map_entry(tile + 1u, pal);
        }
        if ((u32)(cy + 1) < rows) {
            if ((u32)cx < map_cols) {
                map[(u32)(cy + 1) * map_cols + (u32)cx] =
                    console_map_entry(tile + 2u, pal);
            }
            if ((u32)(cx + 1) < cols && (u32)(cx + 1) < map_cols) {
                map[(u32)(cy + 1) * map_cols + (u32)(cx + 1)] =
                    console_map_entry(tile + 3u, pal);
            }
        }
    }
}

/* ---- Hardware renderer ------------------------------------------------ */

/* Engine-A BG tile VRAM base (bank A mapped to 0x06000000). */
#define UI_FONT_TILE_VRAM  0x06000000u

static void ui_font_store_tile(u16 tile_index, const u8 tile[UI_FONT_TILE_BYTES])
{
    u32 base = UI_FONT_TILE_VRAM + (u32)tile_index * UI_FONT_TILE_BYTES;
    u32 i;

    for (i = 0u; i < UI_FONT_TILE_BYTES; i += 2u) {
        REG16(base + i) = (u16)((u16)tile[i] | ((u16)tile[i + 1u] << 8));
    }
}

static void ui_font_load_palette_bank(const u16 pal[16], u32 count, u8 bank)
{
    u32 i;

    for (i = 0u; i < 16u; i++) {
        u16 c = (i < count) ? pal[i] : 0x0000u;
        REG16(PALETTE_MAIN + ((u32)bank * 16u + i) * 2u) = c;
    }
}

void ui_font_init(void)
{
    u32 g;
    u32 i;

    for (g = 0u; g < FONT8X8_GLYPH_COUNT; g++) {
        u8 tiles[4][UI_FONT_TILE_BYTES];

        ui_font_expand_glyph16(font8x8_basic[g], tiles);
        for (i = 0u; i < 4u; i++) {
            ui_font_store_tile((u16)(UI_FONT_BIG_TILE_BASE + g * 4u + i),
                               tiles[i]);
        }
    }
}

void ui_font_draw_icon(int x, int y, const u8 *banner, u32 bpp)
{
    u8 idx[UI_FONT_ICON_PIXELS];
    u16 pal[UI_FONT_BANNER_PAL_ENTRIES];
    vu16 *map;
    u32 ty;

    if (banner == NULL) {
        return;
    }

    ui_font_decode_icon(banner + UI_FONT_BANNER_ICON_OFF, bpp, idx);
    ui_font_read_palette(banner + UI_FONT_BANNER_PALETTE_OFF,
                         UI_FONT_BANNER_PAL_ENTRIES, pal,
                         UI_FONT_BANNER_PAL_ENTRIES);
    /* Force a black icon background to match the console's black backdrop. */
    pal[0] = 0x0000u;
    ui_font_load_palette_bank(pal, UI_FONT_BANNER_PAL_ENTRIES,
                              UI_FONT_ICON_PALETTE);

    for (ty = 0u; ty < 4u; ty++) {
        u32 tx;

        for (tx = 0u; tx < 4u; tx++) {
            u8 sub[UI_FONT_TILE_PIXELS];
            u8 tile[UI_FONT_TILE_BYTES];
            u32 r;
            u32 c;

            for (r = 0u; r < 8u; r++) {
                for (c = 0u; c < 8u; c++) {
                    sub[r * 8u + c] =
                        (u8)(idx[(ty * 8u + r) * UI_FONT_ICON_DIM +
                                 (tx * 8u + c)] & 0x0Fu);
                }
            }
            ui_font_pack_tile4(sub, tile);
            ui_font_store_tile((u16)(UI_FONT_ICON_TILE_BASE + ty * 4u + tx),
                               tile);
        }
    }

    map = console_map();
    if (map == NULL) {
        return;
    }
    for (ty = 0u; ty < 4u; ty++) {
        u32 tx;

        for (tx = 0u; tx < 4u; tx++) {
            int cx = x + (int)tx;
            int cy = y + (int)ty;

            if (cx >= 0 && cy >= 0 && cx < CONSOLE_COLS && cy < CONSOLE_ROWS) {
                map[(u32)cy * CONSOLE_MAP_COLS + (u32)cx] =
                    console_map_entry((u16)(UI_FONT_ICON_TILE_BASE +
                                            ty * 4u + tx),
                                      UI_FONT_ICON_PALETTE);
            }
        }
    }
}

void ui_font_draw_title16(int x, int y, u8 pal, const char *str)
{
    vu16 *map = console_map();

    if (map == NULL) {
        return;
    }
    ui_font_render_line16(map, CONSOLE_MAP_COLS, x, y, pal, str,
                          CONSOLE_COLS, CONSOLE_ROWS, UI_FONT_BIG_TILE_BASE);
}

void ui_font_clear(int x, int y, int w, int h)
{
    vu16 *map = console_map();
    int cx;
    int cy;

    if (map == NULL) {
        return;
    }
    for (cy = 0; cy < h; cy++) {
        for (cx = 0; cx < w; cx++) {
            int px = x + cx;
            int py = y + cy;

            if (px >= 0 && py >= 0 && px < CONSOLE_COLS && py < CONSOLE_ROWS) {
                map[(u32)py * CONSOLE_MAP_COLS + (u32)px] =
                    console_map_entry(0u, 0u);
            }
        }
    }
}
