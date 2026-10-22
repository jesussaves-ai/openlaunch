#!/usr/bin/env python3
"""Standalone Nintendo LZ77 / RLE decompressor CLI.

This mirrors ``nds_lz77_decompress`` / ``nds_rle_decompress`` in
``src/common/lz77.c`` and is handy for poking at individual blobs:

    python3 scripts/decompress_arm9.py asm/arm9_raw.bin out.bin [offset]

The canonical UNLAUNCH ARM9 pipeline (crt0 relocation + LZ77 + VRAM image) is
``scripts/unpack_unlaunch_arm9.py``; this tool only expands a single stream.
"""

import sys


def lz77_decompress(src: bytes) -> bytes:
    if not src or src[0] not in (0x10, 0x11):
        raise ValueError("not an LZ77 type-0x10/0x11 stream")
    size = src[1] | (src[2] << 8) | (src[3] << 16)
    out = bytearray()
    i = 4
    while len(out) < size:
        flags = src[i]
        i += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if not (flags & (0x80 >> bit)):
                out.append(src[i])
                i += 1
            else:
                b1, b2 = src[i], src[i + 1]
                i += 2
                length = (b1 >> 4) + 3
                disp = ((b1 & 0x0F) << 8) | b2
                if src[0] == 0x11:
                    if length == 3:
                        b3 = src[i]
                        i += 1
                        length = (((b1 & 0x0F) << 12) | (b2 << 4) | (b3 >> 4)) + 0x11
                        disp = ((disp & 0x0F) << 8) | (b3 & 0x0F)
                    elif length == 4:
                        b3, b4 = src[i], src[i + 1]
                        i += 2
                        length = (((b1 & 0x0F) << 18) | (b2 << 10) | (b3 << 2) | (b4 >> 6)) + 0x211
                        disp = (disp << 8) | b4
                if disp + 1 > len(out):
                    raise ValueError("back-reference before start of output")
                for _ in range(length):
                    out.append(out[-disp - 1])
    return bytes(out)


def rle_decompress(src: bytes) -> bytes:
    if not src or src[0] != 0x30:
        raise ValueError("not an RLE type-0x30 stream")
    size = src[1] | (src[2] << 8) | (src[3] << 16)
    out = bytearray()
    i = 4
    while len(out) < size:
        flags = src[i]
        i += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if not (flags & (0x80 >> bit)):
                out.append(src[i])
                i += 1
            else:
                run = (src[i] & 0x7F) + 3
                value = src[i + 1]
                i += 2
                out.extend([value] * run)
    return bytes(out[:size])


def decompress(src: bytes) -> bytes:
    if not src:
        raise ValueError("empty input")
    if src[0] in (0x10, 0x11):
        return lz77_decompress(src)
    if src[0] == 0x30:
        return rle_decompress(src)
    raise ValueError("unsupported type 0x%02X" % src[0])


def main(argv) -> int:
    if len(argv) < 3:
        print(__doc__)
        return 2
    with open(argv[1], "rb") as fh:
        data = fh.read()
    offset = int(argv[3], 0) if len(argv) > 3 else 0
    out = decompress(data[offset:])
    with open(argv[2], "wb") as fh:
        fh.write(out)
    print("[*] %s -> %s (%d bytes)" % (argv[1], argv[2], len(out)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
