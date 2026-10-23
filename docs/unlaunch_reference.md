# Official Unlaunch install and start sequence

Reference notes gathered on 2026-10-23 from:

* **TWLMagician** (`https://github.com/R-YaTian/TWLMagician`, GPL-3.0) — its
  `unlaunch_proc()` and the bundled `Res/unlaunch.zip` / `UNLAUNCH.DSI`.
* The bundled `UNLAUNCH.DSI` (196088 bytes, MD5 `f3fdb593b04f85fe0705635c9f367f0b`),
  disassembled statically (ARM9 at file `0x4000`, ARM7 at file `0xF800`).

These notes exist so the official behaviour can be reproduced without
redistributing TWLMagician code. See `NOTICE` item 4.

## Install method (what TWLMagician does)

1. Locate the **real launcher** title:
   `title/00030017/<region>/content/title.tmd`.
2. If its size is `520` (stock), **open it in append mode and write the whole
   `UNLAUNCH.DSI` after the 520-byte TMD**. The file becomes `520 + 196088 =
   196608` bytes. The stock 520-byte TMD header is kept.
3. Re-encrypt the NAND image.

Our `scripts/dsi_nand_install.py` does the same with:

```sh
python3 scripts/dsi_nand_install.py replace <dec.img> <out.img> 484E4145 \
    --payload UNLAUNCH.DSI --keep-prefix
```

(`484E4145` is the real launcher `HNAE`; `484E4141` is the backup launcher
`HNAA` used by the SafeUnlaunch TMD-overflow route.)

## DSI header (both official and OpenLaunch containers)

| field | value |
| --- | --- |
| unit code (`0x12`) | `0x03` (DSi) |
| ARM9 rom offset / size | `0x4000` / `0xAD80` |
| ARM9 entry / load | `0x02200800` / `0x02200000` |
| ARM7 rom offset / size | `0xF800` / `0x205F8` |
| ARM7 entry / load | `0x02380000` / `0x02380000` |

## ARM9 entry (`0x02200800`)

```
CP15 cache setup
SCFG_EXT (0x04004008) = 0x00057078
r4 = 0x04000000
wait: r2 = [r4+0x180] & 0x0F ; until r2 == 14      ; IPCSYNC from ARM7
[r4+0x180] = 0x4E00                                 ; ack nibble 14 + IRQ
SCFG_EXT = 0x8307F100
VRAMCNT_A/B/C + 0x06030004 = 0xFF
SWI 0x00110000 (LZ77 decompress) 0x02204580 -> 0x02100000
copy 0x931C bytes  0x02100000 -> 0x06023C90 (VRAM)
copy 0x3C90 bytes  0x022008F0 -> 0x06020000 (VRAM)
jump 0x0602572C
```

The ARM9 **never loads or starts the ARM7**. It only waits for the ARM7 to
announce nibble 14.

## ARM7 entry (`0x02380000`)

```
SVC SP = 0x03FFFFC0
IRQ SP = 0x03FFFF80
SYS SP = 0x03FFFD80            ; 0x04000000 - 0x280
wait_for(14):                  ; helper at 0x02380030
    [0x04000180] = 0x4E00      ; output nibble 14 + IRQ
    SWI 0x00030000
    until ([0x04000180] & 0x0F) == 14
wait_for(13):                  ; output nibble 13, wait for 13
jump 0x06023C90                ; ARM7 main code in VRAM
```

## Implication for the overflow boot

The official install is a **real title boot**: the DSi bootloader loads the
ARM9 at `0x02200000` and the ARM7 at `0x02380000` before jumping to the ARM9
entry. Both CPUs are started by the loader, and the 14/13 handshake synchronises
their VRAM setup.

The SafeUnlaunch TMD-overflow route (backup launcher `HNAA`) only reaches the
ARM9 side: on the live test NAND the ARM7 is already past its own boot path
(`PC 0x00000EAC`, BIOS halt helper; `IPCSYNC` output nibble `7`), so the 14/13
handshake never runs and OpenLaunch's staged ARM7 at `0x02380000` is not
started by the loader.

Routes forward:

1. install OpenLaunch into the **real launcher** (`HNAE`) with the 520-byte
   TMD prefix, like the official installer, so the loader starts both CPUs;
2. or make the overflow path start the ARM7 the same way the loader does
   (publish the boot info at `0x02FFFE00`, then drive the boot handshake);
3. hardware validation, where the ARM7/loader timing may differ from melonDS.

## Live real-title boot (2026-10-23)

Test: pristine decrypted NAND, container written to the **real launcher**
(`HNAE`) with the stock 520-byte TMD prefix, re-encrypted, melonDS + GDB.

* The loader boots the container's ARM9: live PC `0x022022E8`, and the
  container's ARM7 image is present at `0x02380000`.
