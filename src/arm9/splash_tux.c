/*
 * src/arm9/splash_tux.c
 *
 * Built-in OpenLaunch boot splash. See include/arm9/splash_tux.h.
 *
 * Hardware side effects: none. This module only writes the supplied BGR555
 * framebuffer; theme_present_splash() (src/arm9/theme.c) is what maps VRAM and
 * blits it, so everything here is host-testable.
 */
#include "arm9/splash_tux.h"
#include "arm9/font8x8.h"
#include "arm9/theme.h"

/* ---- Palette (RGB888 packed once to BGR555) --------------------------- */

#define TUX_RGB_BODY    0x0C1220u   /* near-black navy: reads on a black bg */
#define TUX_RGB_GREY    0x50505Au
#define TUX_RGB_WHITE   0xFFFFFFu
#define TUX_RGB_ORANGE  0xFFA012u
#define TUX_RGB_OUTLINE 0x8FA2BCu   /* auto-generated silhouette outline     */

#define TUX_BODY    ((u16)(THEME_BGR555((TUX_RGB_BODY    >> 16) & 0xFFu, \
                                        (TUX_RGB_BODY    >>  8) & 0xFFu, \
                                         TUX_RGB_BODY           & 0xFFu) \
                            | THEME_SPLASH_ALPHA))
#define TUX_GREY    ((u16)(THEME_BGR555((TUX_RGB_GREY    >> 16) & 0xFFu, \
                                        (TUX_RGB_GREY    >>  8) & 0xFFu, \
                                         TUX_RGB_GREY           & 0xFFu) \
                            | THEME_SPLASH_ALPHA))
#define TUX_WHITE   ((u16)(THEME_BGR555((TUX_RGB_WHITE   >> 16) & 0xFFu, \
                                        (TUX_RGB_WHITE   >>  8) & 0xFFu, \
                                         TUX_RGB_WHITE          & 0xFFu) \
                            | THEME_SPLASH_ALPHA))
#define TUX_ORANGE  ((u16)(THEME_BGR555((TUX_RGB_ORANGE  >> 16) & 0xFFu, \
                                        (TUX_RGB_ORANGE  >>  8) & 0xFFu, \
                                         TUX_RGB_ORANGE         & 0xFFu) \
                            | THEME_SPLASH_ALPHA))
#define TUX_OUTLINE ((u16)(THEME_BGR555((TUX_RGB_OUTLINE >> 16) & 0xFFu, \
                                        (TUX_RGB_OUTLINE >>  8) & 0xFFu, \
                                         TUX_RGB_OUTLINE        & 0xFFu) \
                            | THEME_SPLASH_ALPHA))

/*
 * Pixel-art source, one character per source pixel (upscaled by
 * SPLASH_TUX_SCALE):
 *   '.'  background (left as the backdrop)   'K'  body
 *   'W'  white belly / face                  'Y'  orange beak and feet
 *   'g'  grey shading
 * Rows shorter than SPLASH_TUX_SRC_W are padded with background, so a ragged
 * edit degrades gracefully rather than reading past the string.
 */
static const char *const s_tux[SPLASH_TUX_SRC_H] = {
    "        KKKKKKKK        ",
    "      KKKKKKKKKKKK      ",
    "    KKKKKKKKKKKKKKKK    ",
    "   KKKWWWWKKKKWWWWKKK   ",
    "   KKWWWWWKKKKWWWWWKK   ",
    "   KKWWKWWKKKKWWKWWKK   ",
    "   KKWWWWWKKKKWWWWWKK   ",
    "   KKKWWWWKKKKWWWWKKK   ",
    "    KKKKKKYYYYKKKKKK    ",
    "    KKKKKKYYYYKKKKKK    ",
    "    KKKKKKKYYKKKKKKK    ",
    "   KKKKKKKKKKKKKKKKKK   ",
    "  KKKKKKWWWWWWWWKKKKKK  ",
    "  KKKKKWWWWWWWWWWKKKKK  ",
    "  KKKKWWWWWWWWWWWWKKKK  ",
    "  KKKWWWWWWWWWWWWWWKKK  ",
    "  KKKWWWWWWWWWWWWWWKKK  ",
    "  KKKWWWWWWWWWWWWWWKKK  ",
    "  KKKKWWWWWWWWWWWWKKKK  ",
    "   KKKWWWWWWWWWWWWKKK   ",
    "   KKKKWWWWWWWWWWKKKK   ",
    "    KKKKKWWWWWWKKKKK    ",
    "     KKKKKKKKKKKKKK     ",
    "      KKKK    KKKK      ",
    "      YYYY    YYYY      ",
    "     YYYYYY  YYYYYY     ",
    "    YYYYYY    YYYYYY    ",
    "     YYYY      YYYY     ",
};

