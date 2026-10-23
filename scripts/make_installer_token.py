#!/usr/bin/env python3
"""Build a signed OpenLaunch NAND-installer token (``INSTALL.KEY``).

The installer in the bootloader refuses to write the eMMC unless it is handed a
96-byte token whose HMAC-SHA-256 signature verifies (see
``include/common/installer.h`` and ``src/common/installer.c``). This tool is the
packaging side of that handshake: it hashes the payload image and signs the
target descriptor with the build's authentication key.

Token layout (little-endian, 96 bytes)::

    0x00  u32  magic           0x4F4C4E43 ("OLNC")
    0x04  u16  version         1
    0x06  u16  flags           bit0 = sandbox package, else 0
    0x08  u32  target_lba      first eMMC sector to overwrite
    0x0C  u32  target_sectors  number of sectors to overwrite
    0x10  u32  payload_sectors payload size in sectors (== target_sectors)
    0x14  u8[32] payload_sha256 SHA-256 of the zero-padded payload region
    0x34  u8[12] reserved      zero
    0x40  u8[32] signature     HMAC-SHA-256(key, bytes[0x00:0x40])

The default key matches ``INSTALLER_AUTH_KEY`` in ``src/common/installer.c`` and
is a *development* key. Override it at build time with
``-DINSTALLER_AUTH_KEY=...`` and here with ``--key``.

IMPORTANT: the token is a runtime authorisation layered on top of the mandatory
``ENABLE_NAND_WRITE_DANGER`` compile-time gate (the project conventions). A token alone can
never enable writing in a read-only build.
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import os
import struct
import sys

MAGIC = 0x4F4C4E43          # "OLNC"
VERSION = 1
TOKEN_SIZE = 96
SIGNED_LEN = 0x40
SECTOR = 512
DEFAULT_KEY = b"OpenLaunch-NAND-installer-dev-key"

# Token flag bit 0: sandbox package. Lets the installer bound its backup to a
# size that fits a small emulator SD image (see INSTALLER_TOKEN_FLAG_SANDBOX in
# include/common/installer.h).
FLAG_SANDBOX = 0x0001
# Token flag bit 1: authorises a raw write to a retail console. Only honoured by
# a bootloader built with the loud ENABLE_RETAIL_NAND_WRITE flag. DANGER.
FLAG_RETAIL = 0x0002


def build_token(payload: bytes, target_lba: int, key: bytes,
                target_sectors: int | None = None,
                flags: int = 0) -> bytes:
    payload_sectors = (len(payload) + SECTOR - 1) // SECTOR
    if payload_sectors == 0:
        raise SystemExit("token: payload is empty")
    sectors = payload_sectors if target_sectors is None else target_sectors
    if sectors != payload_sectors:
        raise SystemExit(
            "token: target_sectors (%d) must equal the payload size in sectors "
            "(%d)" % (sectors, payload_sectors))

    padded = payload + b"\x00" * (sectors * SECTOR - len(payload))
    digest = hashlib.sha256(padded).digest()

    head = struct.pack("<IHHIII", MAGIC, VERSION, flags & 0xFFFF,
                       target_lba, sectors, payload_sectors) + digest
    head = head + b"\x00" * 12          # reserved field (0x34..0x3F)
    assert len(head) == SIGNED_LEN, len(head)

    sig = hmac.new(key, head, hashlib.sha256).digest()
    token = head + sig
    assert len(token) == TOKEN_SIZE, len(token)
    return token


def verify_token(token: bytes, key: bytes) -> bool:
    if len(token) != TOKEN_SIZE:
        return False
    return hmac.compare_digest(
        token[SIGNED_LEN:], hmac.new(key, token[:SIGNED_LEN], hashlib.sha256).digest())


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--payload", required=True,
                    help="payload image to install (e.g. OLBOOT.NDS)")
    ap.add_argument("--target-lba", required=True, type=lambda s: int(s, 0),
                    help="first eMMC sector to overwrite (decimal or 0x...)")
    ap.add_argument("--target-sectors", type=lambda s: int(s, 0), default=None,
                    help="override (must equal the padded payload size)")
    ap.add_argument("--key", default=None,
                    help="authentication key string (defaults to the build key)")
    ap.add_argument("--out", default="INSTALL.KEY",
                    help="output token path (default INSTALL.KEY)")
    ap.add_argument("--sandbox", action="store_true",
                    help="mark the package as a sandbox build: the installer "
                         "bounds its backup to fit a small emulator SD image")
    ap.add_argument("--retail", action="store_true",
                    help="DANGER: authorise a raw write to a retail console "
                         "(only honoured by an installer-retail build; the "
                         "target must lie inside the bootsafe window)")
    args = ap.parse_args(argv)

    with open(args.payload, "rb") as fh:
        payload = fh.read()

    key = (args.key.encode("utf-8") if args.key is not None else DEFAULT_KEY)

    flags = (FLAG_SANDBOX if args.sandbox else 0) | \
            (FLAG_RETAIL if args.retail else 0)
    token = build_token(payload, args.target_lba, key, args.target_sectors,
                        flags)
    if not verify_token(token, key):
        raise SystemExit("token: self-verification failed")

    with open(args.out, "wb") as fh:
        fh.write(token)

    digest = token[0x14:0x34].hex()
    print("[*] wrote %s (%d bytes)" % (args.out, len(token)))
    print("    target LBA    : 0x%X (%d)" % (args.target_lba, args.target_lba))
    print("    target sectors: %d" % struct.unpack_from("<I", token, 0x0C)[0])
    print("    payload sha256: %s" % digest)
    if args.key is None:
        print("    key           : <build default>")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
