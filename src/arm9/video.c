/*
 * src/arm9/video.c
 *
 * ARM9 2D engine / VRAM mapping. See include/arm9/video.h.
 */
#include "arm9/video.h"
#include "common/dsi_hw.h"
#include "common/mem.h"

u32 arm9_vram_bank_word(u8 a, u8 b, u8 c, u8 d)
{
    return (u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24);
}

/*
 * @ref FUN_06025b30 (0x06025B30).
 *
 * NOTE: banks H and I live at 0x04000248/0x04000249; the gap at 0x40247 is the
 * WRAMCNT register and is intentionally not touched.
 */
void arm9_vram_set_all_banks(u8 value)
{
    REG8(VRAMCNT_A) = value;
    REG8(VRAMCNT_B) = value;
    REG8(VRAMCNT_C) = value;
    REG8(VRAMCNT_D) = value;
    REG8(VRAMCNT_E) = value;
    REG8(VRAMCNT_F) = value;
    REG8(VRAMCNT_G) = value;
    REG8(VRAMCNT_H) = value;
    REG8(VRAMCNT_I) = value;
}

/* @ref FUN_0602555c (0x0602555C). */
void arm9_vram_configure_boot(void)
{
    REG32(VRAMCNT_A) = VRAMCNT_BOOT_ABCD;
}

/* @ref entry (0x0602572C). */
void arm9_video_reset(void)
{
    /* Force blank on the main engine, then clear DISPSTAT..eff regs. */
    REG32(DISPCNT) = DISPCNT_FORCED_BLANK;
    dsi_memset((void *)(uintptr_t)(DISPCNT + 4u), 0, 0x4Au);

    /* Same for the sub engine. */
    REG32(DISPCNT_SUB) = DISPCNT_FORCED_BLANK;
    dsi_memset((void *)(uintptr_t)(DISPCNT_SUB + 4u), 0, 0x4Au);

    /* Reset the text BG2 control/scroll used by the menu background. */
    REG32(DISPCNT + 0x20u) = 0x100u;
    REG32(DISPCNT_SUB + 0x20u) = 0x100u;
    REG32(DISPCNT + 0x24u) = 0x01000000u;
    REG32(DISPCNT_SUB + 0x24u) = 0x01000000u;
}

/* @ref entry (0x0602572C). */
void arm9_video_power_on(void)
{
    /*
     * POSTFLG is 8-bit: a 32-bit store here would clobber HALTCNT
     * (0x04000301) and can halt the CPU (see dsi_hw.h). POWCNT1 is 16-bit.
     */
    REG8(POSTFLG) = 0x01u;
    REG16(POWCNT1) = (u16)POWCNT1_BOOT_VALUE;
}

void arm9_video_set_backdrop(u16 color)
{
    REG16(PALETTE_MAIN) = color;
}

/*
 * @ref entry (0x0602572C).
 *
 * Both MASTER_BRIGHT registers are 16-bit (GBATEK "Master Brightness"). Direct
 * boot and the DSi firmware can leave the engine fades at a full white value,
 * so the screen stays blank-white even when DISPCNT/BG0 are programmed
 * correctly. MASTER_BRIGHT_NORMAL (0x0000) restores the un-faded picture.
 */
void arm9_video_set_master_brightness(u16 value)
{
    REG16(MASTER_BRIGHT) = value;
    REG16(MASTER_BRIGHT_SUB) = value;
}

/* @ref FUN_06026244 (0x06026244). */
void arm9_video_clear_palettes(void)
{
    dsi_memset((void *)(uintptr_t)PALETTE_MAIN, 0, PALETTE_BYTES);
    dsi_memset((void *)(uintptr_t)PALETTE_SUB, 0, PALETTE_BYTES);
}

/*
 * Bring-up heartbeat (diagnostic). See include/arm9/video.h.
 *
 * This runs before any other video/console/VRAM setup: it only needs the 2D
 * engine powered and the display un-blanked for the backdrop colour to show.
 * Every store below is width-correct (palette RAM is 16-bit, POWCNT1 is 16-bit).
 */
void arm9_video_boot_heartbeat(void)
{
    /* 1. Engine A backdrop (BG palette entry 0) -> bright red. */
    REG16(PALETTE_MAIN) = ARM9_HEARTBEAT_RED;

    /* 2. Kill any residual full-white master-brightness fade. */
    REG16(MASTER_BRIGHT) = MASTER_BRIGHT_NORMAL;

    /* 3. Ensure the 2D engine / LCD are powered (16-bit POWCNT1). */
    REG16(POWCNT1) = (u16)POWCNT1_BOOT_VALUE;

    /* 4. Display on, mode 0, no BG layers, forced-blank clear: backdrop shows. */
    REG32(DISPCNT) = 0u;
}
