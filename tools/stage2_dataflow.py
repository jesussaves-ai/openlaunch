#!/usr/bin/env python3
"""Stage-2 ARM9 static data-flow pass (TMD-overflow control-transfer finder).

Finds indirect control transfers - ``blx``/``bx``/``mov pc``/``ldr pc``/``ldm ..pc``
/``pop ..pc`` - whose target value is anchored inside a memory range (by default
the SafeUnlaunch TMD-overflow range ``[0x037DF06C, 0x0380F06C)``).  Those are the
"task switching structure" slots the oversized launcher ``title.tmd`` clobbers.

This is what located the exploit: stage-2 dispatches ARM7 IPC messages through a
32-entry command-handler table at RAM ``0x037F3B68`` (``ldr r3,[r4,r0,lsl#2]`` /
``blx r3`` at ``0x037CA720``).  The oversized ``title.tmd`` overwrites that
table, and Unlaunch's ``unlaunch.dsi`` fills all 32 entries with a pointer to its
own in-place bootstrap - so the next IPC message jumps into the payload.

Method
------
1. Recursive-descent disassembly (Capstone) from the image entry, seeded with
   every direct branch target harvested from a linear decode (the DSi stage-2
   ARM9 image is code + rodata, so this reaches the whole code graph without
   disassembling literal pools as code).
2. A forward abstract interpretation tracking, per register, either a constant,
   an address expression ``base + terms``, or a value loaded from memory
   (``load[addr]``).  Basic blocks are joined at merge points to a fixpoint.
3. At each computed control transfer, classify the target: a constant, a value
   loaded from a memory field, or an address expression.  Report the ones whose
   field address / constant lies inside ``--range``.

Requires ``capstone`` (``pip install capstone``).

Usage
-----
    # decrypt stage 2 first:
    python3 scripts/dsi_stage2_decrypt.py nand.bin --outdir build/stage2

    python3 tools/stage2_dataflow.py build/stage2/arm9.bin \
            --base 0x037B8000 --range 0x037DF06C 0x0380F06C

    # also show what a payload writes into the found slots
    # (--payload <unlaunch.dsi> --payload-image-base <RAM addr where it loads>):
    python3 tools/stage2_dataflow.py build/stage2/arm9.bin \
            --range 0x037DF06C 0x0380F06C \
            --payload asm/UNLAUNCH.DSI --payload-image-base 0x037DF274
"""
from __future__ import annotations

import argparse
import struct
import sys
from collections import deque

try:
    from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB, CS_MODE_LITTLE_ENDIAN, \
        CS_GRP_JUMP, CS_GRP_CALL, CS_GRP_RET, CS_GRP_INT, CS_GRP_IRET
    from capstone.arm import (
        ARM_OP_REG, ARM_OP_IMM, ARM_OP_MEM,
        ARM_REG_PC, ARM_REG_SP, ARM_CC_AL, ARM_CC_INVALID,
        ARM_INS_BLX,
    )
except ImportError:  # pragma: no cover
    sys.exit("this tool needs the 'capstone' module (pip install capstone)")

TOP = ("T",)


# --------------------------------------------------------------------------- #
# Value domain
# --------------------------------------------------------------------------- #
def to_addr(v):
    """(base, terms) if v is address/constant valued, else None."""
    if v is None:
        return None
    if v[0] == "C":
        return (v[1], frozenset())
    if v[0] == "A":
        return (v[1], v[2])
    return None


def combine(x, y, shift=0):
    ax, ay = to_addr(x), to_addr(y)
    if ax is None and ay is None:
        return TOP
    # An unknown operand still contributes a symbolic (unknown) term, so a known
    # *base* is not lost (e.g. `ldr r3,[base, rIdx, lsl #2]` with rIdx unknown).
    if ax is None:
        ax = (0, frozenset({"unk"}))
    if ay is None:
        ay = (0, frozenset({"unk"}))
    base = ax[0] + (ay[0] << shift)
    terms = set(ax[1])
    for t in ay[1]:
        terms.add((t, shift))
    return ("A", base & 0xFFFFFFFF, frozenset(terms))


def join(a, b):
    if a == b:
        return a
    if a[0] == "T":
        return a
    if b[0] == "T":
        return b
    if a[0] == "M" and b[0] == "M":
        e = set(a[1]) | set(b[1])
        return ("M", frozenset(e)) if len(e) <= 8 else TOP
    return TOP


