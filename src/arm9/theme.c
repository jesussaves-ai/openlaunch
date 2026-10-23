/*
 * src/arm9/theme.c
 *
 * Zero-allocation boot theme engine. See include/arm9/theme.h.
 *
 * The module is split into two layers:
 *
 *   1. Pure helpers (colour parsing, defaults, splash validation / blit and the
 *      FAT splash reader) that contain no MMIO and are covered on the host by
 *      tests/test_theme.c.
 *   2. Thin hardware entry points that map the Engine A palette / backdrop and
 *      the Engine A BG2 bitmap framebuffer. They are only run by the firmware.
 *
 * Register widths (the project conventions): palette RAM, VRAM and the 2D control
 * registers only accept 16-bit stores, so every write below goes through
 * REG16(); DISPCNT is a 32-bit register.
 */
#include "arm9/theme.h"
#include "arm9/console.h"
#include "arm9/video.h"
#include "arm9/splash_tux.h"
#include "common/dsi_hw.h"

/* The built-in theme, shared by theme_defaults() and theme_apply(NULL). */
static const theme_config_t s_theme_default = {
    THEME_ACCENT_BGR555_DEFAULT,
    THEME_TEXT_BGR555_DEFAULT,
    THEME_BG_BGR555_DEFAULT,
    false,
    ""
};

