#!/usr/bin/env python3
"""Check a built release manifest before it is published.

A device cannot follow redirects and refuses unsigned artifacts, so a manifest
pointing anywhere but the production endpoint, or carrying a null signature,
describes a release no device can use — and must not be published as if it were
one. The db-* and fw-* tag workflows run this on the manifest they are about to
attach. See docs/firmware_update.md and docs/sync_protocol.md.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

# Must stay equal to the Kconfig.policy defaults, which release firmware
# compiles in. tools/tests cross-checks this.
DEFAULT_BASE_URL = "http://device.cicala.dev/device"

# The reserved development host, the former project host, and the pre-split
# website host. The last two serve redirects a device cannot follow.
FORBIDDEN = ("cicala.invalid", "pages.dev", "tischkarte")


def check_manifest(path: Path, base_url: str) -> list[str]:
    """Return the findings for one manifest file; empty means it passes."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        return [f"{path.name}: {exc.strerror}"]

    try:
        manifest = json.loads(text)
    except json.JSONDecodeError as exc:
        return [f"{path.name}: not valid JSON ({exc})"]

    findings = []

    for needle in FORBIDDEN:
        if needle in text:
            findings.append(f"{path.name}: contains '{needle}'")

    urls = []
    sigs = []
    schema = manifest.get("schema")
    if schema == 4:
        languages = manifest.get("languages")
        if not languages:
            findings.append(f"{path.name}: no languages")
        for lang, entry in languages.items():
            urls.append((f"languages.{lang}.url", entry.get("url")))
            sigs.append((f"languages.{lang}.sig", entry.get("sig")))
    elif schema == 1:
        urls.append(("url", manifest.get("url")))
        sigs.append(("sig", manifest.get("sig")))
    else:
        findings.append(f"{path.name}: unknown schema {schema!r}")

    prefix = base_url + "/"
    for where, url in urls:
        if not isinstance(url, str) or not url.startswith(prefix):
            findings.append(f"{path.name}: {where} is {url!r}, expected under {prefix}")

    for where, sig in sigs:
        if not isinstance(sig, str) or not sig:
            findings.append(f"{path.name}: {where} is null — no device will install this")

    return findings


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("manifests", type=Path, nargs="+")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL)
    args = parser.parse_args(argv)

    findings = []
    for path in args.manifests:
        findings += check_manifest(path, args.base_url)

    for finding in findings:
        print(f"error: {finding}", file=sys.stderr)

    if not findings:
        print(f"{len(args.manifests)} manifest(s) describe a release devices can use")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
