/*
 * src/arm7/boot.c
 *
 * Minimal ARM7 boot sequence. This mirrors the shape of the reference ARM7
 * start-up (enable IPC, install the receive handler, then service messages)
 * without yet porting every subsystem.
 */
#include "arm7/boot.h"
#include "arm7/ipc.h"
#include "arm7/irq.h"
#include "common/dsi_hw.h"

/* IRQ vector body: delegate to the C dispatcher. */
void arm7_irq_handler(void)
{
    irq_dispatch();
}

void arm7_main(void)
{
    (void)irq_disable_all();   /* enter critical section for setup */

    /* Enable the IPC FIFO (master + send direction) and flush receive. */
    REG16(IPC_FIFOCNT) = IPC_FIFOCNT_INIT_VALUE;

    /* Route the IPC receive interrupt to the command dispatcher. */
    irq_register(IRQ_IPC_RECV, ipc_dispatch_pending);
    irq_enable_bits(IRQ_IPC_RECV);

    /* Unmask interrupts (IME = 1). */
    irq_restore(1u);

    for (;;) {
        /* Poll rather than halt: the boot handshake must not depend on
         * interrupt delivery in the melonDS smoke test. */
        ipc_dispatch_pending();
    }
}
