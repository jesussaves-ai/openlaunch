#!/usr/bin/env python3
"""Unpack the Nintendo DSi ARM9 image inside ``asm/UNLAUNCH.DSI``.

Unlike a plain NDS homebrew ROM, the ARM9 payload in ``UNLAUNCH.DSI`` is not a
flat blob of code.  It is a tiny bootstrap stub followed by two data segments:

    ROM 0x4000 + 0x0000 .. 0x0800   opaque header / padding (never executed)
    ROM 0x4000 + 0x0800 .. 0x08F0   ARM9 crt0 stub (runs at 0x02200800)
    ROM 0x4000 + 0x08F0 .. 0x4580   raw graphics blob   -> copied to VRAM 0x06020000
    ROM 0x4000 + 0x4580 .. 0xAD80   LZ77 (type 0x10) block -> 0x06023C90

The crt0 performs, in order:

    1. disable caches, program CP15 control = 0x00057078
    2. synchronise with the ARM7 over IPCSYNC (0x04000180)
    3. program SCFG_EXT (0x04004008) = 0x8307F100
    4. memcpy(gfx_blob  -> gfx_dst, gfx_size)
    5. BIOS SWI #0x11 (RLUnCompVram-style) decompress(lz_src -> lz_tmp)
    6. memcpy(lz_tmp -> code_dst, code_size)
    7. ldr pc,[entry]            ; jump into the reconstructed image

This script parses the crt0 literal pool, reproduces steps 4-7 and writes:

    asm/arm9_gfx.bin    raw graphics segment (0x3C90 bytes)
    asm/arm9_code.bin   decompressed ARM9 code segment (0x931C bytes)
    asm/arm9_vram.bin   contiguous VRAM image (gfx + code)
    asm/arm9_unpack.json metadata consumed by the Ghidra pipeline

Every offset is read from the binary itself, so the script keeps working even if
the payload is rebuilt.
"""

from __future__ import annotations

import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM = os.path.join(ROOT, "asm", "UNLAUNCH.DSI")
OUT_GFX = os.path.join(ROOT, "asm", "arm9_gfx.bin")
OUT_CODE = os.path.join(ROOT, "asm", "arm9_code.bin")
OUT_IMAGE = os.path.join(ROOT, "asm", "arm9_vram.bin")
OUT_JSON = os.path.join(ROOT, "asm", "arm9_unpack.json")

# NDS header field offsets.
HDR_ARM9_ROM_OFF = 0x20
HDR_ARM9_ENTRY = 0x24
HDR_ARM9_RAM_ADDR = 0x28
HDR_ARM9_SIZE = 0x2C
HDR_ARM7_ROM_OFF = 0x30


def u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def lz77_10(src: bytes) -> bytes:
    """Decode a Nintendo LZ77 "type 0x10" stream (as used by BIOS SWI 0x11)."""
    if src[0] != 0x10:
        raise ValueError("not an LZ77-0x10 stream (first byte 0x%02X)" % src[0])
    size = src[1] | (src[2] << 8) | (src[3] << 16)
    out = bytearray()
    i = 4
    while len(out) < size:
        if i >= len(src):
            raise ValueError("truncated LZ77 stream")
        flags = src[i]
        i += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if not (flags & (0x80 >> bit)):
                out.append(src[i])
                i += 1
            else:
                b1 = src[i]
                b2 = src[i + 1]
                i += 2
                length = (b1 >> 4) + 3
                disp = ((b1 & 0x0F) << 8) | b2
                if disp + 1 > len(out):
                    raise ValueError("LZ77 back-reference before start of output")
                for _ in range(length):
                    out.append(out[-disp - 1])
    return bytes(out)


def crt0_literals(raw: bytes, ram_base: int) -> dict:
    """Read the crt0 literal pool (vaddr base 0x02200800 => file offset 0x800)."""

    def lit(vaddr: int) -> int:
        return u32(raw, vaddr - ram_base)

    # Literal slots as laid out by the assembler; see the disassembly at
    # 0x02200800 in docs/hardware_notes.md.
    #
    #   0x022008BC  ldr r0,[pc,#164]  CP15 control value
    #   0x022008C0  ldr r1,[pc,#132]  SCFG_EXT address
    #   0x022008C4  ldr r0,[pc,#132]  SCFG_EXT value
    #   0x022008C8  ldr r1,[pc,#88]   misc VRAM word (value is an immediate 0xFF)
    #   0x022008CC  ldr r0,[pc,#80]   SWI 0x11 source (LZ77 block)
    #   0x022008D0  ldr r1,[pc,#80]   SWI 0x11 destination
    #   0x022008D4  ldr r0,[pc,#76]   memcpy source (decompressed code)
    #   0x022008D8  ldr r1,[pc,#76]   memcpy destination (VRAM code)
    #   0x022008DC  ldr r2,[pc,#76]   memcpy size
    #   0x022008E0  ldr r0,[pc,#60]   memcpy source (graphics blob)
    #   0x022008E4  ldr r1,[pc,#60]   memcpy destination (VRAM graphics)
    #   0x022008E8  ldr r2,[pc,#60]   memcpy size
    #   0x022008EC  ldr pc,[pc,#44]   final jump target
    return {
        "cp15_control": lit(0x022008BC),
        "scfg_ext_addr": lit(0x022008C0),
        "scfg_ext_val": lit(0x022008C4),
        "vram_misc_addr": lit(0x022008C8),
        "vram_misc_val": 0xFF,
        "lz_src": lit(0x022008CC),
        "lz_tmp": lit(0x022008D0),
        "copy_src": lit(0x022008D4),
        "copy_dst": lit(0x022008D8),
        "copy_size": lit(0x022008DC),
        "gfx_src": lit(0x022008E0),
        "gfx_dst": lit(0x022008E4),
        "gfx_size": lit(0x022008E8),
        "entry": lit(0x022008EC),
    }


