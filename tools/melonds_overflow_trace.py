#!/usr/bin/env python3
"""Trace the DSi stage-2 TMD overflow in melonDS via its GDB stub (:3333).

melonDS runs the real DSi stage 2 from the NAND and exposes a GDB stub, but the
stock gdb client fights the stub over async state, so this drives the remote
protocol directly (breakpoints, memory, registers, continue).

Setup:
  1. ~/.config/melonDS/melonDS.toml
       [Instance0.Gdb]
       Enabled = true
       [Instance0.Gdb.ARM9]
       BreakOnStartup = true
       Port = 3333
     and [DSi] NANDPath / BIOS7Path / BIOS9Path / FirmwarePath set.
  2. QT_QPA_PLATFORM=offscreen melonDS --boot always &
  3. python3 tools/melonds_overflow_trace.py

Observed against a SafeUnlaunch NAND: the loader 0x037BAA18 fires twice
(real launcher HNAE=0x484e4145, then backup HNAA=0x484e4141) and both TMDs are
read into the fixed buffer 0x037DF06C with the same size variable - the HNAA
read is the overflow. See docs/stage2_re.md.
"""
from __future__ import annotations

import argparse
import socket
import struct
import time

LOADER = 0x037BAA18        # vulnerable launcher-TMD loader
BUF = 0x037DF06C           # fixed TMD destination buffer
PAYLOAD_ARM9 = 0x02200800  # Unlaunch container ARM9 entry
PAYLOAD_ARM7 = 0x02380000


class Gdb:
    def __init__(self, host="127.0.0.1", port=3333, timeout=15):
        self.s = socket.create_connection((host, port), timeout=timeout)
        self.s.settimeout(timeout)
        self.buf = b""

    def _byte(self):
        if not self.buf:
            self.buf = self.s.recv(4096)
            if not self.buf:
                raise EOFError("stub closed")
        b, self.buf = self.buf[0:1], self.buf[1:]
        return b

    def _pkt(self):
        while self._byte() != b"$":
            pass
        data = b""
        while True:
            b = self._byte()
            if b == b"#":
                break
            data += b
        self._byte(); self._byte()          # checksum
        self.s.sendall(b"+")
        return data.decode("latin1")

    def raw(self, b):
        self.s.sendall(b)

    def cmd(self, s):
        p = s.encode("latin1")
        self.s.sendall(b"$" + p + b"#" + ("%02x" % (sum(p) & 0xFF)).encode())
        return self._pkt()

    def read_mem(self, addr, n):
        out = bytearray()
        while n:
            k = min(n, 0x400)
            r = self.cmd("m%x,%x" % (addr, k))
            out += bytes.fromhex(r)
            addr += k; n -= k
        return bytes(out)

    def bkpt(self, a):
        return self.cmd("Z0,%x,4" % a)

    def unbkpt(self, a):
        return self.cmd("z0,%x,4" % a)

    def regs(self):
        b = bytes.fromhex(self.cmd("g"))
        return struct.unpack("<%dI" % (len(b) // 4), b)

    def cont(self):
        return self.cmd("c")

    def close(self):
        self.s.close()


def connect(retries=6):
    last = None
    for _ in range(retries):
        try:
            g = Gdb()
            g.raw(b"+")
            g.cmd("qSupported:multiprocess+;swbreak+;hwbreak+")
            g.cmd("?")
            return g
        except Exception as e:
            last = e
            time.sleep(2)
    raise SystemExit("cannot reach melonDS GDB stub: %s" % last)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--watch-payload-entry", action="store_true",
                    help="continue and watch for 0x02200800/0x02380000")
    args = ap.parse_args()

    g = connect()
    print("stop:", g.cmd("?"))
    g.bkpt(LOADER)
    for i in (1, 2):
        print("cont ->", g.cont())
        w = g.regs()
        print("  hit%d pc=0x%08x r0=0x%08x (region)" % (i, w[15], w[0]))
    g.unbkpt(LOADER)

    w = g.regs()
    try:
        head = g.read_mem(BUF, 16)
        print("buffer head now:", head.hex())
    except Exception as e:
        print("buffer read failed:", e)

    if args.watch_payload_entry:
        g.bkpt(PAYLOAD_ARM9)
        g.bkpt(PAYLOAD_ARM7)
        try:
            print("watch ->", g.cont())
            w = g.regs()
            print("payload entry pc=0x%08x" % w[15])
        except Exception as e:
            print("no payload-entry stop:", e)
    g.close()


if __name__ == "__main__":
    main()
