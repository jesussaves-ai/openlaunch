# Makefile — OpenLaunch: Nintendo DSi low-level bootloader (ARM9 + ARM7)
#
# Toolchain: BlocksDS / devkitARM. Override CROSS to point at another
# arm-none-eabi toolchain, e.g.:
#
#     make CROSS=arm-none-eabi- DEVKITARM=/opt/devkitpro/devkitARM
#
# Language: strict C99 (-std=c99). GNU ARM assembly is used only for crt0 /
# vectors / CP15 (see the project conventions).
#
# Targets:
#   make            build the runnable ROM (build/openlaunch.nds)
#   make rom        alias for the above (also stages build/dsi_bootloader.nds)
#   make sd-image   build build/hello_payload.nds + build/sdcard.img
#   make check      strict cross-compile of every object (-Werror)
#   make verify     validate the packaged ROM (CRCs, offsets, size budgets)
#   make host-test  host unit tests for the hardware-independent logic
#   make release    build build/release/ (all variants + SD template + SHA256SUMS)
#   make smoke-test isolated melonDS startup smoke test (skips if unavailable)
#   make configure-emulator  point the local melonDS config at build/sdcard.img
#   make size       arm-none-eabi-size for both ELFs
#   make clean

CROSS      ?= arm-none-eabi-
CC         := $(CROSS)gcc
AS         := $(CROSS)gcc
OBJCOPY    ?= $(CROSS)objcopy
NM         ?= $(CROSS)nm
SIZE       ?= $(CROSS)size

# Toolchain include root (devkitARM layout). Only used to locate helper tools.
DEVKITARM  ?= /opt/devkitpro/devkitARM
export PATH := $(DEVKITARM)/../tools/bin:$(PATH)

BUILD      ?= build
LD_DIR     := ld
INCLUDES   := -Iinclude

ROM        := $(BUILD)/openlaunch.nds
# Legacy alias so existing scripts and muscle-memory commands keep working.
LEGACY_ROM := $(BUILD)/dsi_bootloader.nds
ARM9_ELF   := $(BUILD)/arm9.elf
ARM7_ELF   := $(BUILD)/arm7.elf
ARM9_BIN   := $(BUILD)/arm9.bin
ARM7_BIN   := $(BUILD)/arm7.bin

# ---- Chainload test payload + deterministic SD image ---------------------
HELLO_ROM        := $(BUILD)/hello_payload.nds
SD_IMAGE         := $(BUILD)/sdcard.img
SD_INSTALLER_IMAGE := $(BUILD)/sdcard_installer.img
PAYLOAD_ARM9_ELF := $(BUILD)/payload/arm9.elf
PAYLOAD_ARM7_ELF := $(BUILD)/payload/arm7.elf
PAYLOAD_ARM9_BIN := $(BUILD)/payload/arm9.bin
PAYLOAD_ARM7_BIN := $(BUILD)/payload/arm7.bin

# ---- Shared freestanding flags -------------------------------------------
CFLAGS_COMMON := -std=c99 -Wall -Wextra -Werror -Os -g \
                 -ffreestanding -fno-builtin -fno-common \
                 -fomit-frame-pointer -fno-strict-aliasing \
                 -ffunction-sections -fdata-sections \
                 $(INCLUDES)

# Chainload boot guard. By default the staged image is verified, staged and the
# IPC handoff prepared, but the CPU never branches into it (dry run). Build with
# `make CHAINLOAD_BOOT=1` to compile in the real dual-CPU handoff; `make clean`
# when toggling. See docs/hardware_notes.md §25.
ifeq ($(CHAINLOAD_BOOT),1)
CFLAGS_COMMON += -DENABLE_CHAINLOAD_BOOT
endif

# Unlaunch-style autoboot. By default the startup hotkey policy is resolved and
# BOOT.NDS is probed, but the loader only boots it from menu item [8]. Build
# with `make AUTOBOOT=1` to boot a discovered BOOT.NDS automatically.
ifeq ($(AUTOBOOT),1)
CFLAGS_COMMON += -DENABLE_AUTOBOOT
endif

# ARM7 hand-off for the boot-stage-2 overflow path. EXPERIMENTAL / not yet
# working: it publishes the boot header at 0x02FFFE00 and rings IPCSYNC, but the
# firmware ARM7 does not yet take the hand-off (see docs/stage2_re.md). Disabled
# by default because it overwrites the boot-info the running firmware may use.
ifeq ($(ARM7_HANDOFF),1)
CFLAGS_COMMON += -DENABLE_ARM7_HANDOFF
endif

