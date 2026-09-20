#!/usr/bin/env python3
"""Exercise the native verifier with real corpus bytes signed by a temporary test key."""

import base64
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

import build_bundle
import openssl_tool
import validate


def main() -> None:
    executable = Path(sys.argv[1]).resolve()
    openssl = openssl_tool.executable()
    root = Path(__file__).resolve().parent.parent
    report = validate.Reporter()
    _, config = validate.load_config(root, report)
    entries = validate.parse_file(root / "questions/en/questions.yaml", report)
    raw = build_bundle.build_bundle_bytes("en", "1.0.0", entries, build_bundle.form_tags(config))
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        key = work / "key.pem"
        subprocess.run([openssl, "genpkey", "-algorithm", "ed25519", "-out", str(key)], check=True)
        public = subprocess.check_output(
            [openssl, "pkey", "-in", str(key), "-pubout", "-outform", "DER"]
        )
        (work / "public.bin").write_bytes(public[-32:])
        digest = hashlib.sha256(raw).digest()
        entry = {
            "url": "http://127.0.0.1/bundle.qdb",
            "size": len(raw),
            "count": len(entries),
            "sha256": digest.hex(),
            "sig": build_bundle.sign_digest(digest, key),
        }
        manifest = {"schema": 4, "version": "1.0.0", "min_fw": "0.2.0", "languages": {"en": entry}}

        def check(expected: bool, payload: bytes = raw) -> None:
            (work / "manifest.json").write_text(json.dumps(manifest))
            (work / "bundle.qdb").write_bytes(payload)
            (work / "digest.bin").write_bytes(hashlib.sha256(payload).digest())
            result = subprocess.run(
                [
                    str(executable),
                    str(work / "manifest.json"),
                    str(work / "bundle.qdb"),
                    str(work / "digest.bin"),
                    str(work / "public.bin"),
                ],
                capture_output=True,
            )
            if (result.returncode == 0) != expected:
                raise AssertionError(
                    f"unexpected verifier result: {result.stdout!r} {result.stderr!r}"
                )

        check(True)
        check(False, raw[:-1])
        check(False, raw[:-1] + bytes([raw[-1] ^ 1]))
        manifest["version"] = "2.0.0"
        check(False)
        manifest["version"] = "1.0.0"
        entry["count"] += 1
        check(False)
        entry["count"] -= 1
        manifest["min_fw"] = "99.0.0"
        check(False)
        manifest["min_fw"] = "0.2.0"
        entry["sig"] = base64.b64encode(bytes(64)).decode()
        check(False)
        entry["sig"] = None
        check(False)
    print("Native verifier accepted the signed English corpus and rejected all invalid variants")


if __name__ == "__main__":
    main()
