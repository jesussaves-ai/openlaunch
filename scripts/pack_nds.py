#!/usr/bin/env python3
"""Build a runnable NDS / DSi (TWL) ``.nds`` ROM from the flat ARM9 and ARM7
images produced by the Makefile.

The layout mirrors ``asm/UNLAUNCH.DSI`` and the melonDS ``NDSHeader`` struct
(see the melonDS source, ``src/NDS_Header.h``):

    0x0000 .. 0x3FFF   header (0x200 fields + zero padding)
    0x4000 ..          ARM9 binary (0x4000-aligned)
    align(...,0x200)   ARM7 binary

Both the Nintendo-logo CRC16 (offset 0x15C, over 0xC0..0x15B) and the header
CRC16 (offset 0x15E, over 0x000..0x15D) are computed with the Modbus/ARC
algorithm (seed 0xFFFF, polynomial 0xA001) so the DSi BIOS and melonDS accept
the image without a checksum failure.

This script is dependency-free (stdlib only). If ``ndstool`` is available it is
reported for information but it is not required.
"""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import sys

# --------------------------------------------------------------------------
# Header constants
# --------------------------------------------------------------------------
HEADER_SIZE = 0x4000
TITLE = b"OPENUNLAUNCH"
GAME_CODE = b"HNAA"
MAKER_CODE = b"01"

NINTENDO_LOGO_OFF = 0x0C0
NINTENDO_LOGO_LEN = 0x09C      # 156 bytes
LOGO_CRC_OFF = 0x15C
HEADER_CRC_OFF = 0x15E

# Header CRC covers 0x000..0x15D inclusive (i.e. up to HEADER_CRC_OFF).
HEADER_CRC_LEN = HEADER_CRC_OFF

# Cartridge capacity. melonDS (and the DSi loader) expect a ROM whose size is
# a power of two of at least 32 KiB; anything smaller is rounded up and logged
# as "bad ROM size". The Device Capacity byte (offset 0x014) is the exponent of
# the 128 KiB unit: size = 128 KiB << capacity.
MIN_ROM_SIZE = 0x8000          # 32 KiB
CAPACITY_UNIT = 0x20000        # 128 KiB

# Cartridge unit code (header offset 0x012).
#
#   0x00  NDS only
#   0x02  NDS + DSi (hybrid): runs on both; a DSi launcher/emulator keeps
#         native TWL mode active (SCFG_A9ROM == 0x01)
#   0x03  DSi-exclusive
#
# The hybrid code is what lets the same image boot in DSi mode on a DSi and
# still fall back to DS mode on a legacy DS.
UNIT_CODE_NDS = 0x00
UNIT_CODE_NDS_DSI = 0x02
UNIT_CODE_DSI = 0x03

# TWL extended-header field offsets (melonDS NDSHeader layout).
OFF_MBK_SLOTS = 0x180          # u32[5]  global MBK1..MBK5
OFF_ARM9_MBK = 0x194           # u32[3]  local MBK6..MBK8 (ARM9)
OFF_ARM7_MBK = 0x1A0           # u32[3]  local MBK6..MBK8 (ARM7)
OFF_MBK_WPROT = 0x1AC          # u8[3]   global MBK9 write-protect
OFF_WRAMCNT = 0x1AF            # u8
OFF_REGION_MASK = 0x1B0        # u32
OFF_PERMISSIONS = 0x1B4        # u32[2]
OFF_RESERVED6 = 0x1BC          # u8[3]
OFF_APPFLAGS = 0x1BF           # u8
OFF_ARM9I_ROM = 0x1C0          # u32[4]: ROM offset, resv, RAM addr, size
OFF_ARM7I_ROM = 0x1D0          # u32[4]: ROM offset, sdmmc list, RAM, size

WRAMCNT_DEFAULT = 0x03
REGION_MASK_FREE = 0xFFFFFFFF
PERMISSIONS_DEFAULT = (0x0000001F, 0x80040407)
APPFLAGS_DEFAULT = 0x01

# ROM control words copied from the reference / ndstool defaults.
NORMAL_CMD = 0x00586000
KEY1_CMD = 0x001808F8
SECURE_AREA_DELAY = 0x051E


