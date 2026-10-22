#!/usr/bin/env bash
# scripts/run_melonds.sh
#
# Convenience launcher for the packaged ROM in melonDS.
#
#     ./scripts/run_melonds.sh [path/to/rom.nds]
#
# It looks for a native `melonds`/`melonDS` binary first, then the Flatpak
# `net.kuribo64.melonDS`. The exact melonDS settings (BIOS / DSi firmware /
# NAND) are documented in tests/README.md.
#
# Expected result once booted: the top screen shows the bootloader header and a
# live hardware telemetry table, and the bottom screen shows a D-Pad navigable
# diagnostic menu (A = run, B = re-run the self-test). See tests/README.md.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROM="${1:-$ROOT/build/dsi_bootloader.nds}"

if [ ! -f "$ROM" ]; then
    echo "run_melonds: ROM not found: $ROM" >&2
    echo "             build it first with: make rom" >&2
    exit 1
fi

if command -v melonds >/dev/null 2>&1; then
    echo "[*] launching native melonds: $ROM"
    exec melonds "$ROM"
fi

if command -v melonDS >/dev/null 2>&1; then
    echo "[*] launching native melonDS: $ROM"
    exec melonDS "$ROM"
fi

if command -v flatpak >/dev/null 2>&1 && \
   flatpak list --app 2>/dev/null | grep -q 'net.kuribo64.melonDS'; then
    echo "[*] launching Flatpak melonDS: $ROM"
    exec flatpak run net.kuribo64.melonDS "$ROM"
fi

cat >&2 <<'EOF'
run_melonds: melonDS was not found.

Install one of:
  * a distribution package providing `melonds`
  * the Flatpak:  flatpak install flathub net.kuribo64.melonDS
  * a build from https://github.com/melonDS-emu/melonDS

Then re-run this script, or open build/dsi_bootloader.nds manually.
See tests/README.md for the recommended BIOS/DSi-firmware settings.
EOF
exit 127
