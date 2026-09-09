#!/usr/bin/env python3
"""Parse, check, or apply a question-submission issue.

The issue body comes from ISSUE_BODY. `parse` emits workflow fields, `check`
validates against a temporary database, and `apply` appends an entry without
its generated id and date. Exit 78 identifies a new-language request.
"""

from __future__ import annotations

import html
import io
import json
import os
import re
import shutil
import sys
import tempfile
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SECTION_RE = re.compile(r"^### (.+?)\s*$", re.M)
NO_RESPONSE = "_No response_"
TAG_VOCAB = (
    "icebreaker",
    "reflective",
    "sexual",
    "dark",
    "hypothetical",
    "memory",
    "wouldyourather",
)
CREDIT_MAX = 40

NEW_LANGUAGE = 78
NATIVE_MARKER = "cicala-native:v1"


class Rejected(Exception):
    """The submission cannot become a database entry, and why."""


def parse_sections(body: str) -> dict[str, str]:
    sections = {}
    matches = list(SECTION_RE.finditer(body))
    for i, m in enumerate(matches):
        end = matches[i + 1].start() if i + 1 < len(matches) else len(body)
        value = body[m.end() : end].strip()
        sections[m.group(1).strip().lower()] = value
    return sections


def emit(key: str, value: str):
    out = os.environ.get("GITHUB_OUTPUT")
    if out:
        with open(out, "a", encoding="utf-8") as f:
            f.write(f"{key}={value}\n")
    print(f"{key}: {value}")


def issue_labels() -> set[str]:
    raw = os.environ.get("ISSUE_LABELS", "")
    if not raw:
        return set()
    try:
        parsed = json.loads(raw)
    except json.JSONDecodeError:
        parsed = raw.split(",")
    if not isinstance(parsed, list):
        return set()
    return {str(label).strip() for label in parsed if str(label).strip()}


def native_value(value: str) -> str:
    """Restore text escaped by the native website submission endpoint."""
    return html.unescape(value.removeprefix("> "))


def parse_issue(body: str, *, allow_unclassified_native: bool = False) -> dict:
    """The submission as a database entry, or Rejected with the reason.

    Raises SystemExit(NEW_LANGUAGE) for "other / new language", which is a
    request rather than a rejection.
    """
    if not body.strip():
        raise Rejected("the issue body is empty")

    s = parse_sections(body)
    native = NATIVE_MARKER in body
    labels = issue_labels() if native else set()

    lang_raw = s.get("language", "")
    m = re.search(r"\(([a-z]{2,3})\)", lang_raw)
    if "other" in lang_raw.lower() or not m:
        # The workflow handles new-language requests separately.
        emit("new_language", "true")
        sys.exit(NEW_LANGUAGE)
    lang = m.group(1)

    if native:
        editorial_labels = {
            *(f"depth:{depth}" for depth in (1, 2, 3)),
            *(f"tag:{tag}" for tag in TAG_VOCAB),
        }
        unknown = sorted(
            label
            for label in labels
            if label.startswith(("deck:", "depth:", "tag:")) and label not in editorial_labels
        )
        if unknown:
            raise Rejected(f"unknown editorial label(s): {', '.join(unknown)}")
        depth_labels = [depth for depth in (1, 2, 3) if f"depth:{depth}" in labels]
        if len(depth_labels) > 1:
            raise Rejected("choose exactly one editorial depth label")
        depth = depth_labels[0] if depth_labels else None
        classified = depth is not None
        if not classified and not allow_unclassified_native:
            raise Rejected(
                "classify the suggestion with exactly one depth:<1-3> label before approval"
            )
    else:
        depth_raw = s.get("depth", "")
        match = re.match(r"\s*([123])\b", depth_raw)
        if not match:
            raise Rejected(f"the depth field reads {depth_raw!r}; pick one of the three options")
        depth = int(match.group(1))
        classified = True

    text_raw = s.get("question", "")
    text = " ".join((native_value(text_raw) if native else text_raw).split())
    if not text or text == NO_RESPONSE:
        raise Rejected("the question field is empty")

    if native:
        tags = [tag for tag in TAG_VOCAB if f"tag:{tag}" in labels]
    else:
        tags_raw = s.get("tags (optional)", s.get("tags", ""))
        tags = []
        if tags_raw and tags_raw != NO_RESPONSE:
            tags = [t.strip() for t in tags_raw.split(",") if t.strip()]
            unknown_tags = sorted(set(tags) - set(TAG_VOCAB))
            if unknown_tags:
                raise Rejected(
                    "unknown or retired tags: "
                    + ", ".join(unknown_tags)
                    + "; a maintainer must reclassify them"
                )

    credit = s.get("name for credit (optional)", s.get("name for credit", ""))
    if credit == NO_RESPONSE:
        credit = ""
    if native:
        credit = native_value(credit)
    credit = " ".join(credit.split())[:CREDIT_MAX]

    cc0 = s.get("public domain dedication", "")
    if not re.search(r"- \[[xX]\]", cc0):
        raise Rejected(
            "the CC0 public-domain checkbox is not ticked, and without it the "
            "question cannot be merged"
        )

    return {
        "lang": lang,
        "text": text,
        "depth": depth,
        "tags": tags,
        "credit": credit,
        "native": native,
        "classified": classified,
    }


