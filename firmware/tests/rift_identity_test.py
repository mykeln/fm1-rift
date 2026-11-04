#!/usr/bin/env python3
"""Check the actual update packages, which M-UPGRADE compares before flashing."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from fm1_install import product_of, model_of


def main():
    previous, current = (Path(p) for p in sys.argv[1:3])
    old = product_of(previous.read_bytes())
    new = product_of(current.read_bytes())
    print(f"installed release package: {old}")
    print(f"new release package:       {new}")
    assert model_of(old) == model_of(new) == "FM-1", "Package must retain the FM-1 hardware model"
    assert old != new, "M-UPGRADE rejects these packages as the same device version"
    assert int(new.rsplit("_", 1)[1]) > int(old.rsplit("_", 1)[1]), "Update revision must increase"
    if len(sys.argv) > 3:
        app = Path(sys.argv[3]).read_bytes()
        assert new.encode() + b"\0" in app, "Running firmware must report the same ID as its package"
        print("compiled firmware reports the new package identity: ok")
    print("distinct FM-1 update identities: ok")


if __name__ == "__main__":
    main()
