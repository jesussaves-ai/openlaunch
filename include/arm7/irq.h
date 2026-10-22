/*
 * include/arm7/irq.h
 *
 * ARM7 interrupt-master control.
 *
 * Derived from the reference routines FUN_00020ae4, FUN_00020af8,
 * FUN_00020b04 and FUN_00020cb0 (see docs/hardware_notes.md §2).
 */
#ifndef DSI_ARM7_IRQ_H
#define DSI_ARM7_IRQ_H

#include "../common/types.h"

/*
 * Enter a critical section: return the current IME value and clear it.
 *
 * @ref FUN_00020ae4  (address 0x020AE4)
 *   r0 = old IME.
 *   Hardware side effects: writes IME = 0 (mask all IRQ sources).
 */
u32 irq_disable_all(void);

/*
 * Leave a critical section by restoring a previously saved IME value.
 *
 * @ref FUN_00020af8  (address 0x020AF8)
 *   r0 = IME value.
 */
void irq_restore(u32 ime);

/*
 * Enable (unmask) one or more IRQ sources by OR-ing them into IE.
 *
 * @ref FUN_00020b04  (address 0x020B04)
 *   r0 = bit mask, e.g. IRQ_TIMER3.
 */
void irq_enable_bits(u32 mask);

/*
 * Disable (mask) one or more IRQ sources.
 */
void irq_disable_bits(u32 mask);

/*
 * Acknowledge one or more pending IRQ sources by writing IF.
 *
 * @ref FUN_00020cb0  (address 0x020CB0) writes IF = 0x40 for Timer 3.
 *   On this hardware, writing a 1 to an IF bit clears it.
 */
void irq_ack_bits(u32 mask);

/*
 * Top-level ARM7 IRQ dispatcher.
 *
 * @ref FUN_00027028  (address 0x027028)
 *   Reads the TWL secondary controller's pending & enabled mask, acknowledges
 *   it, then runs the per-source handlers. The public DSi IRQ/BPTWL handlers
 *   are registered with irq_register().
 */
void irq_dispatch(void);

typedef void (*irq_handler_t)(void);

/*
 * Register a handler for a single IE bit (BIT(n)). Passing NULL clears it.
 * The handler table lives in ARM7 WRAM and must stay cache-coherent before the
 * first IRQ is enabled.
 */
void irq_register(u32 mask, irq_handler_t handler);

#endif /* DSI_ARM7_IRQ_H */
