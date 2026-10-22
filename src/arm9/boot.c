/*
 * src/arm9/boot.c
 *
 * ARM9 stage-1 loader and stage-2 initialisation.
 *
 * Reconstructed from the reference image:
 *   @ref crt0  (0x02200800)  stage-1 relocation stub
 *   @ref entry (0x0602572C)  stage-2 boot sequence
 *
 * See include/arm9/boot.h and docs/hardware_notes.md §ARM9 image layout.
 */
#include "arm9/boot.h"
#include "arm9/console.h"
#include "arm9/dma.h"
#include "arm9/input.h"
#include "arm9/ipc.h"
#include "arm9/irq.h"
#include "arm9/menu.h"
#include "arm9/mpu.h"
#include "arm9/scfg.h"
#include "arm9/video.h"
#include "common/cache.h"
#include "common/dsi_hw.h"
#include "common/ipc_protocol.h"
#include "common/lz77.h"
#include "common/mem.h"

/*
 * Boot-handshake timing (poll-loop iterations). The ARM9 runs at ~134 MHz, so
 * 0x04000000 iterations is a comfortable multi-second wait before the status
 * indicator latches to "failure".
 */
#define ARM9_BOOT_IPC_TIMEOUT   0x04000000u
#define ARM9_BOOT_IPC_RESEND    0x00100000u   /* power of two */

/* ------------------------------------------------------------------ */
/* Stage 1 — relocation / extraction                                   */
/* ------------------------------------------------------------------ */

u32 arm9_load_segments(const arm9_segment_t *segments, u32 count)
{
    u32 done;

    for (done = 0u; done < count; done++) {
        const arm9_segment_t *seg = &segments[done];

        if (seg->compressed) {
            u32 produced = nds_lz77_decompress((const u8 *)seg->src,
                                               seg->src_size,
                                               (u8 *)seg->dst,
                                               seg->size);
            if (produced != seg->size) {
                return done;
            }
        } else {
            dsi_memcpy((void *)seg->dst, (const void *)seg->src, seg->size);
        }
    }
    return done;
}

/* The exact job list the reference crt0 executes. */
static const arm9_segment_t arm9_ref_segments[] = {
    { ARM9_REF_GFX_SRC, ARM9_REF_GFX_SIZE, ARM9_REF_GFX_DST,  ARM9_REF_GFX_SIZE,  false },
    { ARM9_REF_LZ_SRC,  ARM9_REF_LZ_SIZE,  ARM9_REF_LZ_TMP,   ARM9_REF_CODE_SIZE, true  },
};

/*
 * @ref crt0 (0x02200800).
 *
 * After the two extraction jobs above, the reference copies the expanded code
 * from ARM9_REF_LZ_TMP to ARM9_REF_CODE_DST and jumps there. We fold that copy
 * into the segment list at run time so the layout stays data-driven.
 */
void arm9_stage1_bootstrap(void)
{
    u32 done;

    /* 1. Cache + CP15. */
    cache_flush_all();

    /* 2. Peer handshake: wait for the ARM7 to present nibble 0xE. */
    arm9_ipc_sync_wait(0xEu);

    /* 3. System configuration (extended RAM + clock mask). */
    scfg_apply_boot_config();

    /* 4. VRAM bank mapping used by the stage-2 image. */
    REG32(VRAMCNT_A) = ARM9_REF_VRAMCNT_WORD;
    REG32(VRAMCNT_E) = ARM9_REF_VRAMCNT_EHI;
    REG32(VRAMCNT_H) = 0u;

    /* 5. Misc VRAM word (observed write of 0xFF). */
    REG32(ARM9_REF_VRAM_MISC) = 0x000000FFu;

    /* 6/7. Graphics copy + compressed-code extraction. */
    done = arm9_load_segments(arm9_ref_segments, ARRAY_SIZE(arm9_ref_segments));
    if (done != ARRAY_SIZE(arm9_ref_segments)) {
        /* Malformed payload: park rather than jump into garbage. */
        for (;;) {
        }
    }

    /* 8. Relocate the expanded code into its run-time VRAM address. */
    dsi_memcpy((void *)(uintptr_t)ARM9_REF_CODE_DST,
               (const void *)(uintptr_t)ARM9_REF_LZ_TMP, ARM9_REF_CODE_SIZE);
    cache_flush_all();
}