/* Cell kinds for the parsed grid. */
enum {
    TUX_CELL_NONE = 0,
    TUX_CELL_BODY,
    TUX_CELL_WHITE,
    TUX_CELL_ORANGE,
    TUX_CELL_GREY,
    TUX_CELL_OUTLINE
};

static u8 tux_cell_of(char c)
{
    switch (c) {
    case 'K': return (u8)TUX_CELL_BODY;
    case 'W': return (u8)TUX_CELL_WHITE;
    case 'Y': return (u8)TUX_CELL_ORANGE;
    case 'g': return (u8)TUX_CELL_GREY;
    default:  return (u8)TUX_CELL_NONE;
    }
}

static u16 tux_color_of(u8 cell)
{
    switch (cell) {
    case TUX_CELL_BODY:    return TUX_BODY;
    case TUX_CELL_WHITE:   return TUX_WHITE;
    case TUX_CELL_ORANGE:  return TUX_ORANGE;
    case TUX_CELL_GREY:    return TUX_GREY;
    case TUX_CELL_OUTLINE: return TUX_OUTLINE;
    default:               return (u16)0u;
    }
}

/* ---- Framebuffer helpers ---------------------------------------------- */

static void put_px(u8 *dst, u32 x, u32 y, u16 color)
{
    u32 off;

    if (x >= THEME_SPLASH_WIDTH || y >= THEME_SPLASH_HEIGHT) {
        return;
    }
    off = (y * THEME_SPLASH_WIDTH + x) * 2u;
    dst[off]      = (u8)(color & 0xFFu);
    dst[off + 1u] = (u8)(color >> 8);
}

static void fill(u8 *dst, u16 color)
{
    u32 i;

    for (i = 0u; i < THEME_SPLASH_PIXELS; i++) {
        dst[i * 2u]      = (u8)(color & 0xFFu);
        dst[i * 2u + 1u] = (u8)(color >> 8);
    }
}

/* Stamp one upscaled 8x8 glyph block at (x, y). */
static void put_glyph(u8 *dst, u32 x, u32 y, u8 ch, u16 color)
{
    const u8 *g = font8x8_glyph(ch);
    u32 row, col, dx, dy;

    if (g == NULL) {
        return;
    }
    for (row = 0u; row < 8u; row++) {
        u8 bits = g[row];

        for (col = 0u; col < 8u; col++) {
            if ((bits & (u8)(0x80u >> col)) == 0u) {
                continue;
            }
            for (dy = 0u; dy < SPLASH_TUX_TITLE_SCALE; dy++) {
                for (dx = 0u; dx < SPLASH_TUX_TITLE_SCALE; dx++) {
                    put_px(dst,
                           x + col * SPLASH_TUX_TITLE_SCALE + dx,
                           y + row * SPLASH_TUX_TITLE_SCALE + dy,
                           color);
                }
            }
        }
    }
}

