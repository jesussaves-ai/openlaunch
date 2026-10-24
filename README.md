# OpenLaunch — Nintendo DSi low-level bootloader (C99)

[OpenLaunch](https://github.com/jesussaves-ai/openlaunch) is a clean-room,
open-source C99 alternative to Unlaunch: a low-level bootloader, hardware
initializer and homebrew menu for the **Nintendo DSi (TWL)**, targeting the
dual-core ARM946E-S (ARM9) and ARM7TDMI (ARM7).

The project translates documented hardware behaviour and the reference logic
disassembled from `asm/UNLAUNCH.DSI` into readable, strictly typed C99. See
[`docs/hardware_notes.md`](docs/hardware_notes.md) for the register-level notes
and [`PLAN.md`](PLAN.md) for the translation roadmap.

OpenLaunch is free software, licensed under the **GNU General Public License,
version 3 or later**. See [`LICENSE`](LICENSE) for the full text and
[`NOTICE`](NOTICE) for third-party and reference-material attribution.

> Development note: this codebase is developed with heavy AI assistance — the
> hardware reverse-engineering, architecture and review are directed by the
> maintainer, and the C99/assembly is largely machine-written and verified by the
> test suites in `tests/` and `make verify`.

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
* **Guarded NAND installer (opt-in, SANDBOX ONLY)** — menu item
  `[10] Install to NAND` drives a signed, sandboxed staging pipeline:
  pre-flight MBR/TWL layout validation with a permanently protected boot
  prefix, a **mandatory** raw NAND backup (`NANDBACK.BIN`, SHA-256 verified
  against live NAND **and by reading the file back from the SD card**), an
  HMAC-SHA-256 authenticated installer token (`INSTALL.KEY`), then a guarded
  eMMC write with byte-for-byte read-back verification and unconditional guard
  release. A modal progress screen walks the six stages
  (`PREFLIGHT → BACKUP → VERIFY_BACKUP → UNLOCK → WRITE → VERIFY_WRITE`) with
  live percentages; an abort paints a high-visibility failure screen naming the
  exact failed stage and the `SD:/NANDBACK.BIN` recovery image. The write glue
  is compiled **only** with `make NAND_WRITE_DANGER=1` (isolated tree:
  `make nand-installer`); the default build reports `GUARD: NAND WRITE LOCKED`
  and refuses `[10]` safely.
* **Retail-NAND refusal** — the installer classifies the eMMC image before
  touching it. A real DSi NAND is AES-CTR encrypted with a **per-console key**
  (derived from the CID + Console ID) and its boot chain is RSA/`.tmd`
  validated, so raw plaintext staging can never be valid there. Such an image
  (or a NAND carrying a `no$gba` / Unlaunch CID+CPU footer) is refused at
  PREFLIGHT with `retail encrypted NAND (AES/TMD required)`. Only a decrypted
  sandbox image (a plaintext MBR) can proceed. **Never run `[10]` on a retail
  console.**
* **Reproducible release** — `make release` builds the diagnostic,
  live-chainload and autoboot variants (plus a ready-to-copy SD card template)
  into `build/release/` with a deterministic `SHA256SUMS.txt` and
  `build/openlaunch_release.tar.gz`.
* **Deterministic test image** — `make sd-image` builds a FAT16 SD card with
  real, loadable and negative-test payloads (including `NDS/SUBAPP.NDS` and a
  root `BOOT.CFG`) for emulator and CI verification.

No eMMC/NAND write path exists outside the `ENABLE_NAND_WRITE_DANGER` build;
the storage pipeline is read-only by default. The opt-in
installer keeps that guarantee: even an armed build refuses to write until a
signed package authenticates *and* a full, SHA-256-verified NAND backup exists
*and* the target image is classified as a decrypted sandbox rather than a retail
encrypted NAND.

> **Retail consoles: do not use `[10]`.** The installer is an unencrypted
> sandbox staging harness, not a production NAND installer. On real hardware it
> refuses a retail (encrypted) NAND with `retail encrypted NAND (AES/TMD
> required)` and writes nothing. Real-install support requires per-console
> AES-CTR (`twln`) and `.tmd` exploit code that does not exist yet. To run
> OpenLaunch on a real Unlaunch DSi **safely, write nothing to NAND**, and
> chainload it from the SD card instead:
>
> 1. `make CHAINLOAD_BOOT=1 rom` (or just `make rom` for the dry-run diagnostics
>    build).
> 2. Copy `build/openlaunch.nds` to the SD card root as `BOOT.NDS`, **or** copy
>    it under any name and pick it from Unlaunch's file menu. Unlaunch loads it
>    from SD into RAM.
> 3. Boot and use the on-screen menu. `[10]` is safe there too: the default
>    build reports `GUARD: NAND WRITE LOCKED` and refuses it.
>
> Never build/run `make nand-installer` on a real console.

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
make openlaunch-payload # build the HNAA exploit container (see below)
make nand-tool  # build the offline NAND decrypt/encrypt tool
make verify     # validate the ROM, the payload, the SD image and the container
make host-test  # host unit/integration tests
make release    # build build/release/ + build/openlaunch_release.tar.gz
make configure-emulator # point the local melonDS config at build/sdcard.img
make smoke-test # isolated melonDS startup smoke test (skips if unavailable)
make size       # arm-none-eabi-size for both ELFs
make clean
```

To arm the real dual-CPU chainload branch (off by default):

```sh
make clean
make CHAINLOAD_BOOT=1 rom sd-image
```

To build the guarded NAND installer (DISABLED by default):

```sh
make nand-installer     # -> build/nand/openlaunch.nds
                        #    + build/nand/sdcard.img (clean)
                        #    + build/nand/sdcard_installer.img (sandbox fixtures)

# Separate WRITE-ENABLED test build (sandbox): arms the eMMC write path but
# still refuses retail encrypted NAND. Use this to exercise the six stages and
# their error paths against a decrypted sandbox NAND / emulator.
make installer-test     # -> build/installer-test/openlaunch.nds + fixtures

# Or build just the sandbox fixtures onto the default tree:
make sd-image-installer # -> build/sdcard_installer.img (INSTALL.KEY + OLBOOT.NDS)

# Standalone token (mark it --sandbox so the backup fits a small SD image):
python3 scripts/make_installer_token.py --payload <image> --target-lba 0x1000 \
        --sandbox --out INSTALL.KEY
```

`make nand-installer` builds in an isolated `build/nand/` tree so the default
`build/` keeps its read-only guarantee. The installer requires an authenticated
`INSTALL.KEY` on the SD root and writes a `NANDBACK.BIN` backup (verified by
reading it back from the SD card) before any eMMC write. The emulated SD card
must be **writable**: `scripts/configure_emulator.py` now writes
`[DSi.SD] ReadOnly = false` by default (opt out with `--read-only`); melonDS
silently drops writes to a read-only card, which stage `3/6 VERIFY_BACKUP` will
now detect and abort. See docs/hardware_notes.md §34.

### Retail raw-write test build (DANGER)

```sh
make installer-retail   # -> build/installer-retail/openlaunch.nds + retail token
```

`make installer-retail` is the **only** build that can raw-write a real, retail
(encrypted) DSi eMMC. It is a last-resort diagnostic path, not an installer:
plaintext bytes written into the AES-CTR-encrypted filesystem will not produce a
working image and may corrupt the System Menu. To limit the damage to the
absolute minimum it (a) still requires an `INSTALL.KEY` signed with `--retail`,
(b) requires a runtime `L + R + START` hold on a red warning screen, (c) writes a
verified full-chip `NANDBACK.BIN` first, and (d) is hard-limited to the
**bootsafe window** `[0x877, 0x78000)` sectors (byte `0x10EE00`..`0xF000000`), so
stage1/stage2 and any existing Unlaunch install are never touched — the console
stays bootable and recoverable. It is **not** a substitute for a real Unlaunch
implementation; see the roadmap in docs/hardware_notes.md §34.11.

### Offline NAND tool (safe — never touches a console)

```sh
make nand-tool                    # -> build/tools/dsi_nand_tool

build/tools/dsi_nand_tool info    nand.bin          # key + MBR partition table
build/tools/dsi_nand_tool decrypt nand.bin dec.bin  # full-image decrypt
build/tools/dsi_nand_tool encrypt dec.bin nand.bin  # full-image encrypt
build/tools/dsi_nand_tool verify  nand.bin          # decrypt+re-encrypt==identity

python3 scripts/dsi_nand_install.py list dec.bin    # System Menu launchers + TMDs
python3 scripts/dsi_nand_install.py extract dec.bin 484E4141 hnaa_tmd.bin
python3 scripts/dsi_nand_install.py replace dec.bin dec_out.bin 484E4141 \
        --payload OLBOOT.BIN
```

The tool derives the per-console key from the `0xFF800` no$gba/Unlaunch footer
(or `--cid`/`--console`). The crypto is validated against a real retail dump;
see docs/hardware_notes.md §34.13.

## DSi boot-stage-2 exploit (the `HNAA` payload)

Boot stage 2 loads the launcher's `title.tmd` with a broken `filesize > filesize`
check, so an oversized TMD overruns a fixed Main-RAM buffer (`0x037DF06C`) and
overwrites stage-2's own IPC handler table (`0x037F3B68`, dispatched at
`0x037CA720`). OpenLaunch reproduces the Unlaunch/SafeUnlaunch exploit: an
OpenLaunch-format container is written into the **backup** launcher (`HNAA`) TMD;
its header is overlaid so that the clobbered IPC table plus a small in-place stub
(`src/arm9/overflow_bootstrap.s`) relocate the ARM9/ARM7 images and jump to the
ARM9 entry (`0x02200800`). The overflow entry and the relocate/jump handoff are
verified end-to-end in melonDS; the ARM7 jumpslide takeover is implemented, but
an initial melonDS run on the test NAND shows the firmware stack is not in
Astronaut's window, so the slide does not reach the ARM7 yet (see
`docs/stage2_re.md`).

```sh
make openlaunch-payload     # -> build/openlaunch_payload.bin (Unlaunch-format container)
python3 scripts/verify_openlaunch_payload.py --payload build/openlaunch_payload.bin
```

Reverse-engineering tooling: `tools/stage2_dataflow.py` (a Capstone-based
recursive-descent disassembler plus an abstract interpreter that finds control
transfers anchored in the overflow range); `docs/stage2_re.md` has the full
write-up of the exploit, the differential that pinned it down, and the offsets.

Installing the container into an offline NAND dump (never touches a console) —
`scripts/dsi_nand_install.py patch` does the full decrypt → rewrite → encrypt:

```sh
python3 scripts/dsi_nand_install.py patch nand.bin nand_patched.bin 484E4141 \
        --payload build/openlaunch_payload.bin
```

### Booting OpenLaunch with Unlaunch (verified, both CPUs)

The complete, verified launch path is Unlaunch's own filemenu: Unlaunch starts
the selected file as a DSi title, so the loader starts **both** the ARM9
(`0x02200800`) and the ARM7 (`0x02380000`) from our header.

```sh
make rom                                    # -> build/openlaunch.nds
cp build/openlaunch.nds /media/<SD>/bootcode.dsi
```

Then, in Unlaunch's filemenu, scroll to the bottom and select `bootcode.dsi`
(it is the last entry and is drawn just below the visible rows). The menu
self-test then reports `ARM7: OK (1)`.

Live melonDS trace: `ARM9 0x02200958` (`arm9_main`), `0x02201700`
(`console_init`), `0x02203D94` (`menu_run`), the ARM7 markers `JMPD` at
`0x0237FFE8` and `RADY` at `0x0237FFF4`, and boot info at `0x02FFE000` holding
our header (`"OPENLAUNCH"`, ARM9 `0x02200800`/`0xB33C`, ARM7
`0x02380000`/`0x117C`). No stage-2 handshake is involved. See
`docs/unlaunch_reference.md` for the full trace and why the launcher-TMD route
reaches only the ARM9.

With `make AUTOBOOT=1 CHAINLOAD_BOOT=1 rom`, the menu also autoboots
`sd:/boot.nds` through the dual-CPU chainloader. That path is verified
end-to-end: the staged test payload takes over the screen (green/blue), and
the ARM7 performs the staged copy + branch before the ARM9 jumps to the new
entry.

#### Hardware smoke test (zero risk)

This path only reads the SD card; it never writes NAND.

1. `make rom SAFE_HW=1` (hardware build: skips the not-yet-validated RTC/SD/
   firmware/NAND/EXTKEY probes, which currently wedge a real DSi). The default
   build keeps those probes for melonDS.
2. Copy `build/openlaunch.nds` to the SD card root as `bootcode.dsi`.
3. Boot the console and start `bootcode.dsi` from Unlaunch's file menu (last
   entry; scroll to the bottom) — or bind it to `NO BUTTON` in `OPTIONS`.
4. The top screen should show:

   ```
   SCFG : DSI/TWL 32MB UNLOCKED
   ARM7 : OK (1)
   SD   : PRESENT RW ...
   BOOT : NO BOOT.NDS
   ```

`ARM7 : OK (1)` is the dual-CPU proof (our ARM7 answers the menu's IPC
self-test). `NAND : ABSENT` is expected on an encrypted retail NAND.
If the ARM7 line is not `OK (1)`, that is the failure worth reporting — note
the surrounding lines and how the payload was started.

## Repository layout

```text
asm/          reference disassembly and extracted binaries (read-only)
docs/         hardware register notes and memory map
include/      shared / ARM9 / ARM7 headers
src/          common, ARM9 and ARM7 C99 + crt0 assembly
payload/      sources for the chainload test payload
scripts/      packaging, SD image builder and verification tools
tools/        reverse-engineering + host tools (stage-2 data-flow pass, NAND tool)
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
2. Run `make run` (or `./scripts/run_melonds.sh`). Before launching, the helper
   `scripts/configure_emulator.py` sets the emulated DSi SD card
   (`build/sdcard.img`) and the DSi console type in the local melonDS config
   automatically, so no manual GUI step is required. `make configure-emulator`
   performs only the configuration; `OPENLAUNCH_SKIP_CONFIG=1` opts out.
   *Manual equivalent:* **Config → Emu settings → Console type:** **DSi**, then
   **DSi-mode → DSi SD card:** select `build/sdcard.img` and enable it.
3. Provide the DSi BIOS/NAND (or use melonDS' direct-boot path).
4. In the ROM's bottom-screen menu choose `[6] Browse SD Card (.nds)`, select
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
* `make configure-emulator` (`scripts/configure_emulator.py`) points the native
  melonDS configuration — `~/.config/melonDS/` (honouring `XDG_CONFIG_HOME`), in
  either `melonDS.toml` (TOML) or legacy `melonDS.ini` form — at
  `build/sdcard.img`, enabling the DSi SD card and DSi console type so no manual
  GUI setup is needed.
* `make smoke-test` boots `build/openlaunch.nds` in melonDS (with
  `build/sdcard.img` as the emulated DSi SD card) under an isolated temporary
  config generated by the same helper, confirms from the emulator log that the
  SD image was attached and that no command was sent to a null device, and skips
  cleanly when melonDS or a display is unavailable.
* `make host-test` builds and runs the host suites, including
  `tests/test_sd_image.c`, which mounts `build/sdcard.img` through
  `fat_mbr_find_fat_partition()` → `fat_mount()` → the root iterator →
  `loader_validate_file()` → `loader_stage_and_boot()` and asserts the four
  outcomes above.

See [`tests/README.md`](tests/README.md) for the full test matrix and
[`docs/hardware_notes.md`](docs/hardware_notes.md) §26 for the SD image format.

## License

Copyright (C) 2026 The OpenLaunch Authors.

OpenLaunch is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but **WITHOUT
ANY WARRANTY**; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

Third-party components and reference material (including the Unlaunch/DSi
firmware notes under `asm/`) keep their own terms; see [`NOTICE`](NOTICE).
