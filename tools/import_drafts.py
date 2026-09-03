#!/usr/bin/env python3
"""Append plain-text question drafts to one language corpus.

A draft file states editorial metadata once per group and lists the questions
under it, so a deck costs one header instead of four YAML lines per question:

    # decks: new_people, close · depth: 1 · tags: icebreaker
    What's the most spontaneous thing you've ever done?
    Which song gets you on the dance floor every single time?

Fields are separated by `·` or `|`, and a header replaces the previous one
outright: a group without a `tags` field has no tags. A `#` line that does not
open with a known field is a comment. A question may wrap over several lines
and closes on the line that ends with the language's terminator.

Entries are appended without `id` and `added`, exactly as a hand-written entry
would be, and the fix pass assigns both. Questions already in the corpus are
reported and skipped, so re-running a draft after editing part of it is safe.

Nothing is written when the draft has an error.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
KNOWN_FIELDS = ("decks", "depth", "tags", "author")
LIST_FIELDS = ("decks", "tags")
FIELD_SEPARATOR = re.compile(r"\s*[·|]\s*")
HEADER_RE = re.compile(rf"({'|'.join(KNOWN_FIELDS)})\s*:", re.IGNORECASE)


@dataclass
class Draft:
    text: str
    source: Path
    line: int
    metadata: dict


def parse_header(body: str, cfg: dict, where: str, errors: list[str]) -> dict:
    values: dict = {}
    for part in FIELD_SEPARATOR.split(body):
        if not part:
            continue
        key, separator, raw = part.partition(":")
        key = key.strip().lower()
        raw = raw.strip()
        if not separator or key not in KNOWN_FIELDS:
            errors.append(
                f"{where}: '{part}' is not one of {', '.join(f'{f}:' for f in KNOWN_FIELDS)}"
            )
            continue
        if key in LIST_FIELDS:
            values[key] = [item.strip() for item in raw.split(",") if item.strip()]
        elif key == "depth":
            if not raw.isdigit():
                errors.append(f"{where}: depth must be a whole number, got '{raw}'")
                continue
            values[key] = int(raw)
        else:
            values[key] = raw

    unknown = [deck for deck in values.get("decks", []) if deck not in cfg["decks"]]
    if unknown:
        errors.append(f"{where}: unknown deck(s) {', '.join(unknown)}; pick from {cfg['decks']}")
    unknown = [tag for tag in values.get("tags", []) if tag not in cfg["tags"]]
    if unknown:
        errors.append(f"{where}: unknown tag(s) {', '.join(unknown)}; pick from {cfg['tags']}")
    if not values.get("decks"):
        errors.append(f"{where}: the header needs at least one deck")
    if "depth" not in values:
        errors.append(f"{where}: the header needs a depth")
    return values


def parse_draft(path: Path, lang: str, cfg: dict, errors: list[str]) -> list[Draft]:
    drafts: list[Draft] = []
    metadata: dict | None = None
    pending: list[str] = []
    opened = 0

    def unterminated() -> None:
        errors.append(
            f"{path}:{opened}: this question never ends with "
            f"one of {validate.question_terminators(lang, cfg)!r}"
        )

    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        where = f"{path}:{number}"
        if not line or line.startswith("#"):
            if pending:
                unterminated()
                pending = []
            if line.startswith("#"):
                body = line.lstrip("#").strip()
                if HEADER_RE.match(body):
                    metadata = parse_header(body, cfg, where, errors)
            continue
        if metadata is None:
            errors.append(f"{where}: no '# decks: … · depth: …' header before this question")
            continue
        if not pending:
            opened = number
        pending.append(line)
        text = " ".join(" ".join(pending).split())
        if not validate.ends_with_terminator(text, lang, cfg):
            continue
        drafts.append(Draft(text, path, opened, dict(metadata)))
        pending = []

    if pending:
        unterminated()
    return drafts


def to_entry(draft: Draft) -> dict:
    entry = {
        "text": draft.text,
        "decks": list(draft.metadata["decks"]),
        "depth": draft.metadata["depth"],
    }
    if draft.metadata.get("tags"):
        entry["tags"] = list(draft.metadata["tags"])
    if draft.metadata.get("author"):
        entry["author"] = draft.metadata["author"]
    return entry


def check_drafts(
    drafts: list[Draft], lang: str, schema: dict, cfg: dict, lang_dir: Path, errors: list[str]
) -> None:
    """Apply the validator's own entry rules before anything is written, so a
    rejected draft never lands in the corpus."""
    denylist = lang_dir / "denylist.txt"
    terms = validate.load_denylist(denylist) if denylist.exists() else []
    text_rule = schema["items"]["properties"]["text"]
    depth_rule = schema["items"]["properties"]["depth"]
    rep = validate.Reporter()
    for draft in drafts:
        entry = to_entry(draft)
        if not text_rule["minLength"] <= len(draft.text) <= text_rule["maxLength"]:
            rep.error(
                draft.source,
                draft.line,
                f"text is {len(draft.text)} characters; the limit is "
                f"{text_rule['minLength']}-{text_rule['maxLength']}",
            )
        if not depth_rule["minimum"] <= entry["depth"] <= depth_rule["maximum"]:
            rep.error(
                draft.source,
                draft.line,
                f"depth {entry['depth']} is outside "
                f"{depth_rule['minimum']}-{depth_rule['maximum']}",
            )
        validate.check_text_rules(draft.text, lang, cfg, draft.source, draft.line, rep)
        validate.check_denylist(draft.text, terms, draft.source, draft.line, rep)
        validate.check_deck_rules(entry, draft.source, draft.line, rep)
    errors.extend(rep.errors)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--lang", required=True, help="target language code, e.g. en")
    parser.add_argument("files", nargs="+", type=Path, help="draft files to import")
    parser.add_argument("--root", type=Path, default=ROOT, help="repository root (for tests)")
    parser.add_argument(
        "--dry-run", action="store_true", help="report what would be appended and write nothing"
    )
    args = parser.parse_args(argv)

    rep = validate.Reporter()
    schema, cfg = validate.load_config(args.root, rep)
    if schema is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1

    corpus = args.root / "questions" / args.lang / cfg.get("questionFile", "questions.yaml")
    if not corpus.exists():
        print(f"{corpus} does not exist — is '{args.lang}' a language here?", file=sys.stderr)
        return 1
    entries = validate.parse_file(corpus, rep)
    if entries is None:
        print("\n".join(rep.errors), file=sys.stderr)
        return 1

    errors: list[str] = []
    drafts: list[Draft] = []
    for path in args.files:
        if not path.exists():
            errors.append(f"{path}: no such file")
            continue
        drafts.extend(parse_draft(path, args.lang, cfg, errors))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    check_drafts(drafts, args.lang, schema, cfg, corpus.parent, errors)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    known = {validate.normalize_text(entry["text"]) for entry in entries if "text" in entry}
    fresh: list[Draft] = []
    skipped: list[Draft] = []
    for draft in drafts:
        key = validate.normalize_text(draft.text)
        (skipped if key in known else fresh).append(draft)
        known.add(key)

    for draft in skipped:
        print(f"skipped (already in {args.lang}): {draft.text}")
    if not fresh:
        print(f"nothing to append to {corpus.relative_to(args.root)}.")
        return 0

    if args.dry_run:
        for draft in fresh:
            print(f"would append: {draft.text}")
        print(f"{len(fresh)} question(s) would go into {corpus.relative_to(args.root)}.")
        return 0

    key_order = cfg["keyOrder"]
    entries.extend(to_entry(draft) for draft in fresh)
    corpus.write_text(validate.format_file(args.lang, entries, key_order), encoding="utf-8")
    print(f"appended {len(fresh)} question(s) to {corpus.relative_to(args.root)}")
    return validate.main(["--fix", "--root", str(args.root)])


if __name__ == "__main__":
    sys.exit(main())
