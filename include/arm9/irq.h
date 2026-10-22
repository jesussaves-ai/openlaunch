/*
 * include/arm9/irq.h
 *
 * ARM9 interrupt-control helpers.
 *
 * Translated from the reference ARM9 image:
 *   @ref FUN_0602bfb4 (0x0602BFB4)  IE  |= mask
 *   @ref FUN_0602bfc8 (0x0602BFC8)  IE  &= ~mask
 *   @ref FUN_0602bfdc (0x0602BFDC)  IE2 &= ~mask
 *   @ref entry        (0x0602572C)  IME/IF handling in the boot sequence
 *
 * AGENTS.md §3: all interrupt state is touched through volatile registers so a
 * critical section cannot be reordered by the compiler.
 */
#ifndef DSI_ARM9_IRQ_H
#define DSI_ARM9_IRQ_H

#include "../common/types.h"

/*
 * Enter a critical section: snapshot IME and clear it. Returns the previous
 * IME value for arm9_irq_restore().
 */
u32 arm9_irq_save(void);

/* Restore IME to a value previously returned by arm9_irq_save(). */
void arm9_irq_restore(u32 ime);

/* Unmask (enable) the given bits in IE; returns the previous IE value. */
u32 arm9_irq_enable(u32 mask);

/* Mask (disable) the given bits in IE; returns the previous IE value. */
u32 arm9_irq_disable(u32 mask);

/* Mask the given bits in IE2; returns the previous IE2 value. */
u32 arm9_irq_ie2_disable(u32 mask);

/* Acknowledge pending IRQ flags by writing the mask to IF. */
void arm9_irq_ack(u32 mask);

/* Read the current pending-and-enabled set (IF & IE). */
u32 arm9_irq_pending(void);

/* Unmask all sources and enable the global IME bit (@ref FUN_06025bf4). */
void arm9_irq_unmask_all(void);

#endif /* DSI_ARM9_IRQ_H */
