/*
 * ld/arm7.ld
 *
 * Bare-metal GNU ld script for the ARM7 (ARM7TDMI) image of the TWL
 * bootloader. Matches the reference UNLAUNCH.DSI ARM7 header fields:
 *
 *   entry == _start == 0x02380000
 *
 * Memory map (AGENTS.md §3):
 *   MAIN_RAM  0x02380000  4 MiB Main RAM (256 KiB modelled here)
 *   IWRAM     0x03800000  64 KiB dedicated ARM7 WRAM (stacks)
 *
 * Unlike ARM9 there is no secure-area prefix, so the reset vector sits at the
 * first byte of the image.
 */
ENTRY(_start)

MEMORY
{
    MAIN_RAM (rwx) : ORIGIN = 0x02380000, LENGTH = 256K
    IWRAM    (rwx) : ORIGIN = 0x03800000, LENGTH = 64K
}

SECTIONS
{
    __rom_start__ = ORIGIN(MAIN_RAM);

    . = ORIGIN(MAIN_RAM);

    /* ---- reset/exception vectors + startup code ---------------------- */
    .crt0 :
    {
        KEEP(*(.crt0))
    } > MAIN_RAM

    /* ---- code + read-only data --------------------------------------- */
    .text :
    {
        *(.text*)
        *(.rodata*)
    } > MAIN_RAM

    /* ---- initialised data -------------------------------------------- */
    .data :
    {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > MAIN_RAM

    /* ---- zero-initialised data --------------------------------------- */
    __bss_start__ = .;
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    } > MAIN_RAM
    __bss_end__ = .;

    /* ---- IWRAM-resident code/data (LMA after .data) ------------------ */
    .iwram : AT (LOADADDR(.data) + SIZEOF(.data))
    {
        __iwram_start__ = .;
        KEEP(*(.iwram))
        . = ALIGN(4);
        __iwram_end__ = .;
    } > IWRAM
    __iwram_lma__ = LOADADDR(.iwram);

    /* ---- stacks at the top of IWRAM ---------------------------------- */
    __sp_svc__ = 0x0380FFC0;
    __sp_irq__ = 0x0380FF00;
    __sp_usr__ = 0x0380FE40;

    /* ---- ELF metadata must not reach the flat image ------------------ */
    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.gnu.attributes)
    }
}
