/* scripts/arm9_image.ld
 *
 * Linker script for the reconstructed Unlaunch ARM9 runtime image.
 *
 * The crt0 copies two segments into VRAM and jumps into the second one:
 *
 *   0x06020000  graphics / asset blob   (raw memcpy)      -> non-executable
 *   0x06023C90  decompressed ARM9 code  (LZ77 -> memcpy)  -> executable
 *
 * Both addresses are read from the crt0 literal pool by
 * scripts/unpack_unlaunch_arm9.py; keep this file in sync with the generated
 * asm/arm9_unpack.json if the payload is ever rebuilt.
 */
SECTIONS
{
    . = 0x06020000;
    .assets : {
        *(.arm9assets)
    }

    . = 0x06023C90;
    .text : {
        *(.text)
    }

    /DISCARD/ : {
        *(.ARM.attributes)
    }
}
