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
#include "common/arm7_handoff.h"
#include "common/dsi_hw.h"

/* IRQ vector body: delegate to the C dispatcher. */
void arm7_irq_handler(void)
{
    irq_dispatch();
}

void arm7_main(void)
{
    (void)irq_disable_all();   /* enter critical section for setup */

    /*
     * Diagnostic handshake for the stage-2 jumpslide: the ARM9 (and a GDB
     * session) can confirm that the hijacked firmware ARM7 is executing our
     * image. It is harmless on the ordinary title-boot path.
     */
    REG32(ARM7_READY_MARKER_ADDR) = ARM7_READY_MARKER_MAGIC;

    /* Enable the IPC FIFO (master + send direction) and flush receive. */
    REG16(IPC_FIFOCNT) = IPC_FIFOCNT_INIT_VALUE;

    /* Route the IPC receive interrupt to the command dispatcher. */
    irq_register(IRQ_IPC_RECV, ipc_dispatch_pending);
    irq_enable_bits(IRQ_IPC_RECV);

    /*
     * The DSi ARM7 exception vector jumps through the top-of-WRAM pointer at
     * 0x03FFFFFC. A takeover enters our image through that pointer, so it
     * must be repointed at our own handler before IME is enabled - otherwise
     * the next IRQ re-enters _start and restarts the image in a loop.
     */
    {
        extern void arm7_irq(void);

        REG32(0x03FFFFFCu) = (u32)(uintptr_t)&arm7_irq;
    }

    /* Unmask interrupts (IME = 1). */
    irq_restore(1u);

    for (;;) {
        /* Poll rather than halt: the boot handshake must not depend on
         * interrupt delivery in the melonDS smoke test. */
        ipc_dispatch_pending();

        /* Service a pending chainload handoff (copies + branches when the
         * boot guard is armed). */
        arm7_boot_poll();
    }
}
