#!/usr/bin/env python3
"""Describe a signed firmware image for devices to find.

Writes `firmware.json` next to the image: the version, where to fetch it, how
big it is, what it hashes to, and an ed25519 signature over that hash made with
the *bundle* key. See docs/firmware_update.md.

Two signatures cover an update and this makes the first of them. The second is
MCUboot's, applied by imgtool during the build with a different key, and it is
what gates execution. This one lets a device refuse a bad image before spending
a download and a reboot on it, and it is the same statement — "the project
published this and it is current" — that a question bundle's signature makes,
which is why it uses the same key.

The image must be the *signed* one, `zephyr.signed.bin`. Publishing the
unsigned `zephyr.bin` produces a manifest that verifies perfectly and an image
MCUboot will not install, so this refuses anything without an MCUboot header.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

from build_bundle import sign_digest

# "Magic" in an MCUboot image header, little-endian at offset 0. An unsigned
# zephyr.bin starts with the SoC's own image header instead, so this is what
# tells the two apart.
MCUBOOT_MAGIC = 0x96F3B83D


def has_mcuboot_header(raw: bytes) -> bool:
    if len(raw) < 4:
        return False
    return int.from_bytes(raw[:4], "little") == MCUBOOT_MAGIC


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", type=Path, help="path to zephyr.signed.bin")
    parser.add_argument(
        "--version", required=True, help="release version, e.g. 0.2.0 — must match the VERSION file"
    )
    parser.add_argument(
        "--out", type=Path, default=None, help="output directory (default: <root>/dist/firmware)"
    )
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument(
        "--sign-key",
        type=Path,
        default=None,
        help="ed25519 private key PEM; omit for an unsigned dev manifest",
    )
    parser.add_argument("--base-url", default="http://tischkarte.invalid/device")
    args = parser.parse_args(argv)

    raw = args.image.read_bytes()

    if not has_mcuboot_header(raw):
        print(
            f"{args.image} has no MCUboot header — this is the unsigned image, and a "
            "device would download it, verify it, and then fail to boot it. Use "
            "zephyr.signed.bin.",
            file=sys.stderr,
        )
        return 1

    out_dir = args.out or args.root / "dist" / "firmware"
    out_dir.mkdir(parents=True, exist_ok=True)

    name = f"tischkarte-{args.version}.bin"
    shutil.copyfile(args.image, out_dir / name)

    # Over the bytes the device actually verifies, which are the bytes it is
    # told to fetch. The same test that guards the bundle pipeline guards this:
    # url, size and sha256 all have to describe one file.
    digest = hashlib.sha256(raw).digest()
    sig = sign_digest(digest, args.sign_key) if args.sign_key else None

    if sig is None:
        print(
            f"warning: {name} is unsigned (no --sign-key); no device will install it",
            file=sys.stderr,
        )

    manifest = {
        "schema": 1,
        "version": args.version,
        "url": f"{args.base_url}/{name}",
        "size": len(raw),
        "sha256": digest.hex(),
        "sig": sig,
    }

    (out_dir / "firmware.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print(f"{name}: {len(raw)} bytes, version {args.version}")
    print(f"firmware.json: {out_dir / 'firmware.json'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
