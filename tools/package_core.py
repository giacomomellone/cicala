#!/usr/bin/env python3
"""Package portable firmware logic and an English offline fallback reproducibly."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import subprocess
import tarfile
from pathlib import Path

import build_bundle
import validate


def package(root: Path, out: Path, release: bool = False) -> str:
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=root))
    if release and dirty:
        raise ValueError("release packages require a clean checkout")
    report = validate.Reporter()
    schema, config = validate.load_config(root, report)
    if schema is None:
        raise ValueError("invalid corpus schema")
    entries = validate.parse_file(root / "questions/en/questions.yaml", report)
    if entries is None or report.errors:
        raise ValueError("invalid English corpus")
    corpus = build_bundle.build_bundle_bytes("en", "0.0.0", entries, build_bundle.form_tags(config))
    if (
        len(corpus) > 32768
        or len(entries) > 512
        or any(len(q["text"].encode()) > 128 for q in entries)
    ):
        raise ValueError("English fallback exceeds core device limits")
    sources = (
        subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z", "core"],
            cwd=root,
        )
        .decode()
        .split("\0")
    )
    files = {
        str(Path(name).relative_to("core")): (root / name).read_bytes()
        for name in sources
        if name and (root / name).is_file()
    }
    files["assets/en.qdb"] = corpus
    fixture_entries = validate.parse_file(root / "firmware/tests/corpus/en/questions.yaml", report)
    if fixture_entries is None or report.errors:
        raise ValueError("invalid firmware test corpus")
    files["tests/fixtures/en.qdb"] = build_bundle.build_bundle_bytes(
        "en", "1.0.0", fixture_entries, build_bundle.form_tags(config)
    )
    files["include/trusted_key.h"] = (root / "firmware/components/sync/trusted_key.h").read_bytes()
    header = (
        "#pragma once\n#include <stdint.h>\nnamespace cicala::assets {\n"
        "inline constexpr uint8_t english[] = {\n"
    )
    header += "\n".join(
        "    " + ", ".join(f"0x{b:02x}" for b in corpus[i : i + 16]) + ","
        for i in range(0, len(corpus), 16)
    )
    header += "\n};\n}\n"
    files["include/cicala_assets.hpp"] = header.encode()
    files["package-provenance.json"] = (
        json.dumps(
            {
                "revision": revision,
                "dirty": dirty,
                "corpus_sha256": hashlib.sha256(corpus).hexdigest(),
            },
            sort_keys=True,
        )
        + "\n"
    ).encode()
    files["LICENSE"] = (root / "LICENSE-CODE").read_bytes()
    files["assets/LICENSE-QUESTIONS"] = (root / "LICENSE-QUESTIONS").read_bytes()
    stream = io.BytesIO()
    with tarfile.open(fileobj=stream, mode="w") as archive:
        for name, content in sorted(files.items()):
            info = tarfile.TarInfo(name)
            info.size = len(content)
            info.mode = 0o644
            info.mtime = 0
            archive.addfile(info, io.BytesIO(content))
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("wb") as output:
        with gzip.GzipFile(filename="", mode="wb", fileobj=output, mtime=0) as compressed:
            compressed.write(stream.getvalue())
    digest = hashlib.sha256(out.read_bytes()).hexdigest()
    out.with_name(out.name + ".sha256").write_text(f"{digest}  {out.name}\n")
    return digest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--out", type=Path, default=Path("dist/cicala-core-0.1.0.tar.gz"))
    parser.add_argument("--release", action="store_true")
    args = parser.parse_args()
    print(package(args.root, args.out, args.release))


if __name__ == "__main__":
    main()
