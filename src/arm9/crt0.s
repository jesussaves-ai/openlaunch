/*
 * src/arm9/crt0.s
 *
 * ARM9 (ARM946E-S) reset/exception entry and vector table.
 *
 * The reference ARM9 image is a crt0 stub followed by one LZ77 code segment
 * (see docs/hardware_notes.md §9). This is the local entry vector: it
 * configures the protection unit/TCM, installs the stacks, relocates the
 * TCM-resident segments and clears BSS before calling arm9_main().
 *
 * Linked by ld/arm9.ld: .secure_area occupies 0x02200000..0x022007FF and
 * .crt0 (with _start) begins at 0x02200800.
 */
    .syntax unified
    .arm

/* 0x800-byte reserved prefix so .crt0 lands at 0x02200800. */
    .section .secure_area, "a", %progbits
    .space  0x800

    .section .crt0, "ax", %progbits

    .global _start
    .global arm9_vectors
    .type   _start, %function
_start:
arm9_vectors:
    b   arm9_reset              @ 0x00 Reset
    b   arm9_undefined          @ 0x04 Undefined instruction
    b   arm9_svc                @ 0x08 Supervisor call
    b   arm9_prefetch           @ 0x0C Prefetch abort
    b   arm9_data               @ 0x10 Data abort
    b   arm9_reserved           @ 0x14 Reserved
    b   arm9_irq                @ 0x18 IRQ
    b   arm9_fiq                @ 0x1C FIQ

    .global arm9_reset
    .type   arm9_reset, %function
arm9_reset:
    /*
     * Put DTCM at 0x0B000000 (16 KiB) before touching the stack. CP15 c9,c1,0:
     * bits[31:12] = base, bits[5:1] = size code (5 => 16 KiB), bit0 = enable.
     * Firmware/emulators may default DTCM to 0x03000000, so this is required
     * for the DTCM stacks declared in ld/arm9.ld to be valid.
     */
    ldr     r0, =0x0B00000A
    mcr     p15, 0, r0, c9, c1, 0

    /* CP15 control: I-cache, DTCM enable (bit16), ITCM enable (bit18).
     * Reference boot value (see docs/hardware_notes.md §10). */
    ldr     r0, =0x00057078
    mcr     p15, 0, r0, c1, c0, 0

    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4      @ data synchronisation barrier

    /* Stacks: supervisor, IRQ and user/system, at the top of DTCM. */
    msr     cpsr_c, #0xD3
    ldr     sp, =__sp_svc__
    msr     cpsr_c, #0xD2
    ldr     sp, =__sp_irq__
    msr     cpsr_c, #0xDF
    ldr     sp, =__sp_usr__

    /* Relocate .itcm from its load address to ITCM. */
    ldr     r0, =__itcm_lma__
    ldr     r1, =__itcm_start__
    ldr     r2, =__itcm_end__
    b       2f
1:  ldr     r3, [r0], #4
    str     r3, [r1], #4
2:  cmp     r1, r2
    blo     1b

    /* Relocate .dtcm from its load address to DTCM. */
    ldr     r0, =__dtcm_lma__
    ldr     r1, =__dtcm_start__
    ldr     r2, =__dtcm_end__
    b       4f
3:  ldr     r3, [r0], #4
    str     r3, [r1], #4
4:  cmp     r1, r2
    blo     3b

    /* Zero .bss. */
    ldr     r0, =__bss_start__
    ldr     r1, =__bss_end__
    mov     r2, #0
    b       6f
5:  str     r2, [r0], #4
6:  cmp     r0, r1
    blo     5b

    bl      arm9_main
7:  b       7b

    .macro  ARM9_PARK name
    .global \name
    .type   \name, %function
\name:
    b       .
    .endm

    ARM9_PARK arm9_undefined
    ARM9_PARK arm9_svc
    ARM9_PARK arm9_prefetch
    ARM9_PARK arm9_data
    ARM9_PARK arm9_reserved
    ARM9_PARK arm9_fiq

    .global arm9_irq
    .type   arm9_irq, %function
arm9_irq:
    stmfd   sp!, {r0-r3, r12, lr}
    bl      arm9_irq_handler
    ldmfd   sp!, {r0-r3, r12, lr}
    subs    pc, lr, #4

    .end
