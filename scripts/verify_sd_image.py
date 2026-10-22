#!/usr/bin/env python3
"""Verify the deterministic SD image produced by ``scripts/make_sd_image.py``.

This is an independent re-implementation of the on-disk checks the C parser and
the chainloader safety gate perform, so a regression in either side is caught:

  1. MBR signature + FAT16 partition entry at the expected LBA.
  2. FAT16 BPB geometry (512-byte sectors, power-of-two cluster, FATs, root).
  3. Root-directory entries for HELLO.NDS / BADCRC.NDS / OVERLAP.NDS /
     README.TXT, and that README.TXT is not a .nds/.srl file.
  4. HELLO.NDS follows a multi-cluster (fragmented) chain whose total size
     matches the directory entry and (when ``--hello`` is given) the payload.
  5. BADCRC.NDS has a corrupted header CRC16 (``NDS_ERR_BAD_CRC``).
  6. OVERLAP.NDS has a valid header CRC16 but an ARM9 load range inside the
     bootloader reservation (``NDS_ERR_OVERLAP_BOOTLOADER``).

Exit status is non-zero if any check fails.
"""

from __future__ import annotations

import argparse
import struct
import sys

SECTOR = 512
DIR_ENTRY = 32
PART_LBA = 64
PART_TYPE_FAT16 = 0x06
BOOTLOADER_RESERVE_BASE = 0x02100000
BOOTLOADER_RESERVE_END = 0x02280000
NDS_HEADER_CRC_OFF = 0x15E
NDS_HEADER_CRC_LEN = 0x15E


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


def u16(buf, off):
    return struct.unpack_from("<H", buf, off)[0]


def u32(buf, off):
    return struct.unpack_from("<I", buf, off)[0]


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


def short_name(e: bytes) -> str:
    base = e[0:8].decode("ascii", "replace").rstrip(" ")
    ext = e[8:11].decode("ascii", "replace").rstrip(" ")
    return base + ("." + ext if ext else "")


