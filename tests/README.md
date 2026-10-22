# tests/

Layers of verification.

## 1. Host unit tests — shared primitives

`tests/test_mem.c` covers `dsi_mem{set,cpy,cmp}`, `dsi_strlen`, the
little-endian accessors and the IPC helpers: packing/channel, reply headers,
the parameterised request header, and the shared-Main-RAM window validator used
by the ARM7-gated storage/SPI delegation. Built as `build/host_test_mem`.

## 2. Host unit tests — ARM9 logic

`tests/test_arm9.c` covers the bounded LZ77/RLE decoders
(`src/common/lz77.c`), the stage-1 segment loader (`arm9_load_segments`), the
ARM946E-S MPU descriptor decoders and the VRAM bank packer. Built as
`build/host_test_arm9` (the CP15/register code in the ARM9 modules compiles out
on non-ARM hosts, so only the pure logic is exercised).

## 3. Host unit tests — console

`tests/test_console.c` covers the 1bpp→4bpp glyph/tile packer, the BG map-entry
packer, the font table bounds, and the bounded `console_format` /
`console_format_hex32` formatters (widths, zero-padding, truncation). Built as
`build/host_test_console`.

## 4. Host unit tests — menu + input

`tests/test_menu.c` covers the menu state machine (wrapping, action mapping,
degenerate inputs) and the keypad edge detector (`input_normalize_*`,
`input_edge_*`). Built as `build/host_test_menu`.

## 5. Host unit tests — TMIO SD/MMC engine

`tests/test_sdmmc.c` compiles the ARM7 driver with `-DSDMMC_IO_HOOKS`, routing
every register access to a mock controller. It covers the command serializer,
the FIFO block reader, byte vs block addressing, the bounded timeout path, the
controller-error path, partial-block rejection, the pure command/status/OCR
decoders (`tmio_cmd_word`, `tmio_status_*`, `tmio_ocr_*`) and the full
identification sequence for both SDHC (block) and SDSC (byte) cards. Built as
`build/host_test_sdmmc`.

## 6. Host unit tests — FAT16/FAT32 + NDS header

`tests/test_fat_ro.c` synthesizes complete FAT16 and FAT32 volumes (MBR, BPB,
FAT tables, root directory, data clusters) and exercises `fat_mount()`, the
volume geometry, `fat_get_next_cluster()`, root-directory listing and the
`.nds`/`.srl` filter, single- and multi-cluster reads, truncated chains, mount
failures, the pure MBR partition scan (`fat_mbr_find_fat_partition()`), FAT12
and malformed-BPB rejection, and the NDS header unit code + CRC16
(`src/common/nds_header.c`, including the CRC-16/MODBUS known-answer vector
`0x4B37`). Built as `build/host_test_fat`.

## 6b. Host unit tests — SCFG / MBK decoders

`tests/test_scfg.c` covers the hardware-independent parts of
`src/arm9/scfg.c`: the `SCFG_A9ROM` DSi/NDS mode decode, the `SCFG_CLK` ARM9
clock decode (67.03/134.06 MHz), the `SCFG_EXT` Main-RAM-window decode, the
`mbk_slot_entry()` NWRAM slot packer, the MBK default slot maps and the `MBK9`
write-protect field layout. Built as `build/host_test_scfg`.

## 6c. Host unit tests — chainloader

`tests/test_loader.c` covers the safe read-only `.nds`/`.srl` loader:

* `nds_header_validate_loadable()` (`src/common/nds_header.c`) against valid
  homebrew headers, bad CRC16, ARM9/ARM7 out-of-bounds and unaligned ranges,
  bootloader-reservation overlap, ARM7-WRAM targets and truncated files, plus
  the platform default bounds (`nds_load_bounds_default()`).
* `fat_read_file_range()` (`src/common/fat_ro.c`) across fragmented FAT16 and
  FAT32 cluster chains, byte offsets and EOF clamping, and the corrupt-chain
  guards: self-loop / two-cluster ping-pong (`FAT_CHAIN_ERR_LOOP`), bad/free
  clusters (`FAT_CHAIN_ERR_BAD_CLUSTER`), premature EOC
  (`FAT_CHAIN_ERR_TRUNCATED`) and a failed sector read
  (`FAT_CHAIN_ERR_IO`).
* `loader_validate_file()` end to end over a synthetic volume.

Built as `build/host_test_loader` (`src/arm9/loader.c` compiles on the host with
the hardware CPU handoff compiled out).

## 6d. Host integration test — chainload SD image

`tests/test_sd_image.c` mounts the deterministic image built by
`scripts/make_sd_image.py` (`build/sdcard.img`) and drives the real production
pipeline:

```
fat_mbr_find_fat_partition() -> fat_mount() -> fat_dir_iter_*()
    -> loader_validate_file() -> loader_stage_and_boot() (dry run)
```

It asserts that `HELLO.NDS` is listed, validates as `NDS_LOAD_OK` and stages
cleanly (both payloads copied and exactly one `IPC_CMD_BOOT_HANDOFF` request),
that `BADCRC.NDS` is rejected with `NDS_ERR_BAD_CRC`, that `OVERLAP.NDS` is
rejected with `NDS_ERR_OVERLAP_BOOTLOADER`, and that `README.TXT` is hidden by
the `.nds`/`.srl` filter. It also streams the whole multi-cluster `HELLO.NDS`
back and compares the staged payloads against their ROM offsets. Built as
`build/host_test_sd_image` with `-DLOADER_HOST_TEST`, which redirects the
absolute-address staging writes into a scratch arena (the target build is
unaffected).

