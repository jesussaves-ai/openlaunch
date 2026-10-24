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

    /*
     * Reset and enable the FIFO. Do NOT drain it here: reading the receive
     * register while it is empty sets the sticky error bit, and the observed
     * hardware state (0xCF01) shows exactly that. Disable first so the block
     * re-initialises cleanly after the loader's handshake.
     */
    REG16(IPC_FIFOCNT) = 0x0000u;
    REG16(IPC_FIFOCNT) = IPC_FIFOCNT_INIT_VALUE;


    /*
     * Poll-only by default.
     *
     * The main loop services the IPC FIFO with ipc_dispatch_pending(), so no
     * interrupt is required. Enabling IME and repointing the DSi top-of-WRAM
     * vector (0x03FFFFFC) is only correct when we own the ARM7 exception
     * environment (the stage-2 overflow takeover). In the DSi title-boot
     * environment the loader/firmware owns that vector: hardware testing
     * (2026-10-24) showed the title black-screens with our ARM7 image while
     * the same container with another ARM7 boots, and enabling IRQs there
     * jumps through a vector that is not ours. Keep IRQs off and poll; the
     * IRQ path is available with -DARM7_USE_IRQ for a takeover build.
     */
#ifdef ARM7_USE_IRQ
    irq_register(IRQ_IPC_RECV, ipc_dispatch_pending);
    irq_enable_bits(IRQ_IPC_RECV);
    {
        extern void arm7_irq(void);

        REG32(0x03FFFFFCu) = (u32)(uintptr_t)&arm7_irq;
    }
    irq_restore(1u);
#endif

    for (;;) {
        /* Poll rather than halt: the boot handshake must not depend on
         * interrupt delivery. */
        ipc_dispatch_pending();

        /* Service a pending chainload handoff (copies + branches when the
         * boot guard is armed). */
        arm7_boot_poll();
    }
}
