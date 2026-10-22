/*
 * include/arm9/video.h
 *
 * ARM9 2D video engine and VRAM bank mapping.
 *
 * Translated from the reference ARM9 image:
 *   @ref entry        (0x0602572C)   DISPCNT / sub-engine / BG2 reset
 *   @ref FUN_0602555c (0x0602555C)   VRAMCNT_A..D = 0x81,0x89,0x84,0x8A
 *   @ref FUN_06025b30 (0x06025B30)   VRAMCNT_A..I = value
 *   @ref FUN_06026244 (0x06026244)   palette bank reset
 */
#ifndef DSI_ARM9_VIDEO_H
#define DSI_ARM9_VIDEO_H

#include "../common/types.h"

/* ---- Main 2D engine (ARM9) ---- */
#define DISPCNT          0x04000000u
#define DISPSTAT         0x04000004u
#define VCOUNT           0x04000006u
#define BG0CNT           0x04000008u
#define BG1CNT           0x0400000Au
#define BG2CNT           0x0400000Cu
#define BG3CNT           0x0400000Eu
#define BG0HOFS          0x04000010u
#define BG0VOFS          0x04000012u
#define BG2HOFS          0x04000020u
#define BG2VOFS          0x04000022u

/* ---- Sub 2D engine (ARM9) ---- */
#define DISPCNT_SUB      0x04001000u
#define BG0CNT_SUB       0x04001008u
#define BG0HOFS_SUB      0x04001010u
#define BG0VOFS_SUB      0x04001012u
#define BG2CNT_SUB       0x04001020u
#define BG2HOFS_SUB      0x04001010u

/*
 * ---- Master brightness (fade) ----
 * MASTER_BRIGHT is a 16-bit mixer register on each engine. Direct boot and the
 * DSi firmware can leave it at a full white fade, which renders both screens
 * blank-white even when DISPCNT/BG0 are programmed correctly. Write
 * MASTER_BRIGHT_NORMAL (0x0000) to restore the un-faded picture.
 */
#define MASTER_BRIGHT      0x0400006Cu   /* 16-bit, Engine A (top)  */
#define MASTER_BRIGHT_SUB  0x0400106Cu   /* 16-bit, Engine B (bottom) */
#define MASTER_BRIGHT_NORMAL 0x0000u     /* no fade, normal brightness */

/* ---- Palette RAM ---- */
#define PALETTE_MAIN     0x05000000u   /* 256 entries (BG)                 */
#define PALETTE_SUB      0x05000400u
#define PALETTE_ENTRIES  256u
#define PALETTE_BYTES    (PALETTE_ENTRIES * 2u)   /* 0x200 bytes          */

/*
 * VRAM bank control registers (VRAMCNT_A..I). A single 32-bit store at
 * 0x04000240 sets banks A..D; the reference uses 0x8A848981, i.e.
 * A=0x81, B=0x89, C=0x84, D=0x8A.
 */
#define VRAMCNT_A        0x04000240u
#define VRAMCNT_B        0x04000241u
#define VRAMCNT_C        0x04000242u
#define VRAMCNT_D        0x04000243u
#define VRAMCNT_E        0x04000244u
#define VRAMCNT_F        0x04000245u
#define VRAMCNT_G        0x04000246u
#define VRAMCNT_H        0x04000248u
#define VRAMCNT_I        0x04000249u

#define VRAMCNT_BOOT_ABCD  0x8A848981u

/*
 * ---- Power control -------------------------------------------------
 * POSTFLG (0x04000300) is an 8-bit register; POWCNT1 (0x04000304) is a
 * 16-bit register. Their addresses, widths and bit fields are declared in
 * common/dsi_hw.h so the width is stated exactly once.
 */

/*
 * Backdrop / boot-status colours (BGR555). Palette entry 0 (0x05000000) is the
 * backdrop shown behind everything; the smoke test uses it as a status LED.
 */
#define ARM9_COLOR_STATUS_WAIT 0x0400u   /* dark blue: waiting for ARM7     */
#define ARM9_COLOR_STATUS_OK   0x03E0u   /* green: ARM7 answered            */
#define ARM9_COLOR_STATUS_FAIL 0x001Fu   /* red: handshake timed out        */

/*
 * DISPCNT display mode / blanking (GBATEK "DISPCNT", libnds MODE_* constants).
 *
 * Bits 16-17 select the display source and are NOT optional: 0 = "screen off"
 * (the panel/lcd shows solid white), 1 = normal 2D/3D composition, 2 = VRAM
 * display, 3 = main-memory FIFO. libnds always sets bits 16-17 = 1 via
 * MODE_0_2D (0x10000); leaving them 0 makes the engine output white no matter
 * how the BG/OBJ layers are programmed.
 *
 * Forced blank is a separate, independent control at bit 7 (like the GBA);
 * it overrides the display mode and also outputs white.
 */
#define DISPCNT_DISPLAY_2D   (1u << 16)  /* bits16-17=1: normal 2D/3D display */
#define DISPCNT_FORCED_BLANK BIT(7)      /* real forced-blank bit             */

/*
 * Pure packer matching the reference's single-store VRAM configuration:
 * word = a | (b << 8) | (c << 16) | (d << 24). Host-testable.
 */
u32 arm9_vram_bank_word(u8 a, u8 b, u8 c, u8 d);

/* @ref FUN_06025b30 — write the same value to all nine VRAM bank registers. */
void arm9_vram_set_all_banks(u8 value);

/* @ref FUN_0602555c — apply the boot mapping (A..D = 0x81,0x89,0x84,0x8A). */
void arm9_vram_configure_boot(void);

/* @ref entry — force-blank, clear both engines' registers, reset BG2. */
void arm9_video_reset(void);

/*
 * @ref entry — power on the LCD and both 2D engines (POWCNT1/POWCNT2).
 * Must run before anything is expected on screen.
 */
void arm9_video_power_on(void);

/*
 * @ref entry (0x0602572C) — write the master-brightness fade on both engines.
 * The registers are 16-bit; pass MASTER_BRIGHT_NORMAL (0x0000) at boot so a
 * leftover white fade from direct boot / DSi firmware cannot blank the screen.
 */
void arm9_video_set_master_brightness(u16 value);

/* Write palette entry 0 (the backdrop) with a BGR555 colour. */
void arm9_video_set_backdrop(u16 color);

/*
 * Bring-up heartbeat. Called as the *first* statement of arm9_main() so that a
 * visible colour change proves execution reached C code (i.e. src/arm9/crt0.s
 * completed) before any other subsystem is touched.
 *
 * Paints the Engine A backdrop bright red, removes any leftover white
 * master-brightness fade, powers the 2D engine and enables the display with no
 * BG layers enabled (so the backdrop is what shows).
 *
 * NOTE: the task description referred to "REG_BG_COLOR" at 0x04000050. On the
 * DS that address is BLDCNT (alpha-blend control), not a colour register; the
 * Engine A backdrop colour is BG palette entry 0 at 0x05000000 (PALETTE_MAIN),
 * which is what this routine — and arm9_video_set_backdrop() — actually use.
 */
#define ARM9_HEARTBEAT_RED  0x001Fu   /* BGR555 pure red */
void arm9_video_boot_heartbeat(void);

/* @ref FUN_06026244 — clear the main and sub background palettes. */
void arm9_video_clear_palettes(void);

#endif /* DSI_ARM9_VIDEO_H */
