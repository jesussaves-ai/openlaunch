#!/usr/bin/env python3
"""Verify a packaged ``build/dsi_bootloader.nds`` ROM.

Checks performed:

  1. Nintendo-logo CRC16 (0x15C) and header CRC16 (0x15E) recompute correctly.
  2. ARM9/ARM7 ROM offsets, byte sizes and RAM load addresses are consistent,
     and the entry points match the ``_start`` symbols in the ELF images
     (read with ``arm-none-eabi-nm``); the load addresses match ``__rom_start__``.
  3. No dangerous eMMC/NAND write symbols (``sdmmc_write`` etc.) are linked into
     the ARM7 image (AGENTS.md §5); checked against the linker map files.

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

# Symbols that must never appear (AGENTS.md §5: eMMC is read-only by default).
FORBIDDEN_SYMBOLS = (
    "sdmmc_write",
    "sdmmc_write_sector",
    "sdmmc_write_block",
    "nand_write",
    "emmc_write",
)


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
    unit = rom[0x12]
    c.check(unit in (0x00, 0x03), "unit code is 0x00 (NDS) or 0x03 (TWL)",
            "got 0x%02X" % unit)
    if unit & 0x02:
        region_mask = u32(rom, 0x1B0)
        c.check(region_mask != 0,
                "TWL region mask is non-zero (not a 'bad dump')",
                "got 0x%08X" % region_mask)

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
