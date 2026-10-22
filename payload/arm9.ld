/*
 * payload/arm9.ld
 *
 * Linker script for the chainload test payload's ARM9 half. It is placed at
 * 0x02004000, which is:
 *   - inside the safe Main-RAM window (0x02000000..), and
 *   - well clear of the bootloader reservation (0x02100000..0x02280000).
 *
 * Keeping it below 0x02100000 means the bootloader can stream the payload into
 * its final address while its own code keeps running, exactly as
 * src/arm9/loader.c does. There is no secure-area prefix: unsigned homebrew is
 * never secure-area decrypted, so _start sits at the load address.
 */
ENTRY(_start)

MEMORY
{
    MAIN_RAM (rwx) : ORIGIN = 0x02004000, LENGTH = 256K
}

SECTIONS
{
    __rom_start__ = ORIGIN(MAIN_RAM);

    . = ORIGIN(MAIN_RAM);

    /*
     * The DLDI stub header must be at a 1 KiB boundary so the bootloader's
     * bounded ARM9 prefix scan (dldi_find_offset) sees the whole 12-byte
     * signature at file offset 0. Placed before .text; __rom_start__ stays at
     * ORIGIN, so verify_rom.py's ARM9 RAM-address cross-check is unaffected.
     */
    .dldi : ALIGN(1024)
    {
        KEEP(*(.dldi))
    } > MAIN_RAM

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
