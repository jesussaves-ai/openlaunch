/*
 * include/arm7/timer.h
 *
 * ARM7 hardware timers.
 *
 * Derived from FUN_00020cd0 and FUN_00020b04 (see docs/hardware_notes.md §2).
 */
#ifndef DSI_ARM7_TIMER_H
#define DSI_ARM7_TIMER_H

#include "../common/types.h"
#include "../common/dsi_hw.h"

/*
 * Program Timer 3 and enable its interrupt.
 *
 * @ref FUN_00020cd0  (address 0x020CD0)
 *   r0 = 16-bit reload value / packed control (reference uses 0xC2E66F).
 *   Writes TIMER3_DATA, then enables IRQ bit IRQ_TIMER3.
 *
 * @param reload16   value loaded into TIMER3_DATA
 * @param prescaler  0..3 (TIMER3_CNT prescaler field)
 * @param irq        true to unmask the Timer 3 interrupt
 */
void timer3_start(u16 reload16, u8 prescaler, bool irq);

/* Stop Timer 3 and mask its interrupt. */
void timer3_stop(void);

#endif /* DSI_ARM7_TIMER_H */
