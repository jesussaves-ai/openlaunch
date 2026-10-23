# PLAN.md — OpenLaunch Translation Roadmap

## 0. Current Status

**OpenLaunch** (`https://github.com/jesussaves-ai/openlaunch`) is the
clean-room, open-source C99 alternative to Unlaunch. Reference material:
`asm/decompiled/*.c` (ARM7) and `asm/decompiled/arm9/*.c` (ARM9, 271
functions). Extraction pipeline: `scripts/run_ghidra.sh`.

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
| DLDI scan / patch    | clean-room (Chishm DLDI format)                     | `src/common/dldi.c`       | **new**       |
| BOOT.CFG inspector   | clean-room (menu item [9])                          | `src/arm9/menu.c`         | **new**       |
| NDS banner decoder   | clean-room (0x068 block, CRC16 + UTF-16LE title)    | `src/common/nds_header.c` | **new**       |
| Graphical UI font    | clean-room (runtime 16x16 font + banner icon)       | `src/arm9/ui_font.c`      | **new**       |
| NAND partition table | clean-room (PC MBR + TWL {offset,size} parser)      | `src/common/nand.c`       | **new**       |
| NAND boot redirect   | clean-room (HiyaCFW / System Menu policy)           | `src/arm9/nand_boot.c`    | **new**       |
| BOOT.CFG theme / color parser | clean-room (`THEME_ACCENT`/`TEXT`/`BG`, `SPLASH_IMAGE`) | `src/arm9/theme.c` + `boot_cfg.c` | **new** |
| Theme splash renderer | clean-room (SD 256x192 15-bit BGR555 -> Engine A BG2) | `src/arm9/theme.c`     | **new**       |
| Release packager     | clean-room (variants + SD template + SHA256SUMS)    | `scripts/make_release.py` | **new**       |
| SHA-256 / HMAC       | clean-room (FIPS 180-4 / RFC 2104)                  | `src/common/sha256.c`     | **new**       |
| NAND layout guards   | clean-room (MBR/TWL validation + boot-prefix veto)  | `src/common/nand.c`       | **new**       |
| Installer policy     | clean-room (signed token + preflight + state machine)| `src/common/installer.c`  | **new**       |
| FAT root-file writer | clean-room (bounded create/write/abort)             | `src/common/fat_rw.c`     | **new**       |
| NAND backup/flash    | clean-room (injected I/O + SHA-256 backup + read-back)| `src/arm9/installer_backup.c`/`installer_install.c` | **new** |
| Installer progress UI| clean-room (progress hook + modal stages + recovery) | `src/arm9/installer_ui.c` | **new** |
| Guarded eMMC write   | clean-room (`IPC_CMD_WRITE_SECTOR`/`NAND_GUARD`)    | `src/arm7/ipc.c`          | gated         |

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
8. ARM7 hand-off for the overflow boot — **implemented; validation negative on
   the current test firmware**. Stage 2's own IPCSYNC flow is unreachable from
   the overflow (its boot-mode ring at `0x037DF6D8` is already past messages
   `4`/`5`/`6`), so the ARM9 hijacks the running firmware ARM7 instead: the
   overflow bootstrap stages our ARM7 at `0x02380000`, and
   `arm9_arm7_takeover()` (`src/arm9/arm7_boot.c`) briefly maps NWRAM-C and
   overwrites the firmware's return/stack window at `0x03803040` with
   `0x02380000` (mechanism adapted from Astronaut, GPL-3.0-or-later). The
   takeover is marker-gated, runs before `scfg_init()`, and only keeps WRAM-C
   with the ARM7 when the ARM7 ready marker appears. The ARM7 writes a ready
   marker at `0x0237FFF4`; the old IPCSYNC path remains as
   `make ARM7_HANDOFF=1`.
   A melonDS run shows the slide arms but the firmware ARM7 never reaches our
   entry: the firmware's reset code uses stacks at `0x03FFFFC0` /
   `0x03FFFF80` / `0x03FFFB7C`, not the Astronaut window, and mapping
   WRAM-A/B/C at the top of the NWRAM range did not hit it. The ARM7 IRQ jump
   vector at `0x03FFFFFC` (written by the reference `FUN_00020b58`) is equally
   unreachable: it reads zero through every NWRAM window. Both are ARM7-private
   aliases, so the stack slide and the IRQ-vector hijack are blocked from the
   ARM9. Next: a writable target in main RAM or a firmware IPC handler, an IPC
   command that restarts the ARM7, or a real title install so stage 2 starts
   both CPUs. See `docs/stage2_re.md`.

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
  `ENABLE_NAND_WRITE_DANGER` (docs §24). Covered by the extended
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
* **DLDI scanner / in-RAM patcher + `[9] Inspect BOOT.CFG`** —
  `src/common/dldi.c` scans a staged ARM9 segment for the canonical Chishm DLDI
  signature (`0xBF8DA5ED` + `" Chishm\0"`), validates the header (1 KiB..64 KiB
  allocation bounds) and can replace a default `"NONE"` stub in RAM with the
  built-in clean-room read-only `"DSiR"` driver, relocating the address table
  (`FIX_ALL`/`FIX_GLUE`/`FIX_GOT`) and zeroing BSS (`FIX_BSS`).
  `loader_stage_and_boot()` auto-patches and reports the result in
  `loader_result_t`; `[6]`'s `Y`/`SELECT` verify shows `DLDI : YES (…)` /
  `NONE`. `payload/arm9_payload.s` now carries a 1 KiB `"NONE"` stub so the SD
  image exercises the whole path. Menu item `[9] Inspect BOOT.CFG` renders the
  config source (`SD:/BOOT.CFG` vs `DEFAULTS`), every binding with a live
  SD target check (`OK`/`MISSING`) and the locked `B`/`SELECT` override.
  Covered by `tests/test_dldi.c` (suite 10), `tests/test_sd_image.c` and
  `tests/test_menu.c` (docs §29).
