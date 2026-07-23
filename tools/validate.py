#!/usr/bin/env python3
"""Validate the Tischkarte question database.

Checks every questions/{lang}/{category}.yaml (shipped and incubator) against
questions/schema.json plus the rules JSON Schema cannot express: id format and
global uniqueness, per-language dedup on normalized text, per-language
denylist, terminator rule, and origin references.

With --fix, assigns missing ids and added-dates and rewrites all clean files
with stable formatting (2-space indent, keys in schema order). Ids are derived
from a hash of lang|category|normalized text at assignment time and are stable
forever after — this tool deliberately never re-checks hash equality, only
uniqueness, so typo fixes don't change ids.

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


# ---------------------------------------------------------------- loading

class LineLoader(yaml.SafeLoader):
    """SafeLoader that records the source line of every mapping."""


def _construct_mapping(loader, node, deep=False):
    mapping = yaml.SafeLoader.construct_mapping(loader, node, deep=deep)
    mapping["__line__"] = node.start_mark.line + 1
    return mapping


LineLoader.add_constructor(
    yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _construct_mapping
)


def normalize_text(text: str) -> str:
    return " ".join(unicodedata.normalize("NFC", text).lower().split())


def compute_id(lang: str, category: str, text: str) -> str:
    payload = f"{lang}|{category}|{normalize_text(text)}".encode("utf-8")
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
    cfg = schema.get("x-tischkarte")
    if not cfg:
        rep.error(schema_path, 0, "schema.json is missing the x-tischkarte config block")
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
    """Return the list of entry dicts (each with __line__), or None on error."""
    try:
        data = yaml.load(path.read_text(encoding="utf-8"), Loader=LineLoader)
    except yaml.YAMLError as exc:
        line = getattr(getattr(exc, "problem_mark", None), "line", 0)
        rep.error(path, (line or 0) + 1, f"YAML parse error: {getattr(exc, 'problem', exc)}")
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
        # yaml parses unquoted ISO dates into date objects; the schema wants strings
        if isinstance(entry.get("added"), dt.date):
            entry["added"] = entry["added"].isoformat()
        entries.append(entry)
    return entries


# ---------------------------------------------------------------- checks

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
            path, line,
            f"text must end with one of {terminators!r} "
            "(a closing quote/bracket after it is fine)",
        )


def check_denylist(text: str, terms: list[str], path, line, rep: Reporter):
    normalized = normalize_text(text)
    for term in terms:
        if re.search(rf"(?<!\w){re.escape(term)}(?!\w)", normalized):
            rep.error(
                path, line,
                f'text matches denylist term "{term}" — a maintainer must review '
                "this entry manually (see questions/<lang>/denylist.txt)",
            )


# ---------------------------------------------------------------- fixing / formatting

def format_file(lang: str, category: str, entries: list[dict], key_order: list[str]) -> str:
    lines = [
        f"# questions/{lang}/{category}.yaml — Tischkarte question database (CC0).",
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
            if key == "tags":
                if not value:
                    continue
                rendered = "[" + ", ".join(value) + "]"
            elif key in ("id", "origin"):
                rendered = str(value)
            else:
                escaped = str(value).replace("\\", "\\\\").replace('"', '\\"')
                rendered = f'"{escaped}"'
            lines.append(("- " if first else "  ") + f"{key}: {rendered}")
            first = False
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------- main

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--fix", action="store_true",
                        help="assign missing ids/dates and rewrite files with stable formatting")
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parent.parent,
                        help="repository root (for tests)")
    args = parser.parse_args(argv)

    rep = Reporter()
    schema, cfg = load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1

    categories = cfg["categories"]
    key_order = cfg.get("keyOrder", ["id", "text", "tags", "origin", "author", "added"])
    entry_validator = Draft202012Validator(schema["items"])
    today = dt.date.today().isoformat()

    all_ids: dict[str, str] = {}          # id -> "path:line" of first definition
    origin_refs: list[tuple] = []          # (path, line, origin_id)
    total = 0

    for lang, lang_dir, incubator in discover_languages(args.root):
        rel = lang_dir.relative_to(args.root)

        denylist_path = lang_dir / "denylist.txt"
        if denylist_path.exists():
            denylist = load_denylist(denylist_path)
        else:
            denylist = []
            if incubator:
                rep.warn(rel, 0, "no denylist.txt yet — required before graduation")
            else:
                rep.error(rel, 0, "shipped language is missing denylist.txt")
        if not (lang_dir / "STYLE.md").exists():
            (rep.warn if incubator else rep.error)(
                rel, 0,
                "missing STYLE.md" + (" — required before graduation" if incubator else ""),
            )

        for yaml_file in sorted(lang_dir.glob("*.yaml")):
            if yaml_file.stem not in categories:
                rep.error(yaml_file.relative_to(args.root), 1,
                          f"unknown category '{yaml_file.stem}' (allowed: {', '.join(categories)})")

        seen_texts: dict[str, str] = {}   # normalized text -> "path:line", per language

        for category in categories:
            path = lang_dir / f"{category}.yaml"
            if not path.exists():
                if not incubator:
                    rep.warn(rel, 0, f"missing {category}.yaml for shipped language")
                continue
            rpath = path.relative_to(args.root)
            entries = parse_file(path, rep)
            if entries is None:
                continue

            file_error_count = len(rep.errors)
            for entry in entries:
                line = entry.pop("__line__", 0)
                total += 1

                text = entry.get("text")
                if args.fix and isinstance(text, str):
                    if "id" not in entry:
                        entry["id"] = compute_id(lang, category, text)
                    if "added" not in entry:
                        entry["added"] = today

                for err in sorted(entry_validator.iter_errors(entry), key=str):
                    field = ".".join(str(p) for p in err.path) or "entry"
                    msg = err.message
                    if ("'id' is a required property" in msg
                            or "'added' is a required property" in msg):
                        msg += " — run `tools/validate.py --fix` to assign it"
                    rep.error(rpath, line, f"{field}: {msg}")

                if not isinstance(text, str):
                    continue
                check_text_rules(text, lang, cfg, rpath, line, rep)
                check_denylist(text, denylist, rpath, line, rep)

                norm = normalize_text(text)
                if norm in seen_texts:
                    rep.error(rpath, line,
                              f"duplicate question text within language '{lang}' "
                              f"(first seen at {seen_texts[norm]})")
                else:
                    seen_texts[norm] = f"{rpath}:{line}"

                qid = entry.get("id")
                if isinstance(qid, str) and ID_RE.match(qid):
                    if qid in all_ids:
                        rep.error(rpath, line,
                                  f"duplicate id {qid} (first seen at {all_ids[qid]})")
                    else:
                        all_ids[qid] = f"{rpath}:{line}"

                if "origin" in entry:
                    origin_refs.append((rpath, line, entry["origin"]))

            if args.fix and len(rep.errors) == file_error_count:
                formatted = format_file(lang, category, entries, key_order)
                if formatted != path.read_text(encoding="utf-8"):
                    path.write_text(formatted, encoding="utf-8")
                    print(f"fixed: {rpath}")

    for rpath, line, origin in origin_refs:
        if origin not in all_ids:
            rep.warn(rpath, line,
                     f"origin {origin} does not exist (anymore) in the database")

    for w in rep.warnings:
        print(w, file=sys.stderr)
    for e in rep.errors:
        print(e, file=sys.stderr)
    if rep.errors:
        print(f"\n{total} questions checked — {len(rep.errors)} error(s), "
              f"{len(rep.warnings)} warning(s)", file=sys.stderr)
        return 1
    print(f"{total} questions checked — OK ({len(rep.warnings)} warning(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
