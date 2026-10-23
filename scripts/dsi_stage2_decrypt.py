#!/usr/bin/env python3
"""Decrypt the DSi boot stage-2 ("Tad") ARM9/ARM7 from a NAND dump.

Stage 2 is stored unencrypted in the boot-info block but its ARM9/ARM7 binaries
are AES-128-CTR encrypted with a *console-independent* ("Tad") key:

    keyX = "Nintendo DS\\x00\\x01#!\\x00"
    keyY = first 16 bytes of the PKCS#1 block recovered by RSA-decrypting the
           0x100-byte signature at boot-info-block offset +0x100 (byte 0x300)
           with the NAND RSA-1024 modulus below
    key  = keygen(keyX, keyY)   (hardware key generator)
    CTR  = u128 [size, -size, ~size, 0] (+1 per 16-byte block), little-endian,
           with the DSi AES byte-swap

Validated against a retail dump: the decrypted ARM9 SHA1 matches the value
stored in the signed block. See docs/stage2_re.md.

Usage:
    python3 scripts/dsi_stage2_decrypt.py nand.bin [--outdir DIR]
"""
from __future__ import annotations

import argparse
import hashlib
import os
import struct
import subprocess
import sys

# DSi NAND firmware RSA-1024 modulus (public constant; GBATEK / TWL_FIRM).
NAND_MODULUS = bytes.fromhex(
    "f1f51aff66f9b3694dcb78deaf311b783c072aac943011114a1cf6fe62b091b5"
    "ef0eba3aa9ec3ea01c5df666653e18df22533bd5e8d6ff58970b24e886fa878b"
    "62669924a8fa87f274004fea2ff623e1f2907ca4671fca283e86b6cac546a79c"
    "75c80feb32882c3d1df7d5dc1a1998e9f626d4fc76cb231358cb43a9b3cba3c5")

KEYX = b"Nintendo DS\x00\x01#!\x00"
MASK = (1 << 128) - 1
BOOT_INFO = 0x200
SIG_OFF = BOOT_INFO + 0x100          # RSA signature (0x100 bytes)
ARM9_OFF, ARM9_SIZE, ARM9_ROUND = 0x800, 0x26410, 0x26600
ARM7_OFF, ARM7_SIZE, ARM7_ROUND = 0x26E00, 0x27588, 0x27600


def rsa_signature_block(sig: bytes) -> bytes:
    n = int.from_bytes(NAND_MODULUS, "big")
    p = pow(int.from_bytes(sig, "big"), 0x10001, n).to_bytes(128, "big")
    if p[0] != 0 or p[1] != 1:
        raise SystemExit("RSA padding invalid (wrong signature/modulus)")
    # strip 00 01 FF..FF 00
    i = 2
    while p[i] == 0xFF:
        i += 1
    return p[i + 1:]                   # 0x74-byte signed block


def keygen(kx: bytes, ky: bytes) -> bytes:
    v = ((int.from_bytes(kx, "little") ^ int.from_bytes(ky, "little")) +
         0xFFFEFB4E295902582A680F5F1A4F3E79) & MASK
    v = ((v << 42) | (v >> 86)) & MASK
    return v.to_bytes(16, "big")


def _ecb(key: bytes, data: bytes) -> bytes:
    return subprocess.run(
        ["openssl", "enc", "-aes-128-ecb", "-K", key.hex(), "-nopad", "-nosalt"],
        input=data, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
        check=True).stdout


def decrypt(key: bytes, size: int, off: int, length: int, img: bytes) -> bytes:
    n = length // 16
    base = int.from_bytes(struct.pack("<IIII", size & 0xFFFFFFFF,
                                       (-size) & 0xFFFFFFFF,
                                       (~size) & 0xFFFFFFFF, 0), "little")
    ctr = b"".join((((base + i) & MASK).to_bytes(16, "little"))[::-1]
                   for i in range(n))
    ks = _ecb(key, ctr)
    out = bytearray(length)
    for k in range(n):
        kk = ks[k * 16:k * 16 + 16][::-1]
        blk = img[off + k * 16:off + k * 16 + 16]
        out[k * 16:k * 16 + 16] = bytes(a ^ b for a, b in zip(blk, kk))
    return bytes(out)


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("--outdir", default="build/stage2")
    args = ap.parse_args(argv)

    img = open(args.image, "rb").read()
    signed = rsa_signature_block(img[SIG_OFF:SIG_OFF + 0x80])
    key = keygen(KEYX, signed[0:16])
    print("[*] keyY: %s" % signed[0:16].hex())
    print("[*] key : %s" % key.hex())

    a9 = decrypt(key, ARM9_ROUND, ARM9_OFF, ARM9_SIZE, img)
    a7 = decrypt(key, ARM7_ROUND, ARM7_OFF, ARM7_SIZE, img)
    print("[*] ARM9 SHA1: %s (expect %s) -> %s"
          % (hashlib.sha1(a9).hexdigest(), signed[0x24:0x38].hex(),
             "OK" if hashlib.sha1(a9).hexdigest() == signed[0x24:0x38].hex() else "MISMATCH"))
    print("[*] ARM7 SHA1: %s (expect %s; ARM7 is compressed, so a mismatch"
          " here is expected)" % (hashlib.sha1(a7).hexdigest(), signed[0x38:0x4C].hex()))

    os.makedirs(args.outdir, exist_ok=True)
    open(os.path.join(args.outdir, "arm9.bin"), "wb").write(a9)
    open(os.path.join(args.outdir, "arm7.bin"), "wb").write(a7)
    print("[*] wrote %s/arm9.bin and %s/arm7.bin" % (args.outdir, args.outdir))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