* **NDS banner decoder + reproducible release bundle** —
  `nds_header_t` now carries `banner_offset` (0x068) and
  `nds_banner_parse()` (`src/common/nds_header.c`) validates the banner version
  (`0x0001`/`0x0002`/`0x0003`/`0x0103`), recomputes the CRC-16/MODBUS over
  `0x020..0x83F` and decodes the UTF-16LE title at `0x240` into a single-line
  ASCII summary (`\n` → `" / "`, non-ASCII → `'?'`). `loader_read_banner()`
  streams the 0x840-byte block through the delegated FAT reader and `[6]`'s
  verify view renders `BANNER: <title>` / `BAD CRC` / `NONE`.
  `scripts/pack_nds.py` emits a real v1 banner at `0x840` (`--banner-title`)
  and `scripts/make_sd_image.py` retitles it for the `BOOT.NDS`/`SUBAPP.NDS`
  variants. `make release` (`scripts/make_release.py`) builds the diagnostic,
  live-chainload and autoboot variants in isolated build directories, stages
  `build/release/` with a `BOOT.CFG` template, `NDS/HELLO.NDS` and
  `sdcard.img`, and writes/verifies a deterministic `SHA256SUMS.txt` plus a
  deterministic `build/openlaunch_release.tar.gz`. Covered by
  `tests/test_loader.c`, `tests/test_sd_image.c`, `scripts/verify_rom.py` and
  `scripts/verify_sd_image.py` (docs §30).
* **OpenLaunch rebrand + release hardening** — the ROM title/banner defaults,
  primary artifact (`build/openlaunch.nds`, with a legacy
  `build/dsi_bootloader.nds` symlink), release bundle
  (`openlaunch_diag.nds` / `openlaunch.nds` / `BOOT.NDS` /
  `build/openlaunch_release.tar.gz`) and UI headers now use the **OpenLaunch**
  identity. `scripts/verify_rom.py` enforces the ARM9 `< 64 KiB` / ARM7
  `< 32 KiB` binary size budgets and asserts the OpenLaunch title/banner;
  `scripts/configure_emulator.py` provisions the native melonDS config (XDG,
  TOML or legacy INI) with `build/sdcard.img` so `make run` needs no manual GUI
  step; `scripts/smoke_test_emu.py` (`make smoke-test`) boots the
  ROM in melonDS under an isolated config, confirms the SD card attached and
  skips cleanly when melonDS or a display is absent; and
  `.github/workflows/ci.yml` runs the host tests, builds/verifies all variants,
  gates on byte-for-byte release reproducibility and publishes tagged releases.
* **Graphical font / banner icon + NAND redirect** — `src/arm9/ui_font.c`
  generates a 16x16 double-size font at runtime (no extra ROM table) and decodes
  the 0x020/0x220 NDS banner icon (4bpp and 8bpp) onto Engine A BG0; the SD
  browser's verify view renders the icon and double-size title over the
  telemetry. `src/common/nand.c` parses the DSi NAND PC-style MBR and the
  HiyaCFW TWL `{offset,size}` table (read-only, ARM7 IPC delegation via
  `IPC_STORAGE_DEV_EMMC`), `boot_resolve_nand_target()` implements the
  SD-absent fallback to the DSi System Menu (`00030004`/`484e4c41`), and the top
  screen reports `NAND : DETECTED`/`NAND : ABSENT`. Covered by
  `tests/test_ui_font.c` and `tests/test_nand.c` (docs §31–§32).
