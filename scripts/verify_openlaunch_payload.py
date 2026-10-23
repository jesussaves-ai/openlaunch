#!/usr/bin/env python3
"""Verify a built OpenLaunch HNAA payload (Unlaunch-format + overflow wiring).

Independently re-checks the container produced by
``scripts/make_openlaunch_payload.py``:

  1. the self-describing header (magic name, 0x00030000 / 0x00000001 words,
     the ARM9/ARM7 offset/entry/base/size fields, unit code 0x03 and both
     NDS/TWL CRC16s);
  2. the ARM9 and ARM7 images fit and do not overlap;
  3. the two stage-2 callback slots (RAM 0x037F3870 / 0x037F3898) and the whole
     32-entry handler table (RAM 0x037F3B68) point at the same, in-container
     overflow bootstrap;
  4. that bootstrap region is non-empty and carries the expected PAYLOAD_BASE
     literal (i.e. it was parameterised for this layout).

Exit status is non-zero if any check fails.
"""
from __future__ import annotations

import argparse
import struct
import sys

# Must mirror scripts/make_openlaunch_payload.py.
PAYLOAD_BASE = 0x037DF274
BOOTSTRAP_RAM = 0x037F0EC0
CALLBACK_SLOTS = (0x037F3870, 0x037F3898)
HANDLER_TABLE_RAM = 0x037F3B68
HANDLER_TABLE_COUNT = 32
HEADER_SIZE = 0x4000
EXPECTED_NAME = b"OPENLAUNCH.DSI"


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


def u32(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


def u16(buf, off):
    return struct.unpack_from("<H", buf, off)[0]


def crc16(data):
    """Modbus/ARC CRC16 (seed 0xFFFF, polynomial 0xA001)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc & 0xFFFF


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--payload", required=True)
    ap.add_argument("--name", default=EXPECTED_NAME.decode())
    ap.add_argument("--bootstrap-ram", type=lambda s: int(s, 0),
                    default=BOOTSTRAP_RAM)
    args = ap.parse_args(argv)

    with open(args.payload, "rb") as fh:
        p = fh.read()

    c = Checker()
    print("[*] verifying %s (%d bytes)" % (args.payload, len(p)))

    c.check(len(p) >= HEADER_SIZE, "payload has a 16 KiB header")
    if len(p) < HEADER_SIZE:
        print("[*] %d checks, %d failures" % (c.checks, c.failures))
        return 1

    # ---- header ---------------------------------------------------------
    name = p[0:16].rstrip(b"\x00")
    c.check(name == args.name.encode()[:16],
            "container name is %r" % args.name, "got %r" % name)
    c.check(u32(p, 0x10) == 0x00030000, "header word 0x10 == 0x00030000",
            "got 0x%08X" % u32(p, 0x10))
    c.check(u32(p, 0x1C) == 0x00000001, "header word 0x1C == 1",
            "got 0x%08X" % u32(p, 0x1C))
    c.check(p[0x12] == 0x03, "unit code is DSi-exclusive (0x03)",
            "got 0x%02X" % p[0x12])
    c.check(u16(p, 0x15C) == crc16(p[0xC0:0xC0 + 0x9C]),
            "Nintendo logo CRC16 is valid",
            "stored 0x%04X, expected 0x%04X"
            % (u16(p, 0x15C), crc16(p[0xC0:0xC0 + 0x9C])))
    c.check(u16(p, 0x15E) == crc16(p[:0x15E]),
            "NDS header CRC16 is valid",
            "stored 0x%04X, expected 0x%04X"
            % (u16(p, 0x15E), crc16(p[:0x15E])))

    arm9_off = u32(p, 0x20)
    arm9_entry = u32(p, 0x24)
    arm9_base = u32(p, 0x28)
    arm9_size = u32(p, 0x2C)
    arm7_off = u32(p, 0x30)
    arm7_entry = u32(p, 0x34)
    arm7_base = u32(p, 0x38)
    arm7_size = u32(p, 0x3C)

    c.check(arm9_off >= HEADER_SIZE, "ARM9 offset is past the header",
            "0x%X" % arm9_off)
    c.check(arm9_size > 0 and arm9_off + arm9_size <= len(p),
            "ARM9 image fits in the container",
            "0x%X+0x%X vs %d" % (arm9_off, arm9_size, len(p)))
    c.check(arm9_base == 0x02200000, "ARM9 load base is 0x02200000",
            "got 0x%08X" % arm9_base)
    c.check(arm9_entry == arm9_base + 0x800,
            "ARM9 entry is base + 0x800", "got 0x%08X" % arm9_entry)
    c.check(arm7_off >= arm9_off + arm9_size,
            "ARM7 does not overlap ARM9",
            "arm7 0x%X < arm9 end 0x%X" % (arm7_off, arm9_off + arm9_size))
    c.check(arm7_size > 0 and arm7_off + arm7_size <= len(p),
            "ARM7 image fits in the container",
            "0x%X+0x%X vs %d" % (arm7_off, arm7_size, len(p)))
    c.check(arm7_base == 0x02380000, "ARM7 load base is 0x02380000",
            "got 0x%08X" % arm7_base)

    # ---- overflow wiring ------------------------------------------------
    slot_off = [a - PAYLOAD_BASE for a in CALLBACK_SLOTS]
    slot_vals = [u32(p, o) for o in slot_off]
    ptr = slot_vals[0]
    c.check(all(v == ptr for v in slot_vals),
            "both callback slots hold the same pointer",
            "got %s" % [hex(v) for v in slot_vals])
    c.check(ptr == args.bootstrap_ram,
            "callback slots point at RAM 0x%08X" % args.bootstrap_ram,
            "got 0x%08X" % ptr)

    table_off = HANDLER_TABLE_RAM - PAYLOAD_BASE
    table = [u32(p, table_off + 4 * i) for i in range(HANDLER_TABLE_COUNT)]
    c.check(all(v == ptr for v in table),
            "all %d handler-table entries point at the stub"
            % HANDLER_TABLE_COUNT,
            "distinct values: %s" % sorted(set(hex(v) for v in table))[:4])

    # stage-2 task-control block: the post-overflow path dereferences the
    # task-list head, so it must be a valid in-image pointer (not 0).
    head = u32(p, 0x037F3878 - PAYLOAD_BASE)
    c.check(head != 0 and 0 <= head - PAYLOAD_BASE <= len(p) - 4,
            "task-list head 0x037F3878 is a valid in-image pointer",
            "got 0x%08X" % head)

    # ---- the stub itself ------------------------------------------------
    stub_off = ptr - PAYLOAD_BASE
    c.check(0 <= stub_off <= len(p) - 4,
            "stub RAM 0x%08X maps inside the payload" % ptr,
            "payload offset 0x%X" % stub_off)
    if not (0 <= stub_off <= len(p) - 4):
        print("[*] %d checks, %d failures" % (c.checks, c.failures))
        return 1

    # the stub is a blob of words; require a plausible instruction body and the
    # PAYLOAD_BASE literal the assembler emits for `ldr r9, =PAYLOAD_BASE`.
    stub = p[stub_off:stub_off + 0x200]
    c.check(any(b for b in stub[:16]), "stub body is non-empty")
    literal = struct.pack("<I", PAYLOAD_BASE)
    c.check(literal in stub,
            "stub carries the PAYLOAD_BASE literal (0x%08X)" % PAYLOAD_BASE)
    c.check(stub_off >= HEADER_SIZE,
            "stub does not sit in the header", "offset 0x%X" % stub_off)

    print("[*] %d checks, %d failures" % (c.checks, c.failures))
    return 1 if c.failures else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
