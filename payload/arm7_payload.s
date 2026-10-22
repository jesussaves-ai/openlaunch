/*
 * payload/arm7_payload.s
 *
 * Minimal ARM7 companion for the `make sd-image` integration payload. The
 * chainloader stages it into Main RAM at 0x02380000 and the ARM7 boot stub
 * copies it there and branches to it; the payload then idles.
 *
 * There is no work to do on the ARM7 for this demo (the ARM9 paints the
 * screens), and deliberately no interrupt/timer/DMA setup: the handoff stub
 * has already stopped every peripheral before the branch. Keeping it a bare
 * loop guarantees the ARM7 cannot interfere with the ARM9's display proof.
 */
    .syntax unified
    .arm

    .section .text, "ax", %progbits
    .global _start
    .type   _start, %function
_start:
    b       _start

    .size   _start, . - _start
    .end
