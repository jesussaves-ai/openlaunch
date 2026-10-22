# PLAN.md — Translation Roadmap

## 0. Current Status

Reference material: `asm/decompiled/*.c` (ARM7) and `asm/decompiled/arm9/*.c`
(ARM9, 271 functions). Extraction pipeline: `scripts/run_ghidra.sh`.
Rules of engagement: see `AGENTS.md`.

### What the reference dump actually is

`asm/UNLAUNCH.DSI` is a Nintendo DSi executable. Parsing the NDS header at file
offset `0x00`:

| Field                | Value        | Notes                                        |
| :------------------- | :----------- | :------------------------------------------- |
| ARM9 ROM offset      | `0x00004000` | crt0 + graphics + one LZ77 code segment      |
| ARM9 entry address   | `0x02200800` | stage-1 crt0                                 |
| ARM9 size            | `0x0000AD80` | raw slice at `asm/arm9_raw.bin`              |
| ARM7 ROM offset      | `0x0000F800` | plain-code ARM7 image (analysed)             |
| ARM7 entry address   | `0x02380000` |                                              |
| ARM7 size            | `0x000205F8` |                                              |

Resolved ARM9 structure (see `docs/hardware_notes.md` §9):

* `0x0800`–`0x08F0` is a crt0 stub that programs CP15/SCFG, handshakes over
  `IPCSYNC`, copies a `0x3C90`-byte graphics blob to VRAM `0x06020000`, and
  LZ77-expands a `0x931C`-byte code segment to VRAM `0x06023C90`.
* The stage-2 entry is `0x0602572C`. It was **not** compressed/encrypted; the
  earlier "compressed" note was an artefact of slicing the raw blob and loading
  it at the wrong base.

Consequences:

1. **342 ARM7 functions** live at file `0x1C6BC`–`0x2A7xx` (ARM7 virtual
   `0x0238CEBC`–`0x0239A7xx`) and are exported to `asm/decompiled/`.
2. **271 ARM9 functions** live in the decompressed VRAM code segment and are
   exported to `asm/decompiled/arm9/`. The reconstructed payload is
   `asm/arm9_vram.bin` (base `0x06020000`, entry `0x0602572C`).
3. `scripts/unpack_unlaunch_arm9.py` reproduces the crt0 relocation +
   LZ77 expansion from the raw image; `scripts/run_ghidra.sh` imports an ELF
   wrapper (`build/arm9_code.elf`) so Ghidra's ARMv5T analysis works natively.
4. The files named `UndefinedInstruction_00000004.c` … `FIQ_0000001c.c` are
   **not real vectors** — address `0x04`–`0x1C` falls inside the ASCII title
   `"UNLAUNCH.DSI"`. Do not trust them.
5. `DAT_*` literals were resolved against the raw image; the important ones are
   hardware base pointers, catalogued in `docs/hardware_notes.md` and encoded in
   `include/common/dsi_hw.h`.

## 1. Subsystems identified and their progress