* **Boot theme engine + SD splash** — `BOOT.CFG` now also carries the optional
  `THEME_ACCENT` / `THEME_TEXT` / `THEME_BG` colours (RGB888, converted to
  BGR555; AMOLED defaults `0x00FF88` / `0xFFFFFF` / `0x000000`) and a
  `SPLASH_IMAGE` path, parsed by the same `boot_cfg_parse()` pass. `src/arm9/
  theme.c` applies the accent to the console palette banks (headers, selection,
  text, backdrop) and streams a raw 256x192 15-bit BGR555 `SPLASH.RAW`
  (98304 bytes) through the delegated FAT reader into a fixed 96 KiB staging
  buffer, blitting it to Engine A BG2 (bank B, `0x06020000`) as a 16bpp
  extended-affine bitmap behind the BG0 text console (Engine A mode 5); a
  missing image falls back to the pure `THEME_BG` backdrop. `make sd-image`
  installs a deterministic gradient `SPLASH.RAW` + themed `BOOT.CFG`. Covered by
  `tests/test_theme.c` (suite 13), `tests/test_sd_image.c` and
  `scripts/verify_sd_image.py` (docs §33).
* **Safe NAND installer** — a guarded, sandboxed eMMC installation pipeline,
  default read-only. `src/common/sha256.c` adds freestanding SHA-256 +
  HMAC-SHA-256 (FIPS 180-4 / RFC 2104); `nand_validate_layout()` +
  `nand_region_is_protected()` (`src/common/nand.c`) verify the MBR/TWL
  partition table and permanently protect the boot prefix; `src/common/installer.c`
  implements the 96-byte signed token (`INSTALL.KEY`), the strict pre-flight
  cascade and the `IDLE → … → DONE/ABORTED` state machine; `src/common/fat_rw.c`
  is a bounded FAT16/FAT32 root-file writer used to drop the mandatory
  `NANDBACK.BIN` full-NAND backup on the SD card; and
  `src/arm9/installer_backup.c`/`installer_install.c` stream the
  backup → SHA-256 verify → guarded flash → byte-for-byte read-back. The write
  glue (ARM7 `IPC_CMD_WRITE_SECTOR` + `IPC_CMD_NAND_GUARD`, ARM9
  `arm9_storage_write_sectors`) is compiled **only** under
  `ENABLE_NAND_WRITE_DANGER` (`make nand-installer`); the default build reports
  `GUARD: NAND WRITE LOCKED` and `verify_rom.py` still asserts no write symbols
  are linked. Menu item `[10] Install to NAND` now drives the whole flow through
  a modal progress screen (`src/arm9/installer_ui.c`) fed by the installer's
  optional progress hook — the six stages `PREFLIGHT → BACKUP →
  VERIFY_BACKUP → UNLOCK → WRITE → VERIFY_WRITE`, live backup/write percentages
  and a `n/total` sector counter — and renders a dedicated success or
  high-visibility abort screen that names the exact failed stage
  (`installer_report_t.failed_state`) and points at the mandatory
  `SD:/NANDBACK.BIN` recovery image. `scripts/make_installer_token.py` produces
  and self-verifies the signed package. Covered by `tests/test_installer.c`
  (suite 14) and `tests/test_installer_ui.c` (suite 15), and docs §34.
* **DSi boot-stage-2 exploit + `HNAA` payload** — the launcher-`title.tmd`
  overflow is reversed end to end (`docs/stage2_re.md`): stage 2's oversized TMD
  overruns the fixed buffer `0x037DF06C` and clobbers its ARM9 **IPC handler
  table** at `0x037F3B68` (dispatch `0x037CA720`). `scripts/make_openlaunch_payload.py`
  builds a Unlaunch-format container that arms that table (plus two callback
  slots) with a pointer to an in-place bootstrap
  (`src/arm9/overflow_bootstrap.s`), which relocates the ARM9/ARM7 images and
  branches to the ARM9 entry (`0x02200800`); `tools/stage2_dataflow.py` is the
  static data-flow pass that located the transfer, and
  `scripts/verify_openlaunch_payload.py` checks the container (`make verify`).
  The overflow entry, the relocate/jump and the final ARM9 hand-off are verified
  in melonDS against a patched `HNAA` TMD. The ARM7 is taken over with a
  jumpslide (Astronaut-style) instead of being left on the system firmware; the
  takeover still needs an emulator/hardware run. See `docs/stage2_re.md`.
