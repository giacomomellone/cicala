#!/usr/bin/env python3
"""Build the website's per-language data payloads from the question database.

Emits into website/src/data/ (override with --out):

  questions.{lang}.json   question playback data plus translation provenance
  languages.json          shipped languages with names and question counts
  index.json              id -> lang, for permalinks and cross-language links

Only shipped languages are built — the incubator is excluded by design
(docs/languages.md). Strips author and added from the main payloads.
Fails if any single language payload exceeds 2 MB.
"""

from __future__ import annotations

import argparse
import hashlib
import datetime as dt
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate  # noqa: E402

MAX_PAYLOAD_BYTES = 2 * 1024 * 1024


def git_version(root: Path) -> str:
    try:
        out = subprocess.run(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=10,
        )
        if out.returncode == 0 and out.stdout.strip():
            return out.stdout.strip()
    except OSError:
        pass
    return "dev"


def dump(obj) -> str:
    return json.dumps(obj, ensure_ascii=False, separators=(",", ":"))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument(
        "--out", type=Path, default=None, help="output directory (default: <root>/website/src/data)"
    )
    args = parser.parse_args(argv)
    out_dir = args.out or args.root / "website" / "src" / "data"

    rep = validate.Reporter()
    schema, cfg = validate.load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1
    lang_names = {code: c.get("name", code) for code, c in cfg.get("languages", {}).items()}

    version = git_version(args.root)
    generated = dt.datetime.now(dt.UTC).isoformat(timespec="seconds")

    out_dir.mkdir(parents=True, exist_ok=True)
    languages = []
    index: dict[str, str] = {}
    failed = False

    for lang, lang_dir, incubator in validate.discover_languages(args.root):
        if incubator:
            continue
        payload = {"version": version, "generated": generated, "questions": []}
        path = lang_dir / cfg.get("questionFile", "questions.yaml")
        entries = validate.parse_file(path, rep) if path.exists() else []
        if entries is None:
            failed = True
            continue
        for entry in entries:
            entry.pop("__line__", None)
            if "id" not in entry:
                print(
                    f"{path}: entry without id — run tools/validate.py --fix first", file=sys.stderr
                )
                return 1
            question = {
                "id": entry["id"],
                "text": entry["text"],
                "depth": entry["depth"],
                "tags": entry.get("tags", []),
            }
            if entry.get("origin"):
                question["origin"] = entry["origin"]
            if entry.get("translated_by"):
                question["translated_by"] = entry["translated_by"]
            payload["questions"].append(question)
            index[entry["id"]] = lang
        count = len(entries)

        fingerprint = hashlib.sha256(dump(payload["questions"]).encode()).hexdigest()[:16]
        payload["version"] = f"{version}:{fingerprint}"
        blob = dump(payload)
        if len(blob.encode("utf-8")) > MAX_PAYLOAD_BYTES:
            print(
                f"questions.{lang}.json exceeds 2 MB — split or prune before shipping",
                file=sys.stderr,
            )
            return 1
        (out_dir / f"questions.{lang}.json").write_text(blob, encoding="utf-8")

        languages.append({"code": lang, "name": lang_names.get(lang, lang), "count": count})
        print(f"questions.{lang}.json: {count} questions")

    (out_dir / "languages.json").write_text(dump(languages), encoding="utf-8")
    (out_dir / "index.json").write_text(dump(index), encoding="utf-8")
    print(
        f"languages.json: {[language['code'] for language in languages]}"
        f" · index.json: {len(index)} ids"
    )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
