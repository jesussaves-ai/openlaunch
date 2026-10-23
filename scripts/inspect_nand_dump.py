#!/usr/bin/env python3
"""Inspect a DSi NAND dump: encryption state and no$gba/Unlaunch footer.

The offline installer needs two per-console values to derive the NAND AES key
(the eMMC CID and the CPU/Console ID). Both are often already stored *inside*
the dump as a no$gba / Unlaunch footer, so this tool tries to recover them.

It seeks and reads only a few kilobytes, so it is instant on a 240 MiB image.
It reports:

  * the image size in bytes and 512-byte sectors;
  * whether sector 0 looks like a plaintext DOS MBR (0x55AA at 0x1FE), i.e. an
    already-decrypted image, or is still AES-CTR encrypted (no signature);
  * whether a no$gba / Unlaunch footer is present at byte 0xFF800 or appended at
    the end of the file, and if so prints the eMMC CID and the CPU/Console ID.

Footer layout (GBATEK "DSi SD/MMC Images"; the 0xFF800 copy is what Unlaunch
v2.0 writes when that area is zerofilled):

    0x000  16  "DSi eMMC CID/CPU"
    0x010  16  eMMC CID
    0x020   8  CPU/Console ID
    0x028  24  reserved (0)

Nothing is modified; the dump is opened read-only.

Usage:

    python3 scripts/inspect_nand_dump.py /path/to/nand.bin
"""

from __future__ import annotations

import argparse
import os
import sys

SECTOR = 512
MBR_SIG_OFF = 0x1FE
FOOTER_INPLACE = 0xFF800          # the in-image (Unlaunch) footer location
FOOTER_SIZE = 0x40                # 16 id + 16 CID + 8 console id + 24 reserved
FOOTER_MAGIC = b"DSi eMMC CID/CPU"


def hx(data: bytes) -> str:
    return " ".join("%02X" % b for b in data)


def find_footer(fh, size: int):
    """Return (offset, blob) for the first footer found, or (None, None)."""
    candidates = []
    if size >= FOOTER_INPLACE + FOOTER_SIZE:
        candidates.append(FOOTER_INPLACE)
    if size >= FOOTER_SIZE:
        candidates.append(size - FOOTER_SIZE)   # appended-at-end variant
    for off in candidates:
        fh.seek(off)
        blob = fh.read(FOOTER_SIZE)
        if len(blob) >= FOOTER_SIZE and blob[:len(FOOTER_MAGIC)] == FOOTER_MAGIC:
            return off, blob
    return None, None


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", help="path to the NAND dump (nand.bin)")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.dump):
        print("error: %s not found" % args.dump, file=sys.stderr)
        return 1

    size = os.path.getsize(args.dump)
    print("[*] %s" % args.dump)
    print("    size          : %d bytes (%d sectors, %.1f MiB)"
          % (size, size // SECTOR, size / (1024.0 * 1024.0)))
    if size % SECTOR != 0:
        print("    [!] size is not a whole number of 512-byte sectors")

    with open(args.dump, "rb") as fh:
        # ---- sector 0: plaintext MBR or encrypted? -----------------------
        sec0 = fh.read(SECTOR)
    if len(sec0) >= MBR_SIG_OFF + 2:
        sig = sec0[MBR_SIG_OFF] | (sec0[MBR_SIG_OFF + 1] << 8)
        if sig == 0xAA55:
            print("    sector 0      : plaintext MBR (0xAA55) -> DECRYPTED image")
            print("    partition 1   : type 0x%02X lba 0x%X count 0x%X"
                  % (sec0[0x1BE + 4],
                     int.from_bytes(sec0[0x1BE + 8:0x1BE + 12], "little"),
                     int.from_bytes(sec0[0x1BE + 12:0x1BE + 16], "little")))
        else:
            print("    sector 0      : no 0xAA55 -> ENCRYPTED (retail) image")
    else:
        print("    sector 0      : file too small to hold an MBR")

    # ---- no$gba / Unlaunch footer ----------------------------------------
    with open(args.dump, "rb") as fh:
        off, blob = find_footer(fh, size)

    if off is None:
        print("    footer        : none (no 'DSi eMMC CID/CPU' id found)")
        print("    -> provide CID + Console ID separately (see README notes)")
    else:
        cid = blob[0x10:0x20]
        console_id = blob[0x20:0x28]
        print("    footer        : present at 0x%X" % off)
        print("    eMMC CID      : %s" % hx(cid))
        print("    CPU/Console ID: %s" % hx(console_id))
        print("    reserved      : %s" % hx(blob[0x28:0x40]))
        print("    -> these are the two values the installer key derivation needs")

    if size >= 0x10EE00 and sec0 and (sec0[MBR_SIG_OFF] | (sec0[MBR_SIG_OFF + 1] << 8)) != 0xAA55:
        print("    note          : main FAT partition normally starts at byte")
        print("                    0x10EE00 (sector 0x877) on a retail DSi")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
