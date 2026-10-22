/*
 * include/arm9/boot.h
 *
 * ARM9 boot contract.
 *
 * The reference executable contains two distinct ARM9 stages (see
 * docs/hardware_notes.md §ARM9 image layout and PLAN.md):
 *
 *   Stage 1  @ 0x02200800   tiny crt0: cache/CP15 + SCFG + IPC handshake,
 *                           VRAMCNT setup, then copies the graphics segment
 *                           and LZ77-expands the code segment into VRAM.
 *   Stage 2  @ 0x0602572C   the real ARM9 program, running from VRAM, which
 *                           brings up the 2D engine, DMA, interrupts, IPC and
 *                           finally hands off to the payload at 0x023FEE00.
 *
 * `arm9_main` is the C entry the local crt0 (src/arm9/crt0.s) calls.
 */
#ifndef DSI_ARM9_BOOT_H
#define DSI_ARM9_BOOT_H

#include "../common/types.h"

/*
 * One relocation/extraction job performed by the stage-1 loader.
 *
 * When `compressed` is false the bytes are copied verbatim; otherwise the
 * source is a Nintendo LZ77 (type 0x10) stream expanded with
 * nds_lz77_decompress().
 */
typedef struct {
    uintptr_t src;    /* source address (ROM/RAM)                         */
    u32  src_size;    /* readable bytes at `src` (compressed or raw)      */
    uintptr_t dst;    /* destination address (VRAM/RAM)                   */
    u32  size;        /* expected uncompressed size                       */
    bool compressed;
} arm9_segment_t;

/* ------------------------------------------------------------------ */
/* Reference stage-1 layout (asm/arm9_unpack.json).                    */
/* ------------------------------------------------------------------ */
#define ARM9_REF_LOAD_ADDR      0x02200000u
#define ARM9_REF_ENTRY_ADDR     0x02200800u
#define ARM9_REF_GFX_SRC        0x022008F0u
#define ARM9_REF_GFX_DST        0x06020000u
#define ARM9_REF_GFX_SIZE       0x00003C90u
#define ARM9_REF_LZ_SRC         0x02204580u
#define ARM9_REF_LZ_SIZE        0x00006800u   /* compressed bytes remaining */
#define ARM9_REF_LZ_TMP         0x02100000u
#define ARM9_REF_CODE_DST       0x06023C90u
#define ARM9_REF_CODE_SIZE      0x0000931Cu
#define ARM9_REF_VRAM_MISC      0x06030004u
#define ARM9_REF_STAGE2_ENTRY   0x0602572Cu
#define ARM9_REF_VRAMCNT_WORD   0x80848981u
#define ARM9_REF_VRAMCNT_EHI    0x03000000u

/*
 * Execute a list of segment jobs. Returns the number of jobs completed, or 0
 * if a compressed segment failed to expand. Pure apart from the memory
 * accesses; host-testable with synthetic buffers.
 */
u32 arm9_load_segments(const arm9_segment_t *segments, u32 count);

/*
 * Stage-1 bootstrap, mirroring @ref crt0 (0x02200800): cache/CP15 setup, SCFG
 * programming, IPC sync, VRAM bank mapping and segment extraction. In the
 * reference this runs from 0x02200800 and tail-jumps to 0x0602572C.
 */
void arm9_stage1_bootstrap(void);

/*
 * ARM9 C entry point (called by src/arm9/crt0.s). Mirrors the reference
 * stage-2 @ref entry (0x0602572C): bring up interrupts, DMA, 2D and IPC, then
 * hand off to the loaded payload.
 */
void arm9_main(void) __attribute__((noreturn));

/* ARM9 interrupt dispatcher, called from the IRQ vector. */
void arm9_irq_handler(void);

#endif /* DSI_ARM9_BOOT_H */
