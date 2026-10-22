/*
 * include/arm9/console.h
 *
 * Dual-screen tile/text-mode text console for the bootloader.
 *
 * Both 2D engines are configured in BG mode 0 with BG0 as a 256x192 text
 * layer (32x24 visible 8x8 cells). The 8x8 1bpp font from src/arm9/font8x8.c
 * is expanded to 4bpp tiles and installed into BG VRAM:
 *
 *   Engine A (top screen)     tiles 0x06000000, map 0x06008000, palette 0x05000000
 *   Engine B (bottom screen)  tiles 0x06200000, map 0x06201000, palette 0x05000400
 *
 * Engine A's map sits at screen base block 16 so tiles 96..495 stay free for
 * the graphical overlay (see include/arm9/ui_font.h).
 *
 * A palette bank is selected per string via console_puts(); bank 0 is white on
 * the black backdrop, then cyan, green, yellow, red, grey and blue.
 *
 * The tile packer, map-entry packer and the bounded formatters are pure and
 * exercised by tests/test_console.c on the host.
 */
#ifndef DSI_ARM9_CONSOLE_H
#define DSI_ARM9_CONSOLE_H

#include "../common/types.h"
#include "../arm9/font8x8.h"

/* Visible grid (256x192 px) and the full 32x32 BG map. */
#define CONSOLE_COLS       32
#define CONSOLE_ROWS       24
#define CONSOLE_MAP_COLS   32
#define CONSOLE_MAP_ROWS   32

/* One 8x8 4bpp tile is 32 bytes. */
#define CONSOLE_TILE_BYTES 32

/* Palette banks installed by console_init(). */
#define CONSOLE_PAL_WHITE   0u
#define CONSOLE_PAL_CYAN    1u
#define CONSOLE_PAL_GREEN   2u
#define CONSOLE_PAL_YELLOW  3u
#define CONSOLE_PAL_RED     4u
#define CONSOLE_PAL_GREY    5u
#define CONSOLE_PAL_BLUE    6u
#define CONSOLE_PAL_COUNT   8u

/* Which screen subsequent console_* calls target. */
typedef enum {
    CONSOLE_MAIN = 0,   /* 2D engine A, top screen    */
    CONSOLE_SUB  = 1,   /* 2D engine B, bottom screen */
    CONSOLE_COUNT
} console_id_t;

/* ---- Hardware bring-up ------------------------------------------------ */

/* Map VRAM banks, expand the font and enable both text layers. */
void console_init(void);

/* Select which screen the cursorless puts/printf helpers draw to. */
void console_select(console_id_t id);

/* Clear the active screen only (keeps the other screen intact). */
void console_clear(void);

/* Clear both screens. */
void console_clear_all(void);

/* Draw a NUL-terminated string at cell (x, y) using palette bank `pal`. */
void console_puts(int x, int y, u8 pal, const char *str);

/*
 * The active engine's 32x32 BG map as a `vu16` array (row-major, 32 entries
 * per row). Used by the graphical overlay renderer (src/arm9/ui_font.c) so it
 * can address the same text layer as the console.
 */
vu16 *console_map(void);

/*
 * Pure text-map renderer backing console_puts(). Writes `str` into a
 * caller-provided BG map (`map_cols` entries per row) starting at cell
 * (x, y). Each character advances exactly one cell -- the console is
 * fixed-width, so cell (col, row) is `map[row * map_cols + col]` and no
 * two characters can ever share a tile. The string is clipped to `cols`
 * visible columns and rows outside [0, rows) are ignored.
 *
 * `map` is `vu16` so the hardware path keeps the volatile MMIO store
 * semantics required by AGENTS.md while tests can pass a plain buffer.
 */
void console_render_line(vu16 *map, u32 map_cols, int x, int y, u8 pal,
                         const char *str, u32 cols, u32 rows);

/* ---- Bounded formatters (pure / host-testable) ------------------------ */

/*
 * Expand one 1bpp 8x8 glyph into a 4bpp tile (CONSOLE_TILE_BYTES bytes).
 * Pixel value 1 (ink) becomes nibble 1; pixel value 0 stays 0.
 */
void console_glyph_to_tile(const u8 *glyph, u8 tile[CONSOLE_TILE_BYTES]);

/* Pack a text-BG map entry: tile index in bits 0-9, palette bank in 12-15. */
u16 console_map_entry(u16 tile_index, u8 pal);

/*
 * Render `value` as 8 upper-case hex digits (no "0x" prefix) into `dst`.
 * Always NUL-terminates and never writes more than `cap` bytes.
 * Returns the number of characters written (excluding the terminator).
 */
int console_format_hex32(char *dst, size_t cap, u32 value);

/*
 * Minimal bounded printf supporting %%c %%s %%d %%i %%u %%x %%X and the
 * '-' / '0' flags plus a decimal width. No libc formatting is pulled in.
 * Returns the number of characters written (excluding the terminator).
 */
int console_format(char *dst, size_t cap, const char *fmt, ...);

/* As console_format() but drawing directly to the active screen. */
void console_printf(int x, int y, u8 pal, const char *fmt, ...);

/* Convenience wrappers built on console_printf(). */
void console_print_hex32(int x, int y, u8 pal, u32 value);
void console_print_dec(int x, int y, u8 pal, s32 value);

#endif /* DSI_ARM9_CONSOLE_H */
