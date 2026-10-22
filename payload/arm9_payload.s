/*
 * payload/arm9_payload.s
 *
 * Minimal, self-contained ARM9 chainload target for the `make sd-image`
 * integration payload (build/hello_payload.nds).
 *
 * It is linked at 0x02004000 — well below the bootloader's reservation
 * (0x02100000..0x02280000, see include/common/nds_header.h) — and is entered
 * directly by the chainloader (or by melonDS direct boot) with no crt0, no
 * stack and no runtime. The only job is to prove the handoff happened:
 *
 *   1. clear any leftover master-brightness fade on both engines;
 *   2. paint Engine A's backdrop bright green and Engine B's bright blue;
 *   3. enable normal 2D display (DISPCNT bits 16-17 = 1, forced blank off);
 *   4. spin in a VBlank-synchronised idle loop.
 *
 * No hardware register here is written outside the documented GBATEK map
 * (AGENTS.md §3); no NAND/eMMC access is performed.
 */
    .syntax unified
    .arm

    .section .text, "ax", %progbits
    .global _start
    .type   _start, %function
_start:
    /* ---- Engine A (top): backdrop bright green -------------------- */
    ldr     r0, =0x0400006C         @ MASTER_BRIGHT (16-bit)
    mov     r1, #0
    strh    r1, [r0]
    ldr     r0, =0x05000000         @ PALETTE_MAIN entry 0 (backdrop)
    ldr     r1, =0x03E0             @ BGR555 bright green
    strh    r1, [r0]
    ldr     r0, =0x04000000         @ DISPCNT
    ldr     r1, =0x00010000         @ bits16-17=1 (normal 2D), forced blank 0
    str     r1, [r0]

    /* ---- Engine B (bottom): backdrop bright blue ------------------ */
    ldr     r0, =0x0400106C         @ MASTER_BRIGHT_SUB (16-bit)
    mov     r1, #0
    strh    r1, [r0]
    ldr     r0, =0x05000400         @ PALETTE_SUB entry 0 (backdrop)
    ldr     r1, =0x7C00             @ BGR555 bright blue
    strh    r1, [r0]
    ldr     r0, =0x04001000         @ DISPCNT_SUB
    ldr     r1, =0x00010000
    str     r1, [r0]

    /*
     * Idle: wait for the start of VBlank (VCOUNT == 192) and for it to end,
     * so the loop is paced by the display rather than free-running. The
     * backdrop colour stays latched, which makes the handoff unmistakable.
     */
    ldr     r2, =0x04000006         @ VCOUNT
.Lwait_line:
    ldrh    r3, [r2]
    cmp     r3, #192
    bne     .Lwait_line
.Lwait_clear:
    ldrh    r3, [r2]
    cmp     r3, #192
    beq     .Lwait_clear
    b       .Lwait_line

    .size   _start, . - _start
    .end
