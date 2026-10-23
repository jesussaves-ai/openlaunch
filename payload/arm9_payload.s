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
 * (the project conventions); no NAND/eMMC access is performed.
 */
    .syntax unified
    .arm

    /*
     * ---- DLDI stub section (1 KiB, allocated_size_log2 = 10) ------------
     *
     * A canonical Chishm DLDI header with the default "no interface" identity
     * (IOType "NONE"), exactly as a stock homebrew .nds would carry. The
     * bootloader's clean-room DLDI scanner finds it in the staged ARM9 image
     * and patches it in RAM with the built-in read-only driver (see
     * include/common/dldi.h), so HELLO.NDS / BOOT.NDS / NDS/SUBAPP.NDS on the
     * deterministic SD image exercise real DLDI detection and patching.
     *
     * Placed first (and 1 KiB aligned) so it lives at offset 0 of the ARM9
     * binary and is found by a bounded prefix scan. It is data, never code.
     */
    .section .dldi, "a", %progbits
    .balign 1024
    .global dldi_stub
    .type   dldi_stub, %object
dldi_stub:
    .word   0xBF8DA5ED              @ magic (LE bytes ED A5 8D BF)
    .asciz  " Chishm"              @ 7-char signature + NUL (8 bytes)
    .byte   0x00                    @ version
    .byte   0x0A                    @ driver_size_log2 = 10 (1 KiB)
    .byte   0x00                    @ fix_flags (stub, no relocation)
    .byte   0x0A                    @ allocated_size_log2 = 10 (1 KiB)
    .ascii  "Default (No interface)"@ friendly name (22 chars)
    .zero   48 - 22                 @ pad the name field to 48 bytes
    .word   0x00000000              @ data_start  (set by the patcher)
    .word   0x00000000              @ data_end
    .word   0x00000000              @ glue_start
    .word   0x00000000              @ glue_end
    .word   0x00000000              @ got_start
    .word   0x00000000              @ got_end
    .word   0x00000000              @ bss_start
    .word   0x00000000              @ bss_end
    .ascii  "NONE"                  @ ioType (4 bytes)
    .word   0x00000001              @ features (FEATURE_MEDIUM_CANREAD)
    .word   0x00000000              @ startup
    .word   0x00000000              @ isInserted
    .word   0x00000000              @ readSectors
    .word   0x00000000              @ writeSectors
    .word   0x00000000              @ clearStatus
    .word   0x00000000              @ shutdown
    .zero   896                     @ pad the section to exactly 1 KiB
    .size   dldi_stub, . - dldi_stub

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
