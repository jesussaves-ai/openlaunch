/*
 * src/arm9/console.c
 *
 * Dual-screen tile/text console. See include/arm9/console.h.
 *
 * The layout follows GBATEK "DS Video BG Modes / Control" and the DSi VRAM
 * bank table (docs/hardware_notes.md):
 *
 *   VRAMCNT_A = 0x81  -> bank A (128 KiB) as Engine A BG at 0x06000000
 *   VRAMCNT_C = 0x84  -> bank C (128 KiB) as Engine B BG at 0x06200000
 *
 * Each engine runs BG mode 0 with BG0 as a text layer. BG0CNT selects character
 * base block 0 (tiles at the bank base) and screen base block 2 (map at
 * base + 0x1000), 4bpp colour depth, 256x256 px.
 *
 * NOTE: VRAM and palette RAM only accept 16- and 32-bit stores on the DS, so
 * every write below goes through REG16().
 */
#include "arm9/console.h"
#include "arm9/video.h"
#include "common/dsi_hw.h"

#include <stdarg.h>

/* ---- Engine descriptors ---------------------------------------------- */

typedef struct {
    u32 dispcnt;     /* DISPCNT register address            */
    u32 bgcnt;       /* BG0CNT register address             */
    u32 scroll;      /* BG0HOFS register address            */
    u32 tile_base;   /* BG character (tile) VRAM base       */
    u32 map_base;    /* BG screen (map) VRAM base           */
    u32 pal_base;    /* BG palette base                     */
} console_engine_t;

static const console_engine_t s_engines[CONSOLE_COUNT] = {
    {
        DISPCNT,
        BG0CNT,
        BG0HOFS,
        0x06000000u,   /* Engine A tiles */
        0x06001000u,   /* Engine A map   */
        PALETTE_MAIN
    },
    {
        DISPCNT_SUB,
        BG0CNT_SUB,
        BG0HOFS_SUB,
        0x06200000u,   /* Engine B tiles */
        0x06201000u,   /* Engine B map   */
        PALETTE_SUB
    },
};

static console_id_t s_active = CONSOLE_MAIN;

/*
 * Foreground colours (BGR555) for palette banks 0..7. Bank 0 is white so the
 * default text colour is high-contrast white on black.
 */
static const u16 s_palette_fg[CONSOLE_PAL_COUNT] = {
    0x7FFFu,   /* 0 white  */
    0x7FE0u,   /* 1 cyan   */
    0x03E0u,   /* 2 green  */
    0x03FFu,   /* 3 yellow */
    0x001Fu,   /* 4 red    */
    0x4210u,   /* 5 grey   */
    0x7C00u,   /* 6 blue   */
    0x7FFFu,   /* 7 white  */
};

/* ---- Pure helpers (host-testable) ------------------------------------ */

void console_glyph_to_tile(const u8 *glyph, u8 tile[CONSOLE_TILE_BYTES])
{
    u32 row;

    for (row = 0u; row < FONT8X8_GLYPH_BYTES; row++) {
        u8 bits = glyph[row];
        u32 base = row * 4u;

        /* Pixel 0 (bit 7) is the low nibble of the first byte, pixel 1 the
         * high nibble, and so on: 4bpp stores two pixels per byte. */
        tile[base + 0u] = (u8)(((bits & 0x80u) ? 1u : 0u) |
                               ((bits & 0x40u) ? 0x10u : 0u));
        tile[base + 1u] = (u8)(((bits & 0x20u) ? 1u : 0u) |
                               ((bits & 0x10u) ? 0x10u : 0u));
        tile[base + 2u] = (u8)(((bits & 0x08u) ? 1u : 0u) |
                               ((bits & 0x04u) ? 0x10u : 0u));
        tile[base + 3u] = (u8)(((bits & 0x02u) ? 1u : 0u) |
                               ((bits & 0x01u) ? 0x10u : 0u));
    }
}

u16 console_map_entry(u16 tile_index, u8 pal)
{
    return (u16)((tile_index & 0x03FFu) |
                 (((u16)pal & 0x0Fu) << 12));
}

/* ---- Bounded formatter ----------------------------------------------- */

typedef struct {
    char  *dst;
    size_t cap;    /* total capacity including terminator */
    size_t len;    /* characters written so far           */
} fmt_out_t;

static void fmt_putc(fmt_out_t *o, char c)
{
    if (o->len + 1u < o->cap) {
        o->dst[o->len] = c;
    }
    o->len++;
}

