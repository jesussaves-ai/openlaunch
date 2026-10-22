/*
 * src/arm9/dma.c
 *
 * ARM9 DMA helpers. See include/arm9/dma.h.
 */
#include "arm9/dma.h"
#include "arm9/scfg.h"
#include "common/dsi_hw.h"
#include "common/mem.h"

/* @ref entry (0x0602572C). */
void arm9_dma_reset(void)
{
    /* Disable each channel before clearing its registers. */
    for (u32 ch = 0u; ch < DMA_CHANNELS; ch++) {
        REG32(DMA_BASE + ch * DMA_CH_STRIDE + 0x0Cu) = 0u;
    }
    dsi_memset((void *)(uintptr_t)DMA_BASE, 0, DMA_BLOCK_SIZE);
}

/*
 * @ref FUN_0602bf88 (0x0602BF88).
 *
 * The auxiliary DMA block at 0x04004100 is the DSi New-DMA (NDMA) engine and
 * only exists on a DSi. On a legacy DS the write is ignored by the bus but
 * melonDS logs every access as "unknown ARM9 IO", so the clear is gated on the
 * runtime TWL probe.
 */
void arm9_dma_aux_reset(void)
{
    if (!scfg_is_twl_mode()) {
        return;
    }
    dsi_memset((void *)(uintptr_t)DMA_AUX_BASE, 0, DMA_AUX_SIZE);
}

void arm9_dma_start(u32 channel, const void *src, void *dst, u32 control)
{
    if (channel >= DMA_CHANNELS) {
        return;
    }

    {
        u32 base = DMA_BASE + channel * DMA_CH_STRIDE;

        REG32(base + 0x08u) = 0u;                            /* disable   */
        REG32(base + 0x00u) = (u32)(uintptr_t)src;
        REG32(base + 0x04u) = (u32)(uintptr_t)dst;
        REG32(base + 0x08u) = control;
    }
}