u16 theme_rgb_to_bgr555(u32 rgb)
{
    u32 r = (rgb >> 16) & 0xFFu;
    u32 g = (rgb >> 8) & 0xFFu;
    u32 b = rgb & 0xFFu;

    return (u16)(((r >> 3) << 0) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

void theme_defaults(theme_config_t *out)
{
    if (out == NULL) {
        return;
    }
    out->accent = THEME_ACCENT_BGR555_DEFAULT;
    out->text = THEME_TEXT_BGR555_DEFAULT;
    out->bg = THEME_BG_BGR555_DEFAULT;
    out->has_splash = false;
    out->splash_path[0] = '\0';
}

bool theme_parse_color(const u8 *tok, u32 n, u16 *out_bgr555)
{
    u32 digits[6];
    u32 count = 0u;
    u32 i = 0u;
    u32 rgb;

    if (tok == NULL || out_bgr555 == NULL || n == 0u) {
        return false;
    }

    /* Optional radix prefix: '#' or "0x"/"0X". */
    if (tok[0] == '#') {
        i = 1u;
    } else if (n >= 2u && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
        i = 2u;
    }

    for (; i < n; i++) {
        u8 c = tok[i];
        u32 v;

        if (c == 0x00u) {
            break;   /* a NUL ends the token even if `n` is larger */
        }
        if (c >= '0' && c <= '9') {
            v = (u32)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v = (u32)(c - 'a') + 10u;
        } else if (c >= 'A' && c <= 'F') {
            v = (u32)(c - 'A') + 10u;
        } else {
            return false;
        }
        if (count >= 6u) {
            return false;   /* more than six digits: reject the token */
        }
        digits[count++] = v;
    }

    if (count == 6u) {
        rgb = (digits[0] << 20) | (digits[1] << 16) | (digits[2] << 12) |
              (digits[3] << 8)  | (digits[4] << 4)  | digits[5];
    } else if (count == 3u) {
        /* #RGB shorthand: each nibble is replicated (0xF -> 0xFF). */
        rgb = ((digits[0] * 17u) << 16) |
              ((digits[1] * 17u) << 8)  |
               (digits[2] * 17u);
    } else {
        return false;
    }

    *out_bgr555 = theme_rgb_to_bgr555(rgb);
    return true;
}

/* ---- Splash validation ------------------------------------------------ */

/* True when `name`'s extension (after the last '.') equals `ext` (upper-case). */
static bool name_has_ext(const char *name, const char *ext)
{
    const char *dot = NULL;
    u32 i;

    for (i = 0u; name[i] != '\0'; i++) {
        if (name[i] == '.') {
            dot = &name[i];
        }
    }
    if (dot == NULL) {
        return false;
    }
    dot++;
    for (i = 0u; ext[i] != '\0'; i++) {
        char c = dot[i];

        if (c >= 'a' && c <= 'z') {
            c = (char)(c - ('a' - 'A'));
        }
        if (c != ext[i]) {
            return false;
        }
    }
    return dot[i] == '\0';
}

const char *theme_splash_status_text(theme_splash_status_t status)
{
    switch (status) {
    case THEME_SPLASH_OK:        return "OK";
    case THEME_SPLASH_NONE:      return "NONE";
    case THEME_SPLASH_ERR_ARG:   return "ARG";
    case THEME_SPLASH_ERR_KIND:  return "KIND";
    case THEME_SPLASH_ERR_SIZE:  return "SIZE";
    case THEME_SPLASH_ERR_IO:    return "IO";
    case THEME_SPLASH_BUILTIN:   return "BUILTIN";
    default:                     return "?";
    }
}

theme_splash_status_t theme_splash_check(const char *name, u32 file_size)
{
    if (name == NULL) {
        return THEME_SPLASH_ERR_ARG;
    }
    if (!name_has_ext(name, "RAW") && !name_has_ext(name, "BIN")) {
        return THEME_SPLASH_ERR_KIND;
    }
    if (file_size != THEME_SPLASH_BYTES) {
        return THEME_SPLASH_ERR_SIZE;
    }
    return THEME_SPLASH_OK;
}

theme_splash_status_t theme_splash_read(fat_volume_t *vol, const char *path,
                                        u8 *dst, u32 cap, u32 *out_len)
{
    fat_file_entry_t entry;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;
    theme_splash_status_t st;
    u32 got = 0u;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (vol == NULL || path == NULL || dst == NULL || cap < THEME_SPLASH_BYTES) {
        return THEME_SPLASH_ERR_ARG;
    }
    if (!fat_find_path(vol, path, &entry) || entry.is_dir) {
        return THEME_SPLASH_ERR_IO;
    }

    st = theme_splash_check(entry.name, entry.file_size);
    if (st != THEME_SPLASH_OK) {
        return st;
    }

    if (!fat_read_file_range(vol, &entry, 0u, dst, THEME_SPLASH_BYTES,
                             &got, &chain) ||
        got != THEME_SPLASH_BYTES) {
        return THEME_SPLASH_ERR_IO;
    }
    if (out_len != NULL) {
        *out_len = got;
    }
    return THEME_SPLASH_OK;
}

void theme_splash_blit(const u8 *src, vu16 *dst, u32 pixels)
{
    u32 i;

    if (src == NULL || dst == NULL) {
        return;
    }
    for (i = 0u; i < pixels; i++) {
        u16 px = (u16)((u16)src[i * 2u] | ((u16)src[i * 2u + 1u] << 8));

        dst[i] = (u16)(px | THEME_SPLASH_ALPHA);
    }
}

/* ---- Hardware: splash staging + presentation -------------------------- */

/* Staging buffer for the raw bitmap. Lives in .bss (shared Main RAM). */
static u8   s_splash_stage[THEME_SPLASH_BYTES];
static bool s_splash_ready;

theme_splash_status_t theme_load_splash(fat_volume_t *vol,
                                        const theme_config_t *cfg,
                                        u32 *out_len)
{
    theme_splash_status_t st;
    u32 got = 0u;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (cfg == NULL || !cfg->has_splash || cfg->splash_path[0] == '\0') {
        /*
         * No SPLASH_IMAGE configured: render the built-in OpenLaunch penguin.
         * This needs no SD card, so the boot screen is never a bare backdrop.
         */
        u16 bg     = (cfg != NULL) ? cfg->bg     : THEME_BG_BGR555_DEFAULT;
        u16 accent = (cfg != NULL) ? cfg->accent : THEME_ACCENT_BGR555_DEFAULT;

        if (splash_tux_render(s_splash_stage, (u32)sizeof(s_splash_stage),
                              bg, accent)) {
            s_splash_ready = true;
            if (out_len != NULL) {
                *out_len = THEME_SPLASH_BYTES;
            }
            return THEME_SPLASH_BUILTIN;
        }
        s_splash_ready = false;
        return THEME_SPLASH_NONE;
    }

    st = theme_splash_read(vol, cfg->splash_path, s_splash_stage,
                           (u32)sizeof(s_splash_stage), &got);
    s_splash_ready = (st == THEME_SPLASH_OK);
    if (out_len != NULL) {
        *out_len = got;
    }
    return st;
}

const u8 *theme_splash_pixels(void)
{
    return s_splash_ready ? s_splash_stage : NULL;
}

bool theme_present_splash(void)
{
    if (!s_splash_ready) {
        return false;
    }

    /*
     * Map VRAM bank B as Engine A BG at offset 1 (0x06020000): MST=1 (Engine A
     * BG), OFS=1, enable. Bank A stays the console BG; bank C is Engine B.
     */
    REG8(VRAMCNT_B) = 0x89u;

    theme_splash_blit(s_splash_stage, (vu16 *)(uintptr_t)THEME_SPLASH_VRAM,
                      THEME_SPLASH_PIXELS);

    /*
     * BG2 as a 16bpp extended-affine bitmap, 256x256, bitmap base 8
     * (8 * 0x4000 = 0x20000 -> 0x06020000), priority 3 (behind the BG0 text).
     * 0x4084 = size 256x256 (bits 14-15) | 16bpp (bit 7) | bitmap flag (bit 2).
     */
    REG16(BG2CNT) = (u16)(0x4084u | (8u << 8) | 3u);
    REG16(BG2HOFS) = 0u;
    REG16(BG2VOFS) = 0u;

    /*
     * Engine A mode 5: BG0/BG1 text + BG2/BG3 extended bitmap (libnds mode
     * table). BG0 (the console) has priority 0 so the text still draws on top;
     * the display-source bits (16-17) must stay at normal 2D.
     */
    REG32(DISPCNT) = DISPCNT_DISPLAY_2D | 0x0005u | 0x00000100u | 0x00000400u;
    return true;
}

/* ---- Hardware: theme colours ------------------------------------------ */

/* Write a BGR555 foreground into entries 1..15 of one palette bank. */
static void theme_set_bank(u32 pal_base, u8 bank, u16 color)
{
    u32 i;

    for (i = 1u; i < 16u; i++) {
        REG16(pal_base + ((u32)bank * 16u + i) * 2u) = color;
    }
}

void theme_apply(const theme_config_t *cfg)
{
    const theme_config_t *t = (cfg != NULL) ? cfg : &s_theme_default;

    theme_set_bank(PALETTE_MAIN, CONSOLE_PAL_WHITE, t->text);
    theme_set_bank(PALETTE_SUB,  CONSOLE_PAL_WHITE, t->text);

    /* Accent recolours the headers (cyan) and the selected row (yellow). */
    theme_set_bank(PALETTE_MAIN, CONSOLE_PAL_CYAN, t->accent);
    theme_set_bank(PALETTE_SUB,  CONSOLE_PAL_CYAN, t->accent);
    theme_set_bank(PALETTE_MAIN, CONSOLE_PAL_YELLOW, t->accent);
    theme_set_bank(PALETTE_SUB,  CONSOLE_PAL_YELLOW, t->accent);

    /* Palette entry 0 is the backdrop shown behind the (transparent) cells. */
    REG16(PALETTE_MAIN) = t->bg;
    REG16(PALETTE_SUB) = t->bg;
}
