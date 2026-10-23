# Boot stage-2 reverse engineering (the TMD overflow)

The DSi boot chain is: ARM7/ARM9 boot ROM -> **stage 1** -> **stage 2** (loads the
launcher) -> System Menu. The Unlaunch/SafeUnlaunch exploit lives in stage 2's
launcher load path, so reproducing it for OpenLaunch requires stage-2 code.

## Stage-2 decryption (done, verified)

Stage 2 is not console-unique; its ARM9/ARM7 binaries are AES-128-CTR encrypted
with a universal ("Tad") key:

```
keyX = "Nintendo DS\x00\x01#!\x00"
keyY = signed_block[0:16]      # from the RSA-decrypted signature
key  = keygen(keyX, keyY)      # ((keyX^keyY)+C2) ROL 42, LE->BE
CTR  = u128 [size, -size, ~size, 0] (+1/block), little-endian, DSi AES byteswap
```

* The boot-info block at `0x200` holds the ARM9/ARM7 offsets and a 0x100-byte RSA
  signature at `0x300`; RSA-1024 (modulus in `scripts/dsi_stage2_decrypt.py`,
  e=0x10001) recovers the `0x74`-byte signed block containing `keyY` and the
  SHA1s of the plaintext binaries.
* Validated against a retail dump: the decrypted ARM9 SHA1 (starts `e3a0c301` =
  `mov r12,#0x10000`) **matches** the value in the signed block. The ARM7 SHA1
  does not match because the ARM7 section is compressed (decompression TODO).

Reproduce with:

```sh
python3 scripts/dsi_stage2_decrypt.py nand.bin --outdir build/stage2
python3 -c "print(open('build/stage2/arm9.bin','rb').read()[:4].hex())"  # e3a0c301
arm-none-eabi-objdump -D -b binary -m arm --adjust-vma=0x37B8000 \
        build/stage2/arm9.bin > build/stage2/arm9.s
```

## What the ARM9 contains

The decrypted ARM9 (loads at `0x037B8000`) embeds the launcher path templates:

| RAM address | string |
| --- | --- |
| `0x037DDA00` | `%s/%08x/%08x/content/title.tmd` |
| `0x037DDA88` | `%s/%s/content/title.tmd` |
| `0x037D0B1B` | `/title/00000000/00000000/content/title.tmd` |
| `0x037DDB2C` | `nand:/import/` |

Key ARM9 routines so far:

* `0x037B9D14` — formats a `title.tmd` path (uses `0x037DDA00`) and calls the
  open helper; this is the **TMD path builder**.
* `0x037B9C30` — reads a file into a caller-supplied buffer. It *does* bounds-check
  (`ldr r2,[sp,#28]` file size vs `ldr r0,[r5]` max; `cmp r2,r0; bhi -> mvn r0,#0x3f8`),
  so it is **not** the vulnerable copy. The launcher load path is separate.

## The vulnerable TMD load (found)

The launcher TMD is read by **`0x037BAA18`**, and it has exactly the bug
nocash described:

* It calls the read helper `0x037B9C30` **twice** with the *same* size variable
  (`[sp+12]`):
  1. `dest = 0` -> the helper treats this as a **size query** and stores the
     file size into the size variable;
  2. `dest = <buffer>` -> the helper checks `file_size <= *size_var`, but
     `*size_var` now *is* `file_size`, so the check is `file_size <= file_size`
     (the "filesize > filesize" bug) and always passes.
* The destination buffer is a **fixed** Main-RAM address, **`0x037DF06C`**
  (literal at `0x037BAB94`), which sits immediately past the decrypted stage-2
  image (`0x037B8000 + 0x26410 = 0x037DE410`).

So an oversized `title.tmd` is copied in full into `0x037DF06C` onward, overrunning
whatever follows. For SafeUnlaunch's 196,608-byte TMD the payload bytes (TMD
offset 520) land at `0x037DF274`, and the append clobbers stage-2's task/title
structures in Main RAM - which is what hands control to the payload.

Caller: `0x037C0C40` walks a title array `(title_lo, title_hi)` and calls
`0x037BAA18` for each, passing `0xFFFF` and `0x02FFDFC0` (a Main-RAM structure).

