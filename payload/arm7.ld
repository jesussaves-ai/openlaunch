/*
 * payload/arm7.ld
 *
 * Linker script for the chainload test payload's ARM7 half. It is placed at
 * the canonical homebrew ARM7 load address 0x02380000 (Main RAM). The
 * chainloader stages it through its shared bounce buffer and the ARM7 boot
 * stub copies it here at the last moment, so a Main-RAM target is safe.
 */
ENTRY(_start)

MEMORY
{
    MAIN_RAM (rwx) : ORIGIN = 0x02380000, LENGTH = 256K
}

SECTIONS
{
    __rom_start__ = ORIGIN(MAIN_RAM);

    . = ORIGIN(MAIN_RAM);

    .text :
    {
        KEEP(*(.text*))
        *(.rodata*)
    } > MAIN_RAM

    .data :
    {
        *(.data*)
    } > MAIN_RAM

    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    } > MAIN_RAM

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
