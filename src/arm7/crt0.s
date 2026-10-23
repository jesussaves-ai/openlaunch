/*
 * src/arm7/crt0.s
 *
 * ARM7 (ARM7TDMI) reset/exception entry and vector table.
 *
 * The reference ARM7 image begins at file offset 0xF800 (virtual 0x02380000)
 * with straight-line setup code, not a vector table. For our own payload we
 * build a conventional 8-entry vector table and let the reset vector perform
 * the equivalent of the reference prologue (SVC mode, IWRAM stacks).
 *
 * Linked by ld/arm7.ld: _start is placed at 0x02380000.
 */
    .syntax unified
    .arm

    .section .crt0, "ax", %progbits

    .global _start
    .global arm7_vectors
    .type   _start, %function
_start:
arm7_vectors:
    b   arm7_reset              @ 0x00 Reset
    b   arm7_undefined          @ 0x04 Undefined instruction
    b   arm7_svc                @ 0x08 Supervisor call
    b   arm7_prefetch           @ 0x0C Prefetch abort
    b   arm7_data               @ 0x10 Data abort
    b   arm7_reserved           @ 0x14 Reserved
    b   arm7_irq                @ 0x18 IRQ
    b   arm7_fiq                @ 0x1C FIQ

/*
 * Reset entry. Sets up the supervisor/IRQ/system stacks in ARM7 IWRAM, then
 * clears BSS and calls the C boot routine.
 */
    .global arm7_reset
    .type   arm7_reset, %function
arm7_reset:
    /*
     * Diagnostic: prove the stage-2 jumpslide reached our entry. These
     * constants mirror include/common/arm7_handoff.h (ARM7_JUMP_MARKER_*).
     * Kept before the stack setup so it works even if the stacks are bad.
     */
    ldr     r0, =0x0237FFE8
    ldr     r1, =0x44504D4A          @ "JMPD"
    str     r1, [r0]

    msr     cpsr_c, #0xD3            @ SVC mode, IRQ+FIQ disabled
    ldr     sp, =__sp_svc__

    msr     cpsr_c, #0xD2            @ IRQ mode
    ldr     sp, =__sp_irq__

    msr     cpsr_c, #0xDF            @ system mode, IRQ+FIQ disabled
    ldr     sp, =__sp_usr__

    /* Relocate .iwram from its load address into IWRAM. */
    ldr     r0, =__iwram_lma__
    ldr     r1, =__iwram_start__
    ldr     r2, =__iwram_end__
    b       2f
1:  ldr     r3, [r0], #4
    str     r3, [r1], #4
2:  cmp     r1, r2
    blo     1b

    /* Zero .bss. */
    ldr     r0, =__bss_start__
    ldr     r1, =__bss_end__
    mov     r2, #0
    b       4f
3:  str     r2, [r0], #4
4:  cmp     r0, r1
    blo     3b

    bl      arm7_main
5:  b       5b                        @ never returns

/* ---- Default exception stubs: park the core for a post-mortem. ---- */
    .macro  ARM7_PARK name
    .global \name
    .type   \name, %function
\name:
    b       .
    .endm

    ARM7_PARK arm7_undefined
    ARM7_PARK arm7_svc
    ARM7_PARK arm7_prefetch
    ARM7_PARK arm7_data
    ARM7_PARK arm7_reserved
    ARM7_PARK arm7_fiq

/*
 * IRQ vector: hand off to the C dispatcher. The core automatically banks
 * r0-r3, r12, lr and SPSR on IRQ entry.
 */
    .global arm7_irq
    .type   arm7_irq, %function
arm7_irq:
    stmfd   sp!, {r0-r3, r12, lr}
    bl      arm7_irq_handler
    ldmfd   sp!, {r0-r3, r12, lr}
    subs    pc, lr, #4

    .end
