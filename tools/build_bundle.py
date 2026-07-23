#!/usr/bin/env python3
"""Build per-language device bundles (.tkb) plus the sync manifest.

Bundle format (docs/SYNC-PROTOCOL.md has the worked example) — gzip of:

  magic        4 bytes  "TKB1"
  version      u8 length + UTF-8 bytes (e.g. "2026.07.1")
  lang         u8 length + UTF-8 bytes (e.g. "en")
  per category, in the fixed order from questions/schema.json:
    count      u16 LE
    per question:
      text     u16 LE length + UTF-8 bytes
      flags    u8, bit0 = spicy
      display  u16 LE, decimal of first 2 id-hash bytes mod 10000 (cosmetic)

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


def display_id(qid: str) -> int:
    return int(qid[2:6], 16) % 10000


def build_bundle_bytes(lang: str, version: str, per_category: dict[str, list[dict]],
                       categories: list[str]) -> bytes:
    out = bytearray()
    out += b"TKB1"
    for s in (version, lang):
        raw = s.encode("utf-8")
        out += struct.pack("<B", len(raw)) + raw
    for category in categories:
        entries = per_category.get(category, [])
        out += struct.pack("<H", len(entries))
        for entry in entries:
            text = entry["text"].encode("utf-8")
            flags = 1 if "spicy" in entry.get("tags", []) else 0
            out += struct.pack("<H", len(text)) + text
            out += struct.pack("<B", flags)
            out += struct.pack("<H", display_id(entry["id"]))
    return gzip.compress(bytes(out), mtime=0)


def parse_bundle(blob: bytes):
    """Inverse of build_bundle_bytes, for tests and debugging."""
    raw = gzip.decompress(blob)
    if raw[:4] != b"TKB1":
        raise ValueError("bad magic")
    pos = 4

    def take_str8():
        nonlocal pos
        n = raw[pos]; pos += 1
        s = raw[pos:pos + n].decode("utf-8"); pos += n
        return s

    version, lang = take_str8(), take_str8()
    result = {"version": version, "lang": lang, "categories": {}}
    cats = json.loads((Path(__file__).resolve().parent.parent /
                       "questions" / "schema.json").read_text())["x-tischkarte"]["categories"]
    for category in cats:
        (count,) = struct.unpack_from("<H", raw, pos); pos += 2
        items = []
        for _ in range(count):
            (tlen,) = struct.unpack_from("<H", raw, pos); pos += 2
            text = raw[pos:pos + tlen].decode("utf-8"); pos += tlen
            flags = raw[pos]; pos += 1
            (disp,) = struct.unpack_from("<H", raw, pos); pos += 2
            items.append({"text": text, "spicy": bool(flags & 1), "display": disp})
        result["categories"][category] = items
    if pos != len(raw):
        raise ValueError(f"{len(raw) - pos} trailing bytes")
    return result


def sign_digest(digest: bytes, key_path: Path) -> str:
    with tempfile.NamedTemporaryFile() as tf:
        tf.write(digest)
        tf.flush()
        out = subprocess.run(
            ["openssl", "pkeyutl", "-sign", "-inkey", str(key_path),
             "-rawin", "-in", tf.name],
            capture_output=True, check=True,
        )
    return base64.b64encode(out.stdout).decode("ascii")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--out", type=Path, default=None,
                        help="output directory (default: <root>/dist/bundles)")
    parser.add_argument("--version", default=None,
                        help="release version (default: git describe)")
    parser.add_argument("--min-fw", default="0.1.0")
    parser.add_argument("--sign-key", type=Path, default=None,
                        help="ed25519 private key PEM; omit for an unsigned dev build")
    parser.add_argument("--base-url", default="https://tischkarte.pages.dev/device")
    args = parser.parse_args(argv)

    out_dir = args.out or args.root / "dist" / "bundles"
    out_dir.mkdir(parents=True, exist_ok=True)
    version = args.version or git_version(args.root)

    rep = validate.Reporter()
    schema, cfg = validate.load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1
    categories = cfg["categories"]

    manifest = {"schema": 1, "version": version, "min_fw": args.min_fw, "languages": {}}
    for lang, lang_dir, incubator in validate.discover_languages(args.root):
        if incubator:
            continue
        per_category, count = {}, 0
        for category in categories:
            path = lang_dir / f"{category}.yaml"
            entries = validate.parse_file(path, rep) if path.exists() else []
            if entries is None:
                return 1
            for entry in entries:
                if "id" not in entry:
                    print(f"{path}: entry without id — run tools/validate.py --fix first",
                          file=sys.stderr)
                    return 1
            per_category[category] = entries
            count += len(entries)

        blob = build_bundle_bytes(lang, version, per_category, categories)
        name = f"bundle-{lang}-{version}.tkb"
        (out_dir / name).write_bytes(blob)
        digest = hashlib.sha256(blob).digest()
        sig = sign_digest(digest, args.sign_key) if args.sign_key else None
        if sig is None:
            print(f"warning: {name} is unsigned (no --sign-key)", file=sys.stderr)
        manifest["languages"][lang] = {
            "url": f"{args.base_url}/{name}",
            "size": len(blob),
            "sha256": digest.hex(),
            "sig": sig,
            "count": count,
        }
        print(f"{name}: {count} questions, {len(blob)} bytes")

    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"manifest.json: version {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
