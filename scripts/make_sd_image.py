#!/usr/bin/env python3
"""Deterministic MBR + FAT16 SD-card image builder (pure Python, no root).

Builds ``build/sdcard.img``: a 16 MiB MBR-partitioned FAT16 volume populated
with real, loadable and negative-test ``.nds`` payloads for the chainloader:

  HELLO.NDS     the chainloadable payload from ``--hello`` (spans many clusters,
                deliberately fragmented so the loader's cluster walk is
                exercised)
  BADCRC.NDS    a synthetic ``.nds`` whose stored header CRC16 is wrong
                (``NDS_ERR_BAD_CRC``)
  OVERLAP.NDS   a synthetic ``.nds`` whose ARM9 load range hits the bootloader
                reservation (``NDS_ERR_OVERLAP_BOOTLOADER``)
  README.TXT    a non-executable file, ignored by the ``.nds``/``.srl`` browser
                filter

Nothing here needs ``sudo``, ``mount`` or ``mtools``: the MBR, BPB, FAT tables,
root-directory entries and data clusters are written byte by byte.

The output is byte-for-byte deterministic (fixed geometry, fixed volume serial,
no timestamps), so it can be diffed and regenerated in CI.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys

# --------------------------------------------------------------------------
# Geometry (all sizes in 512-byte sectors unless stated otherwise)
# --------------------------------------------------------------------------
SECTOR = 512
DIR_ENTRY = 32

DEFAULT_SIZE_MIB = 16
PART_LBA = 64                 # partition starts well past the MBR
SECTORS_PER_CLUSTER = 4       # 2 KiB clusters
RESERVED_SECTORS = 1
NUM_FATS = 2
ROOT_ENTRIES = 512            # 32 sectors of fixed root directory
ROOT_SECTORS = (ROOT_ENTRIES * DIR_ENTRY + SECTOR - 1) // SECTOR
MEDIA_DESCRIPTOR = 0xF8
PART_TYPE_FAT16 = 0x06

# Fixed, deterministic volume identity.
VOLUME_SERIAL = 0x1234ABCD
VOLUME_LABEL = b"CHAINLOAD  "
FS_TYPE = b"FAT16   "

# NDS header field offsets (mirror include/common/nds_header.h).
NDS_HEADER_CRC_OFF = 0x15E
NDS_HEADER_CRC_LEN = 0x15E
NDS_ARM9_ROM_OFFSET_OFF = 0x020
NDS_ARM9_ENTRY_OFF = 0x024
NDS_ARM9_RAM_ADDR_OFF = 0x028
NDS_ARM9_SIZE_OFF = 0x02C
NDS_ARM7_ROM_OFFSET_OFF = 0x030
NDS_ARM7_ENTRY_OFF = 0x034
NDS_ARM7_RAM_ADDR_OFF = 0x038
NDS_ARM7_SIZE_OFF = 0x03C
NDS_UNIT_CODE_OFF = 0x012
UNIT_CODE_NDS_DSI = 0x02

# Bootloader reservation (include/common/nds_header.h) used by OVERLAP.NDS.
BOOTLOADER_RESERVE_BASE = 0x02100000
BOOTLOADER_RESERVE_END = 0x02280000
OVERLAP_ARM9_ADDR = 0x02200000    # inside [RESERVE_BASE, RESERVE_END)


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


def put16(buf: bytearray, off: int, value: int) -> None:
    struct.pack_into("<H", buf, off, value & 0xFFFF)


def put32(buf: bytearray, off: int, value: int) -> None:
    struct.pack_into("<I", buf, off, value & 0xFFFFFFFF)


def compute_fat_size(total_sectors: int, reserved: int, nfats: int,
                     root_sectors: int, spc: int) -> int:
    """Sectors per FAT for a FAT16 volume, converged by fixed-point iteration."""
    fat_size = 1
    while True:
        data_sectors = total_sectors - (reserved + nfats * fat_size + root_sectors)
        clusters = data_sectors // spc
        needed = ((clusters + 2) * 2 + SECTOR - 1) // SECTOR
        if needed == fat_size:
            return fat_size
        fat_size = needed


# --------------------------------------------------------------------------
# Synthetic .nds files (valid header, one specific defect)
# --------------------------------------------------------------------------

def build_nds_stub(*, arm9_off: int, arm9_entry: int, arm9_ram: int,
                   arm9_size: int, arm7_off: int, arm7_entry: int,
                   arm7_ram: int, arm7_size: int, size: int,
                   title: bytes, gamecode: bytes, corrupt_crc: bool,
                   unit: int = UNIT_CODE_NDS_DSI) -> bytes:
    """Return a `size`-byte synthetic .nds with a computed (or corrupted) CRC."""
    hdr = bytearray(SECTOR)
    hdr[0:len(title)] = title
    hdr[0x0C:0x10] = gamecode
    hdr[NDS_UNIT_CODE_OFF] = unit

    put32(hdr, NDS_ARM9_ROM_OFFSET_OFF, arm9_off)
    put32(hdr, NDS_ARM9_ENTRY_OFF, arm9_entry)
    put32(hdr, NDS_ARM9_RAM_ADDR_OFF, arm9_ram)
    put32(hdr, NDS_ARM9_SIZE_OFF, arm9_size)
    put32(hdr, NDS_ARM7_ROM_OFFSET_OFF, arm7_off)
    put32(hdr, NDS_ARM7_ENTRY_OFF, arm7_entry)
    put32(hdr, NDS_ARM7_RAM_ADDR_OFF, arm7_ram)
    put32(hdr, NDS_ARM7_SIZE_OFF, arm7_size)

    crc = crc16(bytes(hdr[:NDS_HEADER_CRC_LEN]))
    if corrupt_crc:
        crc ^= 0xFFFF
    put16(hdr, NDS_HEADER_CRC_OFF, crc)

    out = bytearray(size)
    out[0:SECTOR] = hdr
    return bytes(out)


# --------------------------------------------------------------------------
# FAT16 volume writer
# --------------------------------------------------------------------------

class Fat16Writer:
    """Writes an MBR + FAT16 volume into a bytearray."""

    def __init__(self, image_sectors: int):
        if image_sectors <= PART_LBA + 1:
            raise ValueError("image too small for the chosen partition start")
        self.image_sectors = image_sectors
        self.part_sectors = image_sectors - PART_LBA
        self.fat_size = compute_fat_size(self.part_sectors, RESERVED_SECTORS,
                                         NUM_FATS, ROOT_SECTORS,
                                         SECTORS_PER_CLUSTER)
        self.fat_start = PART_LBA + RESERVED_SECTORS
        self.root_start = self.fat_start + NUM_FATS * self.fat_size
        self.data_start = self.root_start + ROOT_SECTORS
        data_sectors = self.part_sectors - (RESERVED_SECTORS +
                                            NUM_FATS * self.fat_size +
                                            ROOT_SECTORS)
        self.cluster_count = data_sectors // SECTORS_PER_CLUSTER
        if self.cluster_count < 16:
            raise ValueError("image too small for a usable FAT16 volume")

        self.img = bytearray(image_sectors * SECTOR)
        self._write_mbr()
        self._write_bpb()

    # -- low level --------------------------------------------------------
    def _sector(self, lba: int) -> int:
        return lba * SECTOR

    def _cluster_lba(self, cluster: int) -> int:
        return self.data_start + (cluster - 2) * SECTORS_PER_CLUSTER

    def set_fat16(self, cluster: int, value: int) -> None:
        for f in range(NUM_FATS):
            base = self._sector(self.fat_start + f * self.fat_size)
            put16(self.img, base + cluster * 2, value)

    # -- structures -------------------------------------------------------
    def _write_mbr(self) -> None:
        pe = 446
        self.img[pe + 0] = 0x80               # bootable flag (informational)
        # CHS start/end are informational for LBA-addressed FAT volumes.
        self.img[pe + 1] = 0x01
        self.img[pe + 2] = 0x01
        self.img[pe + 3] = 0x00
        self.img[pe + 4] = PART_TYPE_FAT16
        self.img[pe + 5] = 0xFE
        self.img[pe + 6] = 0xFF
        self.img[pe + 7] = 0xFF
        put32(self.img, pe + 8, PART_LBA)
        put32(self.img, pe + 12, self.part_sectors)
        self.img[510] = 0x55
        self.img[511] = 0xAA

    def _write_bpb(self) -> None:
        b = self._sector(PART_LBA)
        self.img[b + 0] = 0xEB
        self.img[b + 1] = 0x3C
        self.img[b + 2] = 0x90
        self.img[b + 3:b + 11] = b"MSDOS5.0"
        put16(self.img, b + 11, SECTOR)
        self.img[b + 13] = SECTORS_PER_CLUSTER
        put16(self.img, b + 14, RESERVED_SECTORS)
        self.img[b + 16] = NUM_FATS
        put16(self.img, b + 17, ROOT_ENTRIES)
        put16(self.img, b + 19, self.part_sectors)      # total16
        self.img[b + 21] = MEDIA_DESCRIPTOR
        put16(self.img, b + 22, self.fat_size)          # FAT16 size
        put16(self.img, b + 24, 63)                     # sectors per track
        put16(self.img, b + 26, 255)                    # heads
        put32(self.img, b + 28, PART_LBA)               # hidden sectors
        put32(self.img, b + 32, 0)                      # total32 (unused)
        self.img[b + 36] = 0x80                         # drive number
        self.img[b + 38] = 0x29                         # extended boot signature
        put32(self.img, b + 39, VOLUME_SERIAL)
        self.img[b + 43:b + 54] = VOLUME_LABEL
        self.img[b + 54:b + 62] = FS_TYPE
        self.img[b + 510] = 0x55
        self.img[b + 511] = 0xAA

        # Reserved FAT entries: media descriptor + end-of-chain.
        self.set_fat16(0, 0xFFF8)
        self.set_fat16(1, 0xFFFF)

    def add_file(self, name8: bytes, ext3: bytes, cluster: int, size: int,
                 data: bytes, chain: list[int]) -> None:
        """Write `data` across `chain` (starting at `cluster`) and add the
        root-directory entry."""
        cluster_bytes = SECTORS_PER_CLUSTER * SECTOR
        if chain[0] != cluster:
            raise ValueError("chain does not start at the entry cluster")
        if len(data) > len(chain) * cluster_bytes:
            raise ValueError("%s does not fit its cluster chain" % name8)

        for i, cl in enumerate(chain):
            if cl < 2 or cl > self.cluster_count + 1:
                raise ValueError("cluster %d out of range" % cl)
            chunk = data[i * cluster_bytes:(i + 1) * cluster_bytes]
            base = self._sector(self._cluster_lba(cl))
            self.img[base:base + len(chunk)] = chunk
            nxt = chain[i + 1] if i + 1 < len(chain) else 0xFFFF
            self.set_fat16(cl, nxt)

        # Root-directory entry (short 8.3 name).
        idx = self._next_root_index()
        e = self._sector(self.root_start) + idx * DIR_ENTRY
        self.img[e + 0:e + 8] = name8.ljust(8, b" ")
        self.img[e + 8:e + 11] = ext3.ljust(3, b" ")
        self.img[e + 11] = 0x20                          # archive
        put16(self.img, e + 26, cluster & 0xFFFF)
        put32(self.img, e + 28, size)

    def _next_root_index(self) -> int:
        base = self._sector(self.root_start)
        for idx in range(ROOT_ENTRIES):
            if self.img[base + idx * DIR_ENTRY] == 0x00:
                return idx
        raise ValueError("root directory is full")

    def finish(self) -> bytes:
        return bytes(self.img)


# --------------------------------------------------------------------------
# Layout
# --------------------------------------------------------------------------

def build_image(hello_bytes: bytes, size_mib: int) -> bytes:
    w = Fat16Writer(size_mib * 1024 * 1024 // SECTOR)
    cluster_bytes = SECTORS_PER_CLUSTER * SECTOR

    # HELLO.NDS: deliberately fragmented (every other cluster) and multi-cluster.
    hello_clusters = max(1, (len(hello_bytes) + cluster_bytes - 1) // cluster_bytes)
    hello_chain = [2 + 2 * i for i in range(hello_clusters)]
    cursor = 2 + 2 * hello_clusters + 1          # first free (odd) cluster

    badcrc_bytes = build_nds_stub(
        arm9_off=0x200, arm9_entry=0x02000000, arm9_ram=0x02000000,
        arm9_size=0x400,
        arm7_off=0x400, arm7_entry=0x02380000, arm7_ram=0x02380000,
        arm7_size=0x400, size=0x1000,
        title=b"BADCRC", gamecode=b"TSTL", corrupt_crc=True,
    )
    overlap_bytes = build_nds_stub(
        arm9_off=0x200, arm9_entry=OVERLAP_ARM9_ADDR,
        arm9_ram=OVERLAP_ARM9_ADDR, arm9_size=0x400,
        arm7_off=0x400, arm7_entry=0x02380000, arm7_ram=0x02380000,
        arm7_size=0x400, size=0x1000,
        title=b"OVERLAP", gamecode=b"TSTL", corrupt_crc=False,
    )
    readme_bytes = (
        b"OPENUNLAUNCH chainload SD image\r\n"
        b"\r\n"
        b"HELLO.NDS    valid, loadable chainload payload (CHAINLOAD OK)\r\n"
        b"BADCRC.NDS   rejected: bad header CRC16\r\n"
        b"OVERLAP.NDS  rejected: ARM9 range hits the bootloader\r\n"
        b"\r\n"
        b"This file is intentionally not .nds/.srl and must be hidden by\r\n"
        b"the bootloader's browser filter.\r\n"
    )

    badcrc_chain = [cursor, cursor + 2]
    overlap_chain = [cursor + 4, cursor + 6]
    readme_chain = [cursor + 8]

    w.add_file(b"HELLO", b"NDS", hello_chain[0], len(hello_bytes),
               hello_bytes, hello_chain)
    w.add_file(b"BADCRC", b"NDS", badcrc_chain[0], len(badcrc_bytes),
               badcrc_bytes, badcrc_chain)
    w.add_file(b"OVERLAP", b"NDS", overlap_chain[0], len(overlap_bytes),
               overlap_bytes, overlap_chain)
    w.add_file(b"README", b"TXT", readme_chain[0], len(readme_bytes),
               readme_bytes, readme_chain)
    return w.finish()


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--hello", default="build/hello_payload.nds",
                    help="chainloadable payload to install as HELLO.NDS")
    ap.add_argument("--out", default="build/sdcard.img",
                    help="output image path")
    ap.add_argument("--size-mib", type=int, default=DEFAULT_SIZE_MIB,
                    help="image size in MiB (default %d)" % DEFAULT_SIZE_MIB)
    args = ap.parse_args(argv)

    if not os.path.isfile(args.hello):
        ap.error("payload %s not found (run `make sd-image`)" % args.hello)
    with open(args.hello, "rb") as fh:
        hello_bytes = fh.read()
    if len(hello_bytes) < SECTOR:
        ap.error("%s is smaller than one sector" % args.hello)

    image = build_image(hello_bytes, args.size_mib)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as fh:
        fh.write(image)

    cluster_bytes = SECTORS_PER_CLUSTER * SECTOR
    print("[*] wrote %s (%d bytes, %d sectors)"
          % (args.out, len(image), len(image) // SECTOR))
    print("    MBR: partition 1 type 0x%02X at LBA %d, %d sectors"
          % (PART_TYPE_FAT16, PART_LBA, len(image) // SECTOR - PART_LBA))
    print("    FAT16: %d sectors/cluster, %d FATs, %d root entries"
          % (SECTORS_PER_CLUSTER, NUM_FATS, ROOT_ENTRIES))
    print("    HELLO.NDS   %6d bytes over %d clusters (fragmented)"
          % (len(hello_bytes),
             (len(hello_bytes) + cluster_bytes - 1) // cluster_bytes))
    print("    BADCRC.NDS  0x1000 bytes (header CRC16 corrupted)")
    print("    OVERLAP.NDS 0x1000 bytes (ARM9 load 0x%08X)"
          % OVERLAP_ARM9_ADDR)
    print("    README.TXT  non-.nds filter sample")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
