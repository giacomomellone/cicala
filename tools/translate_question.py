#!/usr/bin/env python3
"""Plan and create reviewed Google Cloud question-translation drafts."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from collections import defaultdict
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
MATERIAL_FIELDS = ("text", "depth", "tags")
MAX_TEXTS_PER_REQUEST = 128
RETRYABLE_HTTP_CODES = {429, 500, 502, 503, 504}
TRANSLATION_PROVIDER = "google"


class TranslationError(RuntimeError):
    pass


@dataclass(frozen=True)
class SourceQuestion:
    language: str
    entry: dict
    text_changed: bool = True

    @property
    def origin(self) -> str:
        return self.entry.get("origin") or self.entry["id"]


def material_revision(entry: dict) -> str:
    values = {field: entry.get(field) for field in MATERIAL_FIELDS}
    values["tags"] = sorted(entry.get("tags") or [])
    return hashlib.sha256(
        json.dumps(values, sort_keys=True, ensure_ascii=False).encode()
    ).hexdigest()


def load_config(root: Path) -> dict:
    schema = json.loads((root / "questions" / "schema.json").read_text(encoding="utf-8"))
    return schema["x-cicala"]


def load_yaml(text: str) -> list[dict]:
    data = yaml.safe_load(text)
    if data is None:
        return []
    if not isinstance(data, list) or any(not isinstance(entry, dict) for entry in data):
        raise TranslationError("question corpus must be a YAML list of mappings")
    return data


def corpus_at_ref(root: Path, ref: str, language: str, question_file: str) -> list[dict]:
    rel = f"questions/{language}/{question_file}"
    result = subprocess.run(
        ["git", "show", f"{ref}:{rel}"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        return load_yaml(result.stdout)
    if "does not exist" in result.stderr or "exists on disk, but not in" in result.stderr:
        return []
    raise TranslationError(f"cannot read {rel} at {ref}: {result.stderr.strip()}")


def is_human_original(entry: dict) -> bool:
    return not entry.get("origin") and not entry.get("translated_by")


def changed_questions(
    before: dict[str, list[dict]], after: dict[str, list[dict]]
) -> list[SourceQuestion]:
    changed = []
    for language, current_entries in after.items():
        previous_by_id = {entry.get("id"): entry for entry in before.get(language, [])}
        for entry in current_entries:
            if not entry.get("id"):
                continue
            previous = previous_by_id.get(entry["id"])
            if previous is None:
                if is_human_original(entry):
                    changed.append(SourceQuestion(language, entry))
            elif material_revision(previous) != material_revision(entry):
                if entry.get("translation_sync", {}).get("revision") == material_revision(entry):
                    continue
                changed.append(
                    SourceQuestion(
                        language,
                        entry,
                        text_changed=previous.get("text") != entry.get("text"),
                    )
                )
    return changed


def translation_plan(
    changes: list[SourceQuestion],
    target_corpora: dict[str, list[dict]],
    config: dict,
) -> dict[str, list[SourceQuestion]]:
    planned: dict[str, list[SourceQuestion]] = defaultdict(list)
    languages = config.get("languages", {})
    families: dict[str, SourceQuestion] = {}
    for source in changes:
        source_api = languages.get(source.language, {}).get("google", {}).get("source")
        if not source_api:
            continue
        if source.origin in families:
            other = families[source.origin]
            raise TranslationError(
                f"conflicting edits for {source.origin}: {other.entry['id']} ({other.language}) "
                f"and {source.entry['id']} ({source.language}); rerun with --source-id "
                "to choose the accepted source"
            )
        families[source.origin] = source
        for target_language, target_config in languages.items():
            target_api = target_config.get("google", {}).get("target")
            if target_language == source.language or not target_api:
                continue
            entries = target_corpora.get(target_language, [])
            if any(entry.get("id") == source.origin for entry in entries):
                continue
            existing = [entry for entry in entries if entry.get("origin") == source.origin]
            if len(existing) > 1:
                raise TranslationError(
                    f"multiple translations of {source.origin} in {target_language}; "
                    "keep one linked version before synchronizing"
                )
            planned[target_language].append(source)
    return dict(planned)


class GoogleTranslateClient:
    def __init__(
        self,
        api_key: str,
        *,
        api_url: str | None = None,
        opener: Callable = urllib.request.urlopen,
        sleeper: Callable[[float], None] = time.sleep,
        attempts: int = 4,
    ):
        if not api_key:
            raise TranslationError("GOOGLE_TRANSLATE_API_KEY is not set")
        self.endpoint = api_url or "https://translation.googleapis.com/language/translate/v2"
        self.api_key = api_key
        self.opener = opener
        self.sleeper = sleeper
        self.attempts = attempts
        self.characters_sent = 0

    def translate(
        self,
        texts: list[str],
        source_lang: str,
        target_lang: str,
    ) -> list[str]:
        translated = []
        for offset in range(0, len(texts), MAX_TEXTS_PER_REQUEST):
            translated.extend(
                self._translate_batch(
                    texts[offset : offset + MAX_TEXTS_PER_REQUEST],
                    source_lang,
                    target_lang,
                )
            )
        return translated

    def _translate_batch(self, texts: list[str], source_lang: str, target_lang: str) -> list[str]:
        request_body = {
            "q": texts,
            "source": source_lang,
            "target": target_lang,
            "format": "text",
            "model": "nmt",
        }
        payload = json.dumps(request_body).encode()
        request = urllib.request.Request(
            self.endpoint,
            data=payload,
            headers={
                "X-Goog-Api-Key": self.api_key,
                "Content-Type": "application/json; charset=utf-8",
                "User-Agent": "cicala-question-translator/1",
            },
            method="POST",
        )
        for attempt in range(self.attempts):
            try:
                with self.opener(request, timeout=30) as response:
                    body = json.loads(response.read().decode("utf-8"))
                rows = body.get("data", {}).get("translations", [])
                if len(rows) != len(texts):
                    raise TranslationError(
                        "Google Cloud Translation returned an unexpected number of translations"
                    )
                self.characters_sent += sum(len(text) for text in texts)
                return [" ".join(row["translatedText"].split()) for row in rows]
            except urllib.error.HTTPError as exc:
                if exc.code == 403:
                    raise TranslationError(
                        "Google Cloud Translation rejected the API key or project (HTTP 403)"
                    ) from exc
                if exc.code not in RETRYABLE_HTTP_CODES or attempt + 1 == self.attempts:
                    if exc.code == 429:
                        raise TranslationError(
                            "Google Cloud Translation quota or rate limit exceeded (HTTP 429)"
                        ) from exc
                    raise TranslationError(
                        f"Google Cloud Translation request failed with HTTP {exc.code}"
                    ) from exc
            except urllib.error.URLError as exc:
                if attempt + 1 == self.attempts:
                    raise TranslationError(
                        f"Google Cloud Translation request failed: {exc.reason}"
                    ) from exc
            self.sleeper(2**attempt)
        raise AssertionError("unreachable")


def apply_translations(
    target_entries: list[dict],
    sources: list[SourceQuestion],
    translated_texts: list[str],
) -> int:
    if len(sources) != len(translated_texts):
        raise TranslationError("translation count does not match source count")
    changed = 0
    for source, text in zip(sources, translated_texts, strict=True):
        if any(entry.get("id") == source.origin for entry in target_entries):
            raise TranslationError(f"cannot translate over human-written original {source.origin}")
        candidates = [entry for entry in target_entries if entry.get("origin") == source.origin]
        if len(candidates) > 1:
            raise TranslationError(f"multiple translations of {source.origin} in target corpus")
        values = {
            "text": text,
            "depth": source.entry["depth"],
            "origin": source.origin,
        }
        if source.text_changed or not candidates:
            values["translated_by"] = TRANSLATION_PROVIDER
        if source.entry.get("tags"):
            values["tags"] = list(source.entry["tags"])
        values["translation_sync"] = {
            "source": source.entry["id"],
            "source_revision": material_revision(source.entry),
            "revision": material_revision(values),
        }
        if candidates:
            candidate = candidates[0]
            if any(candidate.get(key) != value for key, value in values.items()) or (
                "tags" in candidate and "tags" not in values
            ):
                candidate.update(values)
                if "tags" not in values:
                    candidate.pop("tags", None)
                changed += 1
        else:
            target_entries.append(values)
            changed += 1
    return changed


def corpora_for_refs(root: Path, before_ref: str, after_ref: str, config: dict):
    languages = config.get("languages", {})
    question_file = config.get("questionFile", "questions.yaml")
    before = {
        language: corpus_at_ref(root, before_ref, language, question_file) for language in languages
    }
    after = {
        language: corpus_at_ref(root, after_ref, language, question_file) for language in languages
    }
    return before, after


def build_plan(
    root: Path, before_ref: str, after_ref: str, config: dict, source_id: str | None = None
):
    if before_ref and set(before_ref) == {"0"}:
        return {}
    before, after = corpora_for_refs(root, before_ref, after_ref, config)
    changes = changed_questions(before, after)
    if source_id:
        changes = [source for source in changes if source.entry["id"] == source_id]
        if not changes:
            raise TranslationError(f"source {source_id} has no human edit in the selected range")
    return translation_plan(changes, after, config)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    subparsers = parser.add_subparsers(dest="command", required=True)

    plan_parser = subparsers.add_parser("plan", help="print target languages as a JSON array")
    plan_parser.add_argument("--before-ref", required=True)
    plan_parser.add_argument("--after-ref", required=True)
    plan_parser.add_argument("--source-id", help="resolve simultaneous edits using this question")

    apply_parser = subparsers.add_parser("apply", help="translate and update one target corpus")
    apply_parser.add_argument("--before-ref", required=True)
    apply_parser.add_argument("--after-ref", required=True)
    apply_parser.add_argument("--target", required=True)
    apply_parser.add_argument("--source-id", help="resolve simultaneous edits using this question")

    args = parser.parse_args(argv)
    try:
        config = load_config(args.root)
        plan = build_plan(args.root, args.before_ref, args.after_ref, config, args.source_id)
        if args.command == "plan":
            print(json.dumps(sorted(plan)))
            return 0

        sources = plan.get(args.target, [])
        if not sources:
            print(f"No translation drafts needed for {args.target}.")
            return 0
        target_config = config["languages"].get(args.target, {}).get("google", {})
        target_path = (
            args.root / "questions" / args.target / config.get("questionFile", "questions.yaml")
        )
        target_entries = load_yaml(target_path.read_text(encoding="utf-8"))
        translated_by_index = {}
        grouped = defaultdict(list)
        for index, source in enumerate(sources):
            existing = next(
                (entry for entry in target_entries if entry.get("origin") == source.origin),
                None,
            )
            if existing and not source.text_changed:
                translated_by_index[index] = existing["text"]
                continue
            source_code = config["languages"][source.language]["google"]["source"]
            grouped[source_code].append((index, source))
        client = None
        if grouped:
            client = GoogleTranslateClient(
                os.environ.get("GOOGLE_TRANSLATE_API_KEY", ""),
                api_url=os.environ.get("GOOGLE_TRANSLATE_API_URL"),
            )
        for source_code, indexed_sources in grouped.items():
            texts = [source.entry["text"] for _, source in indexed_sources]
            results = client.translate(
                texts,
                source_code,
                target_config["target"],
            )
            for (index, _), text in zip(indexed_sources, results, strict=True):
                translated_by_index[index] = text
        translated = [translated_by_index[index] for index in range(len(sources))]
        count = apply_translations(target_entries, sources, translated)
        target_path.write_text(
            yaml.safe_dump(target_entries, allow_unicode=True, sort_keys=False), encoding="utf-8"
        )
        print(
            f"Updated {count} Google Cloud draft(s) for {args.target}; "
            f"characters sent: {client.characters_sent if client else 0}."
        )
        return 0
    except (OSError, ValueError, KeyError, TranslationError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