# Astronaut-style NWRAM-C jumpslide (fill the firmware's stack window and let a
# return pop our entry). DISABLED by default: the live test NAND's stage-2 ARM7
# runs from NWRAM-C parts 0..7 with its window at 0x037B8000, and Astronaut's
# MBK5/MBK8 values remove the parts it is executing from, so the ARM7 takes an
# illegal-instruction abort instead of sliding. Build with `make ASTRO_SLIDE=1`
# only when testing a firmware whose layout matches Astronaut's.
ifeq ($(ASTRO_SLIDE),1)
CFLAGS_COMMON += -DENABLE_ASTRO_SLIDE
endif

# NAND installer write path. DISABLED by default: the eMMC is read-only
# (the project conventions, scripts/verify_rom.py). Build with `make NAND_WRITE_DANGER=1`
# to compile in the guarded backup + flash pipeline (menu item [10]).
ifeq ($(NAND_WRITE_DANGER),1)
CFLAGS_COMMON += -DENABLE_NAND_WRITE_DANGER
endif

# Opt-in RETAIL raw-write path. DANGER: this arms raw plaintext writes on a
# retail, AES-CTR encrypted eMMC. It cannot produce a valid install and may
# corrupt the System Menu; it is a last-resort diagnostic path only. It also
# requires NAND_WRITE_DANGER=1. Build with `make installer-retail`. See
# docs/hardware_notes.md 34.11.
ifeq ($(RETAIL_NAND_WRITE),1)
CFLAGS_COMMON += -DENABLE_RETAIL_NAND_WRITE
endif

# ---- Per-CPU flags -------------------------------------------------------
CFLAGS_ARM7 := $(CFLAGS_COMMON) -mcpu=arm7tdmi -marm -mfloat-abi=soft
CFLAGS_ARM9 := $(CFLAGS_COMMON) -mcpu=arm946e-s -marm -mfloat-abi=soft

ASFLAGS_ARM7 := $(CFLAGS_ARM7)
ASFLAGS_ARM9 := $(CFLAGS_ARM9)

# ---- Linker flags --------------------------------------------------------
# Bare-metal link: our own crt0 supplies _start, and the linker scripts place
# every section. -lgcc is linked for compiler intrinsics (__aeabi_*).
LDFLAGS_FREESTANDING := -nostartfiles -nostdlib \
                        -Wl,--gc-sections -Wl,--build-id=none \
                        -Wl,--no-warn-rwx-segments