| Subsystem            | Reference symbols                                   | Target file(s)            | State         |
| :------------------- | :-------------------------------------------------- | :------------------------ | :------------ |
| Memory primitives    | `FUN_0001c714/72c/c7d0/ca50/caa8/c8d8/c8fc/c918/c92c`| `src/common/mem.c`        | **translated**|
| LZ77 / RLE           | crt0 SWI `0x11` (`@ref 0x02200800`)                 | `src/common/lz77.c`       | **translated**|
| Cache / CP15         | `FUN_0001cae0`, `FUN_0602c338`                       | `src/common/cache.c`      | **translated**|
| IRQ control          | `FUN_00020ae4/af8/b04/cb0/df84/27028`               | `src/arm7/irq.c`          | **translated**|
| IPC FIFO / sync      | `FUN_0001cc34/cb38/20be4`, `FUN_0001f21c`           | `src/arm7/ipc.c`          | **started**   |
| IPC storage protocol | clean-room (ARM7-gated delegation)                  | `include/common/ipc_protocol.h` | **new** |
| ARM9 storage client  | clean-room (IPC only, no MMIO)                      | `src/arm9/storage.c`      | **new**       |
| SPI bus (ARM7)       | `FUN_0001c96c/c978/c984/c9d8/c9f4/ca10/ca30`        | `src/arm7/spi.c`          | **translated**|
| Firmware read (ARM7) | clean-room SPI READ (0x03)                          | `src/arm7/spi.c`          | **new**       |
| RTC (bit-banged)     | `FUN_0001cf1c/40/54/60/cfb0/cfdc`                   | `src/arm7/rtc.c`          | **translated**|
| Timers               | `FUN_00020cd0/b04`                                  | `src/arm7/timer.c`        | **translated**|
| TMIO engine          | `FUN_0001d154/d930/d958/d750/1d328/d6b8/d6e4`       | `src/arm7/sdmmc.c`        | **translated**|
| SD/MMC block read    | clean-room CMD17 + FIFO (reference is SDIO; §20)    | `src/arm7/sdmmc.c`        | **new**       |
| BPTWL / I2C (PMIC)   | *(not in dump)*                                     | `src/arm7/i2c_bptwl.c`    | TODO          |
| AES / crypto         | *(not in dump)*                                     | `src/common/crypto.c`     | TODO          |
| ARM7 entry / vectors | `@ref 0x02380000`                                   | `src/arm7/crt0.s`         | TODO          |
| ARM9 entry / vectors | `@ref crt0 0x02200800`, `@ref entry 0x0602572C`     | `src/arm9/boot.c`/`crt0.s`| **translated**|
| ARM9 MPU / TCM       | `FUN_0602c374`                                      | `src/arm9/mpu.c`          | **translated**|
| ARM9 IRQ / IE        | `FUN_0602bfb4/bfc8/bfdc`, `FUN_06025bf4`            | `src/arm9/irq.c`          | **translated**|
| ARM9 IPC             | `FUN_0602bff0/c008`, crt0 sync                      | `src/arm9/ipc.c`          | **translated**|
| ARM9 2D / VRAM       | `FUN_0602555c/25b30/26244`, `entry`                 | `src/arm9/video.c`        | **translated**|
| ARM9 text console    | clean-room (GBATEK BG/VRAM)                         | `src/arm9/console.c`      | **new**       |
| ARM9 8x8 font        | clean-room                                          | `src/arm9/font8x8.c`      | **new**       |
| ARM9 keypad input    | `FUN_06024fa0` (KEYINPUT polling)                   | `src/arm9/input.c`        | **new**       |
| ARM9 boot menu       | clean-room                                          | `src/arm9/menu.c`         | **new**       |
| ARM9 DMA             | `FUN_0602bf88`, `entry`                             | `src/arm9/dma.c`          | **translated**|
| SCFG clock / 16 MB   | crt0, `FUN_06027158`                                | `src/arm9/scfg.c`         | **translated + MBK**|
| ARM9 SPI / firmware  | `FUN_0602546c/478/484/b760/b828/bd94`               | *(no ARM9 MMIO; via IPC)* | **gated**     |
| eMMC read path       | `FUN_0001d6b8/d6e4/d760`                            | `src/arm7/sdmmc.c`        | **translated**|
| SD/MMC write path    | `FUN_0001d760`                                      | `src/arm7/sdmmc.c`        | gated (RO)    |
| FAT16/FAT32 parser   | clean-room (read-only, zero-allocation)             | `src/common/fat_ro.c`     | **new**       |
| NDS header validator | clean-room (CRC16 + title/gamecode)                 | `src/common/nds_header.c` | **new**       |
| SD browser (ARM9)    | clean-room (mount + list + header load)             | `src/arm9/menu.c`         | **new**       |
| NDS load validator   | clean-room (CRC16 + ARM9/ARM7 range safety)         | `src/common/nds_header.c` | **new**       |
| Chainload stager     | clean-room (bounded cluster stream + handoff)       | `src/arm9/loader.c`       | **new**       |
| Dual-CPU handoff     | clean-room (`IPC_CMD_BOOT_HANDOFF`)                 | `src/arm7/ipc.c`          | **new**       |
| BOOT.CFG hotkey map  | clean-room (zero-allocation text parser)            | `src/arm9/boot_cfg.c`     | **new**       |
| 1-level subdir FAT   | clean-room (`fat_find_path`/`fat_list_dir_cluster`) | `src/common/fat_ro.c`     | **new**       |

## 2. Next steps

1. ARM9 auxiliary-SPI / firmware block
   (`FUN_0602b760`/`FUN_0602b828`/`FUN_0602546c`). Per the architectural rule
   this must **not** become an ARM9 MMIO driver: the firmware is reached from
   the ARM9 only through `arm9_firmware_read()` → IPC → `src/arm7/spi.c`.
2. Translate the ARM9 handoff at `@ref entry 0x0602572C` (payload copy from
   `0x023FEE00` and indirect call).
3. Flesh out `FUN_0001f21c` (the IPC command dispatcher) into a typed
   request/response protocol in `src/arm7/ipc.c` + `include/common/ipc_protocol.h`.
   *(done for the telemetry commands: `IPC_CMD_GET_RTC`,
   `IPC_CMD_GET_SDMMC_STATUS`, `IPC_CMD_GET_FW_INFO`, `IPC_CMD_GET_EXTKEY`.)*
   *(done for the ARM7-gated delegated reads: `IPC_CMD_READ_SECTOR`,
   `IPC_CMD_READ_FIRMWARE` — see docs/hardware_notes.md §19.)*
