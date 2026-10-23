/*
 * src/arm9/overflow_bootstrap.s
 *
 * Stage-2 TMD-overflow bootstrap (ARM9) for the OpenLaunch HNAA payload.
 *
 * Background: boot stage 2 reads the backup launcher's oversized title.tmd into
 * a fixed Main-RAM buffer (0x037DF06C) and, via the "filesize > filesize" bug,
 * overruns the adjacent callback/handler structures. The launcher payload is
 * overlaid at 0x037DF274 (= buffer + 520); see docs/stage2_re.md.
 *
 * Stage 2 later calls the (now clobbered) callback at RAM 0x037F3870 / 0x037F3898
 * through `blx`. This stub is what those pointers point at: it runs *in place*
 * from inside the overlaid payload (RAM 0x037F0EC0 = payload offset 0x11C4C),
 * relocates the real OpenLaunch ARM9/ARM7 images to their linked addresses and
 * jumps to the ARM9 entry.
 *
 * The literal PAYLOAD_BASE below must match scripts/make_openlaunch_payload.py
 * (PAYLOAD_BASE) and the buffer base in docs/stage2_re.md.
 *
 * Hardware side effects:
 *   - enters SVC mode with IRQ/FIQ masked;
 *   - disables the MMU and the ARM946 I/D caches (CP15 c1), drains the write
 *     buffer and invalidates the TLB/I-cache (CP15 c7/c8);
 *   - copies the ARM9 and ARM7 flat images into Main RAM;
 *   - branches to the ARM9 entry point (never returns).
 */
    .syntax unified
    .arm

/* Container header field offsets (see docs/openlaunch_payload.md). */
    .equ HDR_ARM9_ENTRY, 0x24
    .equ HDR_ARM9_BASE,  0x28
    .equ HDR_ARM9_SIZE,  0x2C
    .equ HDR_ARM7_BASE,  0x38
    .equ HDR_ARM7_SIZE,  0x3C
    .equ HDR_ARM9_OFF,   0x20
    .equ HDR_ARM7_OFF,   0x30

/* RAM address the container image is overlaid at (buffer 0x037DF06C + 520). */
    .equ PAYLOAD_BASE,   0x037DF274

    .section .text
    .global overflow_bootstrap_start
    .type   overflow_bootstrap_start, %function
overflow_bootstrap_start:
    ldr     r9, =PAYLOAD_BASE           @ r9 = container image base

    /* Supervisor mode, IRQ + FIQ masked. */
    msr     cpsr_c, #0xD3

    /* ARM9 image -> arm9 base; keep the entry point in r10. */
    ldr     r10, [r9, #HDR_ARM9_ENTRY]
    ldr     r1,  [r9, #HDR_ARM9_OFF]
    add     r2,  r9, r1                 @ src
    ldr     r3,  [r9, #HDR_ARM9_BASE]   @ dst
    ldr     r1,  [r9, #HDR_ARM9_SIZE]   @ len
    bl      copy_words

    /* ARM7 image -> arm7 base (staged for the later IPC handoff). */
    ldr     r1,  [r9, #HDR_ARM7_OFF]
    add     r2,  r9, r1
    ldr     r3,  [r9, #HDR_ARM7_BASE]
    ldr     r1,  [r9, #HDR_ARM7_SIZE]
    bl      copy_words

    /* Make the fresh code visible, then hand off to the ARM9 entry. */
    mov     r0, #0
    mcr     p15, 0, r0, c7, c10, 4      @ drain write buffer
    mcr     p15, 0, r0, c7, c14, 0      @ clean + invalidate D-cache
    mcr     p15, 0, r0, c7, c5, 0       @ invalidate I-cache
    mcr     p15, 0, r0, c7, c10, 4
    bx      r10                         @ -> arm9_entry (e.g. 0x02200800)

    /* r2 = src, r3 = dst, r1 = byte length (rounded up to a word). */
copy_words:
    add     r1, r1, #3
    bic     r1, r1, #3
    add     r1, r1, r2                  @ end = src + aligned(len)
1:  cmp     r2, r1
    ldrlo   r4, [r2], #4
    strlo   r4, [r3], #4
    blo     1b
    bx      lr
    .size   overflow_bootstrap_start, . - overflow_bootstrap_start

    .end
