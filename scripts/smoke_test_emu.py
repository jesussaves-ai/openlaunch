#!/usr/bin/env python3
"""Non-destructive melonDS startup smoke test for the OpenLaunch ROM.

The script boots ``build/openlaunch.nds`` in melonDS (with ``build/sdcard.img``
configured as the emulated DSi SD card) for a short window and asserts that the
emulator starts OpenLaunch without crashing.

It is deliberately conservative and never touches the user's real emulator
configuration:

  * melonDS is looked up as a native ``melonDS``/``melonds`` binary on ``PATH``
    first, then the Flatpak ``net.kuribo64.melonDS``.
  * A display is required. ``WAYLAND_DISPLAY``/``DISPLAY`` are used when set;
    otherwise ``xvfb-run`` is used to provide a virtual X server.
  * The emulator runs with ``XDG_CONFIG_HOME`` pointed at a fresh temporary
    directory; the isolated ``melonDS/melonDS.toml`` written there points
    ``SDCardPath`` (and the modern ``[DSi.SD] ImagePath``) at ``build/sdcard.img``.
    ``~/.config/melonDS/melonDS.toml`` and ``~/.var/app/net.kuribo64.melonDS``
    are never read or written.

If melonDS or a display helper is unavailable (for example on headless CI), the
script prints ``[SKIP] melonDS or display environment not available`` and exits
``0``. A premature non-zero emulator exit (including a segfault) is a failure.

Usage:

    python3 scripts/smoke_test_emu.py [--rom build/openlaunch.nds] \
        [--sd build/sdcard.img] [--timeout 2.0]
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

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
    """Return (argv_prefix, is_flatpak), or None when melonDS is unavailable."""
    for name in ("melonDS", "melonds"):
        path = shutil.which(name)
        if path:
            return ([path], False)
    if shutil.which("flatpak"):
        try:
            out = subprocess.run(["flatpak", "list", "--app"],
                                 capture_output=True, text=True, timeout=30)
        except (OSError, subprocess.SubprocessError):
            out = None
        if out is not None and "net.kuribo64.melonDS" in out.stdout:
            return (["flatpak", "run", "net.kuribo64.melonDS"], True)
    return None


def display_helper():
    """Return the argv prefix providing a display, or None if none is available."""
    if os.environ.get("WAYLAND_DISPLAY") or os.environ.get("DISPLAY"):
        return []
    if shutil.which("xvfb-run"):
        return ["xvfb-run", "-a"]
    return None


def toml_path(path: str) -> str:
    """Quote a filesystem path for a TOML basic string."""
    return '"' + path.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_config(config_home: str, sd_path: str) -> str:
    """Write an isolated melonDS configuration and return its path."""
    cfg_dir = os.path.join(config_home, "melonDS")
    os.makedirs(cfg_dir, exist_ok=True)
    cfg_path = os.path.join(cfg_dir, "melonDS.toml")
    quoted = toml_path(sd_path)
    contents = (
        "# OpenLaunch isolated smoke-test configuration.\n"
        "# Generated in a temporary directory; the user's real melonDS config\n"
        "# is never modified.\n"
        "[Emu]\n"
        "ConsoleType = 1\n"
        "DirectBoot = true\n"
        "\n"
        "[DSi]\n"
        "SDCardPath = %s\n"
        "\n"
        "[DSi.SD]\n"
        "ImagePath = %s\n"
        "Enable = true\n"
        "ReadOnly = true\n"
        % (quoted, quoted)
    )
    with open(cfg_path, "w", encoding="utf-8") as fh:
        fh.write(contents)
    return cfg_path


def run(cmd, env, timeout):
    """Run `cmd`; return (status, returncode). status is 'pass'/'timeout'/'fail'."""
    log("launching: %s" % " ".join(cmd))
    proc = subprocess.Popen(cmd, env=env, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
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


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", default=DEFAULT_ROM)
    ap.add_argument("--sd", default=DEFAULT_SD)
    ap.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT,
                    help="seconds to let melonDS run (default %.1f)"
                         % DEFAULT_TIMEOUT)
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

    prefix, is_flatpak = melonds

    with tempfile.TemporaryDirectory(prefix="openlaunch-melonds-") as tmp:
        cfg_path = write_config(tmp, sd)
        log("using isolated config %s" % cfg_path)

        env = os.environ.copy()
        env["XDG_CONFIG_HOME"] = tmp

        cmd = list(helper)
        if is_flatpak:
            # Keep the Flatpak sandbox config isolated from ~/.var/app.
            cmd += ["flatpak", "run", "--env=XDG_CONFIG_HOME=%s" % tmp,
                    "net.kuribo64.melonDS", rom]
        else:
            cmd += prefix + [rom]

        status, rc = run(cmd, env, args.timeout)

    if status == "timeout":
        print("[PASS] OpenLaunch ran in melonDS without crashing", flush=True)
        return 0
    if rc == 0:
        print("[PASS] OpenLaunch ran in melonDS and exited cleanly", flush=True)
        return 0
    print("[FAIL] melonDS exited early with code %d" % rc, flush=True)
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
