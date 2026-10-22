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

# --------------------------------------------------------------------------
# NDS banner block (header offset 0x068 -> a 0x840-byte block)
# --------------------------------------------------------------------------
# The canonical banner referenced by the 32-bit word at header 0x068. It fits
# inside the 0x4000-byte header block, before the ARM9 binary at 0x4000.
BANNER_OFFSET = 0x840
BANNER_SIZE = 0x840
BANNER_VERSION = 0x0001
BANNER_VERSION_OFF = 0x000
BANNER_CRC_OFF = 0x002
BANNER_CRC_START = 0x020
BANNER_CRC_LEN = 0x820
BANNER_TITLE_OFF = 0x240
BANNER_TITLE_BYTES = 256       # 128 UTF-16LE code units

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


def build_banner(title_text: str) -> bytes:
    """Return a valid 0x840-byte v1 NDS banner with `title_text` as the title.

    The English/primary title is stored as UTF-16LE at banner + 0x240 (up to 128
    code units) and the CRC-16/MODBUS over bytes 0x020..0x83F is written at
    banner + 0x002.
    """
    banner = bytearray(BANNER_SIZE)
    struct.pack_into("<H", banner, BANNER_VERSION_OFF, BANNER_VERSION)

    encoded = title_text.encode("utf-16-le")[:BANNER_TITLE_BYTES]
    if len(encoded) % 2:                  # never split a code unit
        encoded = encoded[:-1]
    banner[BANNER_TITLE_OFF:BANNER_TITLE_OFF + len(encoded)] = encoded

    crc = crc16(bytes(banner[BANNER_CRC_START:BANNER_CRC_START + BANNER_CRC_LEN]))
    struct.pack_into("<H", banner, BANNER_CRC_OFF, crc)
    return bytes(banner)


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
                 capacity: int, title: bytes = TITLE,
                 game_code: bytes = GAME_CODE,
                 maker_code: bytes = MAKER_CODE,
                 banner_offset: int = BANNER_OFFSET) -> bytearray:
    """Return the 0x4000-byte header block."""
    arm9_off, arm9_entry, arm9_ram, arm9_size = arm9
    arm7_off, arm7_entry, arm7_ram, arm7_size = arm7

    h = bytearray(HEADER_SIZE)

    def put32(off, value):
        struct.pack_into("<I", h, off, value & 0xFFFFFFFF)

    def put16(off, value):
        struct.pack_into("<H", h, off, value & 0xFFFF)

    # ---- NTR header ----
    h[0x00:0x00 + len(title)] = title
    h[0x0C:0x10] = game_code
    h[0x10:0x12] = maker_code
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
    put32(0x68, banner_offset)        # banner block file offset (0 = none)
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
    ap.add_argument("--title", default=None,
                    help="ROM title, up to 12 bytes (default: %s)"
                         % TITLE.decode("ascii"))
    ap.add_argument("--gamecode", default=None,
                    help="4-byte game code (default: %s)"
                         % GAME_CODE.decode("ascii"))
    ap.add_argument("--maker", default=None,
                    help="2-byte maker code (default: %s)"
                         % MAKER_CODE.decode("ascii"))
    ap.add_argument("--banner-title", default=None,
                    help="UTF-16LE banner title at 0x840+0x240 (default: the "
                         "ROM title). Literal '\\n' separators become ' / '.")
    ap.add_argument("--no-banner", dest="banner", action="store_false",
                    default=True,
                    help="omit the 0x840-byte banner block (banner_offset 0)")
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

    title = args.title.encode("ascii") if args.title else TITLE
    game_code = args.gamecode.encode("ascii") if args.gamecode else GAME_CODE
    maker_code = args.maker.encode("ascii") if args.maker else MAKER_CODE
    if len(title) > 12:
        ap.error("--title must be at most 12 bytes (got %d)" % len(title))
    if len(game_code) != 4:
        ap.error("--gamecode must be exactly 4 bytes (got %d)" % len(game_code))
    if len(maker_code) != 2:
        ap.error("--maker must be exactly 2 bytes (got %d)" % len(maker_code))

    # Banner title defaults to the ROM title. A literal "\n" in the argument is
    # honoured as a line break so the decoded summary exercises " / ".
    if args.banner:
        if args.banner_title is not None:
            banner_title = args.banner_title.replace("\\n", "\n")
        else:
            banner_title = title.decode("ascii", "replace")
    else:
        banner_title = None

    header = build_header(
        (arm9_off, arm9_entry, args.arm9_ram, len(arm9_bin)),
        (arm7_off, arm7_entry, args.arm7_ram, len(arm7_bin)),
        args.twl, logo, padded_size, capacity,
        title=title, game_code=game_code, maker_code=maker_code,
        banner_offset=BANNER_OFFSET if args.banner else 0,
    )

    if banner_title is not None:
        banner = build_banner(banner_title)
        header[BANNER_OFFSET:BANNER_OFFSET + BANNER_SIZE] = banner

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
    print("    title %-12s  game code %-4s  maker %-2s"
          % (title.decode("ascii", "replace"),
             game_code.decode("ascii", "replace"),
             maker_code.decode("ascii", "replace")))
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
    if banner_title is not None:
        banner_crc = struct.unpack_from("<H", header, BANNER_OFFSET + BANNER_CRC_OFF)[0]
        shown = banner_title.replace("\n", " / ")
        print("    banner off 0x%05X  v%04X  crc 0x%04X  title %r"
              % (BANNER_OFFSET, BANNER_VERSION, banner_crc, shown))
    else:
        print("    banner: none")
    print("    ndstool: %s" % (ndstool if ndstool else "not found (not required)"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
