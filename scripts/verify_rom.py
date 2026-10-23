#!/usr/bin/env python3
"""Verify a packaged ``build/openlaunch.nds`` ROM.

Checks performed:

  1. Nintendo-logo CRC16 (0x15C) and header CRC16 (0x15E) recompute correctly.
  2. The 0x012 unit code is valid and, when DSi-capable, is the NDS+DSi hybrid
     0x02 with a non-zero TWL region mask.
  3. ARM9/ARM7 ROM offsets, byte sizes and RAM load addresses are consistent,
     and the entry points match the ``_start`` symbols in the ELF images
     (read with ``arm-none-eabi-nm``); the load addresses match ``__rom_start__``.
  4. The ARM9 payload is < 64 KiB and the ARM7 payload is < 32 KiB (the OpenLaunch
     binary size budget; both fit SD chainloading and the NAND .tmd stage-2 /
     ARM7 WRAM constraints). A size telemetry summary is printed.
  5. No dangerous eMMC/NAND write symbols (``sdmmc_write`` etc.) are linked into
     the ARM7 image (the project conventions); checked against the linker map files.
  6. The banner block referenced by offset 0x068 carries a supported version
     (0x0001/0x0002/0x0003/0x0103), a valid CRC16 over 0x020..0x83F and a
     decodable UTF-16LE title at 0x240.

``--expect-title`` / ``--expect-banner-title`` optionally assert the OpenLaunch
identity (12-byte ROM title and the decoded single-line banner title).

Exit status is non-zero if any check fails.
"""

from __future__ import annotations

import argparse
import os
import struct
import subprocess
import sys

HEADER_SIZE = 0x4000
NINTENDO_LOGO_OFF = 0x0C0
NINTENDO_LOGO_LEN = 0x09C
LOGO_CRC_OFF = 0x15C
HEADER_CRC_OFF = 0x15E
HEADER_CRC_LEN = HEADER_CRC_OFF

# NDS banner block (header offset 0x068).
BANNER_OFF_OFF = 0x068
BANNER_SIZE = 0x840
BANNER_VERSION_OFF = 0x000
BANNER_CRC_OFF = 0x002
BANNER_CRC_START = 0x020
BANNER_CRC_LEN = 0x820
BANNER_TITLE_OFF = 0x240
BANNER_TITLE_CHARS = 128
BANNER_VERSIONS = (0x0001, 0x0002, 0x0003, 0x0103)

# Symbols that must never appear (the project conventions: eMMC is read-only by default).
FORBIDDEN_SYMBOLS = (
    "sdmmc_write",
    "sdmmc_write_sector",
    "sdmmc_write_block",
    "nand_write",
    "emmc_write",
)

# Binary size budget (OpenLaunch must stay lightweight enough for both SD
# chainloading and the NAND .tmd stage-2 / ARM7 WRAM constraints).
ARM9_SIZE_BUDGET = 64 * 1024    # < 64 KiB
ARM7_SIZE_BUDGET = 32 * 1024    # < 32 KiB

