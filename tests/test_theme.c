/*
 * tests/test_theme.c
 *
 * Host-side unit tests for the BOOT.CFG boot theme engine (suite 13):
 *
 *   - RGB888 -> BGR555 conversion and hex colour parsing (`#RRGGBB`,
 *     `0xRRGGBB`, bare `RRGGBB`, `#RGB`, invalid/short/huge tokens);
 *   - the AMOLED default theme fallbacks;
 *   - BOOT.CFG theme-directive extraction (`THEME_ACCENT` / `THEME_TEXT` /
 *     `THEME_BG` / `SPLASH_IMAGE`) alongside the hotkey map, including the
 *     `#RRGGBB`-is-not-a-comment rule and the malformed-directive fallbacks;
 *   - raw splash validation (name + exact size) and the BGR555 blit;
 *   - the staged splash reader against the generated build/sdcard.img.
 *
 * `theme_present_splash()` (Engine A VRAM) is intentionally not exercised: it
 * is the only MMIO-touching entry point.
 *
 * Build/run with:  make host-test   (passes build/sdcard.img as argv[1])
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arm9/boot_cfg.h"
#include "arm9/theme.h"
#include "common/fat_ro.h"

/* ------------------------------------------------------------------ */
/* Colour conversion + parsing                                          */
/* ------------------------------------------------------------------ */

static void test_rgb_to_bgr555(void)
{
    assert(theme_rgb_to_bgr555(0x000000u) == 0x0000u);
    assert(theme_rgb_to_bgr555(0xFFFFFFu) == 0x7FFFu);
    assert(theme_rgb_to_bgr555(0xFF0000u) == 0x001Fu);   /* red   in bits 0-4  */
    assert(theme_rgb_to_bgr555(0x00FF00u) == 0x03E0u);   /* green in bits 5-9  */
    assert(theme_rgb_to_bgr555(0x0000FFu) == 0x7C00u);   /* blue  in bits 10-14 */

    /* The default accent expands to the documented BGR555 word. */
    assert(theme_rgb_to_bgr555(THEME_ACCENT_RGB_DEFAULT) ==
           THEME_ACCENT_BGR555_DEFAULT);
    assert(THEME_TEXT_BGR555_DEFAULT == 0x7FFFu);
    assert(THEME_BG_BGR555_DEFAULT == 0x0000u);
}

