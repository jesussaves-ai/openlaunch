#!/usr/bin/env bash
# scripts/run_ghidra.sh
#
# End-to-end headless extraction of the Unlaunch ARM9 codebase.
#
#   1. unpack_unlaunch_arm9.py  -> reconstructs the flat VRAM image (LZ77 + gfx)
#   2. objcopy/ld               -> wraps the code segment in a real ELF whose
#                                  load address and entry point are correct
#   3. Ghidra headless          -> auto-analysis + ExportToC.java -> asm/decompiled/arm9
#
# The Docker container is run as the calling user (no sudo, no root-owned files).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

IMAGE="${GHIDRA_IMAGE:-blacktop/ghidra}"
OUT_DIR="$ROOT/asm/decompiled/arm9"

echo "[*] 1/3 Reconstructing ARM9 payload..."
python3 scripts/unpack_unlaunch_arm9.py

# Read the reconstructed load base + entry from the metadata the unpacker wrote.
read -r CODE_BASE ENTRY GFX_BASE < <(python3 -c \
  "import json;d=json.load(open('asm/arm9_unpack.json'));print('0x%X'%d['code_base'],'0x%X'%d['entry'],'0x%X'%d['gfx_base'])")

echo "[*] 2/3 Building ELF wrapper (assets @ 0x$(printf '%X' "$GFX_BASE"), code @ 0x$(printf '%X' "$CODE_BASE"), entry @ 0x$(printf '%X' "$ENTRY"))..."
mkdir -p build
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm \
    --rename-section .data=.arm9assets,alloc,load,readonly,data,contents \
    asm/arm9_gfx.bin build/arm9_gfx.o
arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm \
    --rename-section .data=.text,alloc,load,readonly,code,contents \
    asm/arm9_code.bin build/arm9_code.o
arm-none-eabi-ld -T scripts/arm9_image.ld -e "$ENTRY" \
    build/arm9_code.o build/arm9_gfx.o -o build/arm9_code.elf

echo "[*] 3/3 Running Ghidra headless ($IMAGE)..."
rm -rf workspace
mkdir -p workspace
mkdir -p "$OUT_DIR"
# Clear stale output so the index reflects exactly this run.
find "$OUT_DIR" -maxdepth 1 -name '*.c' -delete 2>/dev/null || true
rm -f "$OUT_DIR/index.tsv"

docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -v "$ROOT:/work" \
    "$IMAGE" \
    /ghidra/support/analyzeHeadless \
    /work/workspace UnlaunchARM9Project \
    -import /work/build/arm9_code.elf \
    -processor "ARM:LE:32:v5t" \
    -scriptPath /work/scripts \
    -analysisTimeoutPerFile 900 \
    -postScript ExportToC.java /work/asm/decompiled/arm9 "$ENTRY"

echo "[*] ARM9 headless extraction complete -> asm/decompiled/arm9/"