/* Centre `s` horizontally at y using the shared 8x8 font, upscaled. */
static void put_title(u8 *dst, const char *s, u32 y, u16 color)
{
    u32 len = 0u;
    u32 advance = 8u * SPLASH_TUX_TITLE_SCALE;
    u32 i, x;

    if (s == NULL) {
        return;
    }
    while (s[len] != '\0') {
        len++;
    }
    if (len * advance > THEME_SPLASH_WIDTH) {
        return;
    }
    x = (THEME_SPLASH_WIDTH - len * advance) / 2u;
    for (i = 0u; i < len; i++) {
        put_glyph(dst, x + i * advance, y, (u8)s[i], color);
    }
}

/* ---- Entry point ------------------------------------------------------ */

/* Parsed source grid plus the derived outline mask (static: 2 * 672 bytes). */
static u8 s_grid[SPLASH_TUX_SRC_H][SPLASH_TUX_SRC_W];
static u8 s_outline[SPLASH_TUX_SRC_H][SPLASH_TUX_SRC_W];

bool splash_tux_render(u8 *dst, u32 cap, u16 bg, u16 accent)
{
    u32 row, col, dx, dy;
    u32 x0, y0, title_y;

    if (dst == NULL || cap < THEME_SPLASH_BYTES) {
        return false;
    }

    /* 1. Parse the art into the cell grid. */
    for (row = 0u; row < SPLASH_TUX_SRC_H; row++) {
        const char *line = s_tux[row];

        for (col = 0u; col < SPLASH_TUX_SRC_W; col++) {
            s_grid[row][col] = (line[col] == '\0')
                                   ? (u8)TUX_CELL_NONE
                                   : tux_cell_of(line[col]);
            s_outline[row][col] = 0u;
        }
    }

    /*
     * 2. Derive a one-source-pixel silhouette outline: any empty cell that
     *    borders a drawn cell becomes an outline cell. This keeps the shape
     *    legible on any backdrop (the body is deliberately near-black).
     */
    for (row = 0u; row < SPLASH_TUX_SRC_H; row++) {
        for (col = 0u; col < SPLASH_TUX_SRC_W; col++) {
            u32 ny, nx;

            if (s_grid[row][col] == (u8)TUX_CELL_NONE) {
                continue;
            }
            for (ny = (row == 0u) ? 0u : row - 1u;
                 ny <= row + 1u && ny < SPLASH_TUX_SRC_H; ny++) {
                for (nx = (col == 0u) ? 0u : col - 1u;
                     nx <= col + 1u && nx < SPLASH_TUX_SRC_W; nx++) {
                    if (s_grid[ny][nx] == (u8)TUX_CELL_NONE) {
                        s_outline[ny][nx] = 1u;
                    }
                }
            }
        }
    }

    /* 3. Backdrop. */
    fill(dst, (u16)(bg | THEME_SPLASH_ALPHA));

    /* 4. Upscale the grid, art over outline. */
    x0 = (THEME_SPLASH_WIDTH - SPLASH_TUX_W) / 2u;
    y0 = 10u;

    for (row = 0u; row < SPLASH_TUX_SRC_H; row++) {
        for (col = 0u; col < SPLASH_TUX_SRC_W; col++) {
            u8 cell = s_grid[row][col];
            u16 color;

            if (cell == (u8)TUX_CELL_NONE) {
                if (s_outline[row][col] == 0u) {
                    continue;
                }
                cell = (u8)TUX_CELL_OUTLINE;
            }
            color = tux_color_of(cell);
            if (color == 0u) {
                continue;
            }
            for (dy = 0u; dy < SPLASH_TUX_SCALE; dy++) {
                for (dx = 0u; dx < SPLASH_TUX_SCALE; dx++) {
                    put_px(dst,
                           x0 + col * SPLASH_TUX_SCALE + dx,
                           y0 + row * SPLASH_TUX_SCALE + dy,
                           color);
                }
            }
        }
    }

    /* 5. Wordmark. */
    title_y = y0 + SPLASH_TUX_H + 10u;
    put_title(dst, SPLASH_TUX_TITLE, title_y,
              (u16)(accent | THEME_SPLASH_ALPHA));
    return true;
}