def render_entry(entry: dict) -> str:
    """The YAML for one entry, without id and added — validate.py --fix owns
    those."""

    def quoted(value: str) -> str:
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'

    lines = [f"- text: {quoted(entry['text'])}"]
    lines.append(f"  depth: {entry['depth']}")
    if entry["tags"]:
        lines.append(f"  tags: [{', '.join(entry['tags'])}]")
    if entry["credit"]:
        lines.append(f"  author: {quoted(entry['credit'])}")
    return "\n".join(lines) + "\n"


def target_for(lang: str) -> Path:
    target = ROOT / "questions" / lang / "questions.yaml"
    if not target.parent.is_dir():
        raise Rejected(
            f"'{lang}' is not a shipped language (there is no questions/{lang}/ "
            "directory); see docs/languages.md"
        )
    return target


def append_entry(target: Path, entry: dict) -> None:
    existing = target.read_text(encoding="utf-8") if target.exists() else ""
    if existing and not existing.endswith("\n"):
        existing += "\n"
    target.write_text(existing + render_entry(entry), encoding="utf-8")


# `--fix` assigns id and added, but only to a file that is otherwise clean.
ASSIGNED_BY_FIX_RE = re.compile(r"'(id|added)' is a required property")


def check_entry(entry: dict) -> list[str]:
    """Run the real validator over the database with this entry appended.

    The database on disk is already clean, so every error the validator
    reports belongs to the submission. Nothing is written outside a temp copy.
    """
    target_for(entry["lang"])  # rejects an unshipped language before any work

    import validate  # here, so `parse` mode runs without pyyaml or jsonschema

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        shutil.copytree(ROOT / "questions", root / "questions")
        candidate = entry
        if not entry["classified"]:
            candidate = {**entry, "depth": 1, "tags": []}
        append_entry(root / "questions" / entry["lang"] / "questions.yaml", candidate)

        err = io.StringIO()
        with redirect_stdout(io.StringIO()), redirect_stderr(err):
            validate.main(["--fix", "--root", str(root)])

    problems = []
    for line in err.getvalue().splitlines():
        if ": error: " not in line or ASSIGNED_BY_FIX_RE.search(line):
            continue
        # Remove the temporary file prefix from validator errors.
        problem = re.sub(r"^\S+: error: ", "", line)
        if problem not in problems:
            problems.append(problem)
    return problems


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "parse"
    body = os.environ.get("ISSUE_BODY", "")
    issue = os.environ.get("ISSUE_NUMBER", "0")

    try:
        native = NATIVE_MARKER in body
        emit("native", str(native).lower())
        entry = parse_issue(body, allow_unclassified_native=native and mode in {"parse", "check"})

        emit("lang", entry["lang"])
        emit("classified", str(entry["classified"]).lower())
        first_words = " ".join(entry["text"].split()[:6])
        emit("pr_title", f"question({entry['lang']}): {first_words}… (#{issue})")
        if mode == "parse":
            return 0

        target = target_for(entry["lang"])

        if mode == "check":
            problems = check_entry(entry)
            for problem in problems:
                print(f"error: {problem}", file=sys.stderr)
            if problems:
                return 1
            print("the submission passes every automatic check")
            return 0

        append_entry(target, entry)
        print(f"appended to {target.relative_to(ROOT)} — now run tools/validate.py --fix")
        return 0
    except Rejected as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
