# OpenLaunch NAND install payload (Unlaunch-compatible container)

This documents how OpenLaunch installs into the DSi's internal NAND the same way
Unlaunch / the "Safe unlaunch installer" (edo9300) do, based on a real retail
dump and `asm/UNLAUNCH.DSI`.

## The exploit in one paragraph

boot stage 2 loads the launcher's `title.tmd` with a broken `filesize > filesize`
check. When that file is oversized it overflows a fixed Main-RAM buffer and
clobbers a task-switch structure, and the ARM9 ends up executing code taken from
the loaded TMD bytes. The payload therefore lives *inside* the TMD, immediately
after the 520-byte TMD header.

## Where it is installed (SafeUnlaunch)

stage 2 has a failsafe: if the **real** launcher (`HNAE`) TMD is unreadable it
loads the **backup** launcher (`HNAA`). SafeUnlaunch:

* writes its payload into `title/00030017/HNAA/content/title.tmd`, and
* breaks one byte in `title/00030017/HNAE/content/title.tmd` so stage 2 falls
  back to `HNAA`.

On a console already carrying this layout (all our test consoles), replacing the
payload is enough: only `HNAA`'s TMD is rewritten; the real launcher and its
recovery path are untouched.

## On-disk layout

```
title.tmd  =  520 bytes  zeroed TMD header        (SafeUnlaunch writes zeros)
           ++ payload (below)                     -> file size 196608
```

The payload is a self-describing container (decoded from the dump and
`asm/UNLAUNCH.DSI`):

```
0x0000  16 KiB header
  0x00 16   name              e.g. "UNLAUNCH.DSI" / "OPENLAUNCH.DSI"
  0x10 u32  0x00030000
  0x1C u32  0x00000001
  0x20 u32  ARM9 ROM offset   (0x4000)
  0x24 u32  ARM9 entry point  (0x02200800)
  0x28 u32  ARM9 load base    (0x02200000)
  0x2C u32  ARM9 size
  0x30 u32  ARM7 ROM offset   (0xF800)
  0x34 u32  ARM7 entry point  (0x02380000)
  0x38 u32  ARM7 load base    (0x02380000)
  0x3C u32  ARM7 size
0x4000      ARM9 binary
0xF800      ARM7 binary
...         padded to the container size (Unlaunch: 196088; TMD 196608)
```

Unlaunch's numbers: ARM9 @ `0x4000`, size `0xAD80`; ARM7 @ `0xF800`, size
`0x205F8`; `0x4000+0xAD80 = 0xED80`, `0xF800+0x205F8 = 0x2FDF8 = 196088`.

## How OpenLaunch maps onto it

OpenLaunch's linker scripts already place the code at exactly these addresses:

| | load base | entry |
|---|---|---|
| `ld/arm9.ld` | `0x02200000` | `0x02200800` |
| `ld/arm7.ld` | `0x02380000` | `0x02380000` |

The container also carries the **stage-2 overflow bootstrap** and the pointers
that hand control to it (see [`stage2_re.md`](stage2_re.md)). The exploited
transfer is the ARM9 **IPC command-handler table** at RAM `0x037F3B68`
(dispatched at `0x037CA720`: `ldr r3,[r4,r0,lsl#2]; blx r3`):

```
payload offset 0x11C4C   stage-2 overflow stub (RAM 0x037F0EC0)   <- src/arm9/overflow_bootstrap.s
payload offset 0x148F4   32-entry IPC handler table RAM 0x037F3B68 -> 0x037F0EC0   (the real entry)
payload offset 0x145FC   callback slot RAM 0x037F3870 -> 0x037F0EC0   (Unlaunch parity)
payload offset 0x14624   callback slot RAM 0x037F3898 -> 0x037F0EC0   (Unlaunch parity)
payload offset 0x4000    ARM9 image
payload offset 0xF800    ARM7 image
payload offset 0x13000   IPC/task context words (RAM 0x037F22C8..) - see stage2_re.md
```

The context words were found by an additive/removal differential against the
reference payload; with the IPC table armed and those four words set, the patched
`HNAA` TMD is observed in melonDS to reach the stub and OpenLaunch's ARM9 entry.

The stub runs *in place* at `0x037F0EC0` (since the container image is overlaid
at `0x037DF274`). It enters SVC mode, reads the header fields above, copies the
ARM9/ARM7 flat images to their linked bases, writes the ARM7 takeover marker at
`0x0237FFF0`, cleans/invalidates the caches and branches to the ARM9 entry. The
ARM9 entry then arms the jumpslide (see [`stage2_re.md`](stage2_re.md)). Building the payload is:

```
make openlaunch-payload          # -> build/openlaunch_payload.bin
```

(`scripts/make_openlaunch_payload.py` builds the container; the Makefile first
assembles `src/arm9/overflow_bootstrap.s` into `build/overflow_bootstrap.bin`.
Addresses/offsets are parameters; `--bootstrap` opts into the overflow wiring.)

The result is checked by `scripts/verify_openlaunch_payload.py` (run as part of
`make verify`): header fields, no ARM9/ARM7 overlap, both callback slots and all
32 handler-table entries pointing at the stub, and the stub carrying the expected
`0x037DF274` image base.

