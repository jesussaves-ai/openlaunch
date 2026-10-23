#!/usr/bin/env python3
"""Offline DSi NAND installer (SafeUnlaunch-style) for a *decrypted* image.

This is the filesystem half of the offline installer. It operates on a
already-decrypted NAND image (produce one with ``build/tools/dsi_nand_tool
decrypt``) and edits the ``twln`` FAT16 in place: it can replace the payload
inside the backup-launcher (HNAA) ``title.tmd`` the same way the "Safe unlaunch
installer" does. Re-encrypt the result with ``dsi_nand_tool encrypt``.

Commands:
    list      <dec.img>                     list System Menu launchers + TMDs
    extract   <dec.img> <region> <out>      dump the region's backup TMD
    replace   <dec.img> <out.img> <region> --payload <blob> [--keep-prefix]
                                            write <blob> into that TMD payload
    verify    <dec.img> <region>            read TMD back and report

The safe-unlaunch install lives in the *backup* launcher (HNAA); the real
launcher's TMD is left alone. See docs/hardware_notes.md for the layout.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys

SEC = 512
DIR_ENTRY = 32
EOC_FAT16 = 0xFFF8


def u16(b, o):
    return struct.unpack_from("<H", b, o)[0]


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


class Fat16:
    """Read/write FAT16 on a plaintext (decrypted) eMMC image."""

    def __init__(self, img: bytearray, part_lba: int):
        self.img = img
        self.part = part_lba
        bpb = self.sector(part_lba)
        self.spc = bpb[13]
        self.reserved = u16(bpb, 14)
        self.nfats = bpb[16]
        self.root_entries = u16(bpb, 17)
        self.fat_size = u16(bpb, 22)
        self.fat_start = part_lba + self.reserved
        self.root_start = self.fat_start + self.nfats * self.fat_size
        self.root_sectors = (self.root_entries * DIR_ENTRY + SEC - 1) // SEC
        self.data_start = self.root_start + self.root_sectors
        self.cluster_bytes = self.spc * SEC

    # -- raw sector access -------------------------------------------------
    def sector(self, lba):
        o = lba * SEC
        return bytes(self.img[o:o + SEC])

    def put(self, addr, data):
        self.img[addr:addr + len(data)] = data

    def cl_lba(self, cl):
        return self.data_start + (cl - 2) * self.spc

    def read_cluster(self, cl):
        o = self.cl_lba(cl) * SEC
        return bytes(self.img[o:o + self.cluster_bytes])

    def write_cluster(self, cl, data):
        assert len(data) == self.cluster_bytes
        self.put(self.cl_lba(cl) * SEC, data)

    # -- FAT ---------------------------------------------------------------
    def fat16(self, cl, f=0):
        o = (self.fat_start + f * self.fat_size) * SEC + cl * 2
        return u16(self.img, o)

    def set_fat(self, cl, value):
        for f in range(self.nfats):
            o = (self.fat_start + f * self.fat_size) * SEC + cl * 2
            struct.pack_into("<H", self.img, o, value)

    def max_cluster(self):
        bpb = self.sector(self.part)
        total = u16(bpb, 19) or u32(bpb, 32)
        data = total - (self.reserved + self.nfats * self.fat_size +
                        self.root_sectors)
        return data // self.spc + 1

    def chain(self, cl):
        out = []
        while 2 <= cl < EOC_FAT16 and len(out) < 200000:
            out.append(cl)
            cl = self.fat16(cl)
        return out

    def alloc(self):
        for cl in range(2, self.max_cluster() + 1):
            if self.fat16(cl) == 0:
                self.set_fat(cl, 0xFFFF)
                return cl
        raise RuntimeError("no free cluster")

    def free_chain(self, chain):
        for c in chain:
            self.set_fat(c, 0)

    def read_file(self, start_cl, size):
        d = bytearray()
        for c in self.chain(start_cl):
            d += self.read_cluster(c)
        return bytes(d[:size])

    # -- directories -------------------------------------------------------
    def dir_raw(self, cl):
        if cl == 0:
            o = self.root_start * SEC
            return self.root_start * SEC, self.root_sectors * SEC
        cl0 = self.chain(cl)
        # directories are contiguous-enough for our use; read first cluster only
        return self.cl_lba(cl0[0]) * SEC, self.cluster_bytes

    def entries(self, cl):
        base, length = self.dir_raw(cl)
        out = []
        for i in range(0, length, DIR_ENTRY):
            e = self.img[base + i:base + i + DIR_ENTRY]
            if e[0] == 0x00:
                break
            if e[0] == 0xE5 or (e[11] & 0x0F) == 0x0F or (e[11] & 0x08):
                continue
            nm = e[0:8].decode("ascii", "replace").rstrip(" ")
            ex = e[8:11].decode("ascii", "replace").rstrip(" ")
            out.append({"name": nm + ("." + ex if ex else ""), "attr": e[11],
                        "cluster": u16(e, 26), "size": u32(e, 28),
                        "addr": base + i})
        return out

    def find(self, cl, name):
        for e in self.entries(cl):
            if e["name"].upper() == name.upper():
                return e
        return None

    def set_entry(self, addr, cluster, size, attr=None):
        e = bytearray(self.img[addr:addr + DIR_ENTRY])
        struct.pack_into("<H", e, 26, cluster & 0xFFFF)
        struct.pack_into("<I", e, 28, size & 0xFFFFFFFF)
        if attr is not None:
            e[11] = attr
        self.put(addr, bytes(e))

    # -- file rewrite ------------------------------------------------------
    def write_file(self, entry, data: bytes, attr=None):
        """Rewrite `entry`'s contents to `data` (reusing then allocating)."""
        need = max(1, (len(data) + self.cluster_bytes - 1) // self.cluster_bytes)
        old = self.chain(entry["cluster"]) if entry["cluster"] >= 2 else []
        chain = old[:need]
        while len(chain) < need:
            chain.append(self.alloc())
        if len(old) > need:
            self.free_chain(old[need:])
        for i, c in enumerate(chain):
            nxt = chain[i + 1] if i + 1 < len(chain) else 0xFFFF
            self.set_fat(c, nxt)
        for i, c in enumerate(chain):
            chunk = data[i * self.cluster_bytes:(i + 1) * self.cluster_bytes]
            self.write_cluster(c, chunk.ljust(self.cluster_bytes, b"\x00"))
        self.set_entry(entry["addr"], chain[0] if chain else 0, len(data), attr)
        return chain[0], len(data)


def open_image(path):
    with open(path, "rb") as fh:
        return bytearray(fh.read())


def find_twln(img):
    if img[0x1FE] != 0x55 or img[0x1FF] != 0xAA:
        raise SystemExit("not a decrypted image (no 0x55AA MBR)")
    return u32(img, 0x1BE + 8), u32(img, 0x1BE + 12)


def menu_regions(fat):
    title = fat.find(0, "title")
    t17 = fat.find(title["cluster"], "00030017")
    return [e for e in fat.entries(t17["cluster"])
            if (e["attr"] & 0x10) and e["name"] not in (".", "..")]


def region_tmd(fat, region):
    title = fat.find(0, "title")
    t17 = fat.find(title["cluster"], "00030017")
    reg = fat.find(t17["cluster"], region)
    if reg is None or not (reg["attr"] & 0x10):
        raise SystemExit("region %s not found" % region)
    content = fat.find(reg["cluster"], "content")
    if content is None:
        raise SystemExit("region %s has no content dir" % region)
    tmd = fat.find(content["cluster"], "TITLE.TMD")
    if tmd is None:
        raise SystemExit("region %s has no TITLE.TMD" % region)
    return tmd


def cmd_list(args):
    img = open_image(args.image)
    part, _ = find_twln(img)
    fat = Fat16(img, part)
    print("twln @ lba 0x%X, %d bytes/cluster" % (part, fat.cluster_bytes))
    for reg in menu_regions(fat):
        tmd = region_tmd(fat, reg["name"])
        data = fat.read_file(tmd["cluster"], min(tmd["size"], 0x10))
        print("  region %-10s tmd size=%-8d attr=0x%02X head=%s"
              % (reg["name"], tmd["size"], tmd["attr"], data.hex()))
    return 0


def cmd_extract(args):
    img = open_image(args.image)
    part, _ = find_twln(img)
    fat = Fat16(img, part)
    tmd = region_tmd(fat, args.region)
    data = fat.read_file(tmd["cluster"], tmd["size"])
    with open(args.out, "wb") as fh:
        fh.write(data)
    print("[*] wrote %s (%d bytes)" % (args.out, len(data)))
    return 0


def cmd_replace(args):
    img = open_image(args.image)
    part, _ = find_twln(img)
    fat = Fat16(img, part)
    tmd = region_tmd(fat, args.region)
    old = fat.read_file(tmd["cluster"], tmd["size"])
    with open(args.payload, "rb") as fh:
        payload = fh.read()
    prefix = old[:520] if args.keep_prefix else bytes(520)
    new = bytearray(prefix + payload)
    target = args.tmd_size if args.tmd_size else len(new)
    if target < len(new):
        raise SystemExit("tmd-size %d smaller than 520+payload %d" % (target, len(new)))
    new.extend(b"\x00" * (target - len(new)))
    # keep the existing read-only/archive attribute
    attr = tmd["attr"] & 0xFF
    start, size = fat.write_file(tmd, bytes(new), attr=attr)
    with open(args.out, "wb") as fh:
        fh.write(img)
    print("[*] %s: tmd %d -> %d bytes (start cluster %d, attr 0x%02X)"
          % (args.region, tmd["size"], size, start, attr))
    print("[*] wrote %s" % args.out)
    return 0


def cmd_verify(args):
    img = open_image(args.image)
    part, _ = find_twln(img)
    fat = Fat16(img, part)
    tmd = region_tmd(fat, args.region)
    data = fat.read_file(tmd["cluster"], tmd["size"])
    print("region %s tmd size=%d attr=0x%02X" % (args.region, tmd["size"], tmd["attr"]))
    if tmd["size"] > 0x208:
        print("  payload[0:24] =", data[0x208:0x220].hex())
    # FAT copies must agree
    mism = 0
    for cl in fat.chain(tmd["cluster"]):
        if fat.fat16(cl, 0) != fat.fat16(cl, 1):
            mism += 1
    print("  fat-copy mismatches on chain:", mism)
    return 0 if mism == 0 else 1


FOOTER_MAGIC = b"DSi eMMC CID/CPU"


def read_footer(img):
    """Return (cid, console_id) from the "DSi eMMC CID/CPU" footer.

    Accepts both conventions: the in-place footer at 0xFF800 (used by our own
    images) and the dumpTool-style footer appended after the 240 MiB image at
    0x0F000000 (equivalently the last 0x40 bytes), so a stock dumpTool image
    decrypts with no extra arguments.
    """
    for off in (0xFF800, 0x0F000000, len(img) - 0x40):
        if off < 0 or off + 0x40 > len(img):
            continue
        blob = img[off:off + 0x40]
        if blob[:len(FOOTER_MAGIC)] == FOOTER_MAGIC:
            return blob[0x10:0x20], blob[0x20:0x28]
    raise SystemExit("no footer at 0xFF800 or appended; pass --cid/--console")


def run_tool(tool, *args):
    import subprocess
    cmd = [sys.executable, tool] if tool.endswith(".py") else [tool]
    cmd += list(args)
    r = subprocess.run(cmd)
    if r.returncode != 0:
        raise SystemExit("tool failed: %s" % " ".join(cmd))


def cmd_patch(args):
    """Full decrypt -> replace -> encrypt pipeline on an *encrypted* image."""
    import tempfile
    with open(args.image, "rb") as fh:
        enc = fh.read()
    cid, con = read_footer(enc)
    if args.cid:
        cid = bytes.fromhex(args.cid)
    if args.console:
        con = bytes.fromhex(args.console)

    with tempfile.TemporaryDirectory() as tmp:
        dec = os.path.join(tmp, "dec.bin")
        out_dec = os.path.join(tmp, "out_dec.bin")
        run_tool(args.tool, "decrypt", args.image, dec)
        # replace on the decrypted image
        rargs = argparse.Namespace(image=dec, out=out_dec, region=args.region,
                                   payload=args.payload,
                                   keep_prefix=args.keep_prefix,
                                   tmd_size=args.tmd_size)
        cmd_replace(rargs)
        run_tool(args.tool, "encrypt", out_dec, args.out,
                 "--cid", cid.hex(), "--console", con.hex())

    # report the changed-sector count vs the input
    with open(args.out, "rb") as fh:
        new = fh.read()
    n = sum(1 for i in range(0, min(len(enc), len(new)), SEC)
            if enc[i:i + SEC] != new[i:i + SEC])
    print("[*] %s: %d sectors changed vs input" % (args.out, n))
    return 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("list"); p.add_argument("image"); p.set_defaults(fn=cmd_list)
    p = sub.add_parser("extract")
    p.add_argument("image"); p.add_argument("region"); p.add_argument("out")
    p.set_defaults(fn=cmd_extract)
    p = sub.add_parser("replace")
    p.add_argument("image"); p.add_argument("out"); p.add_argument("region")
    p.add_argument("--payload", required=True)
    p.add_argument("--keep-prefix", action="store_true",
                   help="keep the existing 520-byte TMD header instead of zeros")
    p.add_argument("--tmd-size", type=lambda s: int(s, 0), default=0,
                   help="pad the TMD to this size (default: exact)")
    p.set_defaults(fn=cmd_replace)
    p = sub.add_parser("verify")
    p.add_argument("image"); p.add_argument("region")
    p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("patch",
                       help="full decrypt->replace->encrypt on an encrypted image")
    p.add_argument("image"); p.add_argument("out"); p.add_argument("region")
    p.add_argument("--payload", required=True)
    p.add_argument("--keep-prefix", action="store_true")
    p.add_argument("--tmd-size", type=lambda s: int(s, 0), default=0)
    p.add_argument("--tool", default=os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "build", "tools", "dsi_nand_tool"))
    p.add_argument("--cid", default=None)
    p.add_argument("--console", default=None)
    p.set_defaults(fn=cmd_patch)
    args = ap.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