4. TMIO command engine (`FUN_0001d930`/`d958`/`d750`). *(done: the FIFO
   command/transfer engine and the `CMD17` block reader are in
   `src/arm7/sdmmc.c`; see docs/hardware_notes.md §20. Multi-block `CMD18`
   and the NDMA path remain future optimisations.)*
5. Add the melonDS / RAM-boot smoke test in `tests/`. *(partial: host
   mock-based tests in `tests/test_sdmmc.c`; the in-ROM self-test now performs
   a delegated LBA-0 read.)*
6. Read-only FAT16/FAT32 parser and SD browser. *(done: `src/common/fat_ro.c`
   mounts the volume over the delegated sector reader; menu item
   `[6] Browse SD Card (.nds)` lists `.nds`/`.srl` files and loads the selected
   header via `src/common/nds_header.c` — see docs/hardware_notes.md §21.)*
7. Safe read-only `.nds`/`.srl` chainloader. *(done: header load-range
   validation (`nds_header_validate_loadable()`), a bounded FAT cluster-chain
   reader (`fat_read_file_range()`), the ARM9 staging pipeline
   (`src/arm9/loader.c`) and the dual-CPU `IPC_CMD_BOOT_HANDOFF` handoff
   (`src/arm7/ipc.c`). The actual CPU branch is gated behind
   `ENABLE_CHAINLOAD_BOOT` (dry run by default) — see
   docs/hardware_notes.md §25. Remaining polish: a hardware/emulator run of a
   real multi-MiB image and the ARM7-WRAM self-copy edge case.)*

## 3. Recent work (C99 bootloader bring-up)

* **Register widths** — `POSTFLG` is now an 8-bit store (a 32-bit write clobbers
  `HALTCNT`), `POWCNT1` is 16-bit, `IPCSYNC`/`IPCFIFOCNT`/`IME` are 16-bit. See
  docs/hardware_notes.md §13.
* **TWL detection** — `scfg_is_twl_mode()` gates the DSi-only SCFG and NDMA
  writes so the ROM degrades gracefully in legacy DS mode (docs §14).
* **ROM padding** — the `.nds` is padded to a power of two ≥ 32 KiB with the
  device-capacity byte and header CRC fixed up (docs §17).
* **UI** — a clean-room 8x8 font and dual-screen text console plus a D-Pad
  navigable diagnostic menu (docs §15).
* **ARM7-gated delegation** — the TMIO SD/eMMC controller and firmware SPI
  flash are now reached only from the ARM7. The ARM9 uses parameterised IPC
  requests (`src/arm9/storage.c`) and never touches their registers; the ARM7
  dispatcher executes the transfer and replies (docs §19).
* **TMIO block reader** — `sdmmc_read()` is a working 512-byte `CMD17` reader
  built on a bounded, clean-room translation of the reference FIFO/command
  engine (docs §20). The reference TMIO routines were found to target the SDIO
  (wifi) controller; the shared register model is applied to SD/MMC. Covered by
  `tests/test_sdmmc.c` and an in-ROM LBA-0 read.
* **FAT16/FAT32 + SD browser** — a zero-allocation, read-only FAT parser
  (`src/common/fat_ro.c`) mounts the SD card over the delegated sector reader;
  menu item `[6]` lists `.nds`/`.srl` files and loads the selected header
  (`src/common/nds_header.c`, CRC16 validated). Covered by
  `tests/test_fat_ro.c` (docs §21).
* **SCFG unlock + NWRAM (MBK) bring-up** — the console-mode probe now uses the
  canonical `SCFG_A9ROM & 3 == 1` test on both cores and treats an explicit
  `0x03` as authoritative (a DSi running an NDS title), fixing the old
  `SCFG_EXT.bit31` fallback that mis-detected DS-compatibility mode.
  `scfg_init()` unlocks `SCFG_EXT.bit31`, applies the reference
  `SCFG_EXT`/`SCFG_CLK` values and programs `MBK1`..`MBK5` (honouring the
  `MBK9` write-protect). The register-inspection screen and top-screen
  telemetry now show the live mode, ARM9 clock, RAM window and all nine MBK
  registers. Covered by `tests/test_scfg.c` (docs §14).