# NDS ROM title field (12 bytes, NUL- or space-padded).
ROM_TITLE_OFF = 0x00
ROM_TITLE_LEN = 12


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def u32(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


def u16(buf, off):
    return struct.unpack_from("<H", buf, off)[0]


def decode_rom_title(rom: bytes) -> str:
    """Decode the 12-byte NDS ROM title (NUL/space padded) to a clean string."""
    raw = rom[ROM_TITLE_OFF:ROM_TITLE_OFF + ROM_TITLE_LEN]
    nul = raw.find(b"\x00")
    if nul >= 0:
        raw = raw[:nul]
    return raw.decode("ascii", "replace").rstrip(" ")


def decode_banner_title(banner: bytes) -> str:
    """Decode the UTF-16LE banner title into the C parser's single-line form."""
    out = []
    i = 0
    while i < BANNER_TITLE_CHARS:
        ch = struct.unpack_from("<H", banner, BANNER_TITLE_OFF + i * 2)[0]
        if ch == 0x0000:
            break
        if ch in (0x000A, 0x000D):
            if (ch == 0x000D and i + 1 < BANNER_TITLE_CHARS and
                    struct.unpack_from("<H", banner,
                                       BANNER_TITLE_OFF + (i + 1) * 2)[0] == 0x000A):
                i += 1
            out.append(" / ")
        elif 0x20 <= ch <= 0x7E:
            out.append(chr(ch))
        else:
            out.append("?")
        i += 1
    return "".join(out).rstrip(" ")


def nm_value(nm: str, elf: str, symbol: str):
    try:
        out = subprocess.run([nm, elf], capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        raise SystemExit("verify: cannot run %s on %s: %s" % (nm, elf, exc))
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[2] == symbol:
            return int(parts[0], 16)
    return None


class Checker:
    def __init__(self):
        self.failures = 0
        self.checks = 0

    def check(self, cond, label, detail=""):
        self.checks += 1
        if cond:
            print("  [ok]   %s" % label)
        else:
            self.failures += 1
            print("  [FAIL] %s%s" % (label, (" -- " + detail) if detail else ""))


def main(argv) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", required=True)
    ap.add_argument("--arm9-elf")
    ap.add_argument("--arm7-elf")
    ap.add_argument("--arm9-bin")
    ap.add_argument("--arm7-bin")
    ap.add_argument("--arm7-map")
    ap.add_argument("--arm9-map")
    ap.add_argument("--expect-title",
                    help="assert the 12-byte ROM title (e.g. OPENLAUNCH)")
    ap.add_argument("--expect-banner-title",
                    help="assert the decoded single-line banner title")
    ap.add_argument("--nm", default=os.environ.get("NM", "arm-none-eabi-nm"))
    args = ap.parse_args(argv)

    with open(args.rom, "rb") as fh:
        rom = fh.read()

    c = Checker()
    print("[*] verifying %s (%d bytes)" % (args.rom, len(rom)))

    # ---- structure -----------------------------------------------------
    c.check(len(rom) >= HEADER_SIZE, "ROM at least one header block")
    if len(rom) < HEADER_SIZE:
        return 1

    arm9_off = u32(rom, 0x20)
    arm9_entry = u32(rom, 0x24)
    arm9_ram = u32(rom, 0x28)
    arm9_size = u32(rom, 0x2C)
    arm7_off = u32(rom, 0x30)
    arm7_entry = u32(rom, 0x34)
    arm7_ram = u32(rom, 0x38)
    arm7_size = u32(rom, 0x3C)

    print("  ARM9 off 0x%05X entry 0x%08X ram 0x%08X size 0x%X"
          % (arm9_off, arm9_entry, arm9_ram, arm9_size))
    print("  ARM7 off 0x%05X entry 0x%08X ram 0x%08X size 0x%X"
          % (arm7_off, arm7_entry, arm7_ram, arm7_size))

    c.check(arm9_off >= 0x200 and arm9_off + arm9_size <= len(rom),
            "ARM9 block inside ROM", "off 0x%X size 0x%X" % (arm9_off, arm9_size))
    c.check(arm7_off >= 0x200 and arm7_off + arm7_size <= len(rom),
            "ARM7 block inside ROM", "off 0x%X size 0x%X" % (arm7_off, arm7_size))
    c.check(arm9_off == 0x4000, "ARM9 ROM offset is 0x4000", "got 0x%X" % arm9_off)
    c.check((arm7_off % 0x200) == 0, "ARM7 ROM offset is 0x200-aligned")
    c.check(arm9_off + arm9_size <= arm7_off, "ARM9 and ARM7 do not overlap")

    # ---- binary size / section budget -----------------------------------
    print("  ARM9: %d B (%.1f%% of 64 KiB budget)"
          % (arm9_size, 100.0 * arm9_size / ARM9_SIZE_BUDGET))
    print("  ARM7: %d B (%.1f%% of 32 KiB budget)"
          % (arm7_size, 100.0 * arm7_size / ARM7_SIZE_BUDGET))
    c.check(arm9_size < ARM9_SIZE_BUDGET,
            "ARM9 payload is under the 64 KiB budget",
            "%d B (0x%X) >= %d B" % (arm9_size, arm9_size, ARM9_SIZE_BUDGET))
    c.check(arm7_size < ARM7_SIZE_BUDGET,
            "ARM7 payload is under the 32 KiB budget",
            "%d B (0x%X) >= %d B" % (arm7_size, arm7_size, ARM7_SIZE_BUDGET))

    # ---- ROM size / device capacity (check #22+) ------------------------
    MIN_ROM_SIZE = 0x8000
    CAPACITY_UNIT = 0x20000
    rom_len = len(rom)
    device_capacity = rom[0x14]
    total_used = u32(rom, 0x80)

    c.check(rom_len >= MIN_ROM_SIZE,
            "ROM is at least 32 KiB (0x8000)",
            "got %d bytes" % rom_len)
    c.check(rom_len != 0 and (rom_len & (rom_len - 1)) == 0,
            "ROM size is a power of two",
            "got %d bytes" % rom_len)
    c.check((CAPACITY_UNIT << device_capacity) >= rom_len,
            "device capacity covers the ROM image",
            "capacity 0x%02X -> %d KiB, ROM %d bytes"
            % (device_capacity, (CAPACITY_UNIT << device_capacity) // 1024, rom_len))
    c.check(total_used <= rom_len and total_used >= arm7_off + arm7_size,
            "total-used ROM size (0x080) is consistent",
            "0x%X (ARM7 ends 0x%X, file %d)" % (total_used, arm7_off + arm7_size, rom_len))
    # Every byte past the ARM7 payload must be the 0xFF padding.
    payload_end = arm7_off + arm7_size
    tail = rom[payload_end:]
    c.check(all(b == 0xFF for b in tail),
            "ROM padding past the payload is 0xFF-filled",
            "first non-FF at %d" % next((i for i, b in enumerate(tail) if b != 0xFF),
                                        len(tail)))

    # ---- checksums -----------------------------------------------------
    logo = rom[NINTENDO_LOGO_OFF:NINTENDO_LOGO_OFF + NINTENDO_LOGO_LEN]
    logo_crc = u16(rom, LOGO_CRC_OFF)
    c.check(crc16(logo) == logo_crc,
            "Nintendo-logo CRC16 (0x15C)",
            "computed 0x%04X, stored 0x%04X" % (crc16(logo), logo_crc))
    c.check(logo_crc == 0xCF56,
            "Nintendo-logo CRC16 is the canonical 0xCF56",
            "got 0x%04X" % logo_crc)
    header_crc = u16(rom, HEADER_CRC_OFF)
    c.check(crc16(rom[:HEADER_CRC_LEN]) == header_crc,
            "header CRC16 (0x15E)",
            "computed 0x%04X, stored 0x%04X" % (crc16(rom[:HEADER_CRC_LEN]), header_crc))

    # ---- unit code / TWL -------------------------------------------------
    # 0x00 = NDS, 0x02 = NDS+DSi hybrid, 0x03 = DSi-exclusive. The hybrid is
    # what keeps native TWL mode active on a DSi while remaining DS-bootable.
    unit = rom[0x12]
    c.check(unit in (0x00, 0x02, 0x03),
            "unit code is 0x00 (NDS), 0x02 (NDS+DSi) or 0x03 (DSi)",
            "got 0x%02X" % unit)
    if unit & 0x02:
        c.check(unit == 0x02,
                "DSi-capable unit code is the NDS+DSi hybrid (0x02)",
                "got 0x%02X" % unit)
        region_mask = u32(rom, 0x1B0)
        c.check(region_mask != 0,
                "TWL region mask is non-zero (not a 'bad dump')",
                "got 0x%08X" % region_mask)

    # ---- ROM title -------------------------------------------------------
    rom_title = decode_rom_title(rom)
    c.check(len(rom_title) > 0, "ROM title decodes to a non-empty string",
            "got %r" % rom_title)
    if args.expect_title is not None:
        c.check(rom_title == args.expect_title,
                "ROM title is %r" % args.expect_title,
                "got %r" % rom_title)
    print("  title %r" % rom_title)

    # ---- NDS banner ------------------------------------------------------
    banner_off = u32(rom, BANNER_OFF_OFF)
    c.check(banner_off != 0, "banner offset (0x068) is non-zero",
            "got 0x%X" % banner_off)
    c.check(banner_off >= 0x160 and banner_off % 4 == 0,
            "banner offset is aligned and past the header",
            "got 0x%X" % banner_off)
    c.check(banner_off != 0 and banner_off + BANNER_SIZE <= len(rom),
            "banner block lies inside the ROM",
            "off 0x%X size 0x%X file %d" % (banner_off, BANNER_SIZE, len(rom)))
    if banner_off != 0 and banner_off + BANNER_SIZE <= len(rom):
        banner = rom[banner_off:banner_off + BANNER_SIZE]
        version = u16(banner, BANNER_VERSION_OFF)
        c.check(version in BANNER_VERSIONS,
                "banner version is 0x0001/0x0002/0x0003/0x0103",
                "got 0x%04X" % version)
        stored = u16(banner, BANNER_CRC_OFF)
        computed = crc16(banner[BANNER_CRC_START:BANNER_CRC_START + BANNER_CRC_LEN])
        c.check(stored == computed, "banner CRC16 (0x002)",
                "stored 0x%04X computed 0x%04X" % (stored, computed))
        title = decode_banner_title(banner)
        c.check(len(title) > 0, "banner title decodes to a non-empty string",
                "got %r" % title)
        if args.expect_banner_title is not None:
            c.check(title == args.expect_banner_title,
                    "banner title is %r" % args.expect_banner_title,
                    "got %r" % title)
        print("  banner off 0x%05X v%04X title %r" % (banner_off, version, title))

    # ---- ELF cross-check -------------------------------------------------
    if args.arm9_elf:
        start = nm_value(args.nm, args.arm9_elf, "_start")
        rom_start = nm_value(args.nm, args.arm9_elf, "__rom_start__")
        c.check(start is not None and start == arm9_entry,
                "ARM9 entry matches _start",
                "_start=%s header=0x%08X" % (hex(start) if start else None, arm9_entry))
        if rom_start is not None:
            c.check(rom_start == arm9_ram,
                    "ARM9 RAM address matches __rom_start__",
                    "__rom_start__=0x%08X header=0x%08X" % (rom_start, arm9_ram))
    if args.arm7_elf:
        start = nm_value(args.nm, args.arm7_elf, "_start")
        rom_start = nm_value(args.nm, args.arm7_elf, "__rom_start__")
        c.check(start is not None and start == arm7_entry,
                "ARM7 entry matches _start",
                "_start=%s header=0x%08X" % (hex(start) if start else None, arm7_entry))
        if rom_start is not None:
            c.check(rom_start == arm7_ram,
                    "ARM7 RAM address matches __rom_start__",
                    "__rom_start__=0x%08X header=0x%08X" % (rom_start, arm7_ram))

    # ---- flat-binary sizes ----------------------------------------------
    if args.arm9_bin and os.path.isfile(args.arm9_bin):
        size = os.path.getsize(args.arm9_bin)
        c.check(size == arm9_size, "ARM9 size matches build/arm9.bin",
                "bin %d header 0x%X" % (size, arm9_size))
    if args.arm7_bin and os.path.isfile(args.arm7_bin):
        size = os.path.getsize(args.arm7_bin)
        c.check(size == arm7_size, "ARM7 size matches build/arm7.bin",
                "bin %d header 0x%X" % (size, arm7_size))

    # ---- NAND-write safety ----------------------------------------------
    for map_path in (args.arm7_map, args.arm9_map):
        if not map_path:
            continue
        c.check(os.path.isfile(map_path), "map file exists: %s" % map_path)
        if not os.path.isfile(map_path):
            continue
        with open(map_path, "r", errors="replace") as fh:
            text = fh.read()
        found = sorted({sym for sym in FORBIDDEN_SYMBOLS if sym in text})
        c.check(not found,
                "no eMMC/NAND write symbols in %s" % os.path.basename(map_path),
                ", ".join(found))

    print("[*] %d checks, %d failures" % (c.checks, c.failures))
    return 1 if c.failures else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
