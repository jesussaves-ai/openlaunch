#!/usr/bin/env python3
"""Build the reproducible OPENUNLAUNCH release bundle.

This script produces ``build/release/`` with every production variant of the
bootloader plus a ready-to-copy SD card template, a deterministic
``SHA256SUMS.txt`` and a deterministic ``build/dsi_bootloader_release.tar.gz``:

  ==========================  ===============================================
  Artifact                    Build
  ==========================  ===============================================
  dsi_bootloader_diag.nds     default (safe dry-run / diagnostic)
  dsi_bootloader.nds          CHAINLOAD_BOOT=1 (live chainload on A / [8])
  BOOT.NDS                    CHAINLOAD_BOOT=1 AUTOBOOT=1 (drop-in autoboot)
  BOOT.CFG                    sample SD-root hotkey map (DEFAULT target)
  NDS/HELLO.NDS               the standalone chainload test payload
  sdcard.img                  the pre-built 16 MiB FAT16 melonDS test image
  SHA256SUMS.txt              deterministic SHA-256 of every artifact above
  ==========================  ===============================================

Each variant is compiled in its own ``build/_release_work/<key>`` directory (so
the default ``build/`` tree is left untouched), then staged under
``build/release/``. The script re-reads every staged artifact and verifies the
generated ``SHA256SUMS.txt`` before returning.

Run with ``make release``. The toolchain selection variables (``CROSS``,
``DEVKITARM``) flow through the environment to the recursive ``make`` calls.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tarfile

# --------------------------------------------------------------------------
# Layout
# --------------------------------------------------------------------------
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RELEASE_DIR = os.path.join(REPO_ROOT, "build", "release")
WORK_DIR = os.path.join(REPO_ROOT, "build", "_release_work")
TARBALL = os.path.join(REPO_ROOT, "build", "dsi_bootloader_release.tar.gz")
TARBALL_PREFIX = "dsi_bootloader_release"

SUMS_NAME = "SHA256SUMS.txt"

# name -> (make variables, staging file name)
VARIANTS = (
    ("diag", (), "dsi_bootloader_diag.nds"),
    ("chainload", ("CHAINLOAD_BOOT=1",), "dsi_bootloader.nds"),
    ("autoboot", ("CHAINLOAD_BOOT=1", "AUTOBOOT=1"), "BOOT.NDS"),
)

# Sample BOOT.CFG written to the SD-card root. It must not point at BOOT.NDS
# (that is the bootloader itself); it targets the bundled test payload instead.
RELEASE_BOOT_CFG = (
    b"# OPENUNLAUNCH release configuration (SD card root)\r\n"
    b"#\r\n"
    b"# Hold a mapped button at startup to select a target; B or SELECT always\r\n"
    b"# forces the interactive menu. Point DEFAULT at your own .nds image.\r\n"
    b"# Additional bindings such as A=/X=/Y=/L=/R=/START= may be added.\r\n"
    b"DEFAULT=NDS/HELLO.NDS\r\n"
)


def log(msg: str) -> None:
    print("[release] %s" % msg, flush=True)


def run_make(build_dir: str, *targets: str) -> None:
    """Run a recursive `make` for one variant in its own build directory."""
    cmd = ["make", "BUILD=%s" % build_dir, *targets]
    log("$ %s" % " ".join(cmd))
    subprocess.run(cmd, cwd=REPO_ROOT, check=True)


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def write_sums(release_dir: str) -> list[str]:
    """Write SHA256SUMS.txt for every artifact (sorted, deterministic)."""
    names = []
    for root, dirs, files in os.walk(release_dir):
        dirs.sort()
        for f in sorted(files):
            if f == SUMS_NAME:
                continue
            rel = os.path.relpath(os.path.join(root, f), release_dir)
            names.append(rel.replace(os.sep, "/"))
    names.sort()

    lines = []
    for rel in names:
        digest = sha256_file(os.path.join(release_dir, rel))
        lines.append("%s  %s" % (digest, rel))
    with open(os.path.join(release_dir, SUMS_NAME), "w", encoding="ascii",
              newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    return names


def verify_sums(release_dir: str) -> int:
    """Re-read SHA256SUMS.txt and confirm every listed digest matches."""
    path = os.path.join(release_dir, SUMS_NAME)
    count = 0
    with open(path, "r", encoding="ascii") as fh:
        for line in fh:
            line = line.rstrip("\n")
            if not line:
                continue
            digest, rel = line.split("  ", 1)
            actual = sha256_file(os.path.join(release_dir, rel))
            if actual != digest:
                print("  [FAIL] %s: %s != %s" % (rel, actual, digest))
                return -1
            count += 1
    return count


def make_tarball(release_dir: str, out_path: str) -> None:
    """Create a deterministic tar.gz (zeroed metadata, fixed mtime)."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w", format=tarfile.GNU_FORMAT) as tar:
        for root, dirs, files in os.walk(release_dir):
            dirs.sort()
            for f in sorted(files):
                full = os.path.join(root, f)
                rel = os.path.relpath(full, release_dir).replace(os.sep, "/")
                info = tar.gettarinfo(full, arcname="%s/%s" % (TARBALL_PREFIX, rel))
                info.uid = 0
                info.gid = 0
                info.uname = ""
                info.gname = ""
                info.mtime = 0
                info.mode = 0o644
                with open(full, "rb") as fh:
                    tar.addfile(info, fh)
    raw = buf.getvalue()
    with open(out_path, "wb") as fh:
        with gzip.GzipFile(fileobj=fh, mode="wb", mtime=0) as gz:
            gz.write(raw)


