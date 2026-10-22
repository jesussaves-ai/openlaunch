/*
 * ld/arm9.ld
 *
 * Bare-metal GNU ld script for the ARM9 (ARM946E-S) image of the TWL
 * bootloader. It reproduces the reference UNLAUNCH.DSI ARM9 layout:
 *
 *   0x02200000 .. 0x022007FF   0x800-byte reserved prefix (secure-area slot)
 *   0x02200800 ..              code + data (entry == _start == 0x02200800)
 *
 * The 0x800 prefix matters: the NDS/DSi BIOS (and melonDS' direct-boot path)
 * always treats the first 0x800 bytes of the ARM9 binary as the "secure area"
 * and may overwrite them. Keeping the real code after the prefix makes the
 * image survive that.
 *
 * Memory map (AGENTS.md §3):
 *   MAIN_RAM  0x02200000  16 MiB window (2 MiB modelled here)
 *   ITCM      0x01FF8000  32 KiB zero-waitstate code
 *   DTCM      0x0B000000  16 KiB data / stacks
 *
 * The DTCM base is programmed by src/arm9/crt0.s (CP15 c9,c1,0) before the
 * stacks are used, so 0x0B000000 is valid on hardware and on emulators.
 */
ENTRY(_start)

MEMORY
{
    MAIN_RAM (rwx) : ORIGIN = 0x02200000, LENGTH = 2M
    ITCM     (rwx) : ORIGIN = 0x01FF8000, LENGTH = 32K
    DTCM     (rw)  : ORIGIN = 0x0B000000, LENGTH = 16K
}

SECTIONS
{
    __rom_start__ = ORIGIN(MAIN_RAM);

    . = ORIGIN(MAIN_RAM);

    /* ---- 0x800-byte secure-area / reserved prefix -------------------- */
    .secure_area ORIGIN(MAIN_RAM) :
    {
        KEEP(*(.secure_area))
    } > MAIN_RAM

    /* ---- entry point ------------------------------------------------- */
    . = ORIGIN(MAIN_RAM) + 0x800;

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

    /*
     * Tightly-coupled memory. The VMA lives in ITCM/DTCM while the load image
     * is appended directly after .data so `objcopy -O binary` stays compact.
     * The LMA is aligned because the crt0 relocation copies 32-bit words.
     */
    .itcm : AT (ALIGN(LOADADDR(.data) + SIZEOF(.data), 32))
    {
        __itcm_start__ = .;
        KEEP(*(.itcm))
        . = ALIGN(4);
        __itcm_end__ = .;
    } > ITCM
    __itcm_lma__ = LOADADDR(.itcm);

    .dtcm : AT (ALIGN(LOADADDR(.itcm) + SIZEOF(.itcm), 32))
    {
        __dtcm_start__ = .;
        KEEP(*(.dtcm))
        . = ALIGN(4);
        __dtcm_end__ = .;
    } > DTCM
    __dtcm_lma__ = LOADADDR(.dtcm);

    /* ---- stacks at the top of DTCM ----------------------------------- */
    __sp_svc__ = ORIGIN(DTCM) + LENGTH(DTCM);
    __sp_irq__ = ORIGIN(DTCM) + LENGTH(DTCM) - 0x400;
    __sp_usr__ = ORIGIN(DTCM) + LENGTH(DTCM) - 0x800;

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
