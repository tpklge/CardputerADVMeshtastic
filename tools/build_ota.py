#!/usr/bin/env python3
"""Compile and prepare an authorized OTA publication or milestone candidate. Never connects to a device."""
import argparse
import hashlib
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess
import sys

from package_ota import package_application
from prepare_upstream import ROOT, verify_sources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--release",action="store_true",help="prepare the authorized publication, keeping unknown installation limits explicit")
    mode.add_argument("--diagnostic", action="store_true",
                        help="allow a development candidate with unverified installed launcher layout")
    parser.add_argument("--milestone", choices=("M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8"), default="M8")
    args = parser.parse_args()
    if args.release and args.milestone!="M8":
        parser.error("The release uses the complete M8 application.")
    lock = verify_sources()
    environment = "cardputer-adv-" + args.milestone.lower()
    version = {"M1":"0.1.1-m1", "M2":"0.2.1-m2", "M3":"0.3.0-m3", "M4":"0.4.0-m4", "M5":"0.5.0-m5", "M6":"0.6.0-m6", "M7":"0.7.2-m7", "M8":"1.0.1"}[args.milestone]
    pio = Path(sys.executable).with_name("pio")
    if not pio.exists():
        located = shutil.which("pio")
        if not located:
            raise RuntimeError("PlatformIO not found; install requirements-build.txt in .venv.")
        pio = Path(located)
    subprocess.run([str(pio), "run", "-e", environment], cwd=ROOT, check=True)
    contract = json.loads((ROOT / "config/ota_contract.json").read_text())
    inventory = subprocess.check_output(
        [str(pio), "pkg", "list", "-e", environment], cwd=ROOT, text=True)
    sources = sorted(path for directory in ("src", "include", "tools")
                     for path in (ROOT / directory).rglob("*")
                     if path.suffix in (".cpp", ".hpp", ".h", ".py"))
    sources += [ROOT / "platformio.ini", ROOT / "upstream.lock.json"]
    source_digest = hashlib.sha256()
    for path in sources:
        source_digest.update(str(path.relative_to(ROOT)).encode() + b"\0" + path.read_bytes())
    metadata = {"version": version, "milestone": "release" if args.release else args.milestone,
                "publication_authorized": args.release,
                "last_user_confirmed_version": "0.8.2-m8" if args.release else None,
                "mesh_runtime_enabled": args.milestone in ("M2", "M3", "M4", "M5", "M6", "M7", "M8"), "built_at_utc": datetime.now(timezone.utc).isoformat(),
                "upstream": lock, "resolved_package_report": inventory,
                "source_sha256": source_digest.hexdigest(),
                "host_requirements_sha256": hashlib.sha256((ROOT / "requirements-build.txt").read_bytes()).hexdigest(),
                "application_schema": 1 if args.milestone in ("M3", "M4", "M5", "M6", "M7", "M8") else None, "build_layout": "default_8MB.csv (offline reference only)"}
    manifest = package_application(ROOT / ".pio/build" / environment / "firmware.bin",
                                   ROOT / "newversion" / ("m7-0.7.2" if args.milestone=="M7" else (version if args.release else "m8-"+version) if args.milestone=="M8" else args.milestone.lower()), contract, metadata, diagnostic=args.diagnostic)
    print(f"OTA application: {manifest['filename']} ({manifest['image']['bytes']} bytes)")
    if not manifest["installation_compatibility_verified"]:
        print("Installed launcher slot/version still unverified; no device was accessed.")


if __name__ == "__main__":
    main()