static void test_parse_color(void)
{
    u16 c;

    /* Six-digit forms: '#', '0x' and bare. */
    assert(theme_parse_color((const u8 *)"#00FFCC", 7u, &c));
    assert(c == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    assert(theme_parse_color((const u8 *)"0x00ffcc", 8u, &c));
    assert(c == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    assert(theme_parse_color((const u8 *)"00FFCC", 6u, &c));
    assert(c == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    assert(theme_parse_color((const u8 *)"0X112233", 8u, &c));
    assert(c == THEME_BGR555(0x11u, 0x22u, 0x33u));

    /* Three-digit shorthand. */
    assert(theme_parse_color((const u8 *)"#0FC", 4u, &c));
    assert(c == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    assert(theme_parse_color((const u8 *)"fff", 3u, &c));
    assert(c == 0x7FFFu);

    /* A NUL terminates the token even when `n` is larger. */
    {
        const u8 nulcut[8] = { '#', '0', '0', 'F', 'F', 'C', 'C', 0x00 };
        assert(theme_parse_color(nulcut, 8u, &c));
        assert(c == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    }

    /* Malformed tokens are rejected and leave `c` untouched. */
    c = 0x1234u;
    assert(!theme_parse_color((const u8 *)"#00FFC", 6u, &c));   /* 5 digits */
    assert(!theme_parse_color((const u8 *)"#00FFCCDD", 9u, &c));/* 8 digits */
    assert(!theme_parse_color((const u8 *)"#GGGGGG", 7u, &c));  /* non-hex  */
    assert(!theme_parse_color((const u8 *)"#", 1u, &c));        /* empty    */
    assert(!theme_parse_color((const u8 *)"", 0u, &c));         /* empty    */
    assert(c == 0x1234u);
    assert(!theme_parse_color(NULL, 4u, &c));
    assert(!theme_parse_color((const u8 *)"#0FC", 4u, NULL));
}

static void test_defaults(void)
{
    theme_config_t t;

    theme_defaults(&t);
    assert(t.accent == THEME_ACCENT_BGR555_DEFAULT);
    assert(t.text == THEME_TEXT_BGR555_DEFAULT);
    assert(t.bg == THEME_BG_BGR555_DEFAULT);
    assert(!t.has_splash);
    assert(t.splash_path[0] == '\0');

    /* The parsed-config defaults agree with the standalone theme defaults. */
    {
        boot_cfg_t cfg;

        boot_cfg_defaults(&cfg);
        assert(cfg.theme.accent == t.accent);
        assert(cfg.theme.text == t.text);
        assert(cfg.theme.bg == t.bg);
        assert(!cfg.theme.has_splash);
    }

    theme_defaults(NULL);   /* must not crash */
}

/* ------------------------------------------------------------------ */
/* BOOT.CFG theme extraction                                            */
/* ------------------------------------------------------------------ */

static void test_boot_cfg_theme(void)
{
    boot_cfg_t cfg;
    const char *text =
        "# OpenLaunch theme demo\r\n"
        "DEFAULT=BOOT.NDS\r\n"
        "A=HELLO.NDS\r\n"
        "THEME_ACCENT=#00FFCC\r\n"
        "THEME_TEXT=0x111213\r\n"
        "THEME_BG=#000000\r\n"
        "SPLASH_IMAGE=SPLASH.RAW\r\n";

    assert(boot_cfg_parse((const u8 *)text, (u32)strlen(text), &cfg));
    assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
    assert(cfg.theme.accent == THEME_BGR555(0x00u, 0xFFu, 0xCCu));
    assert(cfg.theme.text == THEME_BGR555(0x11u, 0x12u, 0x13u));
    assert(cfg.theme.bg == 0x0000u);
    assert(cfg.theme.has_splash);
    assert(strcmp(cfg.theme.splash_path, "SPLASH.RAW") == 0);

    /* A '#RRGGBB' value is a colour, not a comment; a trailing comment is. */
    {
        const char *line = "THEME_ACCENT=#123456 # not a comment victim\n";
        assert(boot_cfg_parse((const u8 *)line, (u32)strlen(line), &cfg));
        assert(cfg.theme.accent == THEME_BGR555(0x12u, 0x34u, 0x56u));
    }

    /* Malformed values and over-long splash paths keep the defaults. */
    {
        const char *bad =
            "THEME_ACCENT=#ZZZ\r\n"
            "THEME_TEXT=12\r\n"
            "THEME_BG=0x12345\r\n"
            "SPLASH_IMAGE=NDS/TOO/LONG/SPLASH.RAW\r\n";
        assert(boot_cfg_parse((const u8 *)bad, (u32)strlen(bad), &cfg));
        assert(cfg.theme.accent == THEME_ACCENT_BGR555_DEFAULT);
        assert(cfg.theme.text == THEME_TEXT_BGR555_DEFAULT);
        assert(cfg.theme.bg == THEME_BG_BGR555_DEFAULT);
        assert(!cfg.theme.has_splash);
    }

    /* Unknown keys are still ignored and do not disturb the theme. */
    {
        const char *unknown = "FOO=BAR\nTHEME_ACCENT=#FFFFFF\n";
        assert(boot_cfg_parse((const u8 *)unknown, (u32)strlen(unknown),
                              &cfg));
        assert(cfg.theme.accent == 0x7FFFu);
    }
}

/* ------------------------------------------------------------------ */
/* Splash validation + blit                                             */
/* ------------------------------------------------------------------ */

static void test_splash_check(void)
{
    assert(theme_splash_check("SPLASH.RAW", THEME_SPLASH_BYTES)
           == THEME_SPLASH_OK);
    assert(theme_splash_check("splash.raw", THEME_SPLASH_BYTES)
           == THEME_SPLASH_OK);
    assert(theme_splash_check("SPLASH.BIN", THEME_SPLASH_BYTES)
           == THEME_SPLASH_OK);

    assert(theme_splash_check("SPLASH.RAW", THEME_SPLASH_BYTES - 2u)
           == THEME_SPLASH_ERR_SIZE);
    assert(theme_splash_check("SPLASH.RAW", 0u) == THEME_SPLASH_ERR_SIZE);
    assert(theme_splash_check("SPLASH.BMP", THEME_SPLASH_BYTES)
           == THEME_SPLASH_ERR_KIND);
    assert(theme_splash_check("SPLASH", THEME_SPLASH_BYTES)
           == THEME_SPLASH_ERR_KIND);
    assert(theme_splash_check(NULL, 0u) == THEME_SPLASH_ERR_ARG);

    assert(strcmp(theme_splash_status_text(THEME_SPLASH_OK), "OK") == 0);
    assert(strcmp(theme_splash_status_text(THEME_SPLASH_NONE), "NONE") == 0);
    assert(strcmp(theme_splash_status_text(THEME_SPLASH_ERR_SIZE), "SIZE") == 0);
    assert(theme_splash_status_text((theme_splash_status_t)99) != NULL);
}

static void test_splash_blit(void)
{
    /* Two 15-bit BGR555 pixels: blue (0x7C00) and white (0x7FFF). */
    const u8 src[4] = { 0x00u, 0x7Cu, 0xFFu, 0x7Fu };
    u16 dst[2] = { 0u, 0u };

    theme_splash_blit(src, (vu16 *)dst, 2u);
    assert(dst[0] == (u16)(0x7C00u | THEME_SPLASH_ALPHA));
    assert(dst[1] == (u16)(0x7FFFu | THEME_SPLASH_ALPHA));

    /* The alpha bit is forced even for a black (0x0000) source pixel. */
    {
        const u8 black[2] = { 0x00u, 0x00u };
        u16 out = 0xFFFFu;

        theme_splash_blit(black, (vu16 *)&out, 1u);
        assert(out == THEME_SPLASH_ALPHA);
    }

    theme_splash_blit(NULL, (vu16 *)dst, 2u);   /* no crash */
    theme_splash_blit(src, NULL, 2u);
}

/* ------------------------------------------------------------------ */
/* Splash read / stage against build/sdcard.img                        */
/* ------------------------------------------------------------------ */

static u8 *g_img;
static u32 g_img_sectors;

static bool img_read(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    (void)ctx;

    if (lba >= g_img_sectors) {
        return false;
    }
    memcpy(out, g_img + (size_t)lba * FAT_SECTOR_SIZE, FAT_SECTOR_SIZE);
    return true;
}

static void test_splash_from_image(void)
{
    fat_volume_t vol;
    theme_config_t cfg;
    u8 *buf;
    u32 got = 0u;

    assert(fat_mount(&vol, img_read, NULL));

    /* Argument / missing-file guards. */
    buf = (u8 *)malloc(THEME_SPLASH_BYTES);
    assert(buf != NULL);
    assert(theme_splash_read(&vol, "SPLASH.RAW", buf, 16u, &got)
           == THEME_SPLASH_ERR_ARG);
    assert(theme_splash_read(NULL, "SPLASH.RAW", buf, THEME_SPLASH_BYTES, &got)
           == THEME_SPLASH_ERR_ARG);
    assert(theme_splash_read(&vol, NULL, buf, THEME_SPLASH_BYTES, &got)
           == THEME_SPLASH_ERR_ARG);
    assert(theme_splash_read(&vol, "MISSING.RAW", buf, THEME_SPLASH_BYTES, &got)
           == THEME_SPLASH_ERR_IO);

    /* The generated image carries a valid SPLASH.RAW of the exact size. */
    assert(theme_splash_read(&vol, "SPLASH.RAW", buf, THEME_SPLASH_BYTES, &got)
           == THEME_SPLASH_OK);
    assert(got == THEME_SPLASH_BYTES);

    /* Deterministic gradient: top-left = blue (0x7C00), bottom-right =
     * 31 | (31 << 5) = 0x03FF. */
    assert(((u16)buf[0] | ((u16)buf[1] << 8)) == 0x7C00u);
    {
        u32 off = (THEME_SPLASH_PIXELS - 1u) * 2u;
        u16 last = (u16)((u16)buf[off] | ((u16)buf[off + 1u] << 8));

        assert(last == 0x03FFu);
    }

    /* Feed the staging path through theme_load_splash (no MMIO). */
    theme_defaults(&cfg);
    assert(theme_load_splash(&vol, &cfg, &got) == THEME_SPLASH_NONE);
    assert(theme_splash_pixels() == NULL);

    cfg.has_splash = true;
    strcpy(cfg.splash_path, "SPLASH.RAW");
    assert(theme_load_splash(&vol, &cfg, &got) == THEME_SPLASH_OK);
    assert(got == THEME_SPLASH_BYTES);
    {
        const u8 *p = theme_splash_pixels();

        assert(p != NULL);
        assert(p[0] == 0x00u && p[1] == 0x7Cu);
    }

    free(buf);
}

/* ------------------------------------------------------------------ */

static int load_image(const char *path)
{
    FILE *fh;
    long size;

    fh = fopen(path, "rb");
    if (fh == NULL) {
        return -1;
    }
    if (fseek(fh, 0L, SEEK_END) != 0) {
        fclose(fh);
        return -1;
    }
    size = ftell(fh);
    if (size <= 0 || (size % FAT_SECTOR_SIZE) != 0) {
        fclose(fh);
        return -1;
    }
    if (fseek(fh, 0L, SEEK_SET) != 0) {
        fclose(fh);
        return -1;
    }
    g_img = (u8 *)malloc((size_t)size);
    if (g_img == NULL) {
        fclose(fh);
        return -1;
    }
    if (fread(g_img, 1u, (size_t)size, fh) != (size_t)size) {
        free(g_img);
        g_img = NULL;
        fclose(fh);
        return -1;
    }
    fclose(fh);
    g_img_sectors = (u32)(size / FAT_SECTOR_SIZE);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "build/sdcard.img";

    test_rgb_to_bgr555();
    test_parse_color();
    test_defaults();
    test_boot_cfg_theme();
    test_splash_check();
    test_splash_blit();

    if (load_image(path) != 0) {
        printf("test_theme: skipping SD-image checks (%s not found)\n", path);
    } else {
        test_splash_from_image();
        free(g_img);
        g_img = NULL;
    }

    puts("test_theme: all assertions passed");
    return 0;
}