* The boot-info at `0x02FFFE00` is **not** populated by the loader (all zero);
  publishing the container's first `0x200` bytes there ourselves gives the
  boot manager its ARM7 entry (`+0x34 = 0x02380000`).
* The stage-2 ARM7 is still in its boot path: it spins in the ring consumer
  `arm7_get_ring_byte` (`0x037B9494`), i.e. it is waiting for messages.
* Pushing bytes `4,5,6` through the IPC command handler (command `2`,
  word = `2 | (byte << 6)`) advances it: it consumes `4` and writes
  **IPCSYNC `0x0600`**.
* The handshake is an echo: the ARM7 writes a nibble and waits for the ARM9
  to write the same value (`wait_for_sync` at `0x0238091C`, called with `6`
  from `0x02381038`). Echoing `6, 5, 8, 4` advances it through the stages.
* After the final echo the ARM7 runs a loader routine at the top of its
  **private WRAM** (`PC 0x03FFF7D0`, `LR 0x03FFF6D8`, `SP 0x03FFFF80`) and
  does not reach `0x02380000`.

So the real-title route succeeds in getting the ARM9 running and the ARM7
image loaded, but the stage-2 ARM7 boot manager still has to finish its
private-WRAM loader before it jumps to the title entry. The next reverse
targets are `0x0238103C` onward (post-handshake setup) and the private-WRAM
loader at `0x03FFF6D8`/`0x03FFF7D0`.

## The final jump: parameter block at `0x037DF578`

The stage-2 ARM7 boot manager ends by calling a generated stub at `0x03FFF600`
with `r5 = 0x03FFF600`. The stub:

1. copies `0xE0` bytes from **`0x037DF578`** to `0x03FFF4F0`;
2. writes IPCSYNC `0x400` and waits for 4 (echo);
3. copies the segment list and calls the memcpy/memset helpers;
4. writes IPCSYNC `0x500` and waits for 5;
5. waits for `VCOUNT == 0`, writes IPCSYNC `0x000` and waits for **0**;
6. does `ldm` on the copied block and jumps to the dword at **`+0x10`**.

So the ARM9 must place the ARM7 entry (`0x02380000`) at
`0x037DF578 + 0x10`. The handshake echo sequence observed live is
`6, 5, 8, 4, 5, 4, 5, 0`.

Important: the ARM7's `0x037DF578` is in **NWRAM-C part 3** (window C starts at
`0x037B8000`, slot = `(addr >> 15) & 7` = 3, offset `0x7578`). The ARM9's
`0x037DF578` falls in its own **window A** (NWRAM-A), so writing that address
from the ARM9 does not reach the ARM7's block. To write it the ARM9 must:

* map NWRAM-C at `0x03800000` (`MBK8 = 0x18303800`);
* take physical part 3 for the ARM9 (`MBK4` byte 3: `0x8D -> 0x8C`);
* write at slot 3 / offset `0x7578`, i.e. `0x0381F578`;
* restore `MBK4`/`MBK8`.

In the GDB test this write did not stick in melonDS (likely a JIT/NWRAM
remap quirk); doing it from the ARM9 C code is the next step.

### Corrections from the live handshake (2026-10-23, later)

* **Register map:** NWRAM-C parts 0-3 are **`MBK4` at `0x0400404C`**
  (`0x04004048` is `MBK3`, NWRAM-B). With the correct register the ARM9 can
  take part 3 and write `0x0381F578`; with `0x04004048` the write silently
  goes to the wrong bank.
* **The ARM7 zeroes its own windows.** After the ring handshake the ARM7's
  `MBK6/7/8` read `0` (and the shared ownership reads `0`). `0x037DF578`
  therefore falls through to **shared WRAM** (`WRAMSTAT = 3`) at offset
  `0x7578`, not NWRAM-C. The parameter block must be placed in that shared
  bank (the equivalent ARM9 view is `0x03007578` with `WRAMSTAT = 0` and the
  ARM9's own NWRAM windows cleared).
* The real-title test's shared-WRAM write did not stick yet (the ARM9's
  `WRAMSTAT`/window state needs to be pinned down); that is the last open
  step before the stub's `+0x10` jump.

### Why the stub still reads zero

