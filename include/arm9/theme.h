/*
 * include/arm9/theme.h
 *
 * Lightweight, zero-allocation boot theme engine driven by BOOT.CFG.
 *
 * The theme extends the hotkey parser (`src/arm9/boot_cfg.c`) with four optional
 * directives:
 *
 *     THEME_ACCENT = #00FFCC      selected borders / active tabs / headers
 *     THEME_TEXT   = 0xFFFFFF     standard console text
 *     THEME_BG     = #000000      backdrop and window frames
 *     SPLASH_IMAGE = SPLASH.RAW   raw 256x192 15-bit BGR555 bitmap (SD root)
 *
 * Colours are written as RGB888 (`#RRGGBB`, `0xRRGGBB`, bare `RRGGBB` or the
 * `#RGB` shorthand) and converted to the hardware BGR555 palette format. When a
 * directive is omitted (or malformed) the AMOLED-friendly defaults below apply:
 *
 *     ACCENT = 0x00FF88   TEXT = 0xFFFFFF   BG = 0x000000
 *
 * The splash is a raw 256x192 image of 98304 little-endian 15-bit BGR555 pixels
 * (SPLASH.RAW). It is streamed through the delegated FAT reader into a static
 * staging buffer and then blitted to Engine A BG2 VRAM (bank B at 0x06020000),
 * which is presented as a 16bpp extended-affine bitmap while the text console
 * keeps running on BG0 (Engine A mode 5). When no valid splash is present the
 * engine falls back to the pure `THEME_BG` backdrop and accent-coloured UI.
 *
 * Everything that does not touch MMIO is pure and host-testable
 * (tests/test_theme.c).
 */
#ifndef DSI_ARM9_THEME_H
#define DSI_ARM9_THEME_H

#include "../common/types.h"
#include "../common/fat_ro.h"

/* Longest accepted SPLASH_IMAGE path, including its NUL (matches BOOT.CFG). */
#define THEME_PATH_MAX 20u

/* ---- RGB888 -> BGR555 ------------------------------------------------- */

/* Pack 5-bit BGR555 from 8-bit RGB components (host-testable constant). */
#define THEME_BGR555(r, g, b) \
    ((u16)((((u32)(r) >> 3) & 0x1Fu) | \
           ((((u32)(g) >> 3) & 0x1Fu) << 5) | \
           ((((u32)(b) >> 3) & 0x1Fu) << 10)))

/* AMOLED-friendly defaults (RGB888 and the derived BGR555 palette words). */
#define THEME_ACCENT_RGB_DEFAULT 0x00FF88u
#define THEME_TEXT_RGB_DEFAULT   0xFFFFFFu
#define THEME_BG_RGB_DEFAULT     0x000000u

#define THEME_ACCENT_BGR555_DEFAULT THEME_BGR555(0x00u, 0xFFu, 0x88u)
#define THEME_TEXT_BGR555_DEFAULT   THEME_BGR555(0xFFu, 0xFFu, 0xFFu)
#define THEME_BG_BGR555_DEFAULT     THEME_BGR555(0x00u, 0x00u, 0x00u)

/* Convert one 0xRRGGBB value into a BGR555 palette word. Pure. */
u16 theme_rgb_to_bgr555(u32 rgb);

/* ---- Parsed theme ----------------------------------------------------- */

typedef struct {
    u16  accent;                     /* BGR555 accent colour              */
    u16  text;                       /* BGR555 console text colour        */
    u16  bg;                         /* BGR555 backdrop colour            */
    bool has_splash;                 /* SPLASH_IMAGE was supplied         */
    char splash_path[THEME_PATH_MAX];/* NUL-terminated SD path ("" = none) */
} theme_config_t;

/* Reset `out` to the built-in AMOLED defaults. Pure; NULL-safe. */
void theme_defaults(theme_config_t *out);

/*
 * Parse one colour token (`#RRGGBB`, `0xRRGGBB`, `RRGGBB` or `#RGB`) into a
 * BGR555 palette word. `tok` is a counted token (not necessarily NUL-escaped).
 * Returns false for NULL/short/non-hex/integer-overflow input. Pure.
 */
