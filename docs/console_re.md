# Nintendo DSi (TWL) — console reverse-engineering index

A living index of everything this project has reversed about the DSi, with a
status for each item and pointers to the tooling. Companion docs:
[`hardware_notes.md`](hardware_notes.md) (register-level detail) and
[`stage2_re.md`](stage2_re.md) (boot stage-2 / TMD overflow).

Legend: **DONE** = reversed + implemented/tooled + validated · **PART** =
reversed, tooling incomplete · **TODO** = not yet reversed.

## 1. SoC and memory map

| Item | Status | Notes |
| --- | --- | --- |
| ARM9 (ARM946E-S) + ARM7 (ARM7TDMI), dual CPU | DONE | `src/arm9`, `src/arm7`, `ld/*.ld` |
| ITCM/DTCM, MPU/CP15 setup | DONE | `src/arm9/mpu.c`, `crt0.s` |
| Main RAM 16 MiB, NWRAM `MBK1..MBK9`, `SCFG_*` | PART | `src/arm9/scfg.c`, `docs/hardware_notes.md` §4/§24 |
| IPC FIFO (ARM9↔ARM7) | DONE | `src/*/ipc.c`, `include/common/ipc_protocol.h` |
| NDMA, 2D engines, SPI, I2C/BPTWL, RTC, audio | PART | per `PLAN.md`; no crypto-relevant gaps |

## 2. Boot chain

| Stage | Where | Encryption | Status |
| --- | --- | --- | --- |
| Boot ROM (stage 1) | SoC, protected | n/a | not dumped (documented) |
| **Stage 2** | NAND `0x200` info block; ARM9 `0x800`, ARM7 `0x26E00` | AES-128-CTR, **universal "Tad" key** | **DONE** — `scripts/dsi_stage2_decrypt.py`, ARM9 SHA1 verified |
| Launcher load (TMD + `.app`) | `title/00030017/…/content` | stage-2 decrypts `twln` files | PART (TMD path + loader located) |
| System Menu (launcher) | `00030017/<region>` | per-console (NAND key) | DONE (filesystem reads) |

Stage-2 crypto (validated): `keyX="Nintendo DS\x00\x01#!\x00"`, `keyY` = first
16 bytes of the RSA-1024-signed block at `0x300`, keygen `((kx^ky)+C2) ROL 42`,
CTR `[size,-size,~size,0]` +1/block, DSi AES byteswap.

> Open question: the stored `sha1_arm7` field does not match a plain SHA1 of the
> decrypted ARM7, even though it disassembles to correct ARM7 boot code (so the
> key/CTR is right). Resolve later; it does not affect the exploit.

## 3. Crypto inventory

| Blob | Key | Status |
| --- | --- | --- |
| eMMC MBR + partitions (`twln`, `twlp`) | per-console `keyX=f(ConsoleID)`, `keyY`=const, IV=`SHA1(CID)+addr/0x10` | **DONE** — `src/common/dsi_crypto.c`, suite 17, validated on a retail dump |
| Stage-2 ARM9/ARM7 | universal Tad key | **DONE (ARM9)** |
| Modcrypt (DSi-enhanced games) | `keyX="Nintendo DS"+gamecode`, `keyY`=arm9i HMAC | TODO (scheme known; `crypto_twl.hpp`) |
| ES block / DSiWare `.app` + ticket | ES/`twl` keys (console-unique) | TODO |
| TWL RSA-1024 signatures (NAND/gamecard/NOR/3DS moduli) | public moduli | PART (NAND modulus used) |

## 4. Filesystem layout (from a real dump)

```
twln  LBA 0x877   205.9 MiB  FAT16  (System Menu + system titles, tickets, shared)
twlp  LBA 0x6784D  32.7 MiB  FAT16  (photos/DsiWare)
extra LBA 0x77E5D   419 sect  FAT12
```

Titles observed: `00030017` System Menu (`HNAE` real, `HNAA` backup), `00030015`,
`0003000F`, `00030005`, `00030004`. Tooling: `scripts/dsi_nand_install.py`,
`tools/dsi_nand_tool.c`.

## 5. Exploit surface (survey)

| Vector | Mechanism | Status |
| --- | --- | --- |
| **Launcher TMD overflow** | stage 2 loads `title.tmd` without a real `filesize` check → overflows an IPC/controller structure | **RE'd + working in melonDS**: bug at `0x037BAA18`, buffer `0x037DF06C`; entry = clobbered ARM9 IPC handler table `0x037F3B68` (dispatch `0x037CA720`); the container arms the table + a stub and is observed to reach OpenLaunch's ARM9 entry, which then arms the **ARM7 jumpslide takeover** (`stage2_re.md`, `tools/stage2_dataflow.py`) |
| SafeUnlaunch variant | exploit installed to the *backup* launcher (`HNAA`); real `HNAE` TMD byte broken → stage 2 falls back | **DONE** (mechanism + tooling) |
| Modcrypt | modcrypt region key/IV from rom header | TODO |
| ES / DSiWare | CCM (non-standard) on SD titles + tickets | TODO |
| Sudokuhax / flipnote (DSiWare saves) | save-game overflow | out of scope (known, closed) |
| Hardware (hardmod eMMC) | direct eMMC read/write | out of scope (documented) |

## 6. Tooling built so far

| Tool | Purpose |
| --- | --- |
| `make nand-tool` (`tools/dsi_nand_tool.c`) | offline eMMC `info`/`decrypt`/`encrypt`/`verify` (per-console key) |
| `scripts/dsi_stage2_decrypt.py` | decrypt stage-2 ARM9/ARM7 (universal Tad key) |
| `scripts/dsi_nand_install.py` | read/rewrite `twln` FAT; `patch` = decrypt→TMD→encrypt |
| `scripts/make_openlaunch_payload.py` | build the Unlaunch-format bootcode container (ARM9/ARM7 + overflow stub + slot pointers) |
| `src/arm9/overflow_bootstrap.s` | stage-2 overflow stub: relocates the images and jumps to the ARM9 entry |
| `scripts/verify_openlaunch_payload.py` | structural check of the container (header, slots, stub) |
| `tools/stage2_dataflow.py` | static data-flow pass that found the clobbered callback slots |
| `scripts/inspect_nand_dump.py` | dump layout + `0xFF800` footer (CID/Console ID) |

## 7. Recommended next steps

1. Finish the stage-2 overflow hunt (ARM9): find the `filesize > filesize` site
   and the clobbered task-switch structure → the OpenLaunch payload bootstrap.
2. Add `firmware` to the C tool (RSA + Tad key) so stage-2 extraction needs no
   Python.
3. Resolve the `sha1_arm7` field (compression/header nuance).
4. Widen crypto: modcrypt, ES/tickets, and the non-NAND RSA moduli.
5. Document each hardware block as it is translated (keep this index current).