def main() -> int:
    with open(ROM, "rb") as fh:
        rom = fh.read()

    arm9_off = u32(rom, HDR_ARM9_ROM_OFF)
    arm9_entry = u32(rom, HDR_ARM9_ENTRY)
    arm9_ram = u32(rom, HDR_ARM9_RAM_ADDR)
    arm9_size = u32(rom, HDR_ARM9_SIZE)

    print("[*] ARM9 ROM offset : 0x%08X" % arm9_off)
    print("[*] ARM9 entry      : 0x%08X" % arm9_entry)
    print("[*] ARM9 RAM addr   : 0x%08X" % arm9_ram)
    print("[*] ARM9 size       : 0x%X (%d bytes)" % (arm9_size, arm9_size))

    raw = rom[arm9_off:arm9_off + arm9_size]
    if len(raw) != arm9_size:
        print("[!] short read for ARM9 payload", file=sys.stderr)
        return 1

    lits = crt0_literals(raw, arm9_ram)
    print("[*] crt0 literal pool:")
    for key, val in lits.items():
        print("      %-14s = 0x%08X" % (key, val))

    # --- step 4: graphics segment (raw memcpy) -----------------------------
    gfx_src = lits["gfx_src"]
    gfx_size = lits["gfx_size"]
    gfx = raw[gfx_src - arm9_ram:gfx_src - arm9_ram + gfx_size]
    if len(gfx) != gfx_size:
        print("[!] graphics segment out of range", file=sys.stderr)
        return 1

    # --- step 5/6: LZ77 segment -------------------------------------------
    lz_src = lits["lz_src"]
    lz_blob = raw[lz_src - arm9_ram:]
    code = lz77_10(lz_blob)
    if len(code) != lits["copy_size"]:
        print(
            "[!] decompressed size 0x%X != crt0 copy size 0x%X"
            % (len(code), lits["copy_size"]),
            file=sys.stderr,
        )
        return 1

    # --- build the contiguous VRAM image ----------------------------------
    image_base = min(lits["gfx_dst"], lits["copy_dst"])
    image = bytearray()
    if lits["gfx_dst"] != image_base or lits["copy_dst"] != image_base + len(gfx):
        print("[!] unexpected VRAM layout", file=sys.stderr)
        return 1
    image += gfx
    image += code

    with open(OUT_GFX, "wb") as fh:
        fh.write(gfx)
    with open(OUT_CODE, "wb") as fh:
        fh.write(code)
    with open(OUT_IMAGE, "wb") as fh:
        fh.write(image)

    meta = {
        "arm9_rom_offset": arm9_off,
        "arm9_ram_addr": arm9_ram,
        "arm9_size": arm9_size,
        "crt0_entry": arm9_entry,
        "image_base": image_base,
        "image_size": len(image),
        "gfx_base": lits["gfx_dst"],
        "gfx_size": len(gfx),
        "code_base": lits["copy_dst"],
        "code_size": len(code),
        "entry": lits["entry"],
        "cp15_control": lits["cp15_control"],
        "scfg_ext_addr": lits["scfg_ext_addr"],
        "scfg_ext_val": lits["scfg_ext_val"],
    }
    with open(OUT_JSON, "w") as fh:
        json.dump(meta, fh, indent=2)

    print("[*] graphics segment : 0x%08X (%d bytes)" % (lits["gfx_dst"], len(gfx)))
    print("[*] code segment     : 0x%08X (%d bytes)" % (lits["copy_dst"], len(code)))
    print("[*] image base       : 0x%08X (%d bytes)" % (image_base, len(image)))
    print("[*] runtime entry    : 0x%08X" % lits["entry"])
    print("[*] wrote %s" % os.path.relpath(OUT_JSON, ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