bool theme_parse_color(const u8 *tok, u32 n, u16 *out_bgr555);

/* ---- Splash image ----------------------------------------------------- */

#define THEME_SPLASH_WIDTH   256u
#define THEME_SPLASH_HEIGHT  192u
#define THEME_SPLASH_PIXELS  (THEME_SPLASH_WIDTH * THEME_SPLASH_HEIGHT)
#define THEME_SPLASH_BYTES   (THEME_SPLASH_PIXELS * 2u)   /* 98304 */

/* Bit 15 of a 15-bit BG/bitmap pixel is the alpha flag (1 = opaque). */
#define THEME_SPLASH_ALPHA   0x8000u

/* Engine A BG2 framebuffer: VRAM bank B mapped at 0x06020000 (offset 1). */
#define THEME_SPLASH_VRAM    0x06020000u

/* Result of validating / loading a splash file. */
typedef enum {
    THEME_SPLASH_OK = 0,        /* a valid raw 256x192 15-bit bitmap         */
    THEME_SPLASH_NONE,          /* no SPLASH_IMAGE configured                */
    THEME_SPLASH_ERR_ARG,       /* NULL / undersized destination             */
    THEME_SPLASH_ERR_KIND,      /* not a RAW image (e.g. .BMP)               */
    THEME_SPLASH_ERR_SIZE,      /* not exactly 256*192*2 bytes               */
    THEME_SPLASH_ERR_IO,        /* file not found / FAT read failed          */
    THEME_SPLASH_BUILTIN        /* rendered the built-in OpenLaunch splash   */
} theme_splash_status_t;

/* Human-readable splash status ("OK", "NONE", "SIZE", ...); never NULL. */
const char *theme_splash_status_text(theme_splash_status_t status);

/*
 * Validate a splash file by name and size. Only uncompressed `*.RAW` / `*.BIN`
 * files of exactly THEME_SPLASH_BYTES are accepted; anything else (including a
 * future indexed `.BMP`) fails closed. Pure; host-testable.
 */
theme_splash_status_t theme_splash_check(const char *name, u32 file_size);

/*
 * Resolve `path` on the mounted volume and stream a validated splash into `dst`
 * (capacity `cap` bytes). On success `*out_len` (optional) is THEME_SPLASH_BYTES.
 * Touches no MMIO, so it runs unchanged on the host test harness.
 */
theme_splash_status_t theme_splash_read(fat_volume_t *vol, const char *path,
                                        u8 *dst, u32 cap, u32 *out_len);

/*
 * Copy `pixels` little-endian 15-bit BGR555 words from `src` into `dst`,
 * forcing the alpha bit so the pixels render opaque (bit 15 = 1). Pure.
 */
void theme_splash_blit(const u8 *src, vu16 *dst, u32 pixels);

/* ---- Hardware entry points (MMIO; not exercised on the host) ---------- */

/*
 * Load the configured splash into the internal zero-allocation staging buffer.
 * Returns THEME_SPLASH_OK on success (or THEME_SPLASH_NONE / an ERR_* code).
 * No VRAM is touched until theme_present_splash() is called.
 */
theme_splash_status_t theme_load_splash(fat_volume_t *vol,
                                        const theme_config_t *cfg,
                                        u32 *out_len);

/* The staged splash pixels, or NULL when none was loaded successfully. */
const u8 *theme_splash_pixels(void);

/*
 * Blit the staged splash to Engine A BG2 (bank B, 0x06020000) and present it as
 * a 16bpp extended-affine bitmap behind the BG0 text console (Engine A mode 5).
 * Returns false when no splash was staged.
 */
bool theme_present_splash(void);

/*
 * Apply the theme colours to the Engine A/B console palettes and the backdrop:
 * BANK 0 (white) -> text, BANK 1 (cyan, headers) and BANK 3 (yellow, selection)
 * -> accent. A NULL `cfg` selects the built-in defaults.
 */
void theme_apply(const theme_config_t *cfg);

#endif /* DSI_ARM9_THEME_H */