/* ------------------------------------------------------------------ */
/* Stage 2 — initialisation                                            */
/* ------------------------------------------------------------------ */

void arm9_irq_handler(void)
{
    /* No ARM9 interrupt sources are enabled during the boot smoke test
     * (IE is left at 0). The reference installs a handler at the top of DTCM
     * via FUN_06025c20 once the menu subsystems are up. */
}

/* ------------------------------------------------------------------ */
/* Boot policy                                                         */
/* ------------------------------------------------------------------ */

boot_policy_t boot_resolve_policy(u16 raw_keyinput)
{
    u16 held = input_normalize_keys(raw_keyinput);

    if ((held & (INPUT_B | INPUT_SELECT)) != 0u) {
        return BOOT_POLICY_MENU;
    }
    return BOOT_POLICY_AUTOBOOT;
}

/*
 * @ref entry (0x0602572C) — boot sequence.
 *
 * Bring up interrupts, DMA, the 2D engines, the text console and IPC, then
 * hand control to the interactive diagnostic menu (src/arm9/menu.c), which
 * never returns. The menu performs the ARM7 IPC handshake as part of its
 * self-test, so the previous backdrop-colour smoke test is no longer needed.
 */
void arm9_main(void)
{
    /*
     * 0. Bring-up heartbeat (diagnostic). MUST stay the first statement: it
     *    paints the Engine A backdrop bright red before any other video, VRAM
     *    or console setup. If the panel turns red instantly, src/arm9/crt0.s
     *    completed and the hang (if any) is in the C init path below; if it
     *    stays white, execution is stuck in crt0.s before reaching C.
     */
    arm9_video_boot_heartbeat();

    /* 1. Full reset state: interrupts off. */
    (void)arm9_irq_disable(0xFFFFFFFFu);
    REG16(IRQ_IME) = 0u;

    /*
     * 2. The protection unit and TCM are programmed by the boot firmware (and
     *    by melonDS' direct-boot path); src/arm9/crt0.s relocates DTCM to
     *    0x0B000000 before using the stacks. Re-running the reference MPU block
     *    would move DTCM to 0x03000000 and invalidate those stacks, so it is
     *    opt-in. See docs/hardware_notes.md §10.
     */
#ifdef ARM9_BOOT_CONFIGURE_MPU
    arm9_mpu_configure();
#endif

    cache_flush_all();

    /*
     * 2b. Apply the reference SCFG configuration and map the NWRAM banks.
     *     scfg_init() verifies TWL mode first, so on a legacy DS it is a
     *     complete no-op (avoiding writes to the DSi-only 0x04004000 page).
     */
    (void)scfg_init();

    /* 3. DMA register blocks are cleared before any user of them. */
    arm9_dma_reset();
    arm9_dma_aux_reset();

    /* 4. Power and bring up the 2D engines. */
    arm9_video_power_on();
    arm9_video_reset();
    /*
     * 4b. Clear any leftover master-brightness fade. Direct boot / DSi
     *     firmware can leave MASTER_BRIGHT at full white, which blanks both
     *     screens even with BG0 correctly configured.
     */
    arm9_video_set_master_brightness(MASTER_BRIGHT_NORMAL);
    arm9_vram_configure_boot();
    arm9_video_clear_palettes();

    /* 5. IPC: enable the FIFO, clear pending flags (IE stays 0 -> polling). */
    arm9_ipc_fifo_init();
    REG32(IRQ_IF) = 0xFFFFFFFFu;
    arm9_irq_unmask_all();

    /* 6. Text console + interactive diagnostics menu (never returns). */
    console_init();
    console_select(CONSOLE_MAIN);
    console_puts(0, 0, CONSOLE_PAL_CYAN, "OPENLAUNCH TWL BOOTLOADER");
    console_puts(0, 1, CONSOLE_PAL_WHITE, "Starting diagnostics...");

    menu_run();
}