def is_nds_name(name: str) -> bool:
    return name.upper().endswith(".NDS") or name.upper().endswith(".SRL")


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--image", required=True)
    ap.add_argument("--hello", help="payload expected to be installed as HELLO.NDS")
    args = ap.parse_args(argv)

    with open(args.image, "rb") as fh:
        img = fh.read()

    c = Checker()
    print("[*] verifying %s (%d bytes)" % (args.image, len(img)))

    c.check(len(img) >= 2 * 1024 * 1024 and len(img) % SECTOR == 0,
            "image is a whole number of 512-byte sectors and >= 2 MiB",
            "got %d bytes" % len(img))
    if len(img) % SECTOR != 0 or len(img) < SECTOR:
        print("[*] %d checks, %d failures" % (c.checks, c.failures))
        return 1

    total_sectors = len(img) // SECTOR

    # ---- MBR ------------------------------------------------------------
    mbr = img[0:SECTOR]
    c.check(mbr[510] == 0x55 and mbr[511] == 0xAA, "MBR 0xAA55 signature")
    pe = 446
    part_type = mbr[pe + 4]
    part_lba = u32(mbr, pe + 8)
    part_count = u32(mbr, pe + 12)
    c.check(part_type in (0x04, 0x06, 0x0B, 0x0C, 0x0E),
            "partition 1 is a FAT type", "got 0x%02X" % part_type)
    c.check(part_lba == PART_LBA, "partition 1 starts at LBA %d" % PART_LBA,
            "got %d" % part_lba)
    c.check(part_count == total_sectors - part_lba,
            "partition size covers the rest of the image",
            "got %d" % part_count)
    c.check(part_lba + part_count <= total_sectors,
            "partition lies inside the image")

    # ---- BPB ------------------------------------------------------------
    base = part_lba
    bpb = img[base * SECTOR:(base + 1) * SECTOR]
    bytes_per_sector = u16(bpb, 11)
    spc = bpb[13]
    reserved = u16(bpb, 14)
    nfats = bpb[16]
    root_entries = u16(bpb, 17)
    total16 = u16(bpb, 19)
    fat_size16 = u16(bpb, 22)
    fs_type = bpb[54:62].decode("ascii", "replace")

    c.check(bytes_per_sector == SECTOR, "bytes per sector is 512",
            "got %d" % bytes_per_sector)
    c.check(spc != 0 and (spc & (spc - 1)) == 0,
            "sectors per cluster is a power of two", "got %d" % spc)
    c.check(nfats >= 1, "at least one FAT", "got %d" % nfats)
    c.check(root_entries > 0 and fat_size16 > 0,
            "FAT16 fixed root + 16-bit FAT size")
    c.check(fs_type.startswith("FAT16"), "volume type string is FAT16",
            "got %r" % fs_type)
    c.check(bpb[510] == 0x55 and bpb[511] == 0xAA,
            "VBR 0xAA55 signature")

    total = total16 or u32(bpb, 32)
    fat_size = fat_size16 or u32(bpb, 36)
    root_sectors = (root_entries * DIR_ENTRY + SECTOR - 1) // SECTOR
    fat_start = base + reserved
    root_start = fat_start + nfats * fat_size
    data_start = root_start + root_sectors
    overhead = reserved + nfats * fat_size + root_sectors
    cluster_count = (total - overhead) // spc
    c.check(total > overhead and cluster_count > 0, "data region is non-empty")

    def fat16(cluster: int) -> int:
        return u16(img, fat_start * SECTOR + cluster * 2)

    def cluster_lba(cluster: int) -> int:
        return data_start + (cluster - 2) * spc

    def read_chain(start: int):
        """Return (clusters, bytes) following the FAT16 chain, bounded."""
        clusters = []
        data = bytearray()
        cl = start
        limit = cluster_count + 4
        while cl >= 2 and cl <= cluster_count + 1 and len(clusters) < limit:
            clusters.append(cl)
            data += img[cluster_lba(cl) * SECTOR:
                        (cluster_lba(cl) + spc) * SECTOR]
            nxt = fat16(cl)
            if nxt >= 0xFFF8 or nxt == 0:
                break
            cl = nxt
        return clusters, bytes(data)

    # ---- root directory -------------------------------------------------
    entries = {}
    for idx in range(root_entries):
        e = img[(root_start * SECTOR) + idx * DIR_ENTRY:
                (root_start * SECTOR) + (idx + 1) * DIR_ENTRY]
        if e[0] == 0x00:
            break
        if e[0] == 0xE5:
            continue
        attr = e[11]
        if (attr & 0x0F) == 0x0F or (attr & 0x08) or (attr & 0x10):
            continue
        name = short_name(e)
        entries[name] = {
            "cluster": u16(e, 26),
            "size": u32(e, 28),
            "attr": attr,
        }

    for name in ("HELLO.NDS", "BADCRC.NDS", "OVERLAP.NDS", "README.TXT"):
        c.check(name in entries, "root entry present: %s" % name)
    c.check(is_nds_name("HELLO.NDS") and is_nds_name("BADCRC.NDS")
            and is_nds_name("OVERLAP.NDS"),
            "the three .nds files pass the browser filter")
    c.check(not is_nds_name("README.TXT"),
            "README.TXT is filtered out by the .nds/.srl browser filter")

    if "HELLO.NDS" not in entries:
        print("[*] %d checks, %d failures" % (c.checks, c.failures))
        return 1

    # ---- HELLO.NDS: fragmented multi-cluster chain ----------------------
    hello = entries["HELLO.NDS"]
    hello_clusters, hello_data = read_chain(hello["cluster"])
    c.check(len(hello_clusters) >= 2,
            "HELLO.NDS spans multiple clusters",
            "chain length %d" % len(hello_clusters))
    c.check(len(hello_clusters) >= 2 and
            any(hello_clusters[i + 1] != hello_clusters[i] + 1
                for i in range(len(hello_clusters) - 1)),
            "HELLO.NDS cluster chain is fragmented (non-contiguous)")
    c.check(len(hello_data) >= hello["size"],
            "HELLO.NDS chain holds the declared size")
    hello_bytes = hello_data[:hello["size"]]
    if args.hello:
        with open(args.hello, "rb") as fh:
            payload = fh.read()
        c.check(hello_bytes == payload,
                "HELLO.NDS bytes match %s" % args.hello,
                "sizes %d vs %d" % (len(hello_bytes), len(payload)))

    # The payload header must itself be a valid, loadable .nds image.
    if len(hello_bytes) >= SECTOR:
        stored = u16(hello_bytes, NDS_HEADER_CRC_OFF)
        computed = crc16(hello_bytes[:NDS_HEADER_CRC_LEN])
        c.check(stored == computed, "HELLO.NDS header CRC16 is valid",
                "stored 0x%04X computed 0x%04X" % (stored, computed))
        c.check(hello_bytes[0x12] == 0x02,
                "HELLO.NDS unit code is the NDS+DSi hybrid (0x02)",
                "got 0x%02X" % hello_bytes[0x12])
        c.check(hello_bytes[0:12].decode("ascii", "replace").rstrip() ==
                "CHAINLOAD OK",
                "HELLO.NDS title is CHAINLOAD OK",
                "got %r" % hello_bytes[0:12])
        c.check(hello_bytes[0x0C:0x10] == b"TSTL",
                "HELLO.NDS game code is TSTL",
                "got %r" % hello_bytes[0x0C:0x10])

    # ---- BADCRC.NDS -----------------------------------------------------
    if "BADCRC.NDS" in entries:
        _, bad = read_chain(entries["BADCRC.NDS"]["cluster"])
        stored = u16(bad, NDS_HEADER_CRC_OFF)
        computed = crc16(bad[:NDS_HEADER_CRC_LEN])
        c.check(stored != computed,
                "BADCRC.NDS header CRC16 is deliberately wrong",
                "stored 0x%04X computed 0x%04X" % (stored, computed))

    # ---- OVERLAP.NDS ----------------------------------------------------
    if "OVERLAP.NDS" in entries:
        _, ov = read_chain(entries["OVERLAP.NDS"]["cluster"])
        stored = u16(ov, NDS_HEADER_CRC_OFF)
        computed = crc16(ov[:NDS_HEADER_CRC_LEN])
        c.check(stored == computed,
                "OVERLAP.NDS header CRC16 is valid (fails on range, not CRC)")
        arm9_ram = u32(ov, 0x028)
        arm9_size = u32(ov, 0x02C)
        overlaps = (arm9_ram < BOOTLOADER_RESERVE_END and
                    arm9_ram + arm9_size > BOOTLOADER_RESERVE_BASE)
        c.check(overlaps,
                "OVERLAP.NDS ARM9 load range hits the bootloader reservation",
                "ram 0x%08X size 0x%X" % (arm9_ram, arm9_size))

    print("[*] %d checks, %d failures" % (c.checks, c.failures))
    return 1 if c.failures else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
