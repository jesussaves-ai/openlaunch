/*
 * include/arm9/splash_tux.h
 *
 * Built-in boot splash: a pixel-art penguin (plus the OpenLaunch wordmark)
 * rendered straight into the 256x192 15-bit BGR555 splash framebuffer.
 *
 * The splash machinery in include/arm9/theme.h already blits a raw bitmap into
 * the Engine-A BG2 framebuffer behind the text console (VRAM 0x06020000). A
 * user can override it with `SPLASH_IMAGE=...` in BOOT.CFG; when none is
 * configured we render this built-in image instead, so the boot screen is
 * never a bare black backdrop.
 *
 * Zero-allocation and MMIO-free: splash_tux_render() only writes the supplied
 * framebuffer, so it is covered on the host by tests/test_theme.c.
 */
#ifndef DSI_ARM9_SPLASH_TUX_H
#define DSI_ARM9_SPLASH_TUX_H

#include "../common/types.h"

/* Source pixel-art grid (one character per pixel, see splash_tux.c). */
#define SPLASH_TUX_SRC_W 24u
#define SPLASH_TUX_SRC_H 28u

/* Integer upscale applied to the source grid. */
#define SPLASH_TUX_SCALE 3u

#define SPLASH_TUX_W (SPLASH_TUX_SRC_W * SPLASH_TUX_SCALE)   /* 72 px */
#define SPLASH_TUX_H (SPLASH_TUX_SRC_H * SPLASH_TUX_SCALE)   /* 84 px */

/* Wordmark drawn under the penguin with the shared 8x8 font. */
#define SPLASH_TUX_TITLE    "OpenLaunch"
#define SPLASH_TUX_TITLE_SCALE 2u

/*
 * Render the built-in splash into `dst`, a THEME_SPLASH_BYTES
 * (256 * 192 * 2) little-endian BGR555 framebuffer:
 *   - the whole framebuffer is filled with `bg` (the theme backdrop);
 *   - the penguin is centred horizontally and sits above centre;
 *   - the wordmark is centred near the bottom in `accent`.
 * Pixel words carry bit 15 set (opaque), matching theme_splash_blit().
 * Returns false for a NULL destination or `cap` smaller than a full frame.
 * Pure.
 */
bool splash_tux_render(u8 *dst, u32 cap, u16 bg, u16 accent);

#endif /* DSI_ARM9_SPLASH_TUX_H */