* **TWL hybrid header** — `scripts/pack_nds.py` now writes unit code `0x02`
  (NDS + DSi) instead of the DSi-exclusive `0x03`, so a DSi launcher/emulator
  (and melonDS in DSi console mode) keeps native TWL mode active
  (`SCFG_A9ROM == 0x01`) while the image stays DS-bootable. `verify_rom.py`
  re-checks the logo/header CRC16s and asserts the hybrid unit code with a
  non-zero TWL region mask. `src/common/nds_header.c` decodes the field for the
  SD browser (docs §24).
* **TMIO0 read-only SD probe/browse** — the ARM7 now brings the DSi SD/MMC
  block up (`sdmmc_enable_clock()` sets `SCFG_EXT7.bit18` + `SCFG_CLK7.bit0`,
  self-gated on DSi mode) and runs the full read-only identification sequence
  (CMD0/CMD8/ACMD41/CMD2/CMD3/CMD9/CMD7/CMD16) once per slot, after the
  presence gate. The OCR CCS bit selects SDSC byte vs SDHC/SDXC block
  addressing for CMD17. `IPC_CMD_GET_SDMMC_STATUS` gained a card-info word, and
  `arm9_storage_probe()` now reports the LBA-0 `0xAA55` signature, the MBR
  partition type, the FAT16/FAT32 volume type and capacity. `[3]` and `[6]`
  render all of it or a specific failure reason. No write command exists outside
  `ENABLE_NAND_WRITE_DANGER` (docs §24, AGENTS.md §5). Covered by the extended
  `tests/test_sdmmc.c` and `tests/test_fat_ro.c`.
* **Homebrew argv + TWL extended segments + BOOT.NDS autoboot** —
  `loader_build_argv()`/`loader_stage_argv()` publish the canonical
  `__system_argv` structure (`0x5F617267`, command line `"sd:/<NAME>"`) at
  `header_base + 0x70`; `nds_header_parse()` decodes the DSi extended header and
  `nds_header_validate_twl_segments()` gates optional `ARM9i`/`ARM7i` segments,
  which `loader_stage_and_boot()` streams in TWL mode; and
  `boot_resolve_policy()` implements the Unlaunch-style hotkey policy (B/SELECT
  forces the menu, otherwise `BOOT.NDS` is probed/booted, with menu item `[8]`
  and an optional `AUTOBOOT=1` build). `make sd-image` now also installs
  `BOOT.NDS` (title `AUTOBOOT OK`). Covered by `tests/test_loader.c`,
  `tests/test_sd_image.c` and `tests/test_menu.c` (docs §27).
* **Read-only chainloader + dual-CPU handoff** — `nds_header_validate_loadable()`
  now enforces the CRC16 and the ARM9/ARM7 load-range safety window
  (`src/common/nds_header.c`), and `fat_read_file_range()` streams a bounded byte
  range with FAT-chain loop/bad-cluster/truncation guards (`src/common/fat_ro.c`).
  Menu `[6]` gained a **Y** verify/dry-run inspect and an **A** stage+handoff
  action; `src/arm9/loader.c` stages the ARM9 payload, bounces the ARM7 payload
  through shared RAM, copies the header to `0x027FFE00`/`0x02FFFE00` and sends
  `IPC_CMD_BOOT_HANDOFF` to `src/arm7/ipc.c`. The CPU branch is gated behind
  `ENABLE_CHAINLOAD_BOOT` (dry run by default). `ld/arm7.ld`/`ld/arm9.ld` now
  align the IWRAM/ITCM/DTCM load addresses for the word-copy crt0 relocation.
  Covered by `tests/test_loader.c` (docs §25).
* **`BOOT.CFG` hotkey map + one-level `/NDS` subdirectory** —
  `boot_cfg_parse()` (`src/arm9/boot_cfg.c`) is a pure, bounds-checked parser
  over one 512-byte sector: `DEFAULT`/`A`/`X`/`Y`/`L`/`R`/`START` bind the
  buttons held at startup to targets, while `B`/`SELECT` can never be overridden
  and always force the interactive menu. `boot_resolve_target()` resolves the
  active (KEYINPUT + EXTKEYIN) mask to a path. `fat_find_path()` and
  `fat_list_dir_cluster()` (`src/common/fat_ro.c`) add a bounded, loop-guarded
  one-level subdirectory lookup (`/NDS/<FILE>.NDS`); the browser lists
  `NDS/<NAME>` entries and `loader_stage_and_boot_path()` publishes the correct
  `"sd:/NDS/<NAME>"` `__system_argv`. `make sd-image` now installs
  `NDS/SUBAPP.NDS` (title `SUBDIR PAYLOAD`) and a root `BOOT.CFG`. Covered by
  `tests/test_fat_ro.c`, `tests/test_menu.c`, `tests/test_loader.c` and
  `tests/test_sd_image.c` (docs §28).