`make host-test` builds `build/sdcard.img` first, so this suite always runs
against the real generated artefact.

Run all of them with `make host-test`.

## 7. ROM packaging + static verification

```
make              # link build/arm{9,7}.elf -> .bin -> build/dsi_bootloader.nds
make sd-image     # build build/hello_payload.nds + build/sdcard.img
make verify       # validate the packaged ROM, the payload and the SD image
```

`make verify` runs `scripts/verify_rom.py` (on both `build/dsi_bootloader.nds`
and `build/hello_payload.nds`), which asserts:

* the Nintendo-logo CRC16 (`0x15C`) and header CRC16 (`0x15E`) recompute;
* ARM9/ARM7 ROM offsets, sizes and RAM load addresses are consistent and match
  the ELF symbol tables (`_start`, `__rom_start__` via `arm-none-eabi-nm`);
* the ROM is a power of two of at least 32 KiB, the `0x014` device capacity
  covers it, the `0x080` total-used size is consistent and the padding is `0xFF`;
* the `0x012` unit code is a valid value and, when DSi-capable, is the
  NDS+DSi hybrid `0x02` with a non-zero TWL region mask;
* no eMMC/NAND write symbols (AGENTS.md §5) are present in the linker maps.

It then runs `scripts/verify_sd_image.py`, which independently re-checks the
generated `build/sdcard.img`: the MBR signature and FAT16 partition entry, the
BPB geometry, the root-directory entries, the fragmented multi-cluster
`HELLO.NDS` chain, and the two negative-test headers (`BADCRC.NDS` bad CRC16,
`OVERLAP.NDS` ARM9 range inside the bootloader reservation).

## 8. melonDS smoke test

```
make run                          # or: ./scripts/run_melonds.sh
```

`scripts/run_melonds.sh` finds a native `melonds`/`melonDS` or the Flatpak and
loads `build/dsi_bootloader.nds`. The ROM now renders a text console:

* **Top screen** — the header `OPENUNLAUNCH C99 BOOTLOADER` and a live telemetry
  table (CPU mode, DTCM base, MPU control, ARM7 IPC link, SD/MMC status, RTC
  readout, firmware ID, NAND write-protection guard, LZ77 self-test).
* **Bottom screen** — a D-Pad navigable menu (`A` runs, `B` re-runs the
  self-test):

  | Item | Action                                              |
  | :--- | :-------------------------------------------------- |
  | `[1]`| Re-run the IPC/hardware self-test                    |
  | `[2]`| Dump SCFG/MBK registers (B returns)                  |
  | `[3]`| Probe SD/TMIO read-only status                       |
  | `[4]`| Decompress the embedded LZ77 vector                  |
  | `[5]`| Halt the CPU safely                                  |
  | `[6]`| Browse the SD card's FAT volume, verify (`Y`/`SELECT`) and chainload (`A`) a `.nds` image |
  | `[7]`| Unlock and initialise SCFG / NWRAM (MBK1-9)          |

  Inside `[6]`, the top screen shows the mounted FAT type and, after a
  verify/dry-run, the NDS title, game code, unit code, ARM9/ARM7
  entry/load/size and `LOADABLE: YES` (or the specific rejection reason);
  `A` stages the image and reports `STAGE: DRY-RUN OK` unless the ROM was built
  with `ENABLE_CHAINLOAD_BOOT`.

### Recommended melonDS configuration

The image is an **unsigned homebrew TWL/DSi-hybrid** ROM (unit code `0x02`:
NDS + DSi). Two configurations work; the second unlocks `SCFG`/extended TWL
hardware.

**A. DS mode / direct boot (simplest; enough for the IPC smoke test)**

* *Config → Emu settings → Console type:* **DS**.
* Leave *BIOS/firmware* empty to use melonDS' built-in FreeBIOS (direct boot),
  or point at a DS `bios9.bin`/`bios7.bin`/`firmware.bin` set.
* The ROM is unencrypted, so no KEY1/title signature is required.

**B. DSi (TWL) mode (for `SCFG_EXT`, 16 MiB RAM, TWL timers)**

* *Config → Emu settings → Console type:* **DSi**.
* Provide a matching NAND + DSi firmware (`dsi_firmware.bin`) and the DSi BIOS
  images (`bios9i.bin`, `bios7i.bin`, and `bios9.bin`/`bios7.bin`).
* The DSi firmware normally verifies title signatures; because this is an
  unsigned homebrew image, use the "boot directly" path if the firmware title
  check rejects it. The ARM9 image keeps a `0x800`-byte secure-area prefix
  (entry `0x02200800`) precisely so a failed secure-area decrypt cannot touch
  the real code.

### Why the ARM9 entry is `0x02200800`

`ld/arm9.ld` reserves the first `0x800` bytes of the ARM9 region
(`.secure_area`) and places `.crt0`/`_start` at `0x02200800`, matching
`asm/UNLAUNCH.DSI`. The DS/DSi BIOS (and melonDS' direct-boot loader) always
treats those `0x800` bytes as the secure area and may overwrite them; keeping
the code after the prefix makes the image survive that.

CI should run at minimum `make check`, `make verify` and `make host-test`.
