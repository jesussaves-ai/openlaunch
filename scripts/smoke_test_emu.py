#!/usr/bin/env python3
"""Non-destructive melonDS startup smoke test for the OpenLaunch ROM.

The script boots ``build/openlaunch.nds`` in melonDS (with ``build/sdcard.img``
configured as the emulated DSi SD card) for a short window and asserts that:

  * the emulator starts OpenLaunch without crashing; and
  * the DSi SD card image is actually attached (the emulator log shows the image
    being opened) and no SD command is issued to a null device (the
    ``CMD ... TO NULL DEVICE`` polling loop described in
    ``docs/hardware_notes.md`` §23).

It is deliberately conservative and never touches the user's real emulator
configuration:

  * melonDS is looked up as the native ``melonDS`` binary on ``PATH``.
  * A display is required. ``WAYLAND_DISPLAY``/``DISPLAY`` are used when set;
    otherwise ``xvfb-run`` is used to provide a virtual X server.
  * The emulator runs with ``XDG_CONFIG_HOME`` pointed at a fresh temporary
    directory; the isolated ``melonDS/melonDS.toml`` written there by
    ``scripts/configure_emulator.py`` points the DSi SD card at
    ``build/sdcard.img``. The user's real ``~/.config/melonDS/melonDS.toml`` is
    never read or written.

If melonDS or a display helper is unavailable (for example on headless CI), the
script prints ``[SKIP] melonDS or display environment not available`` and exits
``0``. A premature non-zero emulator exit (including a segfault) is a failure.

Usage:

    python3 scripts/smoke_test_emu.py [--rom build/openlaunch.nds] \
        [--sd build/sdcard.img] [--timeout 2.0] [--allow-unconfirmed-sd]
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import configure_emulator  # noqa: E402  (shared config injector)

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_ROM = os.path.join(REPO_ROOT, "build", "openlaunch.nds")
DEFAULT_SD = os.path.join(REPO_ROOT, "build", "sdcard.img")
DEFAULT_TIMEOUT = 2.0


def log(msg: str) -> None:
    print("[smoke] %s" % msg, flush=True)


def skip(msg: str) -> int:
    print("[SKIP] %s" % msg, flush=True)
    return 0


def find_melonds():
    """Return the native ``melonDS`` argv prefix, or None when unavailable."""
    path = shutil.which("melonDS")
    if path:
        return [path]
    return None


def display_helper():
    """Return the argv prefix providing a display, or None if none is available."""
    if os.environ.get("WAYLAND_DISPLAY") or os.environ.get("DISPLAY"):
        return []
    if shutil.which("xvfb-run"):
        return ["xvfb-run", "-a"]
    return None


def run(cmd, env, timeout, log_path):
    """Run `cmd`, capturing its output to `log_path`.

    Returns ``(status, returncode)`` where status is 'pass'/'timeout'/'fail'.
    """
    log("launching: %s" % " ".join(cmd))
    with open(log_path, "wb") as log_fh:
        proc = subprocess.Popen(cmd, env=env, stdout=log_fh,
                                stderr=subprocess.STDOUT)
        try:
            return ("pass", proc.wait(timeout=timeout))
        except subprocess.TimeoutExpired:
            # Still alive after the smoke window: the emulator started cleanly.
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            return ("timeout", 0)


# melonDS logs this when a command is issued while no SD image is attached
# (see docs/hardware_notes.md §23): the CMD1 null-device polling loop.
NULL_DEVICE_MARKERS = ("null device",)


def check_sd_attachment(log_path, sd):
    """Inspect the emulator log and report on the emulated DSi SD card.

    Returns ``(verdict, message)`` where verdict is ``True`` (attached),
    ``False`` (a command hit a null device) or ``None`` (no evidence).
    """
    try:
        with open(log_path, "rb") as fh:
            data = fh.read().decode("utf-8", "replace")
    except OSError as exc:
        return None, "could not read emulator log: %s" % exc

    lowered = data.lower()
    for marker in NULL_DEVICE_MARKERS:
        if marker in lowered:
            return False, ("melonDS issued an SD command to a null device "
                           "(the SD image is not attached)")

    # melonDS logs the absolute image path when it attaches the DSi SD card.
    if sd and sd in data:
        return True, "DSi SD card attached: %s" % sd
    if "sd/mmc" in lowered:
        return None, "SD/MMC activity seen but the SD image path was not logged"
    return None, "the emulator log carried no SD/MMC evidence"


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=DEFAULT_ROM)
    ap.add_argument("--sd", default=DEFAULT_SD)
    ap.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT,
                    help="seconds to let melonDS run (default %.1f)"
                         % DEFAULT_TIMEOUT)
    ap.add_argument("--allow-unconfirmed-sd", action="store_true",
                    help="do not fail when the emulator log does not confirm "
                         "that the DSi SD card image was attached")
    args = ap.parse_args(argv)

    rom = os.path.abspath(args.rom)
    sd = os.path.abspath(args.sd)

    melonds = find_melonds()
    helper = display_helper()
    if melonds is None or helper is None:
        return skip("melonDS or display environment not available")

    if not os.path.isfile(rom):
        print("[FAIL] ROM not found: %s (build it with `make rom`)" % rom,
              file=sys.stderr)
        return 1
    if not os.path.isfile(sd):
        print("[FAIL] SD image not found: %s (build it with `make sd-image`)"
              % sd, file=sys.stderr)
        return 1

    prefix = melonds

    with tempfile.TemporaryDirectory(prefix="openlaunch-melonDS-") as tmp:
        cfg_path = configure_emulator.write_isolated_config(tmp, sd)
        log("using isolated config %s" % cfg_path)

        env = os.environ.copy()
        env["XDG_CONFIG_HOME"] = tmp

        cmd = list(helper)
        cmd += prefix + [rom]

        emu_log = os.path.join(tmp, "melonDS.log")
        status, rc = run(cmd, env, args.timeout, emu_log)
        sd_ok, sd_msg = check_sd_attachment(emu_log, sd)

    # Report a crash before anything else for the clearest diagnostic.
    if status != "timeout" and rc != 0:
        print("[FAIL] melonDS exited early with code %d" % rc, flush=True)
        return 1

    if sd_ok is False:
        print("[FAIL] %s" % sd_msg, flush=True)
        return 1
    if sd_ok is True:
        log(sd_msg)
    elif not args.allow_unconfirmed_sd:
        print("[FAIL] could not confirm the DSi SD card was attached: %s"
              % sd_msg, flush=True)
        return 1
    else:
        log("note: %s" % sd_msg)

    if status == "timeout":
        print("[PASS] OpenLaunch ran in melonDS without crashing", flush=True)
    else:
        print("[PASS] OpenLaunch ran in melonDS and exited cleanly", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
