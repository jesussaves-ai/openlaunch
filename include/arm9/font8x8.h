/*
 * include/arm9/font8x8.h
 *
 * Clean-room 8x8 monochrome (1bpp) bitmap font for the bootloader console.
 *
 * The table covers the printable ASCII range 0x20..0x7F (96 glyphs). Each
 * glyph is 8 bytes: one byte per scanline, MSB = leftmost pixel. Glyphs are
 * drawn 5 pixels wide inside the 8-pixel cell, occupying bits 6..2, so the
 * tile packer in src/arm9/console.c can expand them to 4bpp with a simple
 * nibble split.
 *
 * This font was authored for this project (see src/arm9/font8x8.c); it is not
 * copied from any BIOS or third-party font. The reference image's graphics
 * blob (asm/arm9_gfx.bin) is documented separately in docs/hardware_notes.md.
 */
#ifndef DSI_ARM9_FONT8X8_H
#define DSI_ARM9_FONT8X8_H

#include "../common/types.h"

#define FONT8X8_FIRST_CHAR   0x20u
#define FONT8X8_LAST_CHAR    0x7Fu
#define FONT8X8_GLYPH_COUNT  96u   /* 0x20..0x7F inclusive */
#define FONT8X8_GLYPH_BYTES  8u    /* 8 scanlines           */

/* font8x8_basic[c - FONT8X8_FIRST_CHAR] is the bitmap for character c. */
extern const u8 font8x8_basic[FONT8X8_GLYPH_COUNT][FONT8X8_GLYPH_BYTES];

/*
 * Return the 8-byte bitmap for `c`, or the bitmap for '?' when `c` is outside
 * the supported range (so a corrupted string can never index out of bounds).
 */
const u8 *font8x8_glyph(u8 c);

#endif /* DSI_ARM9_FONT8X8_H */
