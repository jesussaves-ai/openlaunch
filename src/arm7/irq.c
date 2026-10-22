/*
 * src/arm7/irq.c
 *
 * ARM7 interrupt-master control and dispatch. See include/arm7/irq.h.
 */
#include "arm7/irq.h"
#include "common/dsi_hw.h"

/* Number of individually dispatchable IE bits we track. */
#define IRQ_TABLE_BITS 32u

static irq_handler_t s_handlers[IRQ_TABLE_BITS];

/*
 * @ref FUN_00020ae4  (address 0x020AE4)
 *   Returns the old IME and clears it.
 */
u32 irq_disable_all(void)
{
    u32 old = REG16(IRQ_IME);
    REG16(IRQ_IME) = 0u;
    return old;
}

/* @ref FUN_00020af8  (address 0x020AF8) */
void irq_restore(u32 ime)
{
    REG16(IRQ_IME) = ime;
}

/* @ref FUN_00020b04  (address 0x020B04) — IE |= mask. */
void irq_enable_bits(u32 mask)
{
    REG32(IRQ_IE) = REG32(IRQ_IE) | mask;
}

void irq_disable_bits(u32 mask)
{
    REG32(IRQ_IE) = REG32(IRQ_IE) & ~mask;
}

/* Writing a 1 to a pending IF bit acknowledges it. */
void irq_ack_bits(u32 mask)
{
    REG32(IRQ_IF) = mask;
}

void irq_register(u32 mask, irq_handler_t handler)
{
    u32 bit;

    for (bit = 0u; bit < IRQ_TABLE_BITS; bit++) {
        if ((mask & BIT(bit)) != 0u) {
            s_handlers[bit] = handler;
        }
    }
}

/*
 * @ref FUN_00027028  (address 0x027028)
 *   Secondary TWL interrupt controller: collect pending & enabled, acknowledge,
 *   then run the per-device handlers.
 */
static void twl_irq_dispatch(void)
{
    u16 pending;

    for (;;) {
        pending = (u16)(REG16(TWL_IRQ_IF) & REG16(TWL_IRQ_IE));
        if (pending == 0u) {
            break;
        }
        REG16(TWL_IRQ_IF) = pending;   /* acknowledge */

        if ((pending & BIT(0)) != 0u) {
            /* TWL device handler slot 0. */
        }
        if ((pending & BIT(1)) != 0u) {
            /* TWL device handler slot 1. */
        }
    }
}

void irq_dispatch(void)
{
    u32 pending = REG32(IRQ_IF) & REG32(IRQ_IE);

    if (pending != 0u) {
        u32 bit;

        REG32(IRQ_IF) = pending;   /* acknowledge all pending-and-enabled */

        for (bit = 0u; bit < IRQ_TABLE_BITS; bit++) {
            if ((pending & BIT(bit)) != 0u && s_handlers[bit] != NULL) {
                s_handlers[bit]();
            }
        }
    }

    twl_irq_dispatch();
}
