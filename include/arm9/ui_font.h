/*
 * include/arm9/ui_font.h
 *
 * Graphical bitmap-font and NDS banner-icon renderer for Engine A (top screen).
 *
 * The renderer augments the text console (src/arm9/console.c) with:
 *   - a runtime-generated 16x16 double-size font (each 8x8 glyph is scaled 2x
 *     into four 4bpp tiles), used for the top-screen title banner; and
 *   - a 32x32 NDS banner icon decoder for both 4bpp (16-colour) and 8bpp
 *     (256-colour) tile data, used to show the selected .nds thumbnail.
 *
 * It is zero-allocation: every buffer is either a fixed-size local or a slice
 * of the reserved Engine-A BG0 tile space. The pure decoders/packers/renderers
 * are host-testable (tests/test_ui_font.c); the MMIO-touching entry points are
 * only compiled for the ARM targets.
 *
 * Layout (Engine A BG0, bank A at 0x06000000):
 *   tiles   0   .. 95    the 8x8 console font (installed by console_init())
 *   tiles   96  .. 479   the 16x16 title font (4 tiles per 96 ASCII glyphs)
 *   tiles   480 .. 495   the 32x32 banner icon (4x4 tiles)
 *   map     0x06008000   the 32x32 text map (screen base block 16)
 *
 * The AMOLED-friendly look is a pure black (#000000) backdrop with crisp
 * white/cyan glyphs; icon palette entry 0 is forced black for the same reason.
 */
#ifndef DSI_ARM9_UI_FONT_H
#define DSI_ARM9_UI_FONT_H

#include "../common/types.h"
#include "../common/nds_header.h"
#include "console.h"
#include "font8x8.h"

/* ---- Geometry --------------------------------------------------------- */
#define UI_FONT_ICON_DIM       32u   /* banner icon is 32x32 pixels          */
#define UI_FONT_ICON_TILES     16u   /* ... stored as 4x4 8x8 tiles          */
#define UI_FONT_TILE_BYTES     32u   /* one 8x8 4bpp tile                    */
#define UI_FONT_TILE_PIXELS    64u   /* one 8x8 tile = 64 pixels             */
#define UI_FONT_ICON_PIXELS    (UI_FONT_ICON_DIM * UI_FONT_ICON_DIM)

/* Accepted icon colour depths. */
#define UI_FONT_BPP_4          4u
#define UI_FONT_BPP_8          8u

/* ---- NDS banner sub-block offsets (relative to the 0x840 banner block) - */
#define UI_FONT_BANNER_ICON_OFF     0x020u  /* 32x32 bitmap (0x200 B at 4bpp) */
#define UI_FONT_BANNER_PALETTE_OFF  0x220u  /* 16 x BGR555 palette entries    */
#define UI_FONT_BANNER_PAL_ENTRIES  16u

/*
 * Tile-map geometry. The 32x32 icon is written as 16 map cells (4x4); the
 * 16x16 font advances two cells per character, so a row holds 16 characters.
 */
#define UI_FONT_TITLE_CELLS     (CONSOLE_COLS / 2)
#define UI_FONT_ICON_COLS       4
#define UI_FONT_ICON_ROWS       4

/* Reserved Engine-A BG0 tile slots (see the layout note above). */
#define UI_FONT_BIG_TILE_BASE   (FONT8X8_GLYPH_COUNT)
#define UI_FONT_ICON_TILE_BASE  (UI_FONT_BIG_TILE_BASE + FONT8X8_GLYPH_COUNT * 4u)

/* Palette bank dedicated to the banner icon (bank 7 is unused by the console). */
#define UI_FONT_ICON_PALETTE    7u

/* ---- Pure decoders / packers (host-testable) -------------------------- */

/*
 * Decode one 8x8 tile into 64 palette indices (row-major, 0 = leftmost).
 * `tile` is 32 bytes for 4bpp (two pixels per byte, low nibble first) and
 * 64 bytes for 8bpp.
 */
void ui_font_decode_tile4(const u8 *tile, u8 out[UI_FONT_TILE_PIXELS]);
void ui_font_decode_tile8(const u8 *tile, u8 out[UI_FONT_TILE_PIXELS]);

/*
 * Decode the full 32x32 icon (4x4 tiles, row-major) into 1024 palette indices.
 * `icon` must hold 0x200 bytes for 4bpp or 0x400 bytes for 8bpp; `bpp` selects
 * the layout. `out` is fully written (unused pixels are zeroed).
 */
void ui_font_decode_icon(const u8 *icon, u32 bpp,
                         u8 out[UI_FONT_ICON_PIXELS]);

/* Pack 64 4bpp palette indices into a 32-byte tile (two pixels per byte). */
void ui_font_pack_tile4(const u8 idx[UI_FONT_TILE_PIXELS],
                        u8 tile[UI_FONT_TILE_BYTES]);

/*
 * Expand one 8x8 1bpp glyph (MSB = leftmost, as in font8x8.h) into a 16x16
 * 4bpp glyph stored as four 8x8 tiles: [0]=top-left, [1]=top-right,
 * [2]=bottom-left, [3]=bottom-right. Ink pixels become nibble 1.
 */
void ui_font_expand_glyph16(const u8 *glyph,
                            u8 tiles[4][UI_FONT_TILE_BYTES]);

/*
 * Read `entries` little-endian BGR555 words from `src` into `out` (capacity
 * `out_cap`), masking to the 15-bit colour range. Returns the number written.
 */
u32 ui_font_read_palette(const u8 *src, u32 entries, u16 *out, u32 out_cap);

/*
 * Sanitize a UTF-16LE English banner title into single-line printable ASCII:
 * line breaks become " / ", printable ASCII is copied, everything else becomes
 * '?', the result is NUL-terminated and trailing spaces are trimmed. `utf16`
 * is a byte pointer. Returns the number of characters written (excluding NUL).
 */
u32 ui_font_sanitize_utf16(const u8 *utf16, u32 code_units, char *dst, u32 cap);

/*
 * Render `str` as the double-size (16x16) font into a BG map. Each character
 * covers a 2x2 cell block, so a string of N characters needs 2N columns. The
 * write is clipped to `cols` visible columns and `rows` visible rows, and to
 * the physical `map_cols` stride; tiles are taken consecutively from
 * `tile_base` (four per glyph). Pure, so the clipping is host-testable.
 */
void ui_font_render_line16(vu16 *map, u32 map_cols, int x, int y, u8 pal,
                           const char *str, u32 cols, u32 rows,
                           u16 tile_base);

/* ---- Hardware entry points -------------------------------------------- */
/*
 * These touch the Engine-A BG VRAM and palette and are only ever called from
 * the hardware menu. They compile on the host too (the MMIO macros expand to
 * ordinary lvalues) so the host test suites can link menu.c unchanged, exactly
 * like src/arm9/console.c.
 */

/*
 * Generate the 16x16 title font into the reserved tile slots. Call once after
 * console_init().
 */
void ui_font_init(void);

/* Decode `banner`'s icon, install it and draw it at map cell (x, y). */
void ui_font_draw_icon(int x, int y, const u8 *banner, u32 bpp);

/* Draw a double-size title string at map cell (x, y) with palette bank `pal`. */
void ui_font_draw_title16(int x, int y, u8 pal, const char *str);

/* Blank a `w` x `h` cell block (clipped to the visible screen). */
void ui_font_clear(int x, int y, int w, int h);

#endif /* DSI_ARM9_UI_FONT_H */