# ---- Sources -------------------------------------------------------------
# Common code is compiled once per CPU because each core has its own cache
# regime and the objects are linked into separate images.
SRCS_ARM7 := $(wildcard src/arm7/*.c) $(wildcard src/common/*.c)
SRCS_ARM9 := $(wildcard src/arm9/*.c) $(wildcard src/common/*.c)
ASM_ARM7  := $(wildcard src/arm7/*.s)
# overflow_bootstrap.s is *not* part of the ARM9 image: it is assembled into a
# standalone raw blob embedded in the HNAA payload (see openlaunch-payload).
ASM_ARM9  := $(filter-out src/arm9/overflow_bootstrap.s,$(wildcard src/arm9/*.s))

OBJS_ARM7 := $(patsubst %.c,$(BUILD)/arm7/%.o,$(SRCS_ARM7)) \
             $(patsubst %.s,$(BUILD)/arm7/%.o,$(ASM_ARM7))
OBJS_ARM9 := $(patsubst %.c,$(BUILD)/arm9/%.o,$(SRCS_ARM9)) \
             $(patsubst %.s,$(BUILD)/arm9/%.o,$(ASM_ARM9))

.PHONY: all rom run check verify size clean host-test arm9-decompile sd-image sd-image-installer release smoke-test configure-emulator nand-installer installer-test installer-retail nand-tool openlaunch-payload

all: rom

rom: $(ROM) $(LEGACY_ROM)

run: $(ROM) $(SD_IMAGE)
	./scripts/run_melonds.sh $(ROM)

# Provision the native melonDS configuration (XDG, TOML or legacy INI) with
# build/sdcard.img so the DSi SD card is mounted without any manual GUI step.
# Idempotent; backs up before the first change.
configure-emulator: $(SD_IMAGE)
	python3 scripts/configure_emulator.py --sd $(SD_IMAGE)

# Isolated, non-destructive melonDS startup smoke test. The shared injector
# (scripts/configure_emulator.py) first provisions the real config so `make run`
# works afterwards, and writes the isolated config the emulator is actually
# launched with. Skips cleanly when melonDS or a display helper is unavailable.
smoke-test: $(ROM) $(SD_IMAGE)
	python3 scripts/configure_emulator.py --sd $(SD_IMAGE) --quiet || true
	python3 scripts/smoke_test_emu.py --rom $(ROM) --sd $(SD_IMAGE)

# Object compilation / strict verification target.
check: all

# ---- Compile -------------------------------------------------------------
$(BUILD)/arm7/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_ARM7) -c $< -o $@

$(BUILD)/arm7/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ARM7) -c $< -o $@

$(BUILD)/arm9/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_ARM9) -c $< -o $@

$(BUILD)/arm9/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ARM9) -c $< -o $@

# ---- Link ----------------------------------------------------------------
$(ARM9_ELF): $(OBJS_ARM9) $(LD_DIR)/arm9.ld
	$(CC) $(CFLAGS_ARM9) $(LDFLAGS_FREESTANDING) -T $(LD_DIR)/arm9.ld \
		-Wl,-Map=$(BUILD)/arm9.map $(OBJS_ARM9) -lgcc -o $@

$(ARM7_ELF): $(OBJS_ARM7) $(LD_DIR)/arm7.ld
	$(CC) $(CFLAGS_ARM7) $(LDFLAGS_FREESTANDING) -T $(LD_DIR)/arm7.ld \
		-Wl,-Map=$(BUILD)/arm7.map $(OBJS_ARM7) -lgcc -o $@

# ---- Flat binaries -------------------------------------------------------
$(ARM9_BIN): $(ARM9_ELF)
	$(OBJCOPY) -O binary $< $@

$(ARM7_BIN): $(ARM7_ELF)
	$(OBJCOPY) -O binary $< $@

# ---- NDS / TWL ROM packaging --------------------------------------------
$(ROM): $(ARM9_BIN) $(ARM7_BIN) scripts/pack_nds.py
	python3 scripts/pack_nds.py \
		--arm9 $(ARM9_BIN) --arm9-elf $(ARM9_ELF) \
		--arm7 $(ARM7_BIN) --arm7-elf $(ARM7_ELF) \
		--title "OPENLAUNCH" \
		--banner-title "OpenLaunch\nClean-Room TWL Bootloader" \
		--twl --out $@

# Legacy alias (build/dsi_bootloader.nds) kept as a relative symlink.
$(LEGACY_ROM): $(ROM)
	ln -sf $(notdir $(ROM)) $@

# ---- Chainload test payload (build/hello_payload.nds) --------------------
# A tiny bare-metal ARM9/ARM7 stub linked outside the bootloader reservation
# (ARM9 0x02004000, ARM7 0x02380000) and packed as a loadable .nds. It paints
# the backdrops and idles, so a live `make CHAINLOAD_BOOT=1` handoff is obvious.
$(BUILD)/payload/arm9_payload.o: payload/arm9_payload.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ARM9) -c $< -o $@

$(BUILD)/payload/arm7_payload.o: payload/arm7_payload.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ARM7) -c $< -o $@

$(PAYLOAD_ARM9_ELF): $(BUILD)/payload/arm9_payload.o payload/arm9.ld
	$(CC) $(CFLAGS_ARM9) $(LDFLAGS_FREESTANDING) -T payload/arm9.ld \
		-Wl,-Map=$(BUILD)/payload/arm9.map \
		$(BUILD)/payload/arm9_payload.o -lgcc -o $@

$(PAYLOAD_ARM7_ELF): $(BUILD)/payload/arm7_payload.o payload/arm7.ld
	$(CC) $(CFLAGS_ARM7) $(LDFLAGS_FREESTANDING) -T payload/arm7.ld \
		-Wl,-Map=$(BUILD)/payload/arm7.map \
		$(BUILD)/payload/arm7_payload.o -lgcc -o $@

$(PAYLOAD_ARM9_BIN): $(PAYLOAD_ARM9_ELF)
	$(OBJCOPY) -O binary $< $@

$(PAYLOAD_ARM7_BIN): $(PAYLOAD_ARM7_ELF)
	$(OBJCOPY) -O binary $< $@

$(HELLO_ROM): $(PAYLOAD_ARM9_BIN) $(PAYLOAD_ARM7_BIN) scripts/pack_nds.py
	python3 scripts/pack_nds.py \
		--arm9 $(PAYLOAD_ARM9_BIN) --arm9-elf $(PAYLOAD_ARM9_ELF) \
		--arm7 $(PAYLOAD_ARM7_BIN) --arm7-elf $(PAYLOAD_ARM7_ELF) \
		--arm9-ram 0x02004000 --arm7-ram 0x02380000 \
		--title "CHAINLOAD OK" --banner-title "CHAINLOAD OK" \
		--gamecode TSTL --twl --out $@

# ---- Deterministic MBR + FAT16 SD image (build/sdcard.img) ---------------
$(SD_IMAGE): $(HELLO_ROM) scripts/make_sd_image.py
	python3 scripts/make_sd_image.py --hello $(HELLO_ROM) --out $@

sd-image: $(HELLO_ROM) $(SD_IMAGE)

# Sandbox installer fixtures: the same SD image plus OLBOOT.NDS and a signed
# sandbox INSTALL.KEY, so the guarded installer's six stages can be exercised
# against a decrypted sandbox NAND. The default build/sdcard.img stays clean
# (tests/test_installer.c asserts it ships no INSTALL.KEY).
sd-image-installer: $(HELLO_ROM) scripts/make_sd_image.py
	python3 scripts/make_sd_image.py --hello $(HELLO_ROM) \
		--out $(SD_INSTALLER_IMAGE) --with-installer-fixtures
	@echo "[*] sandbox installer SD image: $(SD_INSTALLER_IMAGE)"

# ---- Guarded NAND installer variant (DISABLED by default) ---------------
# Builds the full backup + flash pipeline into an isolated tree so the default
# build/ (and its read-only guarantee, the project conventions) is untouched. The result
# is build/nand/openlaunch.nds plus build/nand/sdcard_installer.img; the latter
# is a sandbox-only test fixture, never a retail install package.
nand-installer:
	$(MAKE) BUILD=build/nand NAND_WRITE_DANGER=1 rom sd-image
	$(MAKE) BUILD=build/nand sd-image-installer
	@echo "[*] guarded installer ROM: build/nand/openlaunch.nds"
	@echo "[*] sandbox fixtures   : build/nand/sdcard_installer.img"

# Separate write-enabled TEST build. Arms the full eMMC write path but still
# refuses retail encrypted NAND; intended for a decrypted sandbox NAND image or
# emulator. Safest way to exercise the six stages and their error paths.
installer-test:
	$(MAKE) BUILD=build/installer-test NAND_WRITE_DANGER=1 rom sd-image
	$(MAKE) BUILD=build/installer-test sd-image-installer
	@echo "[*] write-enabled test ROM : build/installer-test/openlaunch.nds"
	@echo "[*] sandbox fixtures       : build/installer-test/sdcard_installer.img"

# DANGEROUS retail raw-write test build. See docs/hardware_notes.md 34.11.
# It writes plaintext to a retail encrypted eMMC and can corrupt the System
# Menu; the boot chain and any Unlaunch install are left intact. Use only on a
# sacrificial or hardmod-recoverable console.
installer-retail:
	@echo "***********************************************************"
	@echo "*** DANGER: building the RETAIL raw-write test image.   ***"
	@echo "*** It can write PLAINTEXT to a retail eMMC, cannot     ***"
	@echo "*** install a working image and may corrupt the System ***"
	@echo "*** Menu. Use only on a sacrificial/hardmod console.    ***"
	@echo "***********************************************************"
	$(MAKE) BUILD=build/installer-retail NAND_WRITE_DANGER=1 RETAIL_NAND_WRITE=1 rom sd-image
	python3 scripts/make_sd_image.py --hello build/installer-retail/hello_payload.nds \
		--out build/installer-retail/sdcard_installer.img \
		--with-installer-fixtures --retail-token
	@echo "[*] RETAIL raw-write ROM   : build/installer-retail/openlaunch.nds"
	@echo "[*] retail fixtures        : build/installer-retail/sdcard_installer.img"

# ---- Host tool: offline NAND decrypt/encrypt/verify + layout info ---------
# Reuses the shared src/common crypto. Never touches a console.
NAND_TOOL := $(BUILD)/tools/dsi_nand_tool
nand-tool:
	@mkdir -p $(dir $(NAND_TOOL))
	$(HOST_CC) $(HOST_CFLAGS) -O2 -o $(NAND_TOOL) tools/dsi_nand_tool.c \
		src/common/dsi_crypto.c src/common/aes.c src/common/sha1.c \
		src/common/mem.c
	@echo "[*] offline NAND tool: $(NAND_TOOL)"

# ---- OpenLaunch bootcode payload (Unlaunch-format container) -------------
# Packs the ARM9/ARM7 binaries into the self-describing container that the
# backup-launcher (HNAA) title.tmd expects, and wires the stage-2 TMD-overflow
# callback slots to a small in-place bootstrap. See docs/openlaunch_payload.md.
BOOTSTRAP_OBJ := $(BUILD)/boot/overflow_bootstrap.o
BOOTSTRAP_BIN := $(BUILD)/overflow_bootstrap.bin
PAYLOAD_BIN   := $(BUILD)/openlaunch_payload.bin

$(BOOTSTRAP_OBJ): src/arm9/overflow_bootstrap.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ARM9) -c -o $@ $<

$(BOOTSTRAP_BIN): $(BOOTSTRAP_OBJ)
	$(OBJCOPY) -O binary -j .text $< $@

$(PAYLOAD_BIN): $(ARM9_BIN) $(ARM7_BIN) $(BOOTSTRAP_BIN) \
                scripts/make_openlaunch_payload.py
	python3 scripts/make_openlaunch_payload.py --arm9 $(ARM9_BIN) \
		--arm7 $(ARM7_BIN) --bootstrap $(BOOTSTRAP_BIN) \
		--out $(PAYLOAD_BIN)

openlaunch-payload: $(PAYLOAD_BIN)

# ---- Reproducible release bundle (build/release/ + tar.gz) ---------------
# Builds the diagnostic, live-chainload and autoboot variants (each in its own
# isolated build directory so the default build/ tree is untouched), stages the
# ready-to-copy SD card template and writes a deterministic SHA256SUMS.txt.
# See scripts/make_release.py.
release:
	python3 scripts/make_release.py

# ---- Verification harness ------------------------------------------------
verify: $(ROM) $(HELLO_ROM) $(SD_IMAGE) $(PAYLOAD_BIN)
	python3 scripts/verify_rom.py --rom $(ROM) \
		--expect-title OPENLAUNCH \
		--expect-banner-title "OpenLaunch / Clean-Room TWL Bootloader" \
		--arm9-elf $(ARM9_ELF) --arm7-elf $(ARM7_ELF) \
		--arm9-bin $(ARM9_BIN) --arm7-bin $(ARM7_BIN) \
		--arm7-map $(BUILD)/arm7.map --arm9-map $(BUILD)/arm9.map
	python3 scripts/verify_rom.py --rom $(HELLO_ROM) \
		--arm9-elf $(PAYLOAD_ARM9_ELF) --arm7-elf $(PAYLOAD_ARM7_ELF) \
		--arm9-bin $(PAYLOAD_ARM9_BIN) --arm7-bin $(PAYLOAD_ARM7_BIN)
	python3 scripts/verify_sd_image.py --image $(SD_IMAGE) --hello $(HELLO_ROM)
	python3 scripts/verify_openlaunch_payload.py --payload $(PAYLOAD_BIN)

# ---- Memory footprint ----------------------------------------------------
size: $(ARM9_ELF) $(ARM7_ELF)
	@echo "== ARM9 =="
	$(SIZE) $(ARM9_ELF)
	@echo "== ARM7 =="
	$(SIZE) $(ARM7_ELF)

clean:
	rm -rf $(BUILD)

# Reverse-engineering pipeline: unpack the ARM9 payload and re-export every
# function to asm/decompiled/arm9 via headless Ghidra. Requires Docker.
arm9-decompile:
	./scripts/run_ghidra.sh

# ---- Host-side unit tests (hardware-independent logic) -------------------
HOST_CC    ?= cc
HOST_CFLAGS := -std=c99 -Wall -Wextra -Werror -Iinclude

# Shared primitives exercised by the memory/IPC test.
HOST_SRCS_MEM  := tests/test_mem.c src/common/mem.c
# ARM9 logic: LZ77/RLE, segment loader, MPU decode, VRAM packing, console,
# input, menu state machine and the full boot path.
HOST_SRCS_ARM9 := tests/test_arm9.c \
                  src/common/mem.c src/common/lz77.c src/common/cache.c \
                  src/common/fat_ro.c src/common/fat_rw.c \
                  src/common/nds_header.c \
                  src/common/dldi.c src/common/nand.c \
                  src/common/sha256.c src/common/installer.c \
                  src/arm9/mpu.c src/arm9/irq.c src/arm9/ipc.c \
                  src/arm9/video.c src/arm9/dma.c src/arm9/scfg.c \
                  src/arm9/boot.c src/arm9/arm7_boot.c src/arm9/boot_cfg.c \
                  src/arm9/theme.c \
                  src/arm9/splash_tux.c \
                  src/arm9/console.c src/arm9/font8x8.c src/arm9/ui_font.c \
                  src/arm9/input.c src/arm9/menu.c src/arm9/storage.c \
                  src/arm9/nand_boot.c src/arm9/loader.c \
                  src/arm9/installer_ui.c \
                  src/arm9/installer_backup.c src/arm9/installer_install.c
# Console tile/glyph packer + bounded formatters.
HOST_SRCS_CONSOLE := tests/test_console.c \
                     src/arm9/console.c src/arm9/font8x8.c src/common/mem.c
# Menu state machine + keypad edge detector + BOOT.CFG/hotkey resolver.
HOST_SRCS_MENU := tests/test_menu.c \
                  src/arm9/menu.c src/arm9/boot.c src/arm9/arm7_boot.c \
                  src/arm9/boot_cfg.c \
                  src/arm9/theme.c src/arm9/splash_tux.c \
                  src/arm9/input.c \
                  src/arm9/console.c src/arm9/font8x8.c src/arm9/ui_font.c \
                  src/arm9/ipc.c src/arm9/scfg.c src/arm9/mpu.c \
                  src/arm9/irq.c src/arm9/video.c src/arm9/dma.c \
                  src/arm9/storage.c src/arm9/nand_boot.c src/arm9/loader.c \
                  src/arm9/installer_ui.c \
                  src/common/mem.c src/common/lz77.c src/common/cache.c \
                  src/common/fat_ro.c src/common/nds_header.c src/common/dldi.c \
                  src/common/nand.c src/common/sha256.c src/common/installer.c \
                  src/common/fat_rw.c \
                  src/arm9/installer_backup.c src/arm9/installer_install.c
# ARM7 TMIO SD/MMC engine, with the register accesses mocked out.
HOST_SRCS_SDMMC := tests/test_sdmmc.c src/arm7/sdmmc.c
HOST_CFLAGS_SDMMC := $(HOST_CFLAGS) -DSDMMC_IO_HOOKS
# Read-only FAT16/FAT32 parser + NDS header validator.
HOST_SRCS_FAT := tests/test_fat_ro.c \
                 src/common/fat_ro.c src/common/nds_header.c src/common/mem.c
# Chainloader safety gate + bounded cluster-chain reader.
HOST_SRCS_LOADER := tests/test_loader.c \
                    src/arm9/loader.c \
                    src/common/fat_ro.c src/common/nds_header.c \
                    src/common/mem.c src/common/cache.c src/arm9/ipc.c \
                    src/common/dldi.c
# SCFG/MBK pure decoders (mode, clock, RAM window, slot packer).
HOST_SRCS_SCFG := tests/test_scfg.c src/arm9/scfg.c
# End-to-end chainload pipeline over the generated build/sdcard.img. loader.c is
# compiled with -DLOADER_HOST_TEST so it stages into the test's scratch arena.
HOST_SRCS_SDIMG := tests/test_sd_image.c \
                   src/arm9/loader.c src/arm9/boot_cfg.c src/arm9/theme.c \
                   src/arm9/splash_tux.c src/arm9/font8x8.c \
                   src/common/fat_ro.c src/common/nds_header.c \
                   src/common/mem.c src/common/cache.c src/arm9/ipc.c \
                   src/common/dldi.c
HOST_CFLAGS_SDIMG := $(HOST_CFLAGS) -DLOADER_HOST_TEST

# Clean-room DLDI scanner / in-RAM patcher + built-in read-only stub driver.
HOST_SRCS_DLDI := tests/test_dldi.c src/common/dldi.c src/common/mem.c
# Graphical bitmap font / banner-icon renderer (decoders, packers, 16x16 text).
HOST_SRCS_UI_FONT := tests/test_ui_font.c \
                     src/arm9/ui_font.c src/arm9/console.c \
                     src/arm9/font8x8.c src/common/mem.c
# NAND partition-table parser + boot-redirect policy + deep FAT path resolver.
HOST_SRCS_NAND := tests/test_nand.c \
                  src/common/nand.c src/common/fat_ro.c \
                  src/common/nds_header.c src/common/mem.c
# Boot theme engine: hex-colour parsing, defaults, BOOT.CFG directives and the
# raw splash validator / FAT reader (suite 13).
HOST_SRCS_THEME := tests/test_theme.c \
                   src/arm9/theme.c src/arm9/boot_cfg.c \
                   src/arm9/splash_tux.c src/arm9/font8x8.c \
                   src/common/fat_ro.c src/common/mem.c
# NAND installer: SHA-256/HMAC-SHA-256, token codec + policy, layout validation,
# the bounded FAT root-file writer, and the backup/flash streaming engines
# (suite 14).
HOST_SRCS_INSTALLER := tests/test_installer.c \
                       src/common/sha256.c src/common/installer.c \
                       src/common/nand.c src/common/fat_ro.c \
                       src/common/fat_rw.c src/common/mem.c \
                       src/arm9/installer_backup.c \
                       src/arm9/installer_install.c
# Installer modal UI: pure stage/percentage formatters (suite 15). The drawing
# entry points are linked but never invoked on the host (they would touch VRAM).
HOST_SRCS_INSTALLER_UI := tests/test_installer_ui.c \
                          src/arm9/installer_ui.c \
                          src/arm9/console.c src/arm9/font8x8.c \
                          src/arm9/input.c \
                          src/common/installer.c src/common/sha256.c \
                          src/common/nand.c src/common/fat_ro.c \
                          src/common/mem.c
# AES-128 (FIPS-197) + SHA-1 (FIPS 180-4) core: the foundation for the DSi
# per-console NAND crypto (suite 16).
HOST_SRCS_CRYPTO := tests/test_crypto.c \
                    src/common/aes.c src/common/sha1.c
# DSi per-console NAND key derivation + AES-CTR, with real-dump known-answer
# vectors (suite 17).
HOST_SRCS_DSICRYPTO := tests/test_dsi_crypto.c \
                       src/common/dsi_crypto.c src/common/aes.c \
                       src/common/sha1.c src/common/mem.c

host-test: $(SD_IMAGE)
	@mkdir -p $(BUILD)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_mem $(HOST_SRCS_MEM)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_arm9 $(HOST_SRCS_ARM9)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_console $(HOST_SRCS_CONSOLE)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_menu $(HOST_SRCS_MENU)
	$(HOST_CC) $(HOST_CFLAGS_SDMMC) -o $(BUILD)/host_test_sdmmc $(HOST_SRCS_SDMMC)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_fat $(HOST_SRCS_FAT)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_loader $(HOST_SRCS_LOADER)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_scfg $(HOST_SRCS_SCFG)
	$(HOST_CC) $(HOST_CFLAGS_SDIMG) -o $(BUILD)/host_test_sd_image $(HOST_SRCS_SDIMG)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_dldi $(HOST_SRCS_DLDI)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_ui_font $(HOST_SRCS_UI_FONT)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_nand $(HOST_SRCS_NAND)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_theme $(HOST_SRCS_THEME)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_installer $(HOST_SRCS_INSTALLER)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_installer_ui $(HOST_SRCS_INSTALLER_UI)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_crypto $(HOST_SRCS_CRYPTO)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_dsi_crypto $(HOST_SRCS_DSICRYPTO)
	$(BUILD)/host_test_mem
	$(BUILD)/host_test_arm9
	$(BUILD)/host_test_console
	$(BUILD)/host_test_menu
	$(BUILD)/host_test_sdmmc
	$(BUILD)/host_test_fat
	$(BUILD)/host_test_loader
	$(BUILD)/host_test_scfg
	$(BUILD)/host_test_sd_image $(SD_IMAGE)
	$(BUILD)/host_test_dldi
	$(BUILD)/host_test_ui_font
	$(BUILD)/host_test_nand $(SD_IMAGE)
	$(BUILD)/host_test_theme $(SD_IMAGE)
	$(BUILD)/host_test_installer $(SD_IMAGE)
	$(BUILD)/host_test_installer_ui
	$(BUILD)/host_test_crypto
	$(BUILD)/host_test_dsi_crypto
