#!/usr/bin/env python3
"""Prepare and verify pinned sources. Does not access a device."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def git(*args, cwd=ROOT):
    return subprocess.check_output(["git", *args], cwd=cwd, text=True).strip()


def verify_sources():
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    repo = ROOT / "vendor/meshtastic-firmware"
    if git("rev-parse", "HEAD", cwd=repo) != lock["firmware"]["commit"]:
        raise RuntimeError("Unexpected firmware revision; refusing to build.")
    if git("status", "--porcelain", "--untracked-files=no", cwd=repo):
        raise RuntimeError("Upstream tracked files are modified; refusing to build.")
    for name, commit in lock["submodules"].items():
        tree = git("ls-tree", "HEAD", name, cwd=repo).split()
        if len(tree) < 3 or tree[2] != commit:
            raise RuntimeError(f"Submodule gitlink changed: {name}")
        if name in lock["required_submodules"]:
            if git("rev-parse", "HEAD", cwd=repo / name) != commit:
                raise RuntimeError(f"Unexpected {name} revision")
            if git("status", "--porcelain", "--untracked-files=no", cwd=repo / name):
                raise RuntimeError(f"Modified submodule: {name}")
    return lock


def prepare():
    lock = json.loads((ROOT / "upstream.lock.json").read_text())
    repo = ROOT / "vendor/meshtastic-firmware"
    if not repo.exists():
        subprocess.run(["git", "clone", "--depth", "1", "--branch",
                        lock["firmware"]["tag"], lock["firmware"]["repository"],
                        str(repo)], check=True)
    if git("rev-parse", "HEAD", cwd=repo) != lock["firmware"]["commit"]:
        raise RuntimeError("Existing checkout differs from lock; preserve it and inspect manually.")
    for name in lock["required_submodules"]:
        subprocess.run(["git", "submodule", "update", "--init", name], cwd=repo, check=True)
    verify_sources()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify existing checkout only")
    args = parser.parse_args()
    if args.check:
        verify_sources()
    else:
        prepare()
    print("Pinned Meshtastic sources verified.")