def build(release_dir: str, work_dir: str) -> None:
    if os.path.isdir(release_dir):
        shutil.rmtree(release_dir)
    os.makedirs(os.path.join(release_dir, "NDS"), exist_ok=True)
    if os.path.isdir(work_dir):
        shutil.rmtree(work_dir)

    # 1. The three bootloader variants, each in an isolated build directory.
    for key, vars_, staged_name in VARIANTS:
        build_dir = os.path.join(work_dir, key)
        shutil.rmtree(build_dir, ignore_errors=True)
        run_make(build_dir, "rom", *vars_)
        shutil.copy2(os.path.join(build_dir, "dsi_bootloader.nds"),
                     os.path.join(release_dir, staged_name))
        log("staged %s" % staged_name)

    # 2. The chainload test payload + deterministic FAT16 SD image.
    sd_dir = os.path.join(work_dir, "sd")
    shutil.rmtree(sd_dir, ignore_errors=True)
    run_make(sd_dir, "sd-image")
    shutil.copy2(os.path.join(sd_dir, "hello_payload.nds"),
                 os.path.join(release_dir, "NDS", "HELLO.NDS"))
    shutil.copy2(os.path.join(sd_dir, "sdcard.img"),
                 os.path.join(release_dir, "sdcard.img"))
    log("staged NDS/HELLO.NDS and sdcard.img")

    # 3. Sample configuration.
    with open(os.path.join(release_dir, "BOOT.CFG"), "wb") as fh:
        fh.write(RELEASE_BOOT_CFG)
    log("staged BOOT.CFG")


def main(argv) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--release-dir", default=RELEASE_DIR)
    ap.add_argument("--work-dir", default=WORK_DIR)
    ap.add_argument("--tarball", default=TARBALL)
    ap.add_argument("--keep-work", action="store_true",
                    help="keep the per-variant build directories for debugging")
    ap.add_argument("--no-archive", action="store_true",
                    help="skip the .tar.gz bundle")
    args = ap.parse_args(argv)

    build(args.release_dir, args.work_dir)

    names = write_sums(args.release_dir)
    log("wrote %s (%d files)" % (SUMS_NAME, len(names)))

    checked = verify_sums(args.release_dir)
    if checked < 0:
        print("[release] SHA256SUMS verification FAILED")
        return 1
    log("verified %s: %d digests OK" % (SUMS_NAME, checked))

    if not args.no_archive:
        make_tarball(args.release_dir, args.tarball)
        log("wrote %s (%d bytes)" % (args.tarball,
                                     os.path.getsize(args.tarball)))

    if not args.keep_work:
        shutil.rmtree(args.work_dir, ignore_errors=True)

    log("release ready in %s" % args.release_dir)
    for rel in names:
        print("    %s" % rel)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
