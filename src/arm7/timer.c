/*
 * src/arm7/timer.c
 *
 * ARM7 hardware timers. See include/arm7/timer.h.
 */
#include "arm7/timer.h"
#include "arm7/irq.h"

/*
 * @ref FUN_00020cd0  (address 0x020CD0)
 *   The reference writes the packed word 0x00C2E66F to 0x0400010C, i.e.
 *   TIMER3_DATA = 0xE66F and TIMER3_CNT = 0xC2E6 (enable | irq | prescaler 2),
 *   then enables IRQ bit 6 via FUN_00020b04(0x40).
 */
void timer3_start(u16 reload16, u8 prescaler, bool irq)
{
    u16 cnt;

    REG16(TIMER3_DATA) = reload16;

    cnt = (u16)(TIMER_CNT_ENABLE | (u16)(prescaler & 3u));
    if (irq) {
        cnt = (u16)(cnt | TIMER_CNT_IRQ);
    }
    REG16(TIMER3_CNT) = cnt;

    if (irq) {
        irq_enable_bits(IRQ_TIMER3);
    }
}

void timer3_stop(void)
{
    REG16(TIMER3_CNT) = 0u;
    irq_disable_bits(IRQ_TIMER3);
}