## Installing it (offline, no console writes)

```sh
make nand-tool                   # offline crypto tool
make openlaunch-payload          # build/openlaunch_payload.bin

# decrypt -> rewrite the HNAA backup TMD -> re-encrypt (one command)
python3 scripts/dsi_nand_install.py patch nand.bin nand_patched.bin HNAA \
        --payload build/openlaunch_payload.bin
```

`patch` verifies the result: the TMD is 196608 bytes, attribute `0x21`
(read-only+archive), both FAT copies agree, and only the intended sectors
changed. `scripts/dsi_nand_install.py verify` re-checks a decrypted image.

## What is proven vs. what remains

**Proven (offline, against a real dump):**
* per-console crypto (key derivation + AES-CTR) over the whole image;
* FAT16 rewrite of the TMD (chain, size, directory entry, both FAT copies),
  validated by identity (0 bytes changed) and by a different-size payload;
* the container is built and written, and reads back byte-exact;
* a full `decrypt -> edit -> encrypt` round-trips and is sector-scoped;
* the container carries the overflow bootstrap + slot/table pointers, verified
  structurally (`make verify`).

**Remaining (validation, not construction):**
* the exploit entry is **reproduced and observed end-to-end in melonDS**: the
  patched `HNAA` TMD runs the overflow (loader x2, read x4), dispatches through
  the armed IPC handler table to our stub (`0x037F0EC0`), and the stub relocates
  the images and branches to OpenLaunch's ARM9 entry (`0x02200800`). The
  differential that found the last requirement (four IPC/task context words) is
  in [`stage2_re.md`](stage2_re.md).
* the **ARM7 hand-off** is implemented with a jumpslide takeover
  (`src/arm9/arm7_boot.c`): the overflow bootstrap stages our ARM7 at
  `0x02380000`, the ARM9 briefly maps NWRAM-C and overwrites the firmware ARM7's
  return/stack window, and the ARM7 starts our image on its next return. The
  ARM7 writes a ready marker at `0x0237FFF4`, which is the emulator/GDB check.
  The mechanism is adapted from Astronaut (GPL-3.0-or-later) and is documented
  in [`stage2_re.md`](stage2_re.md). A melonDS run shows the slide arms but the
  firmware ARM7 stack is at `0x03FFFFC0`, not in Astronaut's window, so the
  ARM7 does not start yet; the old IPCSYNC route remains as
  `make ARM7_HANDOFF=1` for the title-boot case.
* `nand_patched.bin` is still **not hardware-validated**: flash only on a console
  with a full NAND backup (docs §34).

## Safety summary

* Only the `HNAA` backup TMD is modified; the real launcher (`HNAE`) and its
  byte-break are left as SafeUnlaunch wrote them.
* Every build step is offline; the tool never touches a console.
* The mandatory full-NAND backup / recovery guidance in docs §34 still applies
  before any flash.

## Install modes: overflow-hijack vs. legitimate title

`scripts/dsi_nand_install.py` writes the container into the launcher region's
`title.tmd`. There are two modes, selected by `--keep-prefix`:

* **overflow-hijack (default, no `--keep-prefix`)** - the TMD is the container
  prefixed with 520 zero bytes. Stage 2 takes the TMD-overflow path documented
  in [`stage2_re.md`](stage2_re.md), so the ARM9 runs and the ARM7 is taken
  over by the jumpslide.
* **legitimate title (`--keep-prefix`)** - the 520-byte *genuine* TMD header is
  kept and the container appended after it (`520 + payload`). The file is still
  oversized, so the broken size check still overflows; Astronaut uses exactly
  this shape and still needs the ARM7 takeover. Keeping the genuine header is
  therefore not a substitute for the takeover. Whether stage 2 additionally
  accepts the region as an ordinary title (which would start both CPUs through
  the normal boot-info flow) is unverified.

Building a legitimate-title image from a stock dump:

```
python3 scripts/dsi_nand_install.py patch \
    nand.bin out.bin 484E4145 \
    --payload build/openlaunch_payload.bin --keep-prefix
```

Verified offline against the reference dump: the region's `title.tmd` grows
`520 -> 196608` bytes with the header unchanged
(`head=000100013abd67fb8237b03d52af21e9`) and a consistent FAT chain
(`fat-copy mismatches on chain: 0`). Whether stage 2 also completes an ordinary
title boot is a **hardware** question; the ARM7 jumpslide does not depend on it
(see [`stage2_re.md`](stage2_re.md)).

## NAND footer conventions

A DSi eMMC image carries a 0x40-byte `"DSi eMMC CID/CPU"` footer in one of two
places:

* **in-place** at `0x000FF800` (the convention our own images use), and
* **appended** after the 240 MiB image at `0x0F000000`, as `dumpTool` writes it
  (equivalently the last 0x40 bytes of the file). Its `.sha1` covers the 240 MiB
  image *before* the footer is appended.

`dsi_nand_tool` and `dsi_nand_install.py` now accept **either**, so a stock
dumpTool image decrypts with no extra arguments.
