#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Collect a local review bundle. Never opens a MIDI/USB port."""
import hashlib
import json
import shutil
import subprocess
import tarfile
import sys
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / "firmware"
OUT = ROOT / "dist"
VERSION = "0.6"
sys.path.insert(0, str(FW / "tools"))
from fm1_install import product_of


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def main():
    package = FW / "build/felucca.fwsc"
    if not package.is_file():
        raise SystemExit("Build the FM1 package first (sh rift.sh build).")
    identity = product_of(package.read_bytes())
    expected = re.search(r'^#define RIFT_UPDATE_ID "([^"]+)"$',
                         (FW / "firmware/src/rift_update_id.h").read_text(), re.M)
    if not expected or identity != expected[1]:
        raise SystemExit("Package identity differs from RIFT source; rebuild before packaging.")
    OUT.mkdir(exist_ok=True)
    shutil.copy2(package, OUT / f"rift-{VERSION}.fwsc")
    shutil.copy2(FW / "docs/RIFT.md", OUT / "RIFT.md")
    for name in ("LICENSE", "LICENSING.md"):
        shutil.copy2(ROOT / name, OUT / name)
    shutil.copy2(FW / "LICENSING.md", OUT / "FELUCCA-LICENSING.md")
    shutil.copytree(FW / "LICENSES", OUT / "LICENSES", dirs_exist_ok=True)
    for subdir, pattern in (("audio", "*.wav"), ("screens", "*.png")):
        dest = OUT / subdir
        dest.mkdir(exist_ok=True)
        for source in (FW / "build/rift-demo").glob(pattern):
            shutil.copy2(source, dest / source.name)
    long_demo = FW / "build/audio-space/CAVERN-base.wav"
    if long_demo.is_file():
        shutil.copy2(long_demo, OUT / "audio/10-CAVERN-LONG.wav")
    moving_demo = FW / "build/audio-space/CAVERN-evolving.wav"
    if moving_demo.is_file():
        shutil.copy2(moving_demo, OUT / "audio/11-CAVERN-EVOLVING.wav")
    palette = FW / "build/audio-palette"
    if palette.is_dir():
        dest = OUT / "audio/palette"
        dest.mkdir(parents=True, exist_ok=True)
        for source in palette.glob("*.wav"):
            shutil.copy2(source, dest / source.name)
        response = palette / "palette-response.json"
        if response.is_file():
            shutil.copy2(response, OUT / "palette-response.json")
    for name in ("rift-full-test.log", "rift-build.log", "rift-test.log", "rift-controls-test.log", "rift-response-test.log", "rift-space-test.log", "rift-idle-test.log", "rift-palette-test.log", "rift-persistence-test.log", "rift-identity-test.log", "install-test.log", "project-test.log"):
        source = FW / "build" / name
        if source.is_file():
            shutil.copy2(source, OUT / name)
    # Include both pinned dependencies; GitHub's automatic archives omit submodules.
    # Include new local source files too; source_dirty records edits.
    source_archive = OUT / f"rift-{VERSION}-source.tar.gz"
    names = set(git("ls-files", "--recurse-submodules", "-z").decode().split("\0"))
    names.update(git("ls-files", "--others", "--exclude-standard", "-z").decode().split("\0"))
    with tarfile.open(source_archive, "w:gz") as archive:
        for name in sorted(filter(None, names)):
            archive.add(ROOT / name, arcname=f"rift-{VERSION}/{name}", recursive=False)
    manifest = {
        "name": "RIFT", "version": VERSION, "status": "experimental; host tested; this update not installed by agent",
        "base_repository": "https://github.com/hugelton/Felucca",
        "base_commit": "743551712c36b3984392eea1dbe13a309907e17a",
        "source_commit": git("rev-parse", "HEAD").decode().strip(),
        "source_dirty": bool(git("status", "--porcelain")),
        "submodules": git("submodule", "status").decode().strip().splitlines(),
        "package_identity": identity, "connected_device_version_read": "RIFT 0.4 (USB CDC)",
        "hardware_observation": {"version": "RIFT 0.4", "cpu_pct_with_usb_capture": 73,
                                 "audio_late_delta": 9, "uac_underruns": 14, "voices_shed_delta": 0,
                                 "finding": "0.4 allowed background flash saves during drone/tail; 0.5 guards are host tested, hardware pending",
                                 "audio_capture": "Felucca USB audio; original live patch and selected track restored and verified"},
        "firmware_writes_performed_by_agent": False,
        "palette_validation": "27 sustained factory renders; all 13 dials in all 9 modes; subjective listening and 0.6 hardware timing pending",
        "build": {"command": "sh rift.sh build", "sdk_tag": "AC79NN_SDK_V1.2.1_2023-12-13",
                  "toolchain": "jieli-linux-toolchains-20260730.1", "docker_context": "colima-fm1-build"},
        "package_sha256": hashlib.sha256(package.read_bytes()).hexdigest(),
        "source_archive_sha256": hashlib.sha256(source_archive.read_bytes()).hexdigest(),
    }
    (OUT / "build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    sums = []
    for source in sorted(OUT.rglob("*")):
        if source.is_file() and source.name != "SHA256SUMS":
            sums.append(f"{hashlib.sha256(source.read_bytes()).hexdigest()}  {source.relative_to(OUT)}")
    (OUT / "SHA256SUMS").write_text("\n".join(sums) + "\n")
    print(f"Review bundle: {OUT}\nFirmware SHA-256: {manifest['package_sha256']}")


if __name__ == "__main__":
    main()
