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
ARM9/ARM7 flat images to their linked bases, cleans/invalidates the caches and
branches to the ARM9 entry. Building the payload is:

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
* the **ARM7 handoff** is not performed (and the reference does not perform one
  either): the ARM7 keeps running the system firmware. The stub stages our ARM7
  at `0x02380000` but never starts it - verified with a temporary marker (never
  observed). Stage 2 boots an ARM7 through the `0x02FFFE00` boot-info block plus
  an IPCSYNC handshake; reusing that requires reversing the firmware ARM7's
  trigger (see [`stage2_re.md`](stage2_re.md)).
* `nand_patched.bin` is still **not hardware-validated**: flash only on a console
  with a full NAND backup (docs §34).

## Safety summary

* Only the `HNAA` backup TMD is modified; the real launcher (`HNAE`) and its
  byte-break are left as SafeUnlaunch wrote them.
* Every build step is offline; the tool never touches a console.
* The mandatory full-NAND backup / recovery guidance in docs §34 still applies
  before any flash.
