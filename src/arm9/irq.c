/*
 * src/arm9/irq.c
 *
 * ARM9 interrupt-controller accessors. See include/arm9/irq.h.
 */
#include "arm9/irq.h"
#include "common/dsi_hw.h"

/* @ref FUN_0602bfb4 (0x0602BFB4) — IE |= mask. */
u32 arm9_irq_enable(u32 mask)
{
    u32 old = REG32(IRQ_IE);

    REG32(IRQ_IE) = old | mask;
    return old;
}

/* @ref FUN_0602bfc8 (0x0602BFC8) — IE &= ~mask. */
u32 arm9_irq_disable(u32 mask)
{
    u32 old = REG32(IRQ_IE);

    REG32(IRQ_IE) = old & ~mask;
    return old;
}

/* @ref FUN_0602bfdc (0x0602BFDC) — IE2 &= ~mask. */
u32 arm9_irq_ie2_disable(u32 mask)
{
    u32 old = REG32(IRQ_IE2);

    REG32(IRQ_IE2) = old & ~mask;
    return old;
}

void arm9_irq_ack(u32 mask)
{
    REG32(IRQ_IF) = mask;
}

u32 arm9_irq_pending(void)
{
    return REG32(IRQ_IF) & REG32(IRQ_IE);
}

u32 arm9_irq_save(void)
{
    u32 ime = REG16(IRQ_IME);

    REG16(IRQ_IME) = 0u;
    return ime;
}

void arm9_irq_restore(u32 ime)
{
    REG16(IRQ_IME) = ime;
}

/*
 * @ref FUN_06025bf4 / FUN_06025c20 — the reference installs a handler at the
 * top of DTCM and then sets IME = 1.
 */
void arm9_irq_unmask_all(void)
{
    REG16(IRQ_IME) = 1u;
}
