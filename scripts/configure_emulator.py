#!/usr/bin/env python3
"""Provision the local melonDS configuration with ``build/sdcard.img``.

OpenLaunch's chainloader (menu item ``[6]``) and the ARM7 self-test talk to the
emulated DSi SD card through the TMIO controller. melonDS only constructs SD
port 0 when an image is attached (``[DSi.SD] Enable = true``); otherwise
``DSi_SDHost`` logs ``SENDING CMD ... TO NULL DEVICE``, never raises
``CMDRESPEND`` and the bounded ``tmio_wait_bits()`` poll runs to its full budget
(see ``docs/hardware_notes.md`` §23). Developers therefore had to open the GUI
and point *DSi SD card* at ``build/sdcard.img`` by hand.

This helper removes that manual step. It locates the native melonDS
configuration and rewrites the SD-card settings in place:

  * native (XDG):        ``$XDG_CONFIG_HOME/melonDS/melonDS.toml``
                         (default ``~/.config/melonDS/melonDS.toml``)
  * legacy INI fallback: ``melonDS.ini`` / ``config.ini`` in that directory

Only the native XDG location is consulted; the Flatpak sandbox
(``~/.var/app/...``) is never read or written.

Modern melonDS stores its configuration as TOML (``melonDS.toml``); the relevant
keys live in the ``[DSi.SD]`` table::

    [DSi.SD]
    Enable = true
    ImagePath = "/abs/path/to/build/sdcard.img"

Older builds read a flat ``melonDS.ini`` whose legacy keys are ``DSiSDEnable``
and ``DSiSDPath`` (see ``Config.cpp::LegacyFile`` upstream). The
``EnableDSiSDCard`` / ``DSiSDCardPath`` spellings used in some documentation are
written too so both conventions stay in sync.

The DSi console type is enabled as well (``[Emu] ConsoleType = 1``), because the
SD card only exists in DSi/TWL mode.

The script is idempotent, backs up a file before its first modification
(``<name>.openlaunch.bak``) and never touches anything outside the discovered
configuration directories. ``--isolated`` instead writes a throwaway
configuration under a caller-supplied directory, which is what the non-destructive
``make smoke-test`` flow uses.

Usage::

    python3 scripts/configure_emulator.py                 # configure real configs
    python3 scripts/configure_emulator.py --dry-run       # show what would change
    python3 scripts/configure_emulator.py --isolated /tmp/x --quiet
    python3 scripts/configure_emulator.py --check         # non-zero if not ready
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_SD = os.path.join(REPO_ROOT, "build", "sdcard.img")

TOML_NAME = "melonDS.toml"
INI_NAMES = ("melonDS.ini", "config.ini")

# Modern TOML keys (Config.cpp -> DSi.SD.* / Emu.ConsoleType).
TOML_TABLE_SD = "DSi.SD"
TOML_KEY_ENABLE = "Enable"
TOML_KEY_PATH = "ImagePath"
TOML_KEY_READONLY = "ReadOnly"
TOML_TABLE_EMU = "Emu"
TOML_KEY_CONSOLE = "ConsoleType"

# Legacy flat melonDS.ini keys (Config.cpp::LegacyFile).
INI_KEY_ENABLE = "DSiSDEnable"
INI_KEY_PATH = "DSiSDPath"
INI_KEY_CONSOLE = "ConsoleType"
# Aliases written for compatibility with older/alternate documentation.
INI_ALIAS_ENABLE = "EnableDSiSDCard"
INI_ALIAS_PATH = "DSiSDCardPath"

# Emu.ConsoleType: 0 = DS, 1 = DSi. The DSi SD card requires DSi/TWL mode.
DSI_CONSOLE_TYPE = 1

BACKUP_SUFFIX = ".openlaunch.bak"


def log(msg: str, quiet: bool = False) -> None:
    if not quiet:
        print("[config] %s" % msg, flush=True)


def warn(msg: str) -> None:
    print("[config] warning: %s" % msg, file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------
# Configuration locations
# ---------------------------------------------------------------------------
def native_config_dir() -> str:
    """Return the native melonDS configuration directory (XDG).

    Defaults to ``~/.config/melonDS`` and honours ``XDG_CONFIG_HOME``. The
    Flatpak sandbox (``~/.var/app/...``) is deliberately not consulted.
    """
    xdg = os.environ.get("XDG_CONFIG_HOME")
    if not xdg:
        xdg = os.path.join(os.path.expanduser("~"), ".config")
    return os.path.join(xdg, "melonDS")


def config_dirs() -> list:
    """Return the native melonDS configuration directory."""
    return [native_config_dir()]


def discover_targets(create: bool = True):
    """Return a list of ``(path, kind)`` config targets to update.

    ``kind`` is ``"toml"`` or ``"ini"``. When ``create`` is true, a directory
    that exists but holds no config is given a fresh ``melonDS.toml``; when no
    config directory exists at all, the native one is created.
    """
    targets = []
    for d in config_dirs():
        if not os.path.isdir(d):
            continue
        toml = os.path.join(d, TOML_NAME)
        if os.path.isfile(toml):
            targets.append((toml, "toml"))
            continue
        ini = None
        for name in INI_NAMES:
            candidate = os.path.join(d, name)
            if os.path.isfile(candidate):
                ini = candidate
                break
        if ini is not None:
            targets.append((ini, "ini"))
        elif create:
            targets.append((toml, "toml"))

    if not targets and create:
        targets.append((os.path.join(native_config_dir(), TOML_NAME), "toml"))
    return targets


# ---------------------------------------------------------------------------
# Text editing helpers (format preserving)
# ---------------------------------------------------------------------------
def read_text(path: str):
    """Read ``path``; return ``(text, crlf)`` preserving the line ending style."""
    with open(path, "r", encoding="utf-8", errors="surrogateescape") as fh:
        data = fh.read()
    return data, ("\r\n" in data)


def write_text(path: str, text: str, crlf: bool) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    if crlf:
        text = text.replace("\n", "\r\n")
    with open(path, "w", encoding="utf-8", errors="surrogateescape") as fh:
        fh.write(text)


def toml_quote(value: str) -> str:
    """Quote a filesystem path as a TOML basic string."""
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def _toml_ensure_table(text: str, table: str):
    """Ensure ``[table]`` is present as an *explicit* header.

    melonDS (toml11) aborts on save when a table such as ``DSi`` was only
    created implicitly by a dotted subtable header (``[DSi.SD]``) and later
    receives scalar children at runtime. Emitting an explicit empty header marks
    it as a real table, which serializes cleanly. Returns ``(text, changed)``.
    """
    lines = text.split("\n")
    header = "[%s]" % table
    sub_prefix = "[%s." % table
    for line in lines:
        if line.strip() == header:
            return text, False
    insert_at = None
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith(sub_prefix) and stripped.endswith("]"):
            insert_at = i
            break
    if insert_at is None:
        if lines and lines[-1].strip() != "":
            lines.append("")
        lines.append(header)
        return "\n".join(lines), True
    lines.insert(insert_at, header)
    return "\n".join(lines), True


def _toml_set(text: str, table: str, key: str, value_literal: str):
    """Set ``key = value_literal`` inside ``[table]`` of a TOML document.

    The edit is textual so comments, key order and formatting are preserved.
    Returns ``(new_text, changed)``.
    """
    lines = text.split("\n")
    header = "[%s]" % table

    start = None
    for i, line in enumerate(lines):
        if line.strip() == header:
            start = i
            break

    if start is None:
        if lines and lines[-1].strip() != "":
            lines.append("")
        lines.append(header)
        lines.append("%s = %s" % (key, value_literal))
        return "\n".join(lines), True

    end = len(lines)
    for j in range(start + 1, len(lines)):
        stripped = lines[j].strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            end = j
            break

    for j in range(start + 1, end):
        raw = lines[j]
        if "=" not in raw:
            continue
        if raw.split("=", 1)[0].strip() == key:
            indent = raw[: len(raw) - len(raw.lstrip())]
            new_line = "%s%s = %s" % (indent, key, value_literal)
            if raw == new_line:
                return "\n".join(lines), False
            lines[j] = new_line
            return "\n".join(lines), True

    insert_at = end
    while insert_at > start + 1 and lines[insert_at - 1].strip() == "":
        insert_at -= 1
    lines.insert(insert_at, "%s = %s" % (key, value_literal))
    return "\n".join(lines), True


def _ini_set(text: str, key: str, value: str):
    """Set a flat ``key=value`` line in a legacy melonDS INI. Returns (text, changed)."""
    lines = text.split("\n")
    trailing = ""
    if lines and lines[-1] == "":
        lines = lines[:-1]
        trailing = "\n"
    pattern = re.compile(r"^\s*%s\s*=" % re.escape(key), re.IGNORECASE)
    for i, line in enumerate(lines):
        if pattern.match(line):
            new_line = "%s=%s" % (key, value)
            if line == new_line:
                return text, False
            lines[i] = new_line
            return "\n".join(lines) + trailing, True
    lines.append("%s=%s" % (key, value))
    return "\n".join(lines) + trailing, True


def update_toml(text: str, sd_path: str, set_console: bool = True,
                read_only: bool = False):
    """Apply the OpenLaunch SD-card settings to a melonDS TOML document.

    ``read_only`` defaults to False: the guarded installer writes its mandatory
    NANDBACK.BIN backup through the DSi SD card, and melonDS silently drops
    writes to a read-only card. Pass ``read_only=True`` to opt back in.
    """
    changed = False
    # The parent [DSi] table must be explicit (see _toml_ensure_table).
    text, step = _toml_ensure_table(text, "DSi")
    changed = changed or step
    for key, literal in ((TOML_KEY_ENABLE, "true"),
                         (TOML_KEY_PATH, toml_quote(sd_path)),
                         (TOML_KEY_READONLY, "true" if read_only else "false")):
        text, step = _toml_set(text, TOML_TABLE_SD, key, literal)
        changed = changed or step
    if set_console:
        text, step = _toml_set(text, TOML_TABLE_EMU, TOML_KEY_CONSOLE,
                               str(DSI_CONSOLE_TYPE))
        changed = changed or step
    return text, changed


def update_ini(text: str, sd_path: str, set_console: bool = True):
    """Apply the OpenLaunch SD-card settings to a legacy melonDS INI document.

    Both melonDS' real legacy keys (``DSiSDEnable`` / ``DSiSDPath``) and the
    ``EnableDSiSDCard`` / ``DSiSDCardPath`` spellings used in older
    documentation are written; melonDS' legacy parser ignores unknown keys, so
    the extra pair is harmless and keeps both conventions in sync.
    """
    changed = False
    for key, value in ((INI_KEY_ENABLE, "1"),
                       (INI_KEY_PATH, sd_path),
                       (INI_ALIAS_ENABLE, "1"),
                       (INI_ALIAS_PATH, sd_path)):
        text, step = _ini_set(text, key, value)
        changed = changed or step
    if set_console:
        text, step = _ini_set(text, INI_KEY_CONSOLE, str(DSI_CONSOLE_TYPE))
        changed = changed or step
    return text, changed


def apply_target(path: str, kind: str, sd_path: str, *, create: bool,
                 set_console: bool, read_only: bool, backup: bool, dry_run: bool,
                 quiet: bool) -> bool:
    """Update one config file. Returns True when a change is (or would be) made."""
    existed = os.path.isfile(path)
    if not existed and not create:
        return False

    if existed:
        text, crlf = read_text(path)
    else:
        text, crlf = "", False

    if kind == "toml":
        new_text, changed = update_toml(text, sd_path, set_console, read_only)
    else:
        new_text, changed = update_ini(text, sd_path, set_console)

    if not existed:
        header = ("# Generated by scripts/configure_emulator.py for OpenLaunch.\n"
                  "# Points melonDS at build/sdcard.img so no manual GUI setup is\n"
                  "# required. Safe to edit; re-run `make configure-emulator`.\n")
        new_text = header + new_text.lstrip("\n")
        changed = True
    if changed and new_text and not new_text.endswith("\n"):
        new_text += "\n"

    if existed and not changed:
        log("already configured: %s" % path, quiet)
        return False

    if dry_run:
        log("%s: %s" % ("would create" if not existed else "would update", path),
            quiet)
        return True

    if existed and backup:
        backup_path = path + BACKUP_SUFFIX
        try:
            shutil.copy2(path, backup_path)
            log("backed up %s -> %s" % (path, backup_path), quiet)
        except OSError as exc:
            warn("could not back up %s: %s" % (path, exc))

    write_text(path, new_text, crlf)
    log("%s %s" % ("created" if not existed else "updated", path), quiet)
    return True


# ---------------------------------------------------------------------------
# Isolated configuration (used by scripts/smoke_test_emu.py)
# ---------------------------------------------------------------------------
def write_isolated_config(config_home: str, sd_path: str,
                          read_only: bool = False) -> str:
    """Write a throwaway melonDS config under ``config_home`` and return its path.

    This never reads or writes the user's real melonDS configuration; it is the
    same layout ``configure_emulator`` would produce, so the smoke test exercises
    the exact settings the helper installs. The SD card is writable by default so
    the installer's SD backup can persist.
    """
    cfg_dir = os.path.join(config_home, "melonDS")
    os.makedirs(cfg_dir, exist_ok=True)
    cfg_path = os.path.join(cfg_dir, TOML_NAME)
    lines = [
        "# OpenLaunch isolated melonDS configuration.",
        "# Generated by scripts/configure_emulator.py; the user's real melonDS",
        "# configuration is never modified.",
        "[%s]" % TOML_TABLE_EMU,
        "%s = %d" % (TOML_KEY_CONSOLE, DSI_CONSOLE_TYPE),
        "DirectBoot = true",
        "",
        # The parent [DSi] table must be explicit or melonDS aborts on save.
        "[DSi]",
        "",
        "[%s]" % TOML_TABLE_SD,
        "%s = true" % TOML_KEY_ENABLE,
        "%s = %s" % (TOML_KEY_PATH, toml_quote(sd_path)),
        "%s = %s" % (TOML_KEY_READONLY, "true" if read_only else "false"),
        "",
    ]
    with open(cfg_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    return cfg_path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sd", default=DEFAULT_SD,
                    help="path to the SD image (default build/sdcard.img)")
    ap.add_argument("--config", action="append", default=[], metavar="PATH",
                    help="explicit config file to update (repeatable)")
    ap.add_argument("--isolated", metavar="DIR",
                    help="write an isolated config under DIR instead of "
                         "touching the real configs")
    ap.add_argument("--dry-run", action="store_true",
                    help="report what would change without writing")
    ap.add_argument("--no-create", action="store_true",
                    help="do not create a config when none exists")
    ap.add_argument("--no-backup", action="store_true",
                    help="do not write a .openlaunch.bak before updating")
    ap.add_argument("--no-console-type", action="store_true",
                    help="leave Emu.ConsoleType untouched")
    ap.add_argument("--read-only", action="store_true",
                    help="mark the emulated DSi SD card read-only (default is "
                         "writable so the installer's SD backup can persist)")
    ap.add_argument("--check", action="store_true",
                    help="exit non-zero unless every target is already set")
    ap.add_argument("--quiet", action="store_true",
                    help="only print warnings and errors")
    args = ap.parse_args(argv)

    sd_path = os.path.abspath(args.sd)

    if args.isolated:
        cfg = write_isolated_config(args.isolated, sd_path, args.read_only)
        log("wrote isolated config %s" % cfg, args.quiet)
        return 0

    sd_missing = not os.path.isfile(sd_path)
    if sd_missing:
        warn("SD image not found: %s (build it with `make sd-image`)" % sd_path)

    if args.config:
        targets = []
        for path in args.config:
            kind = "toml" if path.lower().endswith(".toml") else "ini"
            targets.append((os.path.abspath(path), kind))
    else:
        targets = discover_targets(create=not args.no_create)

    if not targets:
        log("no melonDS configuration directory found; nothing to do",
            args.quiet)
        return 1 if (args.check and sd_missing) else 0

    if not args.quiet:
        log("SD image: %s" % sd_path)

    changed_any = False
    pending = sd_missing
    for path, kind in targets:
        if args.check:
            existed = os.path.isfile(path)
            if not existed:
                warn("missing config: %s" % path)
                pending = True
                continue
            text, _ = read_text(path)
            _, changed = (update_toml(text, sd_path, not args.no_console_type,
                                      args.read_only)
                          if kind == "toml"
                          else update_ini(text, sd_path, not args.no_console_type))
            if changed:
                warn("not configured: %s" % path)
                pending = True
            else:
                log("already configured: %s" % path, args.quiet)
            continue
        changed_any |= apply_target(
            path, kind, sd_path,
            create=not args.no_create,
            set_console=not args.no_console_type,
            read_only=args.read_only,
            backup=not args.no_backup,
            dry_run=args.dry_run,
            quiet=args.quiet)

    if args.check:
        return 1 if pending else 0

    if not changed_any:
        log("all melonDS configs already point at %s" % sd_path, args.quiet)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
