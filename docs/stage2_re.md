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

### The ARM7 hand-off (mechanism recovered; not yet effective)

The overflow is entirely on the ARM9: the ARM7 keeps running the system
firmware. Verified in melonDS with a marker write at the top of our
`arm7_main()` (read back through the ARM9 stub): the marker never appears, so
our ARM7 image is staged at `0x02380000` by the stub but never executed.

The reference does the same (`unlaunch.dsi`'s ARM9 has **no** reference to
`0x02380000`), so it also relies on the existing firmware ARM7.

**The trigger**, recovered from the decrypted stage 2 (`docs` pipeline +
`scripts/dsi_stage2_decrypt.py`), is a two-sided handshake:

| side | address | behaviour |
| --- | --- | --- |
| ARM9 | `0x037B9AC4` | wait for `IPCSYNC` (`0x04000180`) low byte `== 6`, then write **`0x600`**; then `memcpy 0x200 0x037B91F8 -> 0x023FEE00` and `blx` it (the ARM9 hand-off) |
| ARM9 | `0x037C13E8` | copies **`0x160` (352) bytes to `0x02FFFE00`** - the canonical NDS boot-info |
| ARM7 | `0x02380970` / `0x0238091C` | writes `0x400` to IPCSYNC, waits on IPCSYNC, reads the boot-info at `0x02FFFE00` |
| ARM7 | `0x02381134` | `ldr r4, [r6, #0x34]` - the **ARM7 entry field** (standard NDS header offset `0x34`) |

So the intended sequence is: publish an NDS header at `0x02FFFE00` whose offset
`0x34` is the ARM7 entry, then ring IPCSYNC.

**Implemented** in `src/arm9/arm7_boot.c` (`arm9_boot_arm7()`, gated behind
`make ARM7_HANDOFF=1`): it builds a valid header (logo + both CRC16s, ARM7 entry
`0x02380000`), copies 352 bytes to `0x02FFFE00`, then waits for `IPCSYNC == 6`
(bounded) and writes `0x600`.

**Status: the hand-off is a dead end — the ARM7 is not listening.** The header is
published correctly (melonDS reads `0x02380000` back from `0x02FFFE34`) and the
staged ARM7 image is in place at `0x02380000`, but the ARM7 never runs it, and a
GDB-stub read of ARM7 state at the exploit shows exactly why:

* the ARM7's IPCSYNC rounds are driven by its own **boot-mode message ring**
  (`0x037DF6D8` = write index, `0x037DF6DC` = read index, 8-byte ring at
  `0x037DF6E0`). Its boot path `0x02380164` consumes messages `4`, then `5`, then
  `6`; only on `6` does it call the boot manager `0x02381124` -> `0x02381000`
  (which sends `IPCSYNC = 0x600` and waits for `6`, rendezvousing with the ARM9's
  `0x037B9AC4` hand-off);
* at the moment of the overflow that ring is **untouched** (`write == read == 0`,
  all bytes zero) - i.e. the ARM7 is not parked in its boot-mode flow waiting for
  a message; it is already past it and running the system firmware.

So no ARM9-side IPCSYNC write can make the running firmware ARM7 jump: the
decision to boot a title ARM7 lives in the ARM7's **boot** path, which has long
since run. Reaching it would mean re-entering the ARM7's boot flow (an ARM7
reset), which the ARM9 cannot trigger and which would be far riskier than useful.
This is precisely why the reference (`unlaunch.dsi`) never boots its own ARM7 and
instead talks to the firmware ARM7.

`arm9_boot_arm7()` is therefore kept but off by default (`make ARM7_HANDOFF=1`),
and the work moves to option (2): driving the **firmware** ARM7's own IPC
services from the ARM9. Two consequences matter for that:

* our staged ARM7 image at `0x02380000` overwrites the firmware ARM7's own image
  in shared WRAM, so staging it (and thus the payload's ARM7 section) must be
  dropped before the ARM9 can rely on the firmware ARM7;
* the ARM9's IPC registers follow GBATEK (`IPCSYNC`/`IPCFIFOCNT`/`IPCFIFOSEND` at
  `0x04000180`/`84`/`88`, ARM9 receive at `0x04100000`), but the firmware ARM9
  itself receives at `0x04100010` in at least one path, so the receive-register
  base must be confirmed against the firmware before trusting our IPC layer.

`make openlaunch-payload` builds the wired container; `make verify` and
`scripts/verify_openlaunch_payload.py` check it structurally. The remaining item
is the ARM7 handoff (the stub stages the ARM7 at `0x02380000` but does not start
it).

