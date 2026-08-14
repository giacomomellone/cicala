#!/usr/bin/env python3
"""Turn a question-submission issue into a database entry.

Used by .github/workflows/promote-question.yml. Reads the GitHub issue-form
body from the ISSUE_BODY environment variable (never from argv — issue text is
untrusted), parses the structured sections, and emits `lang` and `pr_title` to
$GITHUB_OUTPUT.

Modes:
  promote_issue.py parse   only parse and emit outputs (used on issue open
                           to add the lang:{code} label)
  promote_issue.py check   parse, then run the entry through the real
                           validator against a copy of the database, and
                           report every problem in prose. Writes nothing.
  promote_issue.py apply   parse and append to questions/{lang}/questions.yaml
                           (without id and added — validate.py --fix assigns
                           those)

Exit 0 on success; 1 on a problem with the submission; 78 for "new language"
submissions (the workflow replies with the incubator explainer instead).
"""

from __future__ import annotations

import io
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
DECK_VOCAB = ("new_people", "close", "family", "work", "here", "wild")
TAG_VOCAB = frozenset(
    {"icebreaker", "reflective", "spicy", "dark", "hypothetical", "memory", "wouldyourather"}
)
CREDIT_MAX = 40

# Exit code the workflow reads as "this is a new-language request, reply with
# the incubator explainer" rather than as a failure.
NEW_LANGUAGE = 78


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


def parse_issue(body: str) -> dict:
    """The submission as a database entry, or Rejected with the reason.

    Raises SystemExit(NEW_LANGUAGE) for "other / new language", which is a
    request rather than a rejection.
    """
    if not body.strip():
        raise Rejected("the issue body is empty")

    s = parse_sections(body)

    lang_raw = s.get("language", "")
    m = re.search(r"\(([a-z]{2,3})\)", lang_raw)
    if "other" in lang_raw.lower() or not m:
        # not an error — the workflow posts the new-language explainer
        emit("new_language", "true")
        sys.exit(NEW_LANGUAGE)
    lang = m.group(1)

    decks_raw = s.get("decks", "")
    decks = [deck for deck in DECK_VOCAB if re.search(rf"\b{deck}\b", decks_raw)]
    if not decks:
        raise Rejected("choose at least one deck")

    depth_raw = s.get("depth", "")
    match = re.match(r"\s*([123])\b", depth_raw)
    if not match:
        raise Rejected(f"the depth field reads {depth_raw!r}; pick one of the three options")
    depth = int(match.group(1))

    text = " ".join(s.get("question", "").split())
    if not text or text == NO_RESPONSE:
        raise Rejected("the question field is empty")

    tags_raw = s.get("tags (optional)", s.get("tags", ""))
    tags = []
    if tags_raw and tags_raw != NO_RESPONSE:
        tags = [t.strip() for t in tags_raw.split(",") if t.strip() in TAG_VOCAB]
    if {"spicy", "dark"}.intersection(tags) and decks != ["wild"]:
        raise Rejected("dark and spicy questions must use only the wild deck")

    credit = s.get("name for credit (optional)", s.get("name for credit", ""))
    if credit == NO_RESPONSE:
        credit = ""
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
        "decks": decks,
        "depth": depth,
        "tags": tags,
        "credit": credit,
    }


def render_entry(entry: dict) -> str:
    """The YAML for one entry, without id and added — validate.py --fix owns
    those."""

    def quoted(value: str) -> str:
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'

    lines = [f"- text: {quoted(entry['text'])}"]
    lines.append(f"  decks: [{', '.join(entry['decks'])}]")
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


# `--fix` assigns id and added, but only to a file that is otherwise clean. On
# a file it refuses to rewrite it therefore also reports these two, which say
# nothing about the submission.
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
        append_entry(root / "questions" / entry["lang"] / "questions.yaml", entry)

        err = io.StringIO()
        with redirect_stdout(io.StringIO()), redirect_stderr(err):
            validate.main(["--fix", "--root", str(root)])

    problems = []
    for line in err.getvalue().splitlines():
        if ": error: " not in line or ASSIGNED_BY_FIX_RE.search(line):
            continue
        # the file:line prefix names a path inside the temp copy; the message
        # body keeps any repo-relative reference, which is the useful part
        problem = re.sub(r"^\S+: error: ", "", line)
        if problem not in problems:
            problems.append(problem)
    return problems


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "parse"
    body = os.environ.get("ISSUE_BODY", "")
    issue = os.environ.get("ISSUE_NUMBER", "0")

    try:
        entry = parse_issue(body)

        emit("lang", entry["lang"])
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
