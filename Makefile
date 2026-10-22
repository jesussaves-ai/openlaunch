# Makefile — Nintendo DSi low-level bootloader (ARM9 + ARM7)
#
# Toolchain: BlocksDS / devkitARM. Override CROSS to point at another
# arm-none-eabi toolchain, e.g.:
#
#     make CROSS=arm-none-eabi- DEVKITARM=/opt/devkitpro/devkitARM
#
# Language: strict C99 (-std=c99). GNU ARM assembly is used only for crt0 /
# vectors / CP15 (see AGENTS.md §2).
#
# Targets:
#   make            build the runnable ROM (build/dsi_bootloader.nds)
#   make rom        alias for the above
#   make check      strict cross-compile of every object (-Werror)
#   make verify     validate the packaged ROM (CRCs, offsets, no NAND writes)
#   make host-test  host unit tests for the hardware-independent logic
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

BUILD      := build
LD_DIR     := ld
INCLUDES   := -Iinclude

ROM        := $(BUILD)/dsi_bootloader.nds
ARM9_ELF   := $(BUILD)/arm9.elf
ARM7_ELF   := $(BUILD)/arm7.elf
ARM9_BIN   := $(BUILD)/arm9.bin
ARM7_BIN   := $(BUILD)/arm7.bin

# ---- Shared freestanding flags -------------------------------------------
CFLAGS_COMMON := -std=c99 -Wall -Wextra -Werror -Os -g \
                 -ffreestanding -fno-builtin -fno-common \
                 -fomit-frame-pointer -fno-strict-aliasing \
                 -ffunction-sections -fdata-sections \
                 $(INCLUDES)

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
ASM_ARM9  := $(wildcard src/arm9/*.s)

OBJS_ARM7 := $(patsubst %.c,$(BUILD)/arm7/%.o,$(SRCS_ARM7)) \
             $(patsubst %.s,$(BUILD)/arm7/%.o,$(ASM_ARM7))
OBJS_ARM9 := $(patsubst %.c,$(BUILD)/arm9/%.o,$(SRCS_ARM9)) \
             $(patsubst %.s,$(BUILD)/arm9/%.o,$(ASM_ARM9))

.PHONY: all rom run check verify size clean host-test arm9-decompile

all: rom

rom: $(ROM)

run: $(ROM)
	./scripts/run_melonds.sh $(ROM)

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
		--twl --out $@

# ---- Verification harness ------------------------------------------------
verify: $(ROM)
	python3 scripts/verify_rom.py --rom $(ROM) \
		--arm9-elf $(ARM9_ELF) --arm7-elf $(ARM7_ELF) \
		--arm9-bin $(ARM9_BIN) --arm7-bin $(ARM7_BIN) \
		--arm7-map $(BUILD)/arm7.map --arm9-map $(BUILD)/arm9.map

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
                  src/common/fat_ro.c src/common/nds_header.c \
                  src/arm9/mpu.c src/arm9/irq.c src/arm9/ipc.c \
                  src/arm9/video.c src/arm9/dma.c src/arm9/scfg.c \
                  src/arm9/boot.c src/arm9/console.c src/arm9/font8x8.c \
                  src/arm9/input.c src/arm9/menu.c src/arm9/storage.c
# Console tile/glyph packer + bounded formatters.
HOST_SRCS_CONSOLE := tests/test_console.c \
                     src/arm9/console.c src/arm9/font8x8.c src/common/mem.c
# Menu state machine + keypad edge detector.
HOST_SRCS_MENU := tests/test_menu.c \
                  src/arm9/menu.c src/arm9/input.c \
                  src/arm9/console.c src/arm9/font8x8.c \
                  src/arm9/ipc.c src/arm9/scfg.c src/arm9/mpu.c \
                  src/arm9/storage.c \
                  src/common/mem.c src/common/lz77.c src/common/cache.c \
                  src/common/fat_ro.c src/common/nds_header.c
# ARM7 TMIO SD/MMC engine, with the register accesses mocked out.
HOST_SRCS_SDMMC := tests/test_sdmmc.c src/arm7/sdmmc.c
HOST_CFLAGS_SDMMC := $(HOST_CFLAGS) -DSDMMC_IO_HOOKS
# Read-only FAT16/FAT32 parser + NDS header validator.
HOST_SRCS_FAT := tests/test_fat_ro.c \
                 src/common/fat_ro.c src/common/nds_header.c src/common/mem.c

host-test:
	@mkdir -p $(BUILD)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_mem $(HOST_SRCS_MEM)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_arm9 $(HOST_SRCS_ARM9)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_console $(HOST_SRCS_CONSOLE)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_menu $(HOST_SRCS_MENU)
	$(HOST_CC) $(HOST_CFLAGS_SDMMC) -o $(BUILD)/host_test_sdmmc $(HOST_SRCS_SDMMC)
	$(HOST_CC) $(HOST_CFLAGS) -o $(BUILD)/host_test_fat $(HOST_SRCS_FAT)
	$(BUILD)/host_test_mem
	$(BUILD)/host_test_arm9
	$(BUILD)/host_test_console
	$(BUILD)/host_test_menu
	$(BUILD)/host_test_sdmmc
	$(BUILD)/host_test_fat