def merge_into(dst, src):
    changed = False
    for r, v in src.items():
        nv = join(dst[r], v) if r in dst else v
        if dst.get(r) != nv:
            dst[r] = nv
            changed = True
    return changed


class Analyzer:
    def __init__(self, img, base, rng):
        self.img = img
        self.base = base
        self.end = base + len(img)
        self.lo, self.hi = rng
        self.md_arm = Cs(CS_ARCH_ARM, CS_MODE_ARM | CS_MODE_LITTLE_ENDIAN)
        self.md_thumb = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
        for md in (self.md_arm, self.md_thumb):
            md.detail = True
        self.insns = {}
        self.succ = {}
        self._build_cfg()

    # -- disassembly ------------------------------------------------------- #
    def _disone(self, addr, thumb):
        o = addr - self.base
        if not (0 <= o < len(self.img)):
            return None
        md = self.md_thumb if thumb else self.md_arm
        for ins in md.disasm(self.img[o:o + 16], addr):
            return ins
        return None

    def _build_cfg(self):
        roots = [(self.base, False)]
        # harvest direct branch targets from a linear decode
        seen = set(roots)
        md = self.md_arm
        for a in range(self.base, self.end - 3, 4):
            for ins in md.disasm(self.img[a - self.base:a - self.base + 4], a):
                if (CS_GRP_JUMP in ins.groups or CS_GRP_CALL in ins.groups) and \
                        ins.operands and ins.operands[0].type == ARM_OP_IMM:
                    t = ins.operands[0].imm
                    if self.base <= t < self.end and (t, False) not in seen:
                        roots.append((t, False))
                        seen.add((t, False))
                break
        work = list(roots)
        while work:
            addr, thumb = work.pop()
            if not (self.base <= addr < self.end) or addr in self.insns:
                continue
            ins = self._disone(addr, thumb)
            if ins is None:
                continue
            self.insns[addr] = (ins, thumb)
            nxt = addr + ins.size
            grp = set(ins.groups)
            is_jump = CS_GRP_JUMP in grp
            is_call = CS_GRP_CALL in grp
            is_ret = CS_GRP_RET in grp
            is_int = CS_GRP_INT in grp or CS_GRP_IRET in grp
            _rd, wr = ins.regs_access()
            writes_pc = ARM_REG_PC in wr
            cond = ins.cc not in (ARM_CC_AL, ARM_CC_INVALID)
            op0 = ins.operands[0] if ins.operands else None
            direct = op0.imm if (op0 is not None and op0.type == ARM_OP_IMM
                                 and (is_jump or is_call)) else None

            def add(t, th=thumb):
                self.succ.setdefault(addr, [])
                if (t, th) not in self.succ[addr]:
                    self.succ[addr].append((t, th))
                work.append((t, th))

            if is_call:
                if direct is not None:
                    tgt, nt = direct, thumb
                    if ins.id == ARM_INS_BLX:
                        nt = not thumb
                        tgt &= ~1
                    add(tgt, nt)
                add(nxt)
            elif is_jump or is_ret or writes_pc:
                if direct is not None:
                    add(direct)
                if cond and not is_ret:
                    add(nxt)
            elif not is_int:
                add(nxt)

    # -- transfer function ------------------------------------------------- #
    def _set(self, out, st, r, v, cond):
        if r == ARM_REG_PC:
            return
        out[r] = join(st.get(r, TOP), v) if cond else v

    def _eval_op(self, st, op, a, thumb):
        if op.type == ARM_OP_IMM:
            return ("C", op.imm & 0xFFFFFFFF)
        if op.type == ARM_OP_REG:
            if op.reg == ARM_REG_PC:
                return ("C", a + (4 if thumb else 8))
            return st.get(op.reg, TOP)
        return TOP

    def _mem_expr(self, st, ins, thumb, op):
        mm = op.mem
        if mm.base == ARM_REG_PC:
            bv = ("C", ins.address + (4 if thumb else 8))
        elif mm.base == 0:
            bv = ("C", 0)
        else:
            bv = st.get(mm.base, TOP)
        ev = bv
        if mm.index != 0:
            ev = combine(ev, st.get(mm.index, TOP), mm.scale)
        if mm.disp:
            ev = combine(ev, ("C", mm.disp & 0xFFFFFFFF))
        return ev

    def transfer(self, ins, thumb, st):
        out = dict(st)
        a = ins.address
        mn = ins.mnemonic
        cond = ins.cc not in (ARM_CC_AL, ARM_CC_INVALID)
        ops = ins.operands

        if mn in ("mov", "movs") and len(ops) >= 2:
            self._set(out, st, ops[0].reg, self._eval_op(st, ops[1], a, thumb), cond)
            return out
        if mn == "mvn" and len(ops) >= 2:
            v = self._eval_op(st, ops[1], a, thumb)
            self._set(out, st, ops[0].reg,
                      ("C", (~v[1]) & 0xFFFFFFFF) if v[0] == "C" else TOP, cond)
            return out
        if mn in ("add", "adds", "sub", "subs", "rsb") and len(ops) >= 3:
            x = self._eval_op(st, ops[1], a, thumb)
            y = self._eval_op(st, ops[2], a, thumb)
            if mn.startswith("sub") or mn.startswith("rsb"):
                if y[0] != "C":
                    self._set(out, st, ops[0].reg, TOP, cond)
                    return out
                y = ("C", (-y[1]) & 0xFFFFFFFF)
            self._set(out, st, ops[0].reg, combine(x, y), cond)
            return out
        if mn in ("ldr", "ldrb", "ldrh", "ldrsb", "ldrsh") and len(ops) >= 2 \
                and ops[0].type == ARM_OP_REG and ops[1].type == ARM_OP_MEM:
            expr = self._mem_expr(st, ins, thumb, ops[1])
            dest = ops[0].reg
            if dest == ARM_REG_PC:
                return out
            ea = to_addr(expr)
            if mn == "ldr" and ea is not None and not ea[1]:
                o = ea[0] - self.base
                if 0 <= o <= len(self.img) - 4:
                    self._set(out, st, dest,
                              ("C", struct.unpack_from("<I", self.img, o)[0]), cond)
                    return out
            self._set(out, st, dest,
                      ("M", frozenset({ea})) if ea is not None else TOP, cond)
            return out
        if mn in ("ldm", "ldmia", "ldmib", "ldmda", "ldmdb", "ldmfd", "pop"):
            base = ARM_REG_SP if mn == "pop" else (ops[0].reg if ops else 0)
            bv = st.get(base, TOP)
            idx = 0
            for op in ops[1:]:
                if op.type == ARM_OP_REG:
                    if op.reg != ARM_REG_PC:
                        ea = to_addr(combine(bv, ("C", idx * 4)))
                        self._set(out, st, op.reg,
                                  ("M", frozenset({ea})) if ea is not None else TOP, cond)
                    idx += 1
            return out
        return out

    # -- indirect transfer classification ---------------------------------- #
    def classify(self, ins, thumb, st):
        a = ins.address
        mn = ins.mnemonic
        _rd, wr = ins.regs_access()
        if ARM_REG_PC not in wr:
            return None
        ops = ins.operands
        if ops and ops[0].type == ARM_OP_IMM and mn in ("b", "bl", "blx"):
            return None
        if mn in ("bx", "blx") and ops and ops[0].type == ARM_OP_REG and ops[0].reg != ARM_REG_PC:
            return ("reg", st.get(ops[0].reg, TOP), None)
        if mn == "mov" and ops and ops[0].reg == ARM_REG_PC and len(ops) >= 2 \
                and ops[1].type == ARM_OP_REG:
            return ("reg", st.get(ops[1].reg, TOP), None)
        if mn in ("add", "sub") and ops and ops[0].reg == ARM_REG_PC and len(ops) >= 3:
            return ("reg", combine(self._eval_op(st, ops[1], a, thumb),
                                   self._eval_op(st, ops[2], a, thumb)), None)
        if mn == "ldr" and ops and ops[0].reg == ARM_REG_PC and ops[1].type == ARM_OP_MEM:
            return ("mem", None, self._mem_expr(st, ins, thumb, ops[1]))
        if mn in ("ldm", "ldmia", "ldmib", "ldmda", "ldmdb", "ldmfd", "pop"):
            base = ARM_REG_SP if mn == "pop" else (ops[0].reg if ops else 0)
            bv = st.get(base, TOP)
            idx = 0
            for op in ops[1:]:
                if op.type == ARM_OP_REG:
                    if op.reg == ARM_REG_PC:
                        return ("mem", None, combine(bv, ("C", idx * 4)))
                    idx += 1
        return None

    def in_range(self, expr):
        ea = to_addr(expr)
        if ea is None:
            return None
        base, terms = ea
        if not terms:
            return "exact" if self.lo <= base < self.hi else None
        if self.lo <= base < self.hi:
            return "base"
        return None

    # -- run --------------------------------------------------------------- #
    def run(self):
        im = {a: v for a, v in self.insns.items() if a < self.end}
        states = {a: {} for a in im}
        work = deque(im)
        inq = set(work)
        while work:
            a = work.popleft()
            inq.discard(a)
            ins, thumb = im[a]
            out = self.transfer(ins, thumb, states.get(a, {}))
            for (b, _bt) in self.succ.get(a, []):
                if b in im and merge_into(states.setdefault(b, {}), out) and b not in inq:
                    work.append(b)
                    inq.add(b)

        findings = []
        for a in sorted(im):
            ins, thumb = im[a]
            ci = self.classify(ins, thumb, states.get(a, {}))
            if not ci:
                continue
            kind, val, fexpr = ci
            hits = []
            if kind == "reg":
                if val[0] == "C" and self.lo <= val[1] < self.hi:
                    hits.append(("const-target", val[1]))
                elif val[0] == "M":
                    for (b, t) in val[1]:
                        r = self.in_range(("A", b, t))
                        if r:
                            hits.append((r, b))
                elif val[0] == "A":
                    r = self.in_range(val)
                    if r:
                        hits.append((r, val[1]))
            else:
                r = self.in_range(fexpr)
                if r:
                    hits.append((r, to_addr(fexpr)[0]))
            if hits:
                findings.append((a, ins.mnemonic, ins.op_str, kind, val, fexpr, hits))
        return findings


