# OPENUNLAUNCH — Nintendo DSi low-level bootloader (C99)

A clean-room, open-source C99 implementation of a low-level bootloader, hardware
initializer and homebrew menu for the **Nintendo DSi (TWL)**, targeting the
dual-core ARM946E-S (ARM9) and ARM7TDMI (ARM7).

The project translates documented hardware behaviour and the reference logic
disassembled from `asm/UNLAUNCH.DSI` into readable, strictly typed C99. See
[`AGENTS.md`](AGENTS.md) for the architectural rules and
[`docs/hardware_notes.md`](docs/hardware_notes.md) for the register-level notes.

## Features

* **Dual-CPU bring-up** — ARM9/ARM7 crt0, vectors, MPU/TCM, SCFG clock and
  NWRAM (`MBK1`–`MBK9`) configuration, IPC FIFO handshake.
* **Read-only storage** — an ARM7 TMIO SD/MMC driver (CMD0/CMD8/ACMD41/CMD2/
  CMD3/CMD9/CMD7/CMD16/CMD17) reached from the ARM9 only through IPC, plus a
  zero-allocation FAT16/FAT32 parser.
* **SD browser + chainloader** — a menu item that lists `.nds`/`.srl` files,
  validates their header CRC16 and load ranges, stages the payloads (including
  optional TWL `ARM9i`/`ARM7i` segments) in RAM and (optionally) hands off to
  the new image; the canonical `__system_argv` launch path (`sd:/<NAME>`) is
  published for the loaded homebrew.
* **Unlaunch-style autoboot** — a startup hotkey policy (**B**/**SELECT** force
  the menu, otherwise `BOOT.NDS` is probed) plus menu item `[8] Autoboot
  BOOT.NDS` and an optional `make AUTOBOOT=1` automatic-boot build.
* **Deterministic test image** — `make sd-image` builds a FAT16 SD card with
  real, loadable and negative-test payloads for emulator and CI verification.

No eMMC/NAND write path exists outside the `ENABLE_NAND_WRITE_DANGER` build
(AGENTS.md §5); the storage pipeline is read-only by default.

## Requirements

* BlocksDS / devkitARM (`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`,
  `arm-none-eabi-nm`). Override with `make CROSS=... DEVKITARM=...`.
* Python 3 (stdlib only) for the packaging/verification scripts.
* A C99 host compiler (`cc`) for `make host-test`.

## Building

```sh
make            # build build/dsi_bootloader.nds
make rom        # alias for the above
make sd-image   # build build/hello_payload.nds + build/sdcard.img
make verify     # validate the ROM, the payload and the SD image
make host-test  # host unit/integration tests
make size       # arm-none-eabi-size for both ELFs
make clean
```

To arm the real dual-CPU chainload branch (off by default):

```sh
make clean
make CHAINLOAD_BOOT=1 rom sd-image
```

## Repository layout

```text
asm/          reference disassembly and extracted binaries (read-only)
docs/         hardware register notes and memory map
include/      shared / ARM9 / ARM7 headers
src/          common, ARM9 and ARM7 C99 + crt0 assembly
payload/      sources for the chainload test payload
scripts/      packaging, SD image builder and verification tools
tests/        host test suites and melonDS notes
ld/           linker scripts for the bootloader
```

## The chainload SD image

`make sd-image` produces a deterministic 16 MiB MBR + FAT16 image:

| File          | Purpose |
| :------------ | :------ |
| `HELLO.NDS`   | the loadable chainload payload (title `CHAINLOAD OK`, code `TSTL`), fragmented across 16 clusters |
| `BOOT.NDS`    | the autoboot variant of the payload (title `AUTOBOOT OK`, code `TSTL`) |
| `BADCRC.NDS`  | rejected with `NDS_ERR_BAD_CRC` |
| `OVERLAP.NDS` | rejected with `NDS_ERR_OVERLAP_BOOTLOADER` |
| `README.TXT`  | hidden by the `.nds`/`.srl` browser filter |

### Testing it in melonDS

1. Build: `make rom sd-image` (or `make CHAINLOAD_BOOT=1 rom sd-image`).
2. **Config → Emu settings → Console type:** **DSi**.
3. **Config → Emu settings → DSi-mode → DSi SD card:** select
   `build/sdcard.img` and enable the SD card.
4. Provide the DSi BIOS/NAND (or use melonDS' direct-boot path).
5. In the ROM's bottom-screen menu choose `[6] Browse SD Card (.nds)`, select
   `HELLO.NDS`, press `Y` to verify (`LOADABLE: YES`) and `A` to stage.
   * Without `ENABLE_CHAINLOAD_BOOT` the UI reports `STAGE: DRY-RUN OK`.
   * With it, the payload takes over: **top screen green, bottom screen blue**.

Selecting `BADCRC.NDS` or `OVERLAP.NDS` shows the specific rejection reason
instead of staging.

## Verification

* `make verify` runs `scripts/verify_rom.py` (CRC16s, ROM size/device capacity,
  ELF entry/load cross-checks, unit code, no NAND-write symbols) on both
  `build/dsi_bootloader.nds` and `build/hello_payload.nds`, then
  `scripts/verify_sd_image.py` on `build/sdcard.img`.
* `make host-test` builds and runs the host suites, including
  `tests/test_sd_image.c`, which mounts `build/sdcard.img` through
  `fat_mbr_find_fat_partition()` → `fat_mount()` → the root iterator →
  `loader_validate_file()` → `loader_stage_and_boot()` and asserts the four
  outcomes above.

See [`tests/README.md`](tests/README.md) for the full test matrix and
[`docs/hardware_notes.md`](docs/hardware_notes.md) §26 for the SD image format.
