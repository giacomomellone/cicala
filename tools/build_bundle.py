#!/usr/bin/env python3
"""Build per-language device bundles (.qdb.gz) plus the sync manifest.

Bundle format (docs/sync_protocol.md has the worked example) — gzip of:

  magic        4 bytes  "QDB2"
  version      u8 length + UTF-8 bytes (e.g. "2026.07.1")
  lang         u8 length + UTF-8 bytes (e.g. "en")
  count        u16 LE
  per question:
    deck mask  u8; bits follow the fixed deck order in questions/schema.json
    metadata   u8; bits 0–1 = depth - 1, bit 2 = spicy, bit 3 = dark
    text       u16 LE length + UTF-8 bytes

All integers little-endian. Signing: ed25519 over the bundle's SHA-256 digest,
via the system openssl (no Python crypto dependency); the private key lives in
a GitHub Actions secret, the public key in firmware/components/sync/trusted_key.h.
Without --sign-key the manifest carries "sig": null (unsigned dev build).
"""

from __future__ import annotations

import argparse
import base64
import gzip
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate  # noqa: E402
from build_site_data import git_version  # noqa: E402


def build_bundle_bytes(lang: str, version: str, questions: list[dict], decks: list[str]) -> bytes:
    out = bytearray()
    out += b"QDB2"
    for s in (version, lang):
        raw = s.encode("utf-8")
        out += struct.pack("<B", len(raw)) + raw
    out += struct.pack("<H", len(questions))
    for entry in questions:
        mask = sum(1 << decks.index(deck) for deck in entry["decks"])
        tags = entry.get("tags", [])
        metadata = entry["depth"] - 1
        metadata |= (1 << 2) if "spicy" in tags else 0
        metadata |= (1 << 3) if "dark" in tags else 0
        text = entry["text"].encode("utf-8")
        out += struct.pack("<BBH", mask, metadata, len(text)) + text
    return bytes(out)


def compress_bundle(raw: bytes) -> bytes:
    """The .qdb.gz published for the website. Deterministic, so mtime is zeroed."""
    return gzip.compress(raw, mtime=0)


def parse_bundle(blob: bytes):
    """Inverse of build_bundle_bytes, for tests and debugging.

    Takes either shape: the raw binary a device downloads, or the gzip around
    it that the website publishes.
    """
    raw = blob if blob[:4] == b"QDB2" else gzip.decompress(blob)
    if raw[:4] != b"QDB2":
        raise ValueError("bad magic")
    pos = 4

    def take_str8():
        nonlocal pos
        n = raw[pos]
        pos += 1
        s = raw[pos : pos + n].decode("utf-8")
        pos += n
        return s

    version, lang = take_str8(), take_str8()
    decks = json.loads(
        (Path(__file__).resolve().parent.parent / "questions" / "schema.json").read_text()
    )["x-tischkarte"]["decks"]
    (count,) = struct.unpack_from("<H", raw, pos)
    pos += 2
    items = []
    for _ in range(count):
        mask, metadata, tlen = struct.unpack_from("<BBH", raw, pos)
        pos += 4
        text = raw[pos : pos + tlen].decode("utf-8")
        pos += tlen
        items.append(
            {
                "text": text,
                "decks": [deck for bit, deck in enumerate(decks) if mask & (1 << bit)],
                "depth": (metadata & 0b11) + 1,
                "spicy": bool(metadata & (1 << 2)),
                "dark": bool(metadata & (1 << 3)),
            }
        )
    result = {"version": version, "lang": lang, "questions": items}
    if pos != len(raw):
        raise ValueError(f"{len(raw) - pos} trailing bytes")
    return result


def sign_digest(digest: bytes, key_path: Path) -> str:
    with tempfile.NamedTemporaryFile() as tf:
        tf.write(digest)
        tf.flush()
        out = subprocess.run(
            ["openssl", "pkeyutl", "-sign", "-inkey", str(key_path), "-rawin", "-in", tf.name],
            capture_output=True,
            check=True,
        )
    return base64.b64encode(out.stdout).decode("ascii")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument(
        "--out", type=Path, default=None, help="output directory (default: <root>/dist/bundles)"
    )
    parser.add_argument("--version", default=None, help="release version (default: git describe)")
    parser.add_argument("--min-fw", default="0.1.0")
    parser.add_argument(
        "--sign-key",
        type=Path,
        default=None,
        help="ed25519 private key PEM; omit for an unsigned dev build",
    )
    # Artifact signatures authenticate downloads; the device has no trusted clock.
    parser.add_argument("--base-url", default="http://tischkarte.invalid/device")
    args = parser.parse_args(argv)

    out_dir = args.out or args.root / "dist" / "bundles"
    out_dir.mkdir(parents=True, exist_ok=True)
    version = args.version or git_version(args.root)

    rep = validate.Reporter()
    schema, cfg = validate.load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1
    decks = cfg["decks"]

    # These fields describe the raw .qdb downloaded by the device.
    manifest = {"schema": 3, "version": version, "min_fw": args.min_fw, "languages": {}}
    for lang, lang_dir, incubator in validate.discover_languages(args.root):
        if incubator:
            continue
        path = lang_dir / cfg.get("questionFile", "questions.yaml")
        entries = validate.parse_file(path, rep) if path.exists() else []
        if entries is None:
            return 1
        for entry in entries:
            entry.pop("__line__", None)
            if "id" not in entry:
                print(
                    f"{path}: entry without id — run tools/validate.py --fix first", file=sys.stderr
                )
                return 1
        count = len(entries)

        raw = build_bundle_bytes(lang, version, entries, decks)
        gz = compress_bundle(raw)

        raw_name = f"bundle-{lang}-{version}.qdb"
        gz_name = f"bundle-{lang}-{version}.qdb.gz"
        (out_dir / raw_name).write_bytes(raw)
        (out_dir / gz_name).write_bytes(gz)

        # Over the bytes the device actually verifies.
        digest = hashlib.sha256(raw).digest()
        sig = sign_digest(digest, args.sign_key) if args.sign_key else None
        if sig is None:
            print(f"warning: {raw_name} is unsigned (no --sign-key)", file=sys.stderr)
        manifest["languages"][lang] = {
            "url": f"{args.base_url}/{raw_name}",
            "size": len(raw),
            "sha256": digest.hex(),
            "sig": sig,
            "count": count,
        }
        print(f"{raw_name}: {count} questions, {len(raw)} bytes ({len(gz)} gzipped)")

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"manifest.json: version {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