def crc16(data: bytes) -> int:
    """Modbus/ARC CRC16: seed 0xFFFF, polynomial 0xA001 (LSB-first)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def align_up(value: int, alignment: int) -> int:
    return (value + (alignment - 1)) & ~(alignment - 1)


def next_power_of_two(value: int) -> int:
    """Smallest power of two >= value (1 for value <= 1)."""
    if value <= 1:
        return 1
    return 1 << (value - 1).bit_length()


def device_capacity_for(size: int) -> int:
    """Device Capacity byte: 128 KiB << capacity >= size, minimum 0."""
    capacity = 0
    while (CAPACITY_UNIT << capacity) < size:
        capacity += 1
    return capacity


def read_elf_entry(path: str) -> int:
    """Read e_entry from a 32-bit little-endian ELF file."""
    with open(path, "rb") as fh:
        ident = fh.read(0x34)
    if len(ident) < 0x34 or ident[:4] != b"\x7fELF":
        raise ValueError("%s is not an ELF file" % path)
    if ident[4] != 1:  # EI_CLASS == ELFCLASS32
        raise ValueError("%s is not a 32-bit ELF" % path)
    return struct.unpack_from("<I", ident, 0x18)[0]


def find_ndstool():
    candidates = [shutil.which("ndstool")]
    devkitpro = os.environ.get("DEVKITPRO")
    if devkitpro:
        candidates.append(os.path.join(devkitpro, "tools", "bin", "ndstool"))
    wonderful = os.environ.get("WONDERFUL_TOOLCHAIN")
    if wonderful:
        candidates.append(os.path.join(wonderful, "bin", "ndstool"))
    for cand in candidates:
        if cand and os.path.isfile(cand):
            return cand
    return None


def load_logo(path: str) -> bytes:
    with open(path, "rb") as fh:
        data = fh.read(NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN)
    if len(data) < NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN:
        raise ValueError("logo source %s is too small" % path)
    logo = data[NINTENDO_LOGO_OFF:NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN]
    # The standard Nintendo logo; sanity-check its known CRC.
    if crc16(logo) != 0xCF56:
        raise ValueError(
            "logo from %s has unexpected CRC16 0x%04X (expected 0xCF56)"
            % (path, crc16(logo))
        )
    return logo


def build_header(arm9, arm7, twl: bool, logo: bytes, rom_size: int,
                 capacity: int) -> bytearray:
    """Return the 0x4000-byte header block."""
    arm9_off, arm9_entry, arm9_ram, arm9_size = arm9
    arm7_off, arm7_entry, arm7_ram, arm7_size = arm7

    h = bytearray(HEADER_SIZE)

    def put32(off, value):
        struct.pack_into("<I", h, off, value & 0xFFFFFFFF)

    def put16(off, value):
        struct.pack_into("<H", h, off, value & 0xFFFF)

    # ---- NTR header ----
    h[0x00:0x00 + len(TITLE)] = TITLE
    h[0x0C:0x10] = GAME_CODE
    h[0x10:0x12] = MAKER_CODE
    h[0x12] = UNIT_CODE_NDS_DSI if twl else UNIT_CODE_NDS
    h[0x13] = 0x00                    # encryption seed select (unencrypted)
    h[0x14] = capacity & 0xFF         # device capacity (128 KiB << capacity)
    h[0x1C] = 0x00                    # DSiCryptoFlags
    h[0x1D] = 0x00                    # NDS region
    h[0x1E] = 0x00                    # ROM version
    h[0x1F] = 0x00                    # autostart

    put32(0x20, arm9_off)
    put32(0x24, arm9_entry)
    put32(0x28, arm9_ram)
    put32(0x2C, arm9_size)

    put32(0x30, arm7_off)
    put32(0x34, arm7_entry)
    put32(0x38, arm7_ram)
    put32(0x3C, arm7_size)

    put32(0x60, NORMAL_CMD)
    put32(0x64, KEY1_CMD)
    put16(0x6C, 0x0000)               # secure-area CRC (unencrypted payload)
    put16(0x6E, SECURE_AREA_DELAY)

    put32(0x80, rom_size)             # total used ROM size (excluding DSi area)
    put32(0x84, HEADER_SIZE)          # header size

    h[NINTENDO_LOGO_OFF:NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN] = logo

    # ---- TWL extended header ----
    if twl:
        # MBK1..MBK5, MBK6..MBK8 (ARM9/ARM7) and MBK9 protect all default to 0,
        # which leaves the boot firmware's mapping in place (as UNLAUNCH does).
        for off in (OFF_MBK_SLOTS, OFF_ARM9_MBK, OFF_ARM7_MBK):
            for i in range(0, 20, 4):
                put32(off + i, 0)
        h[OFF_MBK_WPROT:OFF_MBK_WPROT + 3] = bytes(3)
        h[OFF_WRAMCNT] = WRAMCNT_DEFAULT
        put32(OFF_REGION_MASK, REGION_MASK_FREE)
        put32(OFF_PERMISSIONS + 0, PERMISSIONS_DEFAULT[0])
        put32(OFF_PERMISSIONS + 4, PERMISSIONS_DEFAULT[1])
        h[OFF_RESERVED6:OFF_RESERVED6 + 3] = bytes(3)
        h[OFF_APPFLAGS] = APPFLAGS_DEFAULT
        # No ARM9i / ARM7i secondary binaries.
        for off in (OFF_ARM9I_ROM, OFF_ARM7I_ROM):
            for i in range(0, 16, 4):
                put32(off + i, 0)

    # ---- checksums ----
    put16(LOGO_CRC_OFF, crc16(h[NINTENDO_LOGO_OFF:NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN]))
    put16(HEADER_CRC_OFF, crc16(bytes(h[:HEADER_CRC_LEN])))
    return h


def main(argv) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--arm9", required=True, help="flat ARM9 binary")
    ap.add_argument("--arm7", required=True, help="flat ARM7 binary")
    ap.add_argument("--arm9-elf", help="ARM9 ELF (for the entry point)")
    ap.add_argument("--arm7-elf", help="ARM7 ELF (for the entry point)")
    ap.add_argument("--arm9-entry", type=lambda s: int(s, 0), default=None)
    ap.add_argument("--arm7-entry", type=lambda s: int(s, 0), default=None)
    ap.add_argument("--arm9-ram", type=lambda s: int(s, 0), default=0x02200000)
    ap.add_argument("--arm7-ram", type=lambda s: int(s, 0), default=0x02380000)
    ap.add_argument("--logo-source", default="asm/UNLAUNCH.DSI",
                    help="ROM to copy the 156-byte Nintendo logo from")
    ap.add_argument("--twl", dest="twl", action="store_true", default=True,
                    help="populate the DSi/TWL extended header (default)")
    ap.add_argument("--no-twl", dest="twl", action="store_false",
                    help="build a plain NDS header (unit code 0x00)")
    ap.add_argument("--out", required=True, help="output .nds path")
    args = ap.parse_args(argv)

    with open(args.arm9, "rb") as fh:
        arm9_bin = fh.read()
    with open(args.arm7, "rb") as fh:
        arm7_bin = fh.read()

    arm9_entry = args.arm9_entry
    if arm9_entry is None and args.arm9_elf:
        arm9_entry = read_elf_entry(args.arm9_elf)
    arm7_entry = args.arm7_entry
    if arm7_entry is None and args.arm7_elf:
        arm7_entry = read_elf_entry(args.arm7_elf)
    if arm9_entry is None or arm7_entry is None:
        ap.error("entry point required (pass --arm*-elf or --arm*-entry)")

    arm9_off = HEADER_SIZE
    arm7_off = align_up(arm9_off + len(arm9_bin), 0x200)
    content_size = align_up(arm7_off + len(arm7_bin), 0x200)

    # Pad the image to a power of two of at least 32 KiB. melonDS (and the DSi
    # loader) reject a non-power-of-two cartridge size, and the 0x014 Device
    # Capacity byte must describe a device large enough to hold the image.
    padded_size = max(MIN_ROM_SIZE, next_power_of_two(content_size))
    capacity = device_capacity_for(padded_size)

    logo = load_logo(args.logo_source)

    header = build_header(
        (arm9_off, arm9_entry, args.arm9_ram, len(arm9_bin)),
        (arm7_off, arm7_entry, args.arm7_ram, len(arm7_bin)),
        args.twl, logo, padded_size, capacity,
    )

    # Fill the padding with 0xFF, matching a blank cartridge/EEPROM image.
    rom = bytearray(b"\xFF" * padded_size)
    rom[0:HEADER_SIZE] = header
    rom[arm9_off:arm9_off + len(arm9_bin)] = arm9_bin
    rom[arm7_off:arm7_off + len(arm7_bin)] = arm7_bin

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as fh:
        fh.write(rom)

    ndstool = find_ndstool()
    print("[*] packed %s (%d bytes, content %d, padded to %d)"
          % (args.out, len(rom), content_size, padded_size))
    print("    ARM9 off 0x%05X  entry 0x%08X  ram 0x%08X  size 0x%X"
          % (arm9_off, arm9_entry, args.arm9_ram, len(arm9_bin)))
    print("    ARM7 off 0x%05X  entry 0x%08X  ram 0x%08X  size 0x%X"
          % (arm7_off, arm7_entry, args.arm7_ram, len(arm7_bin)))
    print("    device capacity 0x%02X (%d KiB)   total-used 0x%X"
          % (capacity, (CAPACITY_UNIT << capacity) // 1024, padded_size))
    print("    unit code 0x%02X%s   logo CRC 0x%04X   header CRC 0x%04X"
          % (UNIT_CODE_NDS_DSI if args.twl else UNIT_CODE_NDS,
             " (NDS+DSi hybrid)" if args.twl else " (NDS)",
             crc16(logo), crc16(bytes(header[:HEADER_CRC_LEN]))))
    print("    ndstool: %s" % (ndstool if ndstool else "not found (not required)"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
