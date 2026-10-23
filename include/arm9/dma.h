/*
 * include/arm9/dma.h
 *
 * ARM9 DMA register blocks.
 *
 * Translated from the reference ARM9 image:
 *   @ref entry        (0x0602572C)  clear DMA0..3 (0x040000B0..0x040000EF)
 *   @ref FUN_0602bf88 (0x0602BF88)  clear the auxiliary DMA block at
 *                                   0x04004100 (0x74 bytes)
 *
 * the project conventions: buffers handed to DMA must not live in DTCM, and the data
 * cache must be flushed before a transfer starts.
 */
#ifndef DSI_ARM9_DMA_H
#define DSI_ARM9_DMA_H

#include "../common/types.h"

/* Main DMA controller (channels 0..3). */
#define DMA_BASE         0x040000B0u
#define DMA_CHANNELS     4u
#define DMA_CH_STRIDE    0x0Cu
#define DMA_BLOCK_SIZE   (DMA_CHANNELS * DMA_CH_STRIDE)   /* 0x30 bytes  */

/* Auxiliary DMA engine block cleared by the reference boot path. */
#define DMA_AUX_BASE     0x04004100u
#define DMA_AUX_SIZE     0x74u

/* One DMA channel's register triple. */
typedef struct {
    vu32 sad;    /* source address            */
    vu32 dad;    /* destination address       */
    vu32 cnt;    /* control (bit31 = enable)  */
} dma_channel_t;

/* Volatile view of the whole main DMA block. */
typedef struct {
    dma_channel_t ch[DMA_CHANNELS];
} dma_block_t;

/* @ref entry — disable and clear every main DMA channel. */
void arm9_dma_reset(void);

/* @ref FUN_0602bf88 — clear the auxiliary DMA block at 0x04004100. */
void arm9_dma_aux_reset(void);

/*
 * Program a main DMA channel. The caller is responsible for flushing the
 * source data cache beforehand and for choosing a non-DTCM buffer.
 */
void arm9_dma_start(u32 channel, const void *src, void *dst, u32 control);

#endif /* DSI_ARM9_DMA_H */
