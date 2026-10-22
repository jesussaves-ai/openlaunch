#!/usr/bin/env bash
# scripts/run_melonds.sh
#
# Convenience launcher for the packaged ROM in melonDS.
#
#     ./scripts/run_melonds.sh [path/to/rom.nds]
#
# Before launching it provisions the native melonDS configuration with the
# emulated DSi SD card (build/sdcard.img) so the chainloader and the ARM7
# SD/MMC self-test work without any manual GUI setup. The configuration step is
# handled by scripts/configure_emulator.py, which understands both the modern
# melonDS.toml and the legacy melonDS.ini layouts in the native XDG directory
# (~/.config/melonDS).
#
# Set OPENLAUNCH_SKIP_CONFIG=1 to leave the configuration untouched, or
# OPENLAUNCH_SD_IMAGE=/path/to/image to point at a different SD image.
#
# It launches the native `melonDS` binary. The exact melonDS settings (BIOS /
# DSi firmware / NAND) are documented in tests/README.md.
#
# Expected result once booted: the top screen shows the bootloader header and a
# live hardware telemetry table, and the bottom screen shows a D-Pad navigable
# diagnostic menu (A = run, B = re-run the self-test). See tests/README.md.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROM="${1:-$ROOT/build/openlaunch.nds}"
SD_IMAGE="${OPENLAUNCH_SD_IMAGE:-$ROOT/build/sdcard.img}"

if [ ! -f "$ROM" ]; then
    echo "run_melonds: ROM not found: $ROM" >&2
    echo "             build it first with: make rom" >&2
    exit 1
fi

# Provision the emulated DSi SD card path before starting the emulator so the
# chainloader never hits the `CMD ... TO NULL DEVICE` polling loop. This is
# best-effort: a missing python3 or an unwritable config must not block launch.
if [ "${OPENLAUNCH_SKIP_CONFIG:-0}" != "1" ]; then
    if [ ! -f "$SD_IMAGE" ]; then
        echo "run_melonds: SD image not found: $SD_IMAGE" >&2
        echo "             build it with: make sd-image" >&2
    elif command -v python3 >/dev/null 2>&1; then
        echo "[*] configuring melonDS DSi SD card -> $SD_IMAGE"
        if ! python3 "$ROOT/scripts/configure_emulator.py" --sd "$SD_IMAGE"; then
            echo "run_melonds: configuration step failed; continuing anyway" >&2
        fi
    else
        echo "run_melonds: python3 not found; skipping configuration" >&2
    fi
fi

if command -v melonDS >/dev/null 2>&1; then
    echo "[*] launching native melonDS: $ROM"
    exec melonDS "$ROM"
fi

cat >&2 <<'EOF'
run_melonds: melonDS was not found.

Install one of:
  * a distribution package providing `melonDS`
  * a build from https://github.com/melonDS-emu/melonDS

Then re-run this script, or open build/openlaunch.nds manually.
See tests/README.md for the recommended BIOS/DSi-firmware settings.
EOF
exit 127