static void fmt_pad(fmt_out_t *o, char c, u32 count)
{
    while (count-- != 0u) {
        fmt_putc(o, c);
    }
}

/* Emit `value` in `base`, honouring the '-' (left) and '0' (zero-pad) flags. */
static void fmt_uint(fmt_out_t *o, u32 value, u32 base, bool upper,
                     bool zero_pad, bool left, u32 width)
{
    char tmp[32];
    u32 n = 0u;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    u32 pad;

    if (base < 2u || base > 16u) {
        base = 10u;
    }

    do {
        tmp[n++] = digits[value % base];
        value /= base;
    } while (value != 0u && n < sizeof(tmp));

    if (width < n) {
        width = n;
    }
    pad = width - n;

    if (!left && !zero_pad) {
        fmt_pad(o, ' ', pad);
    }
    if (!left && zero_pad) {
        fmt_pad(o, '0', pad);
    }
    while (n != 0u) {
        fmt_putc(o, tmp[--n]);
    }
    if (left) {
        fmt_pad(o, ' ', pad);
    }
}

static int fmt_vformat(char *dst, size_t cap, const char *fmt, va_list ap)
{
    fmt_out_t o;

    if (cap == 0u) {
        return 0;
    }
    o.dst = dst;
    o.cap = cap;
    o.len = 0u;

    while (*fmt != '\0') {
        if (*fmt != '%') {
            fmt_putc(&o, *fmt++);
            continue;
        }

        fmt++;   /* consume '%' */
        if (*fmt == '%') {
            fmt_putc(&o, '%');
            fmt++;
            continue;
        }

        {
            bool zero_pad = false;
            bool left = false;
            u32 width = 0u;
            char conv;

            for (;;) {
                if (*fmt == '0') {
                    zero_pad = true;
                    fmt++;
                } else if (*fmt == '-') {
                    left = true;
                    fmt++;
                } else {
                    break;
                }
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10u + (u32)(*fmt - '0');
                if (width > 64u) {
                    width = 64u;
                }
                fmt++;
            }

            conv = *fmt;
            if (conv == '\0') {
                break;
            }
            fmt++;

            switch (conv) {
            case 'c':
                fmt_putc(&o, (char)va_arg(ap, int));
                break;
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (s == NULL) {
                    s = "(null)";
                }
                while (*s != '\0') {
                    fmt_putc(&o, *s++);
                }
                break;
            }
            case 'd':
            case 'i': {
                s32 v = va_arg(ap, s32);
                u32 mag;
                if (v < 0) {
                    fmt_putc(&o, '-');
                    mag = (u32)(-(v + 1)) + 1u;   /* avoids INT32_MIN overflow */
                    if (width > 0u) {
                        width--;
                    }
                } else {
                    mag = (u32)v;
                }
                fmt_uint(&o, mag, 10u, false, zero_pad, left, width);
                break;
            }
            case 'u':
                fmt_uint(&o, va_arg(ap, u32), 10u, false, zero_pad, left, width);
                break;
            case 'x':
                fmt_uint(&o, va_arg(ap, u32), 16u, false, zero_pad, left, width);
                break;
            case 'X':
                fmt_uint(&o, va_arg(ap, u32), 16u, true, zero_pad, left, width);
                break;
            case 'p':
                fmt_putc(&o, '0');
                fmt_putc(&o, 'x');
                fmt_uint(&o, (u32)(uintptr_t)va_arg(ap, void *), 16u,
                         false, true, false, 8u);
                break;
            default:
                /* Unknown conversion: emit it literally. */
                fmt_putc(&o, '%');
                fmt_putc(&o, conv);
                break;
            }
        }
    }

    dst[(o.len < cap) ? o.len : (cap - 1u)] = '\0';
    return (int)o.len;
}

int console_format_hex32(char *dst, size_t cap, u32 value)
{
    return console_format(dst, cap, "%08X", value);
}

int console_format(char *dst, size_t cap, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = fmt_vformat(dst, cap, fmt, ap);
    va_end(ap);
    return n;
}

/* ---- Hardware drawing ------------------------------------------------ */

static const console_engine_t *active_engine(void)
{
    return &s_engines[(s_active < CONSOLE_COUNT) ? (u32)s_active : 0u];
}

void console_select(console_id_t id)
{
    if ((u32)id < (u32)CONSOLE_COUNT) {
        s_active = id;
    }
}