def describe(v):
    if v is None:
        return "?"
    if v[0] == "T":
        return "TOP"
    if v[0] == "C":
        return "const 0x%08X" % v[1]
    if v[0] == "A":
        ts = ",".join(sorted(str(t) for t in v[2]))
        return "addr 0x%08X%s" % (v[1], ("+" + ts) if ts else "")
    if v[0] == "M":
        return "load[%s]" % " | ".join(
            "0x%08X%s" % (b, ("+" + ",".join(sorted(str(x) for x in t))) if t else "")
            for (b, t) in v[1])
    return "?"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("arm9", help="decrypted stage-2 ARM9 binary")
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x037B8000,
                    help="load address (default 0x037B8000)")
    ap.add_argument("--range", nargs=2, type=lambda s: int(s, 0),
                    default=[0x037DF06C, 0x0380F06C],
                    metavar=("LO", "HI"), help="overflow range to look for")
    ap.add_argument("--payload", help="payload file (e.g. asm/UNLAUNCH.DSI)")
    ap.add_argument("--payload-image-base", type=lambda s: int(s, 0), default=None,
                    help="RAM address the payload image loads at (to map field -> file offset)")
    args = ap.parse_args(argv)

    img = open(args.arm9, "rb").read()
    az = Analyzer(img, args.base, tuple(args.range))
    findings = az.run()

    print("stage-2 ARM9: %d instructions, base 0x%08X" % (len(az.insns), args.base))
    print("looking for computed transfers anchored in [0x%08X,0x%08X)\n"
          % (args.range[0], args.range[1]))
    print("%-11s %-9s %-18s %-10s %s" % ("call site", "kind", "value", "hit", "field"))
    for a, mn, ops, kind, val, fexpr, hits in findings:
        fld = describe(fexpr) if fexpr is not None else describe(val)
        print("0x%08X  %-9s %-18s %-10s %s"
              % (a, mn, ops, hits[0][0], fld))

    if args.payload and args.payload_image_base is not None:
        pay = open(args.payload, "rb").read()
        print("\npayload '%s' at 0x%08X: values written into each field"
              % (args.payload, args.payload_image_base))
        for a, mn, ops, kind, val, fexpr, hits in findings:
            expr = source_expr(kind, val, fexpr)
            addr = to_addr(expr)[0] if to_addr(expr) is not None else None
            if addr is None:
                continue
            fo = addr - args.payload_image_base
            if 0 <= fo <= len(pay) - 4:
                v = struct.unpack_from("<I", pay, fo)[0]
                print("  field 0x%08X (payload+0x%06X) = 0x%08X" % (addr, fo, v))
    return 0


def source_expr(kind, val, fexpr):
    if kind == "mem":
        return fexpr
    if val[0] == "M":
        b, t = next(iter(val[1]))
        return ("A", b, t)
    if val[0] == "A":
        return val
    return None


if __name__ == "__main__":
    raise SystemExit(main())
