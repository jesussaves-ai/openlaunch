# OpenLaunch — Nintendo DSi low-level bootloader (C99)

[OpenLaunch](https://github.com/jesussaves-ai/openlaunch) is a clean-room,
open-source C99 alternative to Unlaunch: a low-level bootloader, hardware
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
* **SD browser + chainloader** — a menu item that lists `.nds`/`.srl` files in
  the root and in the one-level `/NDS` subdirectory, validates their header
  CRC16 and load ranges, stages the payloads (including optional TWL
  `ARM9i`/`ARM7i` segments) in RAM and (optionally) hands off to the new image;
  the canonical `__system_argv` launch path (`sd:/<NAME>` or `sd:/NDS/<NAME>`)
  is published for the loaded homebrew.
* **Unlaunch-style autoboot** — a startup hotkey policy driven by an optional
  `BOOT.CFG` (**B**/**SELECT** always force the menu, otherwise `DEFAULT` / `A`
  / `X` / `Y` / `L` / `R` / `START` select the target and `BOOT.NDS` is probed)
  plus menu item `[8] Autoboot BOOT.CFG` and an optional `make AUTOBOOT=1`
  automatic-boot build.
* **DLDI compatibility** — a clean-room DLDI section scanner and in-RAM patcher:
  a staged ARM9 image whose default `"NONE"` stub is found is patched with the
  built-in read-only driver before the handoff, with full address-table
  relocation (`FIX_ALL`/`FIX_GLUE`/`FIX_GOT`/`FIX_BSS`).
* **Inspectors** — menu item `[9] Inspect BOOT.CFG` shows the config source
  (`SD:/BOOT.CFG` or `DEFAULTS`), every hotkey binding with a live SD target
  check (`OK`/`MISSING`) and the locked `B`/`SELECT` override; item `[6]`'s
  verify view reports `DLDI : YES (…)`/`NONE` and the decoded
  `BANNER: <title>` (or `BAD CRC` / `NONE`).
* **NDS banner decoder** — the 0x840-byte banner referenced by header offset
  `0x068` is CRC-16/MODBUS verified and its UTF-16LE English title decoded into
  a clean single-line ASCII summary for the file inspector.
* **Graphical bitmap UI** — a zero-allocation 16x16 double-size font (generated
  at runtime from the 8x8 font, so no extra font table is linked) and a 32x32
  NDS banner-icon decoder (4bpp and 8bpp) render the highlighted `.nds`
  thumbnail and title on the top screen while browsing.
* **NAND / HiyaCFW redirect** — a read-only DSi NAND partition-table parser
  (PC-style MBR and HiyaCFW TWL `{offset,size}`), reached through the
  ARM7-gated eMMC IPC, plus a boot policy that falls back to the DSi System
  Menu (`00030004`/`484e4c41`) when no SD boot target is present.
* **Reproducible release** — `make release` builds the diagnostic,
  live-chainload and autoboot variants (plus a ready-to-copy SD card template)
  into `build/release/` with a deterministic `SHA256SUMS.txt` and
  `build/openlaunch_release.tar.gz`.
* **Deterministic test image** — `make sd-image` builds a FAT16 SD card with
  real, loadable and negative-test payloads (including `NDS/SUBAPP.NDS` and a
  root `BOOT.CFG`) for emulator and CI verification.

No eMMC/NAND write path exists outside the `ENABLE_NAND_WRITE_DANGER` build
(AGENTS.md §5); the storage pipeline is read-only by default.

## Requirements

* BlocksDS / devkitARM (`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`,
  `arm-none-eabi-nm`). Override with `make CROSS=... DEVKITARM=...`.
* Python 3 (stdlib only) for the packaging/verification scripts.
* A C99 host compiler (`cc`) for `make host-test`.

## Building

```sh
make            # build build/openlaunch.nds (+ the legacy dsi_bootloader.nds alias)
make rom        # alias for the above
make sd-image   # build build/hello_payload.nds + build/sdcard.img
make verify     # validate the ROM, the payload and the SD image
make host-test  # host unit/integration tests
make release    # build build/release/ + build/openlaunch_release.tar.gz
make smoke-test # isolated melonDS startup smoke test (skips if unavailable)
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

## The release bundle

`make release` builds every production variant in isolation and stages
`build/release/`:

| Artifact | Build / purpose |
| :--- | :--- |
| `openlaunch_diag.nds` | default: safe dry-run / diagnostic |
| `openlaunch.nds` | `CHAINLOAD_BOOT=1`: interactive menu + live chainload (`A` / `[8]`) |
| `BOOT.NDS` | `CHAINLOAD_BOOT=1 AUTOBOOT=1`: drop-in autoboot bootloader (B/SELECT forces the menu) |
| `BOOT.CFG` | sample SD-root hotkey map (`DEFAULT=NDS/HELLO.NDS`) |
| `NDS/HELLO.NDS` | the standalone chainload test payload |
| `sdcard.img` | the pre-built 16 MiB FAT16 image for melonDS testing |
| `SHA256SUMS.txt` | deterministic SHA-256 of every artifact above |

The script writes `SHA256SUMS.txt`, re-reads and re-hashes every entry to verify
it, and then produces a deterministic `build/openlaunch_release.tar.gz`. The
`BOOT.CFG` sample never points at `BOOT.NDS` itself (that is the bootloader), so
the drop-in build cannot recursively boot itself.

## The chainload SD image

`make sd-image` produces a deterministic 16 MiB MBR + FAT16 image:

| File          | Purpose |
| :------------ | :------ |
| `HELLO.NDS`   | the loadable chainload payload (title `CHAINLOAD OK`, code `TSTL`), fragmented across 16 clusters |
| `BOOT.NDS`    | the autoboot variant of the payload (title `AUTOBOOT OK`, code `TSTL`) |
| `NDS/SUBAPP.NDS` | a loadable payload inside the one-level `/NDS` subdirectory (title `SUBDIR PAYLOAD`) |
| `BOOT.CFG`    | the startup hotkey map (`DEFAULT=BOOT.NDS`, `A=HELLO.NDS`, `X=NDS/SUBAPP.NDS`) |
| `BADCRC.NDS`  | rejected with `NDS_ERR_BAD_CRC` |
| `OVERLAP.NDS` | rejected with `NDS_ERR_OVERLAP_BOOTLOADER` |
| `README.TXT`  | hidden by the `.nds`/`.srl` browser filter |

Every loadable payload (`HELLO.NDS`, `BOOT.NDS`, `NDS/SUBAPP.NDS`) carries the
canonical unpatched `"NONE"` DLDI stub in its ARM9 segment, so the bootloader
detects and patches it in RAM with its built-in read-only driver while staging.

### Testing it in melonDS

1. Build: `make rom sd-image` (or `make CHAINLOAD_BOOT=1 rom sd-image`).
2. **Config → Emu settings → Console type:** **DSi**.
3. **Config → Emu settings → DSi-mode → DSi SD card:** select
   `build/sdcard.img` and enable the SD card.
4. Provide the DSi BIOS/NAND (or use melonDS' direct-boot path).
5. In the ROM's bottom-screen menu choose `[6] Browse SD Card (.nds)`, select
   `HELLO.NDS` (or `NDS/SUBAPP.NDS`), press `Y` to verify (`LOADABLE: YES`) and
   `A` to stage.
   * Without `ENABLE_CHAINLOAD_BOOT` the UI reports `STAGE: DRY-RUN OK`.
   * With it, the payload takes over: **top screen green, bottom screen blue**.

Selecting `BADCRC.NDS` or `OVERLAP.NDS` shows the specific rejection reason
instead of staging.

At startup the top screen's `BOOT :` line reflects the `BOOT.CFG` resolution
(`BOOT : BOOT.NDS [CFG]`), and holding a mapped button (e.g. `X`) selects that
target (`BOOT : NDS/SUBAPP.NDS [X]`). `B` or `SELECT` always force the menu.

## Verification

* `make verify` runs `scripts/verify_rom.py` (CRC16s, ROM size/device capacity,
  the OpenLaunch title/banner, the ARM9 `< 64 KiB` / ARM7 `< 32 KiB` binary size
  budgets, ELF entry/load cross-checks, unit code, no NAND-write symbols, and
  the `0x068` banner version/CRC16/title) on `build/openlaunch.nds` and
  `build/hello_payload.nds`, then `scripts/verify_sd_image.py` on
  `build/sdcard.img` (which also re-checks the banner of `HELLO.NDS`,
  `BOOT.NDS` and `NDS/SUBAPP.NDS`).
* `make release` builds `build/release/` and verifies its `SHA256SUMS.txt`
  before producing the `.tar.gz` bundle.
* `make smoke-test` boots `build/openlaunch.nds` in melonDS (with
  `build/sdcard.img` as the emulated DSi SD card) under an isolated temporary
  config, and skips cleanly when melonDS or a display is unavailable.
* `make host-test` builds and runs the host suites, including
  `tests/test_sd_image.c`, which mounts `build/sdcard.img` through
  `fat_mbr_find_fat_partition()` → `fat_mount()` → the root iterator →
  `loader_validate_file()` → `loader_stage_and_boot()` and asserts the four
  outcomes above.

See [`tests/README.md`](tests/README.md) for the full test matrix and
[`docs/hardware_notes.md`](docs/hardware_notes.md) §26 for the SD image format.