void console_puts(int x, int y, u8 pal, const char *str)
{
    const console_engine_t *eng = active_engine();
    u32 cx = (u32)x;
    u32 cy = (u32)y;

    if (x < 0 || y < 0 || cy >= CONSOLE_ROWS) {
        return;
    }

    while (*str != '\0' && cx < CONSOLE_COLS) {
        u8 c = (u8)*str++;
        u8 tile;

        if (c >= FONT8X8_FIRST_CHAR && c <= FONT8X8_LAST_CHAR) {
            tile = (u8)(c - FONT8X8_FIRST_CHAR);
        } else {
            tile = (u8)('?' - FONT8X8_FIRST_CHAR);
        }
        REG16(eng->map_base + (cy * CONSOLE_MAP_COLS + cx) * 2u) =
            console_map_entry(tile, pal);
        cx++;
    }
}

void console_printf(int x, int y, u8 pal, const char *fmt, ...)
{
    char line[CONSOLE_COLS + 1];
    va_list ap;

    va_start(ap, fmt);
    (void)fmt_vformat(line, sizeof(line), fmt, ap);
    va_end(ap);

    console_puts(x, y, pal, line);
}

void console_print_hex32(int x, int y, u8 pal, u32 value)
{
    console_printf(x, y, pal, "0x%08X", value);
}

void console_print_dec(int x, int y, u8 pal, s32 value)
{
    console_printf(x, y, pal, "%d", value);
}

static void console_load_font(u32 tile_base)
{
    u8 tile[CONSOLE_TILE_BYTES];
    u32 g;

    for (g = 0u; g < FONT8X8_GLYPH_COUNT; g++) {
        u32 i;

        console_glyph_to_tile(font8x8_basic[g], tile);
        for (i = 0u; i < CONSOLE_TILE_BYTES; i += 2u) {
            REG16(tile_base + g * CONSOLE_TILE_BYTES + i) =
                (u16)((u16)tile[i] | ((u16)tile[i + 1u] << 8));
        }
    }
}

static void console_load_palette(u32 pal_base)
{
    u32 bank;

    for (bank = 0u; bank < CONSOLE_PAL_COUNT; bank++) {
        u32 i;

        for (i = 0u; i < 16u; i++) {
            /* Entry 0 of every bank is transparent (shows the backdrop). */
            u16 color = (i == 0u) ? 0x0000u : s_palette_fg[bank];
            REG16(pal_base + (bank * 16u + i) * 2u) = color;
        }
    }
}

static void console_clear_engine(const console_engine_t *eng)
{
    u32 cell;

    for (cell = 0u; cell < CONSOLE_MAP_COLS * CONSOLE_MAP_ROWS; cell++) {
        REG16(eng->map_base + cell * 2u) = console_map_entry(0u, 0u);
    }
}

static void console_engine_init(const console_engine_t *eng)
{
    /* Force-blank while reprogramming the layer. */
    REG32(eng->dispcnt) = DISPCNT_FORCED_BLANK;

    console_load_font(eng->tile_base);
    console_load_palette(eng->pal_base);
    console_clear_engine(eng);

    /* BG0CNT: priority 0, char base block 0, 4bpp, screen base block 2. */
    REG16(eng->bgcnt) = 0x0200u;

    /* No scrolling. */
    REG16(eng->scroll + 0u) = 0u;
    REG16(eng->scroll + 2u) = 0u;

    /* Mode 0, BG0 enabled, display on. */
    REG32(eng->dispcnt) = 0x00000100u;
}

void console_init(void)
{
    /*
     * Map VRAM bank A to Engine A BG (0x06000000) and bank C to Engine B BG
     * (0x06200000). Value layout: MST in bits 0-2, OFS in bits 3-4, enable in
     * bit 7. A = MST 1 (Engine A BG), C = MST 4 (Engine B BG).
     */
    REG8(VRAMCNT_A) = 0x81u;
    REG8(VRAMCNT_C) = 0x84u;

    console_engine_init(&s_engines[CONSOLE_MAIN]);
    console_engine_init(&s_engines[CONSOLE_SUB]);

    s_active = CONSOLE_MAIN;
}

void console_clear(void)
{
    console_clear_engine(active_engine());
}

void console_clear_all(void)
{
    console_clear_engine(&s_engines[CONSOLE_MAIN]);
    console_clear_engine(&s_engines[CONSOLE_SUB]);
}
