#!/usr/bin/env python3
"""Turn an approved question-submission issue into a database entry.

Used by .github/workflows/promote-question.yml. Reads the GitHub issue-form
body from the ISSUE_BODY environment variable (never from argv — issue text is
untrusted), parses the structured sections, appends the entry (without id and
added — validate.py --fix assigns those) to questions/{lang}/{category}.yaml,
and emits `lang`, `category` and `pr_title` to $GITHUB_OUTPUT.

Modes:
  promote_issue.py parse   only parse and emit outputs (used on issue open
                           to add the lang:{code} label)
  promote_issue.py apply   parse and append to the YAML file

Exit 0 on success; exit 78 for "new language" submissions (the workflow
replies with the incubator explainer instead of failing).
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SECTION_RE = re.compile(r"^### (.+?)\s*$", re.M)
NO_RESPONSE = "_No response_"


def parse_sections(body: str) -> dict[str, str]:
    sections = {}
    matches = list(SECTION_RE.finditer(body))
    for i, m in enumerate(matches):
        end = matches[i + 1].start() if i + 1 < len(matches) else len(body)
        value = body[m.end():end].strip()
        sections[m.group(1).strip().lower()] = value
    return sections


def fail(msg: str, code: int = 1):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(code)


def emit(key: str, value: str):
    out = os.environ.get("GITHUB_OUTPUT")
    if out:
        with open(out, "a", encoding="utf-8") as f:
            f.write(f"{key}={value}\n")
    print(f"{key}: {value}")


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "parse"
    body = os.environ.get("ISSUE_BODY", "")
    issue = os.environ.get("ISSUE_NUMBER", "0")
    if not body.strip():
        fail("ISSUE_BODY is empty")

    s = parse_sections(body)

    lang_raw = s.get("language", "")
    m = re.search(r"\(([a-z]{2,3})\)", lang_raw)
    if "other" in lang_raw.lower() or not m:
        # not an error — the workflow posts the new-language explainer
        emit("new_language", "true")
        sys.exit(78)
    lang = m.group(1)

    category = s.get("category", "").strip()
    if category not in ("party", "family", "love", "work", "deep"):
        fail(f"unknown category {category!r}")

    text = " ".join(s.get("question", "").split())
    if not text or text == NO_RESPONSE:
        fail("empty question text")

    tags_raw = s.get("tags (optional)", s.get("tags", ""))
    tags = []
    if tags_raw and tags_raw != NO_RESPONSE:
        vocab = {"icebreaker", "reflective", "spicy", "hypothetical", "memory", "wouldyourather"}
        tags = [t.strip() for t in tags_raw.split(",") if t.strip() in vocab]

    credit = s.get("name for credit (optional)", s.get("name for credit", ""))
    if credit == NO_RESPONSE:
        credit = ""
    credit = " ".join(credit.split())[:40]

    cc0 = s.get("public domain dedication", "")
    if not re.search(r"- \[[xX]\]", cc0):
        fail("the CC0 public-domain checkbox is not ticked — cannot promote")

    emit("lang", lang)
    emit("category", category)
    first_words = " ".join(text.split()[:6])
    emit("pr_title", f"question({lang}): {first_words}… (#{issue})")

    if mode != "apply":
        return 0

    target = ROOT / "questions" / lang / f"{category}.yaml"
    if not target.parent.is_dir():
        fail(f"language '{lang}' is not shipped (no questions/{lang}/ directory)")

    escaped = text.replace("\\", "\\\\").replace('"', '\\"')
    entry_lines = [f'- text: "{escaped}"']
    if tags:
        entry_lines.append(f"  tags: [{', '.join(tags)}]")
    if credit:
        cred_escaped = credit.replace("\\", "\\\\").replace('"', '\\"')
        entry_lines.append(f'  author: "{cred_escaped}"')
    entry = "\n".join(entry_lines) + "\n"

    existing = target.read_text(encoding="utf-8") if target.exists() else ""
    if existing and not existing.endswith("\n"):
        existing += "\n"
    target.write_text(existing + entry, encoding="utf-8")
    print(f"appended to {target.relative_to(ROOT)} — now run tools/validate.py --fix")
    return 0


if __name__ == "__main__":
    sys.exit(main())