### Boot flow

The launcher TMD is loaded very early: `0x037B8CAC` (the "load one title" path)
calls the walk `0x037C0BE0` at `0x037B8D14` with a one-entry title array, which
reaches `0x037BAA18`. So the exploit fires during the initial title/launcher
scan.

### Memory layout (physical)

| region | address |
| --- | --- |
| stage-2 ARM9 image | `0x027B8000` (`0x037B8000` mirror), ends `0x027DE410` |
| TMD buffer | `0x037DF06C` (mirror `0x027DF06C`), just past the image |
| stage-2 stacks (SVC/IRQ/USR) | around `0x02FE0000` |

The overflow runs from the TMD buffer *upwards*, clobbering stage-2's own
data/BSS (e.g. `0x037E3A50`, `0x037E7A50`, `0x037EE900`, …), **not** the stack -
so the hijack is via an overwritten data/control structure ("task switching
structure"), not a saved return address.

### Dynamic confirmation (melonDS GDB stub)

melonDS runs the real stage-2 from the NAND image and exposes a GDB stub, so the
overflow can be observed live (`tools/melonds_trace.gdb`). Breaking at the
loader `0x037BAA18` and at the read helper `0x037B9C30` shows:

```
loader: region r0=0x484e4145 title r1=0x00030017
read: dest=0x00000000 sizeptr=0x02fe36f0 path="/title/00030017/484e4145/content/title.tmd"
read: dest=0x037df06c sizeptr=0x02fe36f0 path="/title/00030017/484e4145/content/title.tmd"
read: dest=0x00000000 sizeptr=0x02fe36f0 path="/title/00030017/484e4141/content/title.tmd"
read: dest=0x037df06c sizeptr=0x02fe36f0 path="/title/00030017/484e4141/content/title.tmd"
```

This is the exploit firing exactly as modelled: stage 2 loads the real launcher
(`HNAE`, `0x484e4145`) and then the backup launcher (`HNAA`, `0x484e4141`),
**both into the fixed buffer `0x037DF06C` with the same size variable** - the
second (`HNAA`) is SafeUnlaunch's 196,608-byte TMD, so it overruns the buffer.

## Control-transfer structure (found: static + dynamic)

`tools/stage2_dataflow.py` performs a recursive-descent disassembly of the
decrypted ARM9 followed by a forward abstract interpretation that tracks, per
register, constants / address expressions / values loaded from memory, and
reports every computed control transfer (`blx`/`bx`/`mov pc`/`ldr pc`/`ldm ..pc`)
whose target is anchored in the overflow range:

```sh
python3 tools/stage2_dataflow.py build/stage2/arm9.bin \
        --range 0x037DF06C 0x0380F06C \
        --payload asm/UNLAUNCH.DSI --payload-image-base 0x037DF274
```

The value it finds that actually matters is the **ARM9 IPC command-handler
table** at RAM `0x037F3B68` (32 entries):

```
0x037CA678  IPC receive/dispatch loop (polls IPCSYNC 0x04000184 / 0x04100000)
0x037CA6FC  ... r0 = message word
0x037CA700  lsrs r0, r0, #27        ; r0 = (message >> 0) & 0x1F  (message type)
0x037CA708  ldr  r3, [r4, r0, lsl #2]   ; r3 = table[type]   (r4 = 0x037F3B68)
0x037CA70C  cmp  r3, #0
0x037CA720  blx  r3                 ; <-- dispatch the handler
```

`0x037F3B68` is the handler registry written by `0x037CA580(index, fn)` (e.g.
`0x037CA790` registers handler `#23 = 0x037CA7B8`). An oversized launcher TMD
overwrites the whole table, and `unlaunch.dsi` fills **all 32 entries** with
`0x037F0EC0` (TMD `0x14AFC .. 0x14B7C`, payload `0x148F4 .. 0x14974`). The next
ARM7 -> ARM9 IPC message therefore dispatches into the payload.

> Tooling note: the dispatch reads `table[r0, lsl #2]` with a *symbolic* index,
> so the abstract domain must keep the known base (`0x037F3B68`) and add an
> unknown term - otherwise the field is reported as TOP and the site is missed.

### Dynamic confirmation (melonDS GDB stub)

melonDS runs the real stage 2 from the NAND. With the GDB stub enabled
(`[Instance0.Gdb] Enabled = true`, `[Instance0.Gdb.ARM9] BreakOnStartup = true`),
the stub halts at the stage-2 entry `0x037B8000`; setting a breakpoint at
`0x037F0EC0` and continuing shows:

* the launcher loader `0x037BAA18` fires twice (`HNAE`, then `HNAA`) and the
  read helper `0x037B9C30` four times - the HNAA read is the overflow;
* control then reaches `0x037F0EC0` **without** passing the `blx` sites
  `0x037C8410` / `0x037C8428`, i.e. it is the **IPC dispatch table**, not a
  task callback, that hands over.

(Registers other than PC read as the frozen startup state through this stub, so
LR cannot be used to identify the caller; the transfer was isolated by single
stepping from the read helper's return and by the static scan above.)

### Other clobberable function-pointer slots

The same scan also lists the callback slots that are *dereferenced and called*
but were **not** on the observed path. `unlaunch.dsi` arms two of them to
`0x037F0EC0` as well (belt-and-braces), but the exploited transfer is the IPC
table above:

| call site | callback field (RAM) | TMD offset | payload offset |
| --- | --- | --- | --- |
| `0x037C18FC` | `0x037F278C` | `0x13720` | `0x13518` |
| `0x037C2334` | `0x037F2B70` | `0x13B04` | `0x138FC` |
| `0x037C8410` | `0x037F3870` | `0x14804` | `0x145FC` |
| `0x037C8428` | `0x037F3898` | `0x1482C` | `0x14624` |
| `0x037CA2F4` | `0x037F3B5C` | `0x14AF0` | `0x148E8` |
| `0x037CA85C`/`0x037CA8A0` | `0x037F3BF8` | `0x14B8C` | `0x14984` |
| `0x037CBEA4` | `0x037F3C5C` | `0x14BF0` | `0x149E8` |
| `0x037CBED8` | `0x037F3C58` | `0x14BEC` | `0x149E4` |
| `0x037CBF08` | `0x037F3C60` | `0x14BF4` | `0x149EC` |

`0x037F0EC0` is **code inside the payload** (buffer offset `0x11E54`, payload
offset `0x11C4C`). Disassembled in place:

```
0x037F0EC0  ldr sb, [pc,#0x124]   ; sb = 0x037DF274  (payload base)
0x037F0EC4  mrc p15,0,r0,c1,c0,0  ; disable MMU, I-cache, D-cache
            ... set POWCNT1 (0x04000304), DISPCNT ...
0x037F0F6C  bl #0x37F0FAC         ; cache flush
0x037F0F7C  svc #0x110000         ; Unlaunch's own LZ77/copy staging
0x037F0F90  copy loop             ; payload+0x48F0, 0x3C90 bytes -> 0x06020000
0x037F0FA4  bx r0                 ; -> 0x06025710
```

### Consequences for OpenLaunch (implemented + observed end-to-end)

The build (`src/arm9/overflow_bootstrap.s`, `scripts/make_openlaunch_payload.py`)
arms the mechanism:

1. **all 32 entries of the IPC handler table** at payload `0x148F4` -> `0x037F0EC0`
   (plus the two callback slots at payload `0x145FC` / `0x14624`);
2. the bootstrap at RAM `0x037F0EC0` (payload `0x11C4C`);
3. **four IPC/task "context" words** at payload `[0x13000,0x13400)`.

### Differential: what the exploit actually needs

Method (harness validated on the reference each time): *removal* - zero one
16 KiB block of the reference payload at a time; then *additive* - start from
all-zeros + table + slots and add reference ranges. Result: the entry depends on
exactly the four words in payload `[0x13000,0x13400)`:

```
0x037F22C8 = 0x000000DF     ; read by 0x037C8880 (ldr r0,[r0,#0x24]) and tested
0x037F2308 = 0x037F0EC4     ; context+0x40 -> stub (0x037F0EC0 + 4)
0x037F232C = 0x00000001     ; context+0x64 task state
0x037F2330 = 0x037EEB58     ; context+0x68
```

With everything else zero and the IPC table armed these four suffice. Our own
image bytes left `0x037F22C8` zero (and the `0x13000` block otherwise empty),
which is why our earlier builds did not dispatch: stage 2's post-overflow path
reads that context structure and needs it valid.

### Dynamic status (melonDS GDB stub): works end-to-end

Observed on a patched `HNAA` TMD (our payload; stub first bytes `e59f9064`):

```
loader 0x037BAA18 x2 (HNAE, HNAA), read helper 0x037B9C30 x4 (the overflow)
-> 0x037F0EC0   our bootstrap
-> 0x02200800   OpenLaunch ARM9 entry          [reached]
```

### The ARM7 hand-off (solved: jumpslide takeover)

Stage 2 starts a title's ARM7 through its own boot path, but the TMD overflow
bypasses that path: when stage 2 jumps into our ARM9 code the firmware ARM7 has
already left its boot loop. The reference IPCSYNC route therefore cannot work
from the overflow. It is documented here because it is still the title-boot flow
(and `make ARM7_HANDOFF=1` still tries it when the takeover cannot arm).

#### Why stage 2's own hand-off cannot be reused

The trigger, recovered from the decrypted stage 2 (`docs` pipeline +
`scripts/dsi_stage2_decrypt.py`), is a two-sided handshake:

| side | address | behaviour |
| --- | --- | --- |
| ARM9 | `0x037B9AC4` | wait for `IPCSYNC` (`0x04000180`) low byte `== 6`, then write **`0x600`**; then `memcpy 0x200 0x037B91F8 -> 0x023FEE00` and `blx` it (the ARM9 hand-off) |
| ARM9 | `0x037C13E8` | copies **`0x160` (352) bytes to `0x02FFFE00`** - the canonical NDS boot-info |
| ARM7 | `0x02380970` / `0x0238091C` | writes `0x400` to IPCSYNC, waits on IPCSYNC, reads the boot-info at `0x02FFFE00` |
| ARM7 | `0x02381134` | `ldr r4, [r6, #0x34]` - the **ARM7 entry field** (standard NDS header offset `0x34`) |

So the intended sequence is: publish an NDS header at `0x02FFFE00` whose offset
`0x34` is the ARM7 entry, then ring IPCSYNC. `src/arm9/arm7_boot.c`
(`arm9_boot_arm7()`, gated behind `make ARM7_HANDOFF=1`) implements exactly
that, including the logo and both CRC16s.

It does not work from the overflow because the ARM7's IPCSYNC rounds are driven
by its own **boot-mode message ring** (`0x037DF6D8` = write index,
`0x037DF6DC` = read index, 8-byte ring at `0x037DF6E0`). Its boot path
`0x02380164` consumes messages `4`, then `5`, then `6`; only on `6` does it call
the boot manager `0x02381124` -> `0x02381000` (which sends `IPCSYNC = 0x600` and
waits for `6`, rendezvousing with the ARM9's `0x037B9AC4` hand-off). At the
moment of the overflow that ring is **untouched** (`write == read == 0`, all
bytes zero): the ARM7 is already past it and running the system firmware. No
ARM9-side IPCSYNC write can re-enter a boot path that has already completed.

#### The takeover (implemented)

The working route is a jumpslide adapted from
[Astronaut](https://github.com/Vickerinox/Astronaut) (Vickerinox/Astronaut,
GPL-3.0-or-later, `astronaut/arm9/src/arm7_exploit.rs`). It does not need the
ARM7 to cooperate:

* the firmware ARM7 keeps its stack in NWRAM-C, reachable at `0x03800000`;
* the ARM9 temporarily maps WRAM-C to itself
  (`MBK8 = 0x0C003800`, `MBK5 = 0x00000080`), overwrites the firmware's
  return/stack window (`0x03803040..0x03805040`, `0x800` words) with our ARM7
  entry `0x02380000`, cleans the data cache, and restores the exact stage-2
  MBK5/MBK8 values;
* the next return the firmware ARM7 performs pops `0x02380000` and starts our
  image. Astronaut notes that this is timing sensitive; the remap is held for a
  few microseconds only.

OpenLaunch wires it as follows:

* `src/arm9/overflow_bootstrap.s` stages the container's ARM7 image at
  `0x02380000` and writes the marker `0x54424C4F` ("OLBT") to `0x0237FFF0`;
* `arm9_main()` calls `arm9_arm7_takeover()` after IRQs are disabled and the
  caches are flushed, and **before** `scfg_init()`. The marker makes the call a
  complete no-op on the ordinary `.nds` boot and on the title-boot path;
* after a successful takeover, `mbk_init()` is skipped (only `scfg_unlock()` +
  `scfg_apply_boot_config()` run) so WRAM-C stays with the ARM7 for its IWRAM
  and stacks;
* `src/arm7/boot.c` writes `0x59444152` ("RADY") to `0x0237FFF4` once our ARM7
  is executing. That word is the melonDS/GDB check for a successful hand-off;
  the menu's IPC self-test is the functional check.

The IPCSYNC route remains available as `make ARM7_HANDOFF=1` and is only
attempted when the takeover did not arm (for example if MBK9 write-protects
WRAM-C).

Status: implemented and structurally verified (`make verify`, `make host-test`).
`arm9_main()` additionally waits for the ARM7 ready marker and only keeps
stage 2's NWRAM-C ownership when the ARM7 is actually alive; otherwise it falls
back to the normal `mbk_init()` path.

**Validation note (melonDS, 2026-10-23).** On the available test NAND the
takeover arms and returns true (the MBK5/MBK8 writes stick and the slide is
written), but the firmware ARM7 never reaches `0x02380000`. Its reset code -
found in WRAM-C slot 7 at `0x03838000` - loads these stacks:

```
SVC SP = 0x03FFFFC0
IRQ SP = 0x03FFFF80
SYS SP = 0x03FFFB7C
```

The stack is therefore at the top of the `0x03xxxxxx` NWRAM region, not in
Astronaut's `0x03803040` window. Mapping WRAM-A, WRAM-B or WRAM-C at
`0x03FC0000` and filling the top 64 KiB (`0x03FF0000..0x04000000`) did not make
the ARM7 jump either. That points at one of:

* the firmware stack lives in ARM7-private / old shared WRAM that the ARM9
  cannot reach through MBK;
* the ARM7 is parked in a state that does not pop the window (its IRQs may be
  masked, or it is halted awaiting a specific event).

The same is true of the ARM7's **IRQ jump vector**. The reference ARM7 routine
`FUN_00020b58` writes `0x03FFFFFC = 0x060245A0` (see
`asm/decompiled/FUN_00020b58_00020b58.c`; this is the word the ARM7 exception
vector at `0x00000018` jumps through). Reading `0x03FFFFFC` through every
WRAM-A/B/C window at `0x03FC0000` returns zero, as do the stack words
`0x03FFFFC0` and `0x03FFFF80`. Both are ARM7-private / top-of-WRAM aliases, not
NWRAM, so the ARM9 cannot rewrite them. That rules out both the stack slide and
the IRQ-vector hijack from the ARM9.

A complete NDS/TWL header (Nintendo logo, both CRC16s, banner, unit code 3) was
added to the container to match UNLAUNCH.DSI; stage 2 still reaches the ARM9
through the overflow and the ARM7 markers stay zero. The genuine-header
`--keep-prefix` shape on HNAA and an HNAE (official-Unlaunch) install were also
tested; neither started the ARM7. A retail System Menu boot with a breakpoint
on stage 2's `0x037B9AC4` hand-off did not hit it in the trace window, so the
title-boot route needs an ARM7-side trace.

### Live ARM7 trace (melonDS, both cores running)

The melonDS GDB stub only accepts on port 3334 when the ARM7 is executing, so
leave **Break on startup** off for both cores and connect after the system is
running. On the test NAND the live ARM7 is:

```
PC = 0x037F9304     SP = 0x03FFFB6C     LR = 0x037B94D4
IRQ vector [0x03FFFFFC] = 0x037D4B64           (shared WRAM)
ARM7-local MBK6/7/8 = 0x00300000 / 0x00304000 / 0xB837F807
```

This is the **stage-2 ARM7**, running from shared WRAM - not the VRAM system
firmware (`0x0602xxxx`) seen on a normal retail boot. Writing the IRQ vector to
a `b .` loop at `0x0237F000` works: the ARM7 reaches it with CPSR in IRQ/ARM
mode, so the vector jump and Main-RAM execution are valid. Writing it to our
staged image at `0x02380000` still faults (S04) before `arm7_main`, so the
image's early setup needs another pass (the IRQ vector install and Main-RAM
stacks are now in place but unvalidated).

This matches the official Unlaunch description: the ARM9 starts the ARM7 by
**remapping shared WRAM**, not by the VRAM/system-firmware path. The next step
is to map the shared-WRAM bank containing `0x037D4B64` on the ARM9 and redirect
that handler, and to single-step `0x02380000` with the ARM7 stub.

The ARM7's PC `0x037F9304` is in **shared WRAM**, not the ARM9's container
overlay (`0x037DF274`): filling the corresponding container offset with a
branch did not reach it. The ARM9 mirrors at `0x027D4B64` / `0x027F9304` and
the old-WRAM window (`0x03000000` under `WRAMSTAT` 0..3) all read zero, so the
handler/code is not in main RAM or old WRAM that the ARM9 can see. The ARM7
sits with CPSR `0x9F` (IRQ/FIQ masked) and may be halted/crashed after the
overflow clobbered its stage-2 code. The next step is to identify the
shared-WRAM bank/window from the stage-2 setup (or trace the official Unlaunch
ARM9's MBK writes) and restart/redirect the ARM7.

A useful reference clue: the disassembled reference ARM9 (`asm/arm9_vram.bin`)
contains literals for **MBK1 / MBK6 / MBK9 (WRAM-A)**, not MBK4/5/8 (WRAM-C).
The literals sit around `0x060274F0`; the code that consumes them is around
`0x06025988`. The next attempt should replicate that WRAM-A sequence (or run an
official Unlaunch NAND through the same GDB stubs) instead of extending the
WRAM-C slice.

### Reversed ARM7 hand-off routines

| address | routine | role |
| --- | --- | --- |
| `0x06027070` | `FUN_06027070` | Copy the boot header's global MBK1-5 + ARM9 MBK6-8 (0x02FFE180, 32 bytes) to MBK1..MBK8; write `WRAMSTAT` from header byte 0x1AF. |
| `0x06027090` | `FUN_06027090` | Copy the header's **ARM7 MBK6-8** (0x02FFE1A0, 12 bytes) to the ARM9's local MBK6..MBK8, and the global slots with bit0 XORed. This is the reference "match the ARM7's NWRAM windows" step. |
| `0x00020b58` | `FUN_00020b58` | Write the ARM7 IRQ vector (`0x03FFFFFC`), enable the IPC IE bit and IME (reference writes `0x060245A0`). |

### Corrected NWRAM map (2026-10-23, live melonDS + GDB)

Reading the MBK registers as little-endian fixes the earlier confusion:

| register | scope | meaning |
| --- | --- | --- |
| `MBK1`..`MBK5` | shared | NWRAM part ownership, one byte per part (bit7 enable, bits 0-1 CPU, bits 2-4 window slot) |
| `MBK6`..`MBK8` | **per core** | that core's three NWRAM windows (start/end/size) |

Live stage-2 state (healthy build):

* `MBK4 = 0x9C999591`: physical NWRAM-C parts 0..2 -> ARM7 slots 4/5/6,
  part 3 -> ARM9 slot 7.
* `MBK5 = 0x9D999591`: parts 4..6 -> ARM7 slots 4/5/6, part 7 -> ARM7 slot 7.
* ARM7 `MBK8 = 0x07F837B8`: window C starts at **`0x037B8000`**, size 3
  (mask 7, 32 KiB parts). `MBK6/7 = 0x00003000/0x00403000` place windows A/B
  over `0x03000000..0x0302FFFF`.

So the stage-2 ARM7 **executes from NWRAM-C**, not private WRAM. Its link/load
base is `0x037B8000` (matching `arm7.bin`), the IRQ vector slot `0x03FFFFFC`
is part 0 offset `0x7FFC` (window slot 0), and the IRQ handler `0x037D4B64`
is part 3 offset `0x4B64` (window slot 3). All parts are ARM7-owned through
MBK4/MBK5; MBK9 is `0`, so the ARM9 may take them temporarily.

**Critical bug found and fixed:** `mbk_init()` used to rewrite MBK1..5 with the
DS-style canonical slots (A -> ARM7, B/C -> ARM9). On this firmware that moves
the NWRAM-C parts out from under the running ARM7, so its next instruction
fetch hits an unmapped slot: illegal instruction (GDB `S04`), PC
`0x037F9304`. Every later takeover attempt then failed. On the overflow path
`arm9_main()` now keeps stage 2's slot ownership and runs only
`scfg_apply_boot_config()`; the normal homebrew/.nds path still calls
`mbk_init()`.

Healthy ARM7 with the fix (melonDS, 2026-10-23):

```
PC   0x00000EAC   (BIOS halt helper, just after the HALTCNT write)
SP   0x037F3290
CPSR 0x0000001F   (System mode, IRQ+FIQ enabled)
IME  1
IE   0x00040008   (Timer0 + IPC receive)
```

An ARM9 IPC FIFO write sets `IF` bit 18; continuing the ARM7 shows it wake,
run the dispatcher, clear `IF` and halt again. The wake path works.

### ARM7 IRQ / IPC path (all in ARM7-owned NWRAM-C)

* `[0x03FFFFFC] = 0x037D4B64` -> generic dispatcher: reads IE/IF, acks the
  pending bit with a write-1-to-clear, and calls `table[bit]` from
  `0x037DD7E4` (IF) / `0x037DD7A8` (IF2).
* IPC receive (bit 18) -> `0x037D6FEC`: reads `0x04100000` and dispatches on
  the **low 5 bits** through the command table at `0x037F347C`.
* Live commands: `1` (ring push), `2` (ring consume/advance), `19`, `20`,
  `23`. The boot-message ring is `0x037DF6D8` (indices) / `0x037DF6E0`
  (bytes). Pushing `4,5,6` through command 2 works (write index 3, bytes
  `04 05 06`) but the halted loop never consumes them.
* `0x037B9494` is the ring consumer (`write == read` spin); it is used by the
  boot path at `0x037B81F8`, which expects `4`, then `5`, then `6`.

Attempts that did **not** work, and why:

* matching the ARM9's MBK6-8 to the ARM7's and writing `0x03801304` or
  `0x03FFFFFC` by guesswork — the ARM9 window/slot arithmetic was wrong
  (register reads are little-endian, and the ARM7 window starts at
  `0x037B8000`);
* taking part 7 / part 1 via MBK4/MBK5 — the window slot is derived from the
  start address, not the part index;
* the shared-WRAM (`WRAMSTAT`) jumpslide: with `WRAMSTAT=0` the ARM9 can fill
  the 32 KiB bank only after clearing its own MBK6-8 (otherwise NWRAM-B
  shadows `0x03000000`); with `WRAMSTAT=3` the ARM7 sees the bank at
  `0x03030000+`, but `0x037B8000+` is NWRAM-C for the ARM7, so the handler is
  never fetched from the bank;
* melonDS GDB `P` (register write) segfaults the emulator, so the PC cannot be
  forced from the stub.

Next: derive the part/slot mapping from the **live MBK4/MBK5 bytes** every
boot, take the part containing `0x03FFFFFC` (slot 0) and/or `0x037D4B64`
(slot 3), write the ARM7 entry, restore ownership, and wake via the IPC
receive IRQ. The boot-ring consumer at `0x037B9494` is the fallback once the
loop that calls it can be re-entered.

The remaining routes are:

1. a hijack target in memory the ARM9 *can* write (main RAM or a firmware IPC
   handler);
2. an IPC command that makes the running firmware ARM7 restart/boot a title;
3. installing OpenLaunch as a real title so stage 2 starts both CPUs through
   its normal flow.

`make openlaunch-payload` builds the wired container; `make verify` and
`scripts/verify_openlaunch_payload.py` check it structurally.
