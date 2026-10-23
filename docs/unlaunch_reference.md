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