`WRAMSTAT` stays `3` throughout. A marker written by the ARM9 into shared WRAM
at offset `0x7588` (the ARM7's `0x037DF588` fallback) is **cleared** before the
stub reads it. The boot manager writes its block while its window still maps
NWRAM-C, then zeroes `MBK6/7/8`; the stub then reads `0x037DF578` through the
*new* (empty) windows, i.e. shared WRAM, and sees zeros.

So the official flow needs the ARM7's window state to be consistent across the
block write and the stub read (or the block must be written after the window
switch, before the stub copy). Our title/loader path currently flips that
mapping in between.

The remaining work is therefore narrow:

1. determine the exact point where the boot manager writes the block (and into
   which physical memory);
2. either keep the windows mapping NWRAM-C until the stub has copied the block,
   or write the block after the window switch and before the stub copy.

### The official boot-configuration blob

The stage-2 ARM7 binary contains a configuration blob at file offset
`0x21AA8` (loaded at `0x023A1AA8`):

```
0x037DF578 x4           ; parameter block pointer
0x03800D30
...
0x02FFFE00              ; NDS boot info / header
0x02FFE000              ; secondary boot info
0x03FFF600              ; generated stub
0x8D898581 0x8D898581 0x9D999591 0x8D898581 0x9D999591   ; MBK1..MBK5
0x00003000 0x00003000 0x00003000                          ; MBK6..MBK8
```

So the official state is:

* identity NWRAM ownership (parts -> ARM7 slots 0..7);
* **all three windows empty** (`MBK6/7/8 = 0x00003000`), so
  `0x037DF578` falls through to shared WRAM at offset `0x7578`;
* the parameter block and the stub (`0x03FFF600`) are prepared from the
  boot-info/header (`0x02FFFE00`, `0x02FFE000`).

Our live boot manager instead ends with `MBK6/7/8 = 0` and a zero block, so
it is taking a different path from the official one (the loader/header setup
differs). The next step is to find which branch fills/copies the block from
`0x02FFFE00`/`0x02FFE000` and make our boot-info match what it expects.

### The real boot-info address and the last gate

The boot manager is called with **`r0 = 0x02FFE000`** (not `0x02FFFE00`):
the literals at `0x02380300` / `0x023802FC` are `0x02FFE000` and
`0x02FFE00C`. The ARM7 entry it uses is `[0x02FFE034]`.

The boot path at `0x02380240` has two branches:

* if `0x02381494` (the IPCSYNC input) returns **6**, it calls the boot
  manager with `0x02FFE000` and keeps the header;
* otherwise it falls through to `0x023802B4`, which does
  **`memset(0x02FFE000, 0, 0x1000)`**, then calls the boot manager with the
  now-zeroed header.

Our title-boot ARM9 starts *after* the ARM7 has already taken the fallback
branch, so the header is wiped and the parameter block gets entry `0`. The
official flow has the stage-2 ARM9/loader put IPCSYNC `6` and the boot info in
place *before* that check.

This is the last gate: either get the boot info + IPCSYNC 6 in place before
`0x02380240` runs (loader-side timing), or force the first branch.

The two-part condition at `0x0238025C` is:

```
bl 0x02381A78 ; must return non-zero (boot-mode/title check)
bl 0x02381494 ; must return 6       (IPCSYNC input from the ARM9)
```

Only then does it call the boot manager with the intact `0x02FFE000` header.
The fallback at `0x023802B4` does `memset(0x02FFE000, 0, 0x1000)` first.

On our title boot the ARM9 entry runs after this decision, so it cannot set
IPCSYNC/header in time; the fallback wipes the header, the parameter block
entry is 0, and the stub parks at `0x03FFF6E8`. The official flow must satisfy
`0x02381A78`/IPCSYNC from the loader/stage-2 side before the title's ARM9
starts.

The ARM9 counterpart of the boot manager is `0x022087B0` (called by the
stage-2 ARM9 with `r0 = 0x02FFE000`); it reads the ARM9 entry at `[+0x24]`,
matching the standard NDS header layout (ARM7 entry at `[+0x34]`).

### Status of the two routes (2026-10-23, end of session)

Both routes reach our ARM9 and drive the ARM7 ring/echo handshake:

* **real-launcher (`HNAE`)**: loader boots our ARM9, ARM7 stays in the stage-2
  boot manager;
* **TMD overflow (`HNAA`)**: our ARM9 runs from the overflow, ARM7 in the same
  boot manager.

In both, publishing the header at `0x02FFE000`/`0x02FFFE00` and setting
IPCSYNC `6` before pushing `4/5/6` still leaves the parameter block entry at
`0` and the stub parked at `0x03FFF6E8`. The ARM7's early gate
(`0x02381A78` + IPCSYNC 6) is evaluated before our ARM9 can influence it, so
the header is zeroed.

The official NAND does not boot under melonDS (the GDB stub never reaches its
first `Enter`, even with break-on-startup), so the official loader state cannot
be traced there. The remaining options are a different emulator/debug setup, a
hardware trace, or a different takeover point.

A minimal header (ARM9/ARM7 entry set, sizes and section tables zeroed so
`0x02381A78` skips its section validation) was also tried on both routes; the
stub still parks. So the gate has more state than the header fields alone
(most likely the loader-side boot mode and the IPCSYNC handshake timing).
