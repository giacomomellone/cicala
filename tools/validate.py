#!/usr/bin/env python3
"""Validate the Cicala question database.

Checks every questions/{lang}/questions.yaml (shipped and incubator) against
questions/schema.json plus the rules JSON Schema cannot express: id format and
global uniqueness, per-language dedup on normalized text, per-language
denylist, terminator rules, deck/tone invariants, and origin references.

With --fix, assigns missing ids and added dates and rewrites every clean corpus
with stable formatting. New ids hash `lang|normalized text`; existing ids stay
stable through later text or deck edits.

Exit code 0 = clean (warnings allowed), 1 = errors. Requires pyyaml, jsonschema.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import re
import sys
import unicodedata
from pathlib import Path

import yaml
from jsonschema import Draft202012Validator


class LineLoader(yaml.SafeLoader):
    """SafeLoader that records the source line of every mapping."""


def _construct_mapping(loader, node, deep=False):
    mapping = yaml.SafeLoader.construct_mapping(loader, node, deep=deep)
    mapping["__line__"] = node.start_mark.line + 1
    return mapping


LineLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _construct_mapping)


def normalize_text(text: str) -> str:
    return " ".join(unicodedata.normalize("NFC", text).lower().split())


def compute_id(lang: str, text: str) -> str:
    payload = f"{lang}|{normalize_text(text)}".encode()
    return "q-" + hashlib.sha256(payload).hexdigest()[:8]


ID_RE = re.compile(r"^q-[0-9a-f]{8}$")


class Reporter:
    def __init__(self):
        self.errors: list[str] = []
        self.warnings: list[str] = []

    def error(self, path, line, msg):
        loc = f"{path}:{line}" if line else str(path)
        self.errors.append(f"{loc}: error: {msg}")

    def warn(self, path, line, msg):
        loc = f"{path}:{line}" if line else str(path)
        self.warnings.append(f"{loc}: warning: {msg}")


def load_config(root: Path, rep: Reporter):
    schema_path = root / "questions" / "schema.json"
    try:
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        rep.error(schema_path, 0, f"cannot read schema: {exc}")
        return None, None
    cfg = schema.get("x-cicala")
    if not cfg:
        rep.error(schema_path, 0, "schema.json is missing the x-cicala config block")
        return None, None
    return schema, cfg


def discover_languages(root: Path):
    """Yield (lang, dir, is_incubator) for every language directory."""
    qdir = root / "questions"
    for entry in sorted(qdir.iterdir()):
        if not entry.is_dir():
            continue
        if entry.name == "incubator":
            for sub in sorted(entry.iterdir()):
                if sub.is_dir():
                    yield sub.name, sub, True
        else:
            yield entry.name, entry, False


def load_denylist(path: Path) -> list[str]:
    terms = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        term = raw.strip().lower()
        if term and not term.startswith("#"):
            terms.append(" ".join(term.split()))
    return terms


def parse_file(path: Path, rep: Reporter):
    """Return entry dictionaries with source lines, or None on error."""
    try:
        data = yaml.load(path.read_text(encoding="utf-8"), Loader=LineLoader)
    except (OSError, yaml.YAMLError) as exc:
        line = getattr(getattr(exc, "problem_mark", None), "line", 0)
        rep.error(path, (line or 0) + 1, f"cannot parse YAML: {getattr(exc, 'problem', exc)}")
        return None
    if data is None:
        return []
    if not isinstance(data, list):
        rep.error(path, 1, "file must be a YAML list of question entries")
        return None
    entries = []
    for i, entry in enumerate(data):
        if not isinstance(entry, dict):
            rep.error(path, 1, f"entry {i + 1} is not a mapping")
            return None
        if isinstance(entry.get("added"), dt.date):
            entry["added"] = entry["added"].isoformat()
        entries.append(entry)
    return entries


def load_corpus(lang_dir: Path, cfg: dict, rep: Reporter):
    return parse_file(lang_dir / cfg.get("questionFile", "questions.yaml"), rep)


def check_text_rules(text: str, lang: str, cfg: dict, path, line, rep: Reporter):
    if "\n" in text:
        rep.error(path, line, "text must not contain newlines")
        return
    lang_cfg = cfg.get("languages", {}).get(lang, {})
    terminators = lang_cfg.get("terminators", cfg.get("defaultTerminators", ["?"]))
    closing = set(cfg.get("closingCharacters", []))
    stripped = text.rstrip("".join(closing)) if closing else text
    if not stripped or stripped[-1] not in terminators:
        rep.error(
            path,
            line,
            f"text must end with one of {terminators!r} (a closing quote/bracket after it is fine)",
        )


def check_denylist(text: str, terms: list[str], path, line, rep: Reporter):
    normalized = normalize_text(text)
    for term in terms:
        if re.search(rf"(?<!\w){re.escape(term)}(?!\w)", normalized):
            rep.error(
                path,
                line,
                f'text matches denylist term "{term}" — a maintainer must review '
                "this entry manually (see questions/<lang>/denylist.txt)",
            )


def check_deck_rules(entry: dict, path, line, rep: Reporter):
    decks = entry.get("decks")
    tags = entry.get("tags", [])
    if not isinstance(decks, list) or not isinstance(tags, list):
        return
    daring = {"dark", "spicy"}.intersection(tags)
    if daring and decks != ["wild"]:
        rep.error(
            path,
            line,
            f"{', '.join(sorted(daring))} questions must be exclusive to the wild deck",
        )


def _render_scalar(value) -> str:
    escaped = str(value).replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def format_file(lang: str, entries: list[dict], key_order: list[str]) -> str:
    lines = [
        f"# questions/{lang}/questions.yaml — Cicala question database (CC0).",
        "# Managed by tools/validate.py --fix. Append new entries WITHOUT id/added;",
        "# CI assigns them. Never edit an existing id. See CONTRIBUTING.md.",
        "",
    ]
    for entry in entries:
        first = True
        for key in key_order:
            if key not in entry:
                continue
            value = entry[key]
            if key in ("decks", "tags"):
                if not value and key == "tags":
                    continue
                rendered = "[" + ", ".join(value) + "]"
            elif key == "depth":
                rendered = str(value)
            elif key in ("id", "origin"):
                rendered = str(value)
            else:
                rendered = _render_scalar(value)
            lines.append(("- " if first else "  ") + f"{key}: {rendered}")
            first = False
    return "\n".join(lines) + "\n"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--fix", action="store_true", help="assign missing ids/dates and rewrite clean corpora"
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="repository root (for tests)",
    )
    args = parser.parse_args(argv)

    rep = Reporter()
    schema, cfg = load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1

    decks = cfg["decks"]
    question_file = cfg.get("questionFile", "questions.yaml")
    key_order = cfg.get(
        "keyOrder", ["id", "text", "decks", "depth", "tags", "origin", "author", "added"]
    )
    entry_validator = Draft202012Validator(schema["items"])
    today = dt.date.today().isoformat()
    all_ids: dict[str, str] = {}
    origin_refs: list[tuple] = []
    total = 0

    for lang, lang_dir, incubator in discover_languages(args.root):
        rel = lang_dir.relative_to(args.root)
        denylist_path = lang_dir / "denylist.txt"
        if denylist_path.exists():
            denylist = load_denylist(denylist_path)
        else:
            denylist = []
            (rep.warn if incubator else rep.error)(
                rel,
                0,
                "missing denylist.txt" + (" — required before graduation" if incubator else ""),
            )
        if not (lang_dir / "STYLE.md").exists():
            (rep.warn if incubator else rep.error)(
                rel, 0, "missing STYLE.md" + (" — required before graduation" if incubator else "")
            )

        unexpected = [p.name for p in sorted(lang_dir.glob("*.yaml")) if p.name != question_file]
        for name in unexpected:
            rep.error(rel / name, 1, f"question data must live in {question_file}")

        path = lang_dir / question_file
        if not path.exists():
            (rep.warn if incubator else rep.error)(rel, 0, f"missing {question_file}")
            continue
        rpath = path.relative_to(args.root)
        entries = parse_file(path, rep)
        if entries is None:
            continue

        file_error_count = len(rep.errors)
        seen_texts: dict[str, str] = {}
        coverage = {deck: 0 for deck in decks}

        for entry in entries:
            line = entry.pop("__line__", 0)
            total += 1
            text = entry.get("text")
            if args.fix and isinstance(text, str):
                if "id" not in entry:
                    entry["id"] = compute_id(lang, text)
                if "added" not in entry:
                    entry["added"] = today

            for err in sorted(entry_validator.iter_errors(entry), key=str):
                field = ".".join(str(p) for p in err.path) or "entry"
                msg = err.message
                if "'id' is a required property" in msg or "'added' is a required property" in msg:
                    msg += " — run `tools/validate.py --fix` to assign it"
                rep.error(rpath, line, f"{field}: {msg}")

            if isinstance(text, str):
                check_text_rules(text, lang, cfg, rpath, line, rep)
                check_denylist(text, denylist, rpath, line, rep)
                norm = normalize_text(text)
                if norm in seen_texts:
                    rep.error(
                        rpath,
                        line,
                        f"duplicate question text within language '{lang}' "
                        f"(first seen at {seen_texts[norm]})",
                    )
                else:
                    seen_texts[norm] = f"{rpath}:{line}"

            check_deck_rules(entry, rpath, line, rep)
            for deck in entry.get("decks", []):
                if deck in coverage:
                    coverage[deck] += 1

            qid = entry.get("id")
            if isinstance(qid, str) and ID_RE.match(qid):
                if qid in all_ids:
                    rep.error(rpath, line, f"duplicate id {qid} (first seen at {all_ids[qid]})")
                else:
                    all_ids[qid] = f"{rpath}:{line}"
            if "origin" in entry:
                origin_refs.append((rpath, line, entry["origin"]))

        minimum = 1 if incubator else 10
        for deck, count in coverage.items():
            if count < minimum:
                rep.warn(
                    rpath, 0, f"deck '{deck}' has {count} questions; target is at least {minimum}"
                )

        if args.fix and len(rep.errors) == file_error_count:
            formatted = format_file(lang, entries, key_order)
            if formatted != path.read_text(encoding="utf-8"):
                path.write_text(formatted, encoding="utf-8")
                print(f"fixed: {rpath}")

    for rpath, line, origin in origin_refs:
        if origin not in all_ids:
            rep.warn(rpath, line, f"origin {origin} is missing from the database")

    for warning in rep.warnings:
        print(warning, file=sys.stderr)
    for error in rep.errors:
        print(error, file=sys.stderr)
    if rep.errors:
        print(
            f"\n{total} questions checked — {len(rep.errors)} error(s), "
            f"{len(rep.warnings)} warning(s)",
            file=sys.stderr,
        )
        return 1
    print(f"{total} questions checked — OK ({len(rep.warnings)} warning(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
