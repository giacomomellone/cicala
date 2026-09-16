"""Tests for tools/validate.py, build_site_data.py, build_bundle.py.

Run from the repo root:  python -m unittest discover tools/tests
Uses only the stdlib (unittest) per the tools dependency policy.
"""

import gzip
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import urllib.error
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))

import build_bundle  # noqa: E402
import build_firmware_manifest  # noqa: E402
import build_site_data  # noqa: E402
import check_release  # noqa: E402
import import_drafts  # noqa: E402
import promote_issue  # noqa: E402
import translate_question  # noqa: E402
import validate  # noqa: E402
import yaml  # noqa: E402


def run_quiet(main, argv):
    with redirect_stdout(io.StringIO()) as out, redirect_stderr(io.StringIO()) as err:
        code = main(argv)
    return code, out.getvalue(), err.getvalue()


class TmpDb(unittest.TestCase):
    """Base: a minimal temp database with the real schema."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        q = self.tmp / "questions"
        (q / "en").mkdir(parents=True)
        shutil.copy(REPO / "questions" / "schema.json", q / "schema.json")
        (q / "en" / "STYLE.md").write_text("# style\n")
        (q / "en" / "denylist.txt").write_text("# list\nbadword\nnaughty phrase\n")

    def write(self, rel, text):
        path = self.tmp / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def question(self, text, depth=1, extra=""):
        return f'- text: "{text}"\n  depth: {depth}\n{extra}'


class TestIdAssignment(TmpDb):
    def test_fix_assigns_expected_id_and_date(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "When did you last change your mind about something important?",
                2,
                "  tags: [reflective]\n",
            ),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        content = (self.tmp / "questions/en/questions.yaml").read_text()
        # IDs use the documented hash construction.
        expected = validate.compute_id(
            "en", "When did you last change your mind about something important?"
        )
        self.assertIn(f"id: {expected}", content)
        self.assertIn("added: ", content)

    def test_missing_id_without_fix_fails(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("What matters most to you today?", 2),
        )
        code, _, err = run_quiet(validate.main, ["--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("--fix", err)

    def test_existing_id_is_preserved_after_text_edit(self):
        # Text edits preserve assigned IDs.
        self.write(
            "questions/en/questions.yaml",
            "- id: q-00000000\n"
            '  text: "What matters most to you todayy?"\n'
            "  depth: 2\n"
            '  added: "2026-01-01"\n',
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        self.assertIn("id: q-00000000", (self.tmp / "questions/en/questions.yaml").read_text())

    def test_fix_is_idempotent(self):
        path = self.write(
            "questions/en/questions.yaml",
            self.question("What was the best part of your week so far?"),
        )
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        first = path.read_text()
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(first, path.read_text())


class TestDedup(TmpDb):
    def test_duplicate_within_language_fails(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("What made you laugh today, honestly?")
            + self.question("What  made you LAUGH today,   honestly?", 2),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("duplicate question text", err)

    def test_same_text_in_other_language_is_fine(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("What made you laugh today, honestly?"),
        )
        (self.tmp / "questions/de").mkdir()
        (self.tmp / "questions/de/STYLE.md").write_text("# stil\n")
        (self.tmp / "questions/de/denylist.txt").write_text("egal\n")
        self.write(
            "questions/de/questions.yaml",
            self.question("What made you laugh today, honestly?"),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)


class TestDenylist(TmpDb):
    def test_denylist_hit_fails_with_review_message(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("Who said the badword at dinner?"),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("review", err)

    def test_denylist_matches_whole_words_only(self):
        # Denylist terms match whole words.
        self.write(
            "questions/en/questions.yaml",
            self.question("What does badwording mean to you?"),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)

    def test_denylist_matches_phrases(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("Is a naughty phrase always funny to you?"),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)


class TestTextRules(TmpDb):
    def test_missing_question_mark_fails(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("Tell me about your best day ever.", 2),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("must end with", err)

    def test_terminator_followed_by_closing_quote_passes(self):
        self.write(
            "questions/en/questions.yaml",
            "- text: 'When did you last ask yourself \"why not me?\"'\n  depth: 2\n",
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)

    def test_too_short_fails(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("Why not?", 2),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)


class TestOrigin(TmpDb):
    def test_dangling_origin_warns_but_passes(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                2,
                "  origin: q-deadbeef\n",
            ),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        self.assertIn("q-deadbeef", err)
        self.assertIn("warning", err)

    def test_machine_translation_requires_an_origin(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                2,
                "  translated_by: google\n",
            ),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("origin", err)

    def test_machine_translation_requires_an_existing_origin(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                2,
                "  origin: q-deadbeef\n  translated_by: google\n",
            ),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("missing from the database", err)

    def test_origin_must_point_to_another_language(self):
        self.write(
            "questions/en/questions.yaml",
            "- id: q-11111111\n"
            '  text: "What belief have you outgrown lately?"\n'
            "  depth: 2\n"
            '  added: "2026-01-01"\n'
            "- id: q-22222222\n"
            '  text: "What idea have you stopped believing?"\n'
            "  depth: 2\n"
            "  origin: q-11111111\n"
            '  added: "2026-01-01"\n',
        )
        code, _, err = run_quiet(validate.main, ["--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("another language", err)

    def test_machine_translation_cannot_use_an_adaptation_as_its_source(self):
        self.write(
            "questions/en/questions.yaml",
            "- id: q-11111111\n"
            '  text: "What belief have you outgrown lately?"\n'
            "  depth: 2\n"
            "  origin: q-00000000\n"
            '  added: "2026-01-01"\n',
        )
        (self.tmp / "questions/de").mkdir()
        (self.tmp / "questions/de/STYLE.md").write_text("# stil\n")
        (self.tmp / "questions/de/denylist.txt").write_text("# list\n")
        self.write(
            "questions/de/questions.yaml",
            "- id: q-22222222\n"
            '  text: "Welche Überzeugung hast du in letzter Zeit abgelegt?"\n'
            "  depth: 2\n"
            "  origin: q-11111111\n"
            "  translated_by: google\n"
            '  added: "2026-01-01"\n',
        )
        code, _, err = run_quiet(validate.main, ["--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("not a human-written original", err)


class TestTranslationPlanning(unittest.TestCase):
    def question(self, qid, text, **extra):
        entry = {"id": qid, "text": text, "depth": 2}
        entry.update(extra)
        return entry

    def test_new_originals_and_human_edits_in_any_language_are_selected(self):
        original = self.question("q-11111111", "What changed your mind?")
        machine = self.question(
            "q-22222222",
            "Was hat deine Meinung geändert?",
            origin="q-11111111",
            translated_by="google",
        )
        adaptation = self.question("q-33333333", "What made you reconsider?", origin="q-11111111")
        before = {"en": [original, machine, adaptation]}
        after = {
            "en": [
                {**original, "text": "What changed your mind most recently?"},
                {**machine, "text": "Was änderte deine Meinung?"},
                {**adaptation, "text": "What made you reconsider recently?"},
                self.question("q-44444444", "What did you learn today?"),
            ]
        }

        selected = translate_question.changed_questions(before, after)

        self.assertEqual(
            [item.entry["id"] for item in selected],
            ["q-11111111", "q-22222222", "q-33333333", "q-44444444"],
        )

    def test_author_only_edits_do_not_spend_translation_quota(self):
        original = self.question("q-11111111", "What changed your mind?", author="A")
        selected = translate_question.changed_questions(
            {"en": [original]}, {"en": [{**original, "author": "Ada"}]}
        )
        self.assertEqual(selected, [])

    def test_editorial_edit_updates_descendants_without_retranslating_text(self):
        original = self.question("q-11111111", "What changed your mind?")
        selected = translate_question.changed_questions(
            {"en": [original]}, {"en": [{**original, "tags": ["reflective"]}]}
        )
        self.assertEqual(len(selected), 1)
        self.assertFalse(selected[0].text_changed)

    def test_plan_uses_only_opted_in_targets_including_human_translations(self):
        source = translate_question.SourceQuestion(
            "en", self.question("q-11111111", "What changed your mind?")
        )
        config = {
            "languages": {
                "en": {"google": {"source": "en", "target": "en"}},
                "de": {"google": {"source": "de", "target": "de"}},
                "fr": {},
            }
        }
        plan = translate_question.translation_plan([source], {"de": [], "fr": []}, config)
        self.assertEqual(list(plan), ["de"])

        human_adaptation = self.question(
            "q-22222222", "Was hat deine Meinung geändert?", origin="q-11111111"
        )
        plan = translate_question.translation_plan(
            [source], {"de": [human_adaptation], "fr": []}, config
        )
        self.assertEqual(list(plan), ["de"])

    def test_apply_adds_provenance_and_copies_editorial_metadata(self):
        source = translate_question.SourceQuestion(
            "en",
            self.question(
                "q-11111111",
                "What changed your mind?",
                depth=1,
                tags=["reflective"],
                author="Ada",
            ),
        )
        target = []

        count = translate_question.apply_translations(
            target, [source], ["Was hat deine Meinung geändert?"]
        )

        self.assertEqual(count, 1)
        self.assertEqual(
            target,
            [
                {
                    "text": "Was hat deine Meinung geändert?",
                    "depth": 1,
                    "tags": ["reflective"],
                    "origin": "q-11111111",
                    "translated_by": "google",
                    "translation_sync": {
                        "source": source.entry["id"],
                        "source_revision": translate_question.material_revision(source.entry),
                        "revision": translate_question.material_revision(
                            {
                                "text": "Was hat deine Meinung geändert?",
                                "depth": 1,
                                "tags": ["reflective"],
                            }
                        ),
                    },
                }
            ],
        )

    def test_apply_updates_a_google_descendant_without_replacing_its_id(self):
        source = translate_question.SourceQuestion(
            "en", self.question("q-11111111", "What changed your mind?", tags=[])
        )
        target = [
            self.question(
                "q-22222222",
                "Was hat deine Meinung geändert?",
                origin="q-11111111",
                translated_by="google",
                tags=["reflective"],
                added="2026-01-01",
            )
        ]

        count = translate_question.apply_translations(
            target, [source], ["Was hat dich umgestimmt?"]
        )

        self.assertEqual(count, 1)
        self.assertEqual(target[0]["id"], "q-22222222")
        self.assertEqual(target[0]["added"], "2026-01-01")
        self.assertEqual(target[0]["text"], "Was hat dich umgestimmt?")
        self.assertNotIn("tags", target[0])

    def test_build_plan_reads_the_exact_git_push_range(self):
        with tempfile.TemporaryDirectory() as tmp_name:
            root = Path(tmp_name)
            (root / "questions/en").mkdir(parents=True)
            (root / "questions/de").mkdir(parents=True)
            # A fixture rather than the repo schema: this test is about the commit
            # range, and the shipped languages opt into translation independently.
            (root / "questions/schema.json").write_text(
                json.dumps(
                    {
                        "x-cicala": {
                            "questionFile": "questions.yaml",
                            "languages": {
                                "en": {"google": {"source": "en", "target": "en"}},
                                "de": {"google": {"source": "de", "target": "de"}},
                            },
                        }
                    }
                ),
                encoding="utf-8",
            )
            (root / "questions/de/questions.yaml").write_text("", encoding="utf-8")
            corpus = root / "questions/en/questions.yaml"
            corpus.write_text(
                "- id: q-11111111\n"
                '  text: "What changed your mind most recently?"\n'
                "  depth: 2\n"
                '  added: "2026-01-01"\n',
                encoding="utf-8",
            )
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            subprocess.run(["git", "config", "user.name", "test"], cwd=root, check=True)
            subprocess.run(
                ["git", "config", "user.email", "test@example.invalid"], cwd=root, check=True
            )
            subprocess.run(["git", "add", "questions"], cwd=root, check=True)
            subprocess.run(["git", "commit", "-qm", "before"], cwd=root, check=True)
            before = subprocess.run(
                ["git", "rev-parse", "HEAD"], cwd=root, check=True, capture_output=True, text=True
            ).stdout.strip()

            with corpus.open("a", encoding="utf-8") as handle:
                handle.write(
                    "- id: q-22222222\n"
                    '  text: "What did you learn today?"\n'
                    "  depth: 1\n"
                    '  added: "2026-01-02"\n'
                )
            subprocess.run(["git", "add", "questions/en/questions.yaml"], cwd=root, check=True)
            subprocess.run(["git", "commit", "-qm", "after"], cwd=root, check=True)
            after = subprocess.run(
                ["git", "rev-parse", "HEAD"], cwd=root, check=True, capture_output=True, text=True
            ).stdout.strip()

            plan = translate_question.build_plan(
                root, before, after, translate_question.load_config(root)
            )

            self.assertEqual([source.entry["id"] for source in plan["de"]], ["q-22222222"])


class TestGoogleTranslateClient(unittest.TestCase):
    def response(self, texts):
        payload = {"data": {"translations": [{"translatedText": text} for text in texts]}}
        return io.BytesIO(json.dumps(payload).encode())

    def test_sends_documented_json_and_keeps_key_out_of_url(self):
        captured = {}

        def opener(request, timeout):
            captured["request"] = request
            captured["timeout"] = timeout
            return self.response([" Was hat dich umgestimmt?\n"])

        client = translate_question.GoogleTranslateClient("secret", opener=opener)
        source = "What changed your mind?"
        result = client.translate([source], "en", "de")

        request = captured["request"]
        payload = json.loads(request.data)
        self.assertEqual(
            request.full_url, "https://translation.googleapis.com/language/translate/v2"
        )
        self.assertNotIn("secret", request.full_url)
        self.assertEqual(request.get_header("X-goog-api-key"), "secret")
        self.assertEqual(captured["timeout"], 30)
        self.assertEqual(payload["q"], [source])
        self.assertEqual(payload["source"], "en")
        self.assertEqual(payload["target"], "de")
        self.assertEqual(payload["format"], "text")
        self.assertEqual(payload["model"], "nmt")
        self.assertEqual(result, ["Was hat dich umgestimmt?"])
        self.assertEqual(client.characters_sent, len(source))

    def test_endpoint_can_be_overridden_for_local_testing(self):
        client = translate_question.GoogleTranslateClient(
            "secret", api_url="http://127.0.0.1:8080/translate"
        )
        self.assertEqual(client.endpoint, "http://127.0.0.1:8080/translate")

    def test_retryable_status_uses_exponential_backoff(self):
        calls = []
        sleeps = []

        def opener(request, timeout):
            calls.append(request)
            if len(calls) == 1:
                raise urllib.error.HTTPError(request.full_url, 429, "busy", {}, None)
            return self.response(["Übersetzung?"])

        client = translate_question.GoogleTranslateClient(
            "secret", opener=opener, sleeper=sleeps.append
        )
        self.assertEqual(client.translate(["Question?"], "en", "de"), ["Übersetzung?"])
        self.assertEqual(sleeps, [1])

    def test_quota_error_is_clear_and_does_not_expose_the_key(self):
        def opener(request, timeout):
            raise urllib.error.HTTPError(request.full_url, 429, "quota", {}, None)

        client = translate_question.GoogleTranslateClient(
            "very-secret", opener=opener, sleeper=lambda _: None
        )
        with self.assertRaisesRegex(
            translate_question.TranslationError, "quota or rate limit"
        ) as ctx:
            client.translate(["Question?"], "en", "de")
        self.assertNotIn("very-secret", str(ctx.exception))


class TestSiteData(TmpDb):
    def test_incubator_excluded_and_fields_stripped(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                2,
                '  author: "seed"\n',
            ),
        )
        (self.tmp / "questions/incubator/fr").mkdir(parents=True)
        self.write(
            "questions/incubator/fr/questions.yaml",
            self.question("Quelle certitude as-tu abandonnée récemment?", 2),
        )
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        out = self.tmp / "site_data"
        code, _, _ = run_quiet(build_site_data.main, ["--root", str(self.tmp), "--out", str(out)])
        self.assertEqual(code, 0)
        self.assertTrue((out / "questions.en.json").exists())
        self.assertFalse((out / "questions.fr.json").exists())
        langs = json.loads((out / "languages.json").read_text())
        self.assertEqual([language["code"] for language in langs], ["en"])
        payload = json.loads((out / "questions.en.json").read_text())
        entry = payload["questions"][0]
        self.assertEqual(
            set(entry), {"id", "text", "depth", "tags"}
        )  # author/added omitted; this human original has no provenance
        self.assertFalse((out / "recent.en.json").exists())
        index = json.loads((out / "index.json").read_text())
        self.assertEqual(list(index.values()), ["en"])

    def test_corpus_override_builds_from_another_tree(self):
        self.write("questions/en/questions.yaml", "")
        fixture = self.tmp / "fixtures/en"
        fixture.mkdir(parents=True)
        (fixture / "questions.yaml").write_text(
            "- id: q-deadbeef\n"
            '  text: "Placeholder question 1 - short line, depth 1?"\n'
            "  depth: 1\n"
            '  added: "2026-01-01"\n',
            encoding="utf-8",
        )
        out = self.tmp / "site_data"
        code, _, _ = run_quiet(
            build_site_data.main,
            ["--root", str(self.tmp), "--corpus", str(self.tmp / "fixtures"), "--out", str(out)],
        )
        self.assertEqual(code, 0)
        payload = json.loads((out / "questions.en.json").read_text())
        self.assertEqual([q["id"] for q in payload["questions"]], ["q-deadbeef"])

    def test_shipped_placeholder_corpus_builds(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_site_data.main,
                [
                    "--root",
                    str(REPO),
                    "--corpus",
                    str(REPO / "website" / "placeholder-corpus"),
                    "--out",
                    tmp,
                ],
            )
            self.assertEqual(code, 0)
            langs = json.loads((Path(tmp) / "languages.json").read_text())
            self.assertEqual([language["code"] for language in langs], ["de", "en", "it"])
            self.assertTrue(all(language["count"] for language in langs))

    def test_translation_provenance_reaches_the_site_payload(self):
        (self.tmp / "questions/de").mkdir()
        (self.tmp / "questions/de/STYLE.md").write_text("# stil\n")
        (self.tmp / "questions/de/denylist.txt").write_text("# list\n")
        self.write(
            "questions/en/questions.yaml",
            "- id: q-deadbeef\n"
            '  text: "What belief have you outgrown lately?"\n'
            "  depth: 2\n"
            '  added: "2026-01-01"\n',
        )
        self.write(
            "questions/de/questions.yaml",
            self.question(
                "Welche Überzeugung hast du in letzter Zeit abgelegt?",
                2,
                "  origin: q-deadbeef\n  translated_by: google\n",
            ),
        )
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        out = self.tmp / "site_data"
        code, _, _ = run_quiet(build_site_data.main, ["--root", str(self.tmp), "--out", str(out)])
        self.assertEqual(code, 0)
        entry = json.loads((out / "questions.de.json").read_text())["questions"][0]
        self.assertEqual(entry["origin"], "q-deadbeef")
        self.assertEqual(entry["translated_by"], "google")


class TestBundle(unittest.TestCase):
    def test_round_trip_against_real_database(self):
        # Round-trip both shipped languages.
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_bundle.main, ["--root", str(REPO), "--out", tmp, "--version", "test.1"]
            )
            self.assertEqual(code, 0)
            manifest = json.loads((Path(tmp) / "manifest.json").read_text())
            self.assertEqual(manifest["schema"], 4)
            for lang in ("en", "de"):
                blob = (Path(tmp) / f"bundle-{lang}-test.1.qdb").read_bytes()
                self.assertEqual(len(blob), manifest["languages"][lang]["size"])
                parsed = build_bundle.parse_bundle(blob)
                self.assertEqual(parsed["lang"], lang)
                self.assertEqual(parsed["version"], "test.1")
                self.assertEqual(len(parsed["questions"]), manifest["languages"][lang]["count"])
                for question in parsed["questions"]:
                    self.assertIn(question["depth"], (1, 2, 3))

    def test_manifest_describes_the_raw_bundle_a_device_downloads(self):
        """schema 4: the device takes the .qdb, and size/sha256/sig cover it.

        The gzip is still published for the website, so the failure this
        guards against is the manifest quietly pointing at one artifact while
        the hash describes the other — which no test would notice on either
        side alone.
        """
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_bundle.main, ["--root", str(REPO), "--out", tmp, "--version", "test.1"]
            )
            self.assertEqual(code, 0)
            manifest = json.loads((Path(tmp) / "manifest.json").read_text())

            for lang in ("en", "de"):
                entry = manifest["languages"][lang]
                raw = (Path(tmp) / f"bundle-{lang}-test.1.qdb").read_bytes()
                gz = (Path(tmp) / f"bundle-{lang}-test.1.qdb.gz").read_bytes()

                self.assertTrue(entry["url"].endswith(".qdb"))
                self.assertEqual(entry["size"], len(raw))
                self.assertEqual(entry["sha256"], hashlib.sha256(raw).hexdigest())

                # The raw bundle is the format itself, magic and all.
                self.assertEqual(raw[:4], b"QDB4")

                # Both artifacts are published and carry the same questions.
                self.assertEqual(gzip.decompress(gz), raw)
                self.assertEqual(len(build_bundle.parse_bundle(gz)["questions"]), entry["count"])

    def test_parse_bundle_takes_either_shape(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_bundle.main, ["--root", str(REPO), "--out", tmp, "--version", "test.1"]
            )
            self.assertEqual(code, 0)
            raw = (Path(tmp) / "bundle-en-test.1.qdb").read_bytes()
            gz = (Path(tmp) / "bundle-en-test.1.qdb.gz").read_bytes()

            self.assertEqual(build_bundle.parse_bundle(raw), build_bundle.parse_bundle(gz))

    def test_forms_round_trip_from_the_corpus(self):
        # Every form tag in the YAML must survive the bundle byte-for-byte;
        # the bag's texture preference reads only these bits on device.
        rep = validate.Reporter()
        _, cfg = validate.load_config(REPO, rep)
        forms = build_bundle.form_tags(cfg)
        self.assertEqual(
            forms, ["icebreaker", "reflective", "hypothetical", "memory", "wouldyourather"]
        )
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_bundle.main, ["--root", str(REPO), "--out", tmp, "--version", "test.1"]
            )
            self.assertEqual(code, 0)
            for lang in ("en", "de"):
                entries = validate.parse_file(REPO / "questions" / lang / "questions.yaml", rep)
                expected = {
                    entry["text"]: sorted(tag for tag in entry.get("tags", []) if tag in forms)
                    for entry in entries
                }
                blob = (Path(tmp) / f"bundle-{lang}-test.1.qdb").read_bytes()
                parsed = build_bundle.parse_bundle(blob)
                self.assertEqual(len(parsed["questions"]), len(expected))
                for question in parsed["questions"]:
                    self.assertEqual(
                        sorted(question["forms"]),
                        expected[question["text"]],
                        f"forms mismatch for {question['text']!r}",
                    )


class TestFixtureCorpus(unittest.TestCase):
    """firmware/tests/corpus/ decides whether the firmware suites are green.

    The bag cases read shapes out of it — a deck bigger than the recent ring,
    a deck smaller than it, depth 3 present — and a well meaning edit here
    fails in a Zephyr assertion minutes into a twister run, in a message about
    rings and cycles rather than about the corpus. These cases fail in Python
    instead, next to the file that broke. firmware/tests/corpus/README.md
    documents the same list.
    """

    CORPUS = REPO / "firmware" / "tests" / "corpus"

    @staticmethod
    def policy_default(symbol: str) -> int:
        """Read a Kconfig default, so the numbers cannot drift apart."""
        text = (REPO / "firmware" / "Kconfig.policy").read_text(encoding="utf-8")
        match = re.search(rf"config {symbol}\n(?:.*\n)*?\s*default (\d+)", text)
        assert match, f"no default for {symbol}"
        return int(match.group(1))

    def setUp(self):
        self.ring = self.policy_default("CICALA_RECENT_RING")
        self.max_bytes = self.policy_default("CICALA_MAX_QUESTION_BYTES")
        with tempfile.TemporaryDirectory() as tmp:
            code, _, err = run_quiet(
                build_bundle.main,
                [
                    "--root",
                    str(REPO),
                    "--corpus",
                    str(self.CORPUS),
                    "--out",
                    tmp,
                    "--version",
                    "test.1",
                ],
            )
            self.assertEqual(code, 0, err)
            self.bundles = {
                lang: build_bundle.parse_bundle(
                    (Path(tmp) / f"bundle-{lang}-test.1.qdb").read_bytes()
                )["questions"]
                for lang in ("en", "de")
            }

    def test_default_pool_can_fill_the_recent_ring(self):
        ordinary = [
            q for q in self.bundles["en"] if q["depth"] <= 2 and not q["dark"] and not q["sexual"]
        ]
        self.assertGreater(len(ordinary), self.ring)

    def test_english_exercises_all_permissions(self):
        questions = self.bundles["en"]
        self.assertTrue(any(q["depth"] == 3 for q in questions))
        self.assertTrue(any(q["dark"] for q in questions))
        self.assertTrue(any(q["sexual"] for q in questions))

    def test_text_fits_the_panel(self):
        for lang, questions in self.bundles.items():
            for q in questions:
                self.assertLessEqual(
                    len(q["text"].encode("utf-8")), self.max_bytes, f"{lang}: {q['text']!r}"
                )


if __name__ == "__main__":
    unittest.main()


class TestFirmwareManifest(unittest.TestCase):
    """The firmware side of the same contract the bundle manifest has.

    The bug being guarded against is the one build_bundle's manifest test
    guards against, in a shape that is easier to hit: the build produces
    zephyr.bin and zephyr.signed.bin forty bytes apart in one directory, and
    publishing the wrong one yields a manifest that verifies perfectly and an
    image MCUboot refuses to boot.
    """

    # An MCUboot image header is magic, load address, header size... Only the
    # magic matters here; the rest is padding so the file looks like an image.
    SIGNED = (0x96F3B83D).to_bytes(4, "little") + bytes(28) + b"payload"
    UNSIGNED = b"\xe9\x04\x02\x20" + bytes(28) + b"payload"

    def test_manifest_describes_the_image_it_publishes(self):
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp) / "zephyr.signed.bin"
            image.write_bytes(self.SIGNED)
            out = Path(tmp) / "out"

            code, _, _ = run_quiet(
                build_firmware_manifest.main,
                [
                    str(image),
                    "--version",
                    "0.2.0",
                    "--out",
                    str(out),
                    "--base-url",
                    "http://h/device",
                ],
            )
            self.assertEqual(code, 0)

            manifest = json.loads((out / "firmware.json").read_text())
            published = (out / "cicala-0.2.0.bin").read_bytes()

            self.assertEqual(manifest["schema"], 1)
            self.assertEqual(manifest["version"], "0.2.0")
            self.assertTrue(manifest["url"].endswith("cicala-0.2.0.bin"))
            self.assertEqual(manifest["size"], len(published))
            self.assertEqual(manifest["sha256"], hashlib.sha256(published).hexdigest())

            # What the device downloads is byte for byte what was signed.
            self.assertEqual(published, self.SIGNED)

            # No key was given, so nothing claims one was.
            self.assertIsNone(manifest["sig"])

    def test_the_unsigned_image_is_refused(self):
        """zephyr.bin has no MCUboot header, so a device could never boot it."""
        with tempfile.TemporaryDirectory() as tmp:
            image = Path(tmp) / "zephyr.bin"
            image.write_bytes(self.UNSIGNED)

            code, _, err = run_quiet(
                build_firmware_manifest.main,
                [str(image), "--version", "0.2.0", "--out", str(Path(tmp) / "out")],
            )
            self.assertEqual(code, 1)
            self.assertIn("MCUboot header", err)


# --------------------------------------------------------- contribution path

ISSUE_TEMPLATE = REPO / ".github" / "ISSUE_TEMPLATE" / "new-question.yml"


def issue_form_options():
    """The options of every dropdown in the new-question issue form, by id."""
    form = yaml.safe_load(ISSUE_TEMPLATE.read_text(encoding="utf-8"))
    return {
        field["id"]: field["attributes"]["options"]
        for field in form["body"]
        if field.get("type") == "dropdown"
    }


def schema_config():
    return json.loads((REPO / "questions" / "schema.json").read_text(encoding="utf-8"))["x-cicala"]


class TestIssueFormVocabulary(unittest.TestCase):
    """The issue form is the only place a contributor picks these values, and
    the website prefills it by option string. A value that is not declared
    here is dropped by GitHub, leaving a required field blank."""

    def setUp(self):
        self.options = issue_form_options()
        self.cfg = schema_config()

    def test_categories_are_not_editorial_metadata(self):
        self.assertNotIn("decks", self.options)
        self.assertNotIn("decks", self.cfg)

    def test_tag_options_are_the_schema_tags(self):
        self.assertEqual(self.options["tags"], self.cfg["tags"])

    def test_depth_options_are_the_schema_depth_labels(self):
        self.assertEqual(self.options["depth"], self.cfg["depthLabels"])

    def test_every_shipped_language_can_be_chosen(self):
        expected = [f"{lang['name']} ({code})" for code, lang in self.cfg["languages"].items()]
        self.assertEqual(self.options["language"][: len(expected)], expected)
        self.assertIn("other", self.options["language"][-1])

    def test_each_depth_label_starts_with_its_number(self):
        for i, label in enumerate(self.cfg["depthLabels"], start=1):
            self.assertTrue(label.startswith(str(i)), label)


def rendered_issue(
    language="English (en)",
    depth=None,
    question="When did you last change your mind about something important?",
    tags="_No response_",
    credit="_No response_",
    cc0="- [X] I dedicate this question to the public domain (CC0-1.0).",
):
    """An issue body in the shape GitHub renders an issue form into."""
    if depth is None:
        depth = schema_config()["depthLabels"][1]
    return (
        f"### Language\n\n{language}\n\n"
        f"### Depth\n\n{depth}\n\n"
        f"### Question\n\n{question}\n\n"
        f"### Tags (optional)\n\n{tags}\n\n"
        f"### Name for credit (optional)\n\n{credit}\n\n"
        f"### Public domain dedication\n\n{cc0}\n"
    )


def rendered_native_issue(
    language="English (en)",
    question="Which ordinary day would you happily live again?",
    credit="_No response_",
    cc0="- [x] I dedicate this question to the public domain (CC0-1.0).",
):
    return (
        f"### Language\n\n{language}\n\n"
        f"### Question\n\n> {question}\n\n"
        f"### Name for credit (optional)\n\n{credit}\n\n"
        f"### Public domain dedication\n\n{cc0}\n\n"
        "### Submission channel\n\ncicala.dev\n\n"
        "<!-- cicala-native:v1 -->\n"
    )


class PromoteCase(TmpDb):
    """A temp database plus a way to run promote_issue.py against it."""

    def setUp(self):
        super().setUp()
        self.addCleanup(setattr, promote_issue, "ROOT", promote_issue.ROOT)
        promote_issue.ROOT = self.tmp
        self.write("questions/en/questions.yaml", "")

    def promote(self, body, mode="apply", issue="7", labels=()):
        env = {
            "ISSUE_BODY": body,
            "ISSUE_NUMBER": issue,
            "ISSUE_LABELS": json.dumps(list(labels)),
        }
        old = {k: os.environ.get(k) for k in env}
        os.environ.update(env)
        os.environ.pop("GITHUB_OUTPUT", None)
        self.addCleanup(
            lambda: [
                os.environ.__setitem__(k, v) if v is not None else os.environ.pop(k, None)
                for k, v in old.items()
            ]
        )
        argv = sys.argv
        sys.argv = ["promote_issue.py", mode]
        try:
            with redirect_stdout(io.StringIO()) as out, redirect_stderr(io.StringIO()) as err:
                try:
                    code = promote_issue.main()
                except SystemExit as exc:
                    code = exc.code
            return code, out.getvalue(), err.getvalue()
        finally:
            sys.argv = argv

    def corpus(self):
        return yaml.safe_load((self.tmp / "questions" / "en" / "questions.yaml").read_text("utf-8"))


class TestPromoteIssue(PromoteCase):
    """promote_issue.py turns an approved issue into a database entry. It runs
    on untrusted text, so what it accepts and refuses is the gate."""

    def test_an_approved_issue_becomes_an_entry_the_validator_accepts(self):
        code, _, _ = self.promote(rendered_issue())
        self.assertEqual(code, 0)

        entry = self.corpus()[0]

        self.assertEqual(entry["depth"], 2)
        self.assertNotIn("id", entry)  # validate.py --fix assigns it

        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0, err)
        code, _, err = run_quiet(validate.main, ["--root", str(self.tmp)])
        self.assertEqual(code, 0, err)
        self.assertRegex(self.corpus()[0]["id"], r"^q-[0-9a-f]{8}$")

    def test_every_depth_label_the_form_offers_parses(self):
        for i, label in enumerate(schema_config()["depthLabels"], start=1):
            with self.subTest(depth=i):
                self.write("questions/en/questions.yaml", "")
                code, _, _ = self.promote(
                    rendered_issue(depth=label, question=f"Question number {i} here?")
                )
                self.assertEqual(code, 0)
                self.assertEqual(self.corpus()[0]["depth"], i)

    def test_an_unticked_cc0_box_is_refused(self):
        code, _, err = self.promote(rendered_issue(cc0="- [ ] I dedicate this…"))
        self.assertEqual(code, 1)
        self.assertIn("CC0", err)
        self.assertEqual(self.corpus(), None)

    def test_legacy_spicy_requires_explicit_reclassification(self):
        code, _, err = self.promote(rendered_issue(tags="spicy"))
        self.assertEqual(code, 1)
        self.assertIn("reclassify", err)

    def test_a_new_language_exits_for_the_incubator_explainer(self):
        code, _, _ = self.promote(rendered_issue(language="other / new language"))
        self.assertEqual(code, 78)

    def test_an_unshipped_language_is_refused_rather_than_creating_a_directory(self):
        code, _, err = self.promote(rendered_issue(language="Français (fr)"))
        self.assertEqual(code, 1)
        self.assertIn("not a shipped language", err)
        self.assertFalse((self.tmp / "questions" / "fr").exists())

    def test_quotes_and_backslashes_survive_into_valid_yaml(self):
        text = 'What did you mean by \\"really\\", exactly?'
        code, _, _ = self.promote(rendered_issue(question=text))
        self.assertEqual(code, 0)
        self.assertEqual(self.corpus()[0]["text"], text)

    def test_a_credit_is_kept_and_capped(self):
        code, _, _ = self.promote(rendered_issue(credit="A" * 60))
        self.assertEqual(code, 0)
        self.assertEqual(self.corpus()[0]["author"], "A" * 40)

    def test_unknown_tags_are_refused(self):
        code, _, _ = self.promote(rendered_issue(tags="memory, sneaky-injected-tag"))
        self.assertEqual(code, 1)
        self.assertIsNone(self.corpus())

    def test_parse_mode_reads_the_issue_without_touching_the_database(self):
        code, out, _ = self.promote(rendered_issue(), mode="parse")
        self.assertEqual(code, 0)
        self.assertIn("lang: en", out)
        self.assertEqual(self.corpus(), None)

    def test_a_native_suggestion_takes_editorial_metadata_from_labels(self):
        code, _, err = self.promote(
            rendered_native_issue(credit="Ada"),
            labels=("question-submission", "depth:2", "tag:memory"),
        )
        self.assertEqual(code, 0, err)
        self.assertEqual(
            self.corpus()[0],
            {
                "text": "Which ordinary day would you happily live again?",
                "depth": 2,
                "tags": ["memory"],
                "author": "Ada",
            },
        )

    def test_a_native_suggestion_must_be_classified_before_approval(self):
        code, _, err = self.promote(rendered_native_issue())
        self.assertEqual(code, 1)
        self.assertIn("classify", err)
        self.assertEqual(self.corpus(), None)

    def test_a_native_suggestion_rejects_multiple_depth_labels(self):
        code, _, err = self.promote(rendered_native_issue(), labels=("depth:1", "depth:2"))
        self.assertEqual(code, 1)
        self.assertIn("exactly one", err)

    def test_a_native_suggestion_rejects_unknown_editorial_labels(self):
        code, _, err = self.promote(rendered_native_issue(), labels=("depth:medium",))
        self.assertEqual(code, 1)
        self.assertIn("unknown editorial label", err)

    def test_native_html_escaping_is_reversed_before_storage(self):
        code, _, err = self.promote(
            rendered_native_issue(
                question="Who made you think &#64;home was better than &lt;away&gt;?",
                credit="A &amp; B",
            ),
            labels=("depth:2",),
        )
        self.assertEqual(code, 0, err)
        self.assertEqual(
            self.corpus()[0]["text"], "Who made you think @home was better than <away>?"
        )
        self.assertEqual(self.corpus()[0]["author"], "A & B")


class TestCheckSubmission(PromoteCase):
    """`check` mode answers the submitter while the issue is still open, using
    the same validator that decides at merge time. It writes nothing."""

    def test_a_good_submission_passes_and_writes_nothing(self):
        code, out, _ = self.promote(rendered_issue(), mode="check")
        self.assertEqual(code, 0)
        self.assertIn("passes every automatic check", out)
        self.assertEqual(self.corpus(), None)

    def test_an_unclassified_native_suggestion_gets_text_checks(self):
        code, out, err = self.promote(rendered_native_issue(), mode="check")
        self.assertEqual(code, 0, err)
        self.assertIn("passes every automatic check", out)
        self.assertIn("classified: false", out)
        self.assertEqual(self.corpus(), None)

    def test_a_duplicate_is_caught_before_a_maintainer_reads_it(self):
        existing = "What did you last change your mind about, and why?"
        self.write(
            "questions/en/questions.yaml",
            self.question(existing) + '  id: q-abcdef01\n  added: "2026-01-01"\n',
        )
        code, _, err = self.promote(rendered_issue(question=existing), mode="check")
        self.assertEqual(code, 1)
        self.assertIn("duplicate", err)
        self.assertNotIn(str(self.tmp), err)  # no temp path leaks into the reply

    def test_a_denylisted_word_is_caught(self):
        code, _, err = self.promote(
            rendered_issue(question="What is the worst badword you ever heard?"),
            mode="check",
        )
        self.assertEqual(code, 1)
        self.assertIn("denylist", err)

    def test_a_too_long_question_is_caught(self):
        code, _, err = self.promote(rendered_issue(question="W" * 200 + "?"), mode="check")
        self.assertEqual(code, 1)
        self.assertIn("too long", err)

    def test_a_missing_question_mark_is_caught(self):
        code, _, err = self.promote(
            rendered_issue(question="Tell me about your best day ever."),
            mode="check",
        )
        self.assertEqual(code, 1)
        self.assertIn("must end with", err)

    def test_the_reply_says_each_thing_once_and_nothing_about_ids(self):
        """The comment goes to a contributor, so it carries their problems and
        not the validator's bookkeeping."""
        code, _, err = self.promote(
            rendered_issue(question="Tell me about your best day ever."),
            mode="check",
        )
        self.assertEqual(code, 1)
        lines = [line for line in err.splitlines() if line.startswith("error: ")]
        self.assertEqual(len(lines), 1, lines)
        self.assertNotIn("required property", err)
        self.assertNotIn("--fix", err)

    def test_an_unshipped_language_is_reported_rather_than_crashing(self):
        code, _, err = self.promote(rendered_issue(language="Français (fr)"), mode="check")
        self.assertEqual(code, 1)
        self.assertIn("not a shipped language", err)

    def test_the_check_runs_against_the_real_database_without_touching_it(self):
        before = (REPO / "questions" / "en" / "questions.yaml").read_bytes()
        promote_issue.ROOT = REPO
        try:
            code, out, err = self.promote(
                rendered_issue(question="Which unremarkable Tuesday would you happily live again?"),
                mode="check",
            )
        finally:
            promote_issue.ROOT = self.tmp
        self.assertEqual(code, 0, err)
        self.assertIn("passes every automatic check", out)
        self.assertEqual((REPO / "questions" / "en" / "questions.yaml").read_bytes(), before)


# ------------------------------------------------------- release publication


class TestCheckRelease(unittest.TestCase):
    """check_release.py is the last gate before a manifest is attached to a
    release. The failures it must catch are the ones a public repository makes
    easy to introduce: a null signature, the reserved development host, and
    the pre-split website host whose redirects a device cannot follow."""

    BASE = "http://device.cicala.dev/device"

    def setUp(self):
        self.dir = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.dir)

    def write(self, manifest, name="manifest.json"):
        path = self.dir / name
        path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        return path

    def bundle(self, **overrides):
        entry = {
            "url": f"{self.BASE}/bundle-en-2026.08.2.qdb",
            "size": 100,
            "sha256": "0" * 64,
            "sig": "c2lnbmF0dXJl",
            "count": 1,
        }
        entry.update(overrides)
        return {
            "schema": 4,
            "version": "2026.08.2",
            "min_fw": "0.1.0",
            "languages": {"en": entry},
        }

    def firmware(self, **overrides):
        manifest = {
            "schema": 1,
            "version": "0.1.0",
            "url": f"{self.BASE}/cicala-0.1.0.bin",
            "size": 100,
            "sha256": "0" * 64,
            "sig": "c2lnbmF0dXJl",
        }
        manifest.update(overrides)
        return manifest

    def check(self, path):
        return run_quiet(check_release.main, [str(path)])

    def test_a_production_bundle_manifest_passes(self):
        code, out, err = self.check(self.write(self.bundle()))
        self.assertEqual(code, 0, err)

    def test_a_production_firmware_manifest_passes(self):
        code, out, err = self.check(self.write(self.firmware()))
        self.assertEqual(code, 0, err)

    def test_a_null_signature_is_refused(self):
        code, _, err = self.check(self.write(self.bundle(sig=None)))
        self.assertEqual(code, 1)
        self.assertIn("sig is null", err)

    def test_the_development_host_is_refused(self):
        path = self.write(self.bundle(url="http://cicala.invalid/device/bundle-en-1.qdb", sig="x"))
        code, _, err = self.check(path)
        self.assertEqual(code, 1)
        self.assertIn("cicala.invalid", err)

    def test_a_pages_dev_host_is_refused(self):
        for host in ("tischkarte.pages.dev", "cicala.pages.dev"):
            with self.subTest(host=host):
                path = self.write(
                    self.bundle(url=f"https://{host}/device/bundle-en-1.qdb", sig="x")
                )
                code, _, err = self.check(path)
                self.assertEqual(code, 1)
                self.assertIn("pages.dev", err)

    def test_a_url_outside_the_endpoint_is_refused(self):
        path = self.write(self.bundle(url="http://device.cicala.dev/other/bundle-en-1.qdb"))
        code, _, err = self.check(path)
        self.assertEqual(code, 1)
        self.assertIn("expected under", err)

    def test_a_missing_manifest_is_refused(self):
        code, _, err = self.check(self.dir / "nope.json")
        self.assertEqual(code, 1)
        self.assertIn("nope.json", err)

    def test_a_manifest_without_languages_is_refused(self):
        code, _, err = self.check(self.write({"schema": 4, "languages": {}}))
        self.assertEqual(code, 1)
        self.assertIn("no languages", err)

    def test_an_unknown_schema_is_refused(self):
        code, _, err = self.check(self.write({"schema": 99, "sig": "x"}))
        self.assertEqual(code, 1)
        self.assertIn("unknown schema", err)


class TestProductionEndpoint(unittest.TestCase):
    """Release firmware compiles the sync and OTA endpoints from Kconfig.policy
    defaults, and the tag workflows pass the same base URL to the manifest
    tools and to check_release. These places must not drift apart."""

    def kconfig_default(self, symbol):
        text = (REPO / "firmware" / "Kconfig.policy").read_text(encoding="utf-8")
        m = re.search(rf'config {symbol}\n(?:    [^\n]*\n)*?    default "([^"]+)"', text)
        self.assertIsNotNone(m, f"{symbol} has no string default")
        return m.group(1)

    def test_kconfig_defaults_point_at_the_device_endpoint(self):
        host = self.kconfig_default("CICALA_SYNC_HOST")
        base = f"http://{host}/device"

        self.assertEqual(host, "device.cicala.dev")
        self.assertEqual(self.kconfig_default("CICALA_SYNC_BASE_URL"), base)
        self.assertEqual(self.kconfig_default("CICALA_OTA_BASE_URL"), base)
        self.assertEqual(check_release.DEFAULT_BASE_URL, base)


class TestDraftImport(TmpDb):
    """`just draft` — plain-text drafts become entries the validator accepts."""

    def draft(self, body):
        path = self.tmp / "drafts.txt"
        path.write_text(body, encoding="utf-8")
        return path

    def corpus(self):
        return (self.tmp / "questions/en/questions.yaml").read_text(encoding="utf-8")

    def import_file(self, draft, *extra):
        return run_quiet(
            import_drafts.main, ["--lang", "en", str(draft), "--root", str(self.tmp), *extra]
        )

    def setUp(self):
        super().setUp()
        self.write("questions/en/questions.yaml", "")

    def test_a_group_header_covers_its_questions_and_a_question_may_wrap(self):
        draft = self.draft(
            "# a comment, not a header\n"
            "\n"
            "# depth: 1 · tags: icebreaker\n"
            "What closes on a single line?\n"
            "What wraps across\n"
            "two lines before it closes?\n"
            "\n"
            "# depth: 3 | tags: dark\n"
            "What uses pipes to separate its fields?\n"
        )

        code, _, _ = self.import_file(draft)

        self.assertEqual(code, 0)
        entries = yaml.safe_load(self.corpus())
        self.assertEqual(
            [entry["text"] for entry in entries],
            [
                "What closes on a single line?",
                "What wraps across two lines before it closes?",
                "What uses pipes to separate its fields?",
            ],
        )

        self.assertEqual(entries[0]["tags"], ["icebreaker"])
        # A header replaces the previous one rather than merging with it.

        self.assertEqual(entries[2]["depth"], 3)
        # The fix pass owns ids and dates, exactly as for a hand-written entry.
        for entry in entries:
            self.assertEqual(entry["id"], validate.compute_id("en", entry["text"]))
            self.assertIn("added", entry)

    def test_rerunning_a_draft_skips_what_the_corpus_already_has(self):
        draft = self.draft("# depth: 2\nWhat is already on file here?\n")
        self.import_file(draft)
        before = self.corpus()

        code, out, _ = self.import_file(draft)

        self.assertEqual(code, 0)
        self.assertIn("skipped", out)
        self.assertEqual(self.corpus(), before)

    def test_a_draft_the_validator_would_reject_is_not_written(self):
        cases = {
            "depth": "# depth: 9\nWhat carries an impossible depth?\n",
            "legacy tag": "# depth: 2 · tags: spicy\nWhat wears dark outside wild?\n",
            "length": "# depth: 1\nShort?\n",
            "denylist": "# depth: 1\nWhat about that badword there?\n",
            "unknown deck": "# decks: nope · depth: 1\nWhat sits under an unknown deck?\n",
            "no header": "What has no header above it?\n",
            "unterminated": "# depth: 1\nThis line never ends properly\n",
        }
        for name, body in cases.items():
            with self.subTest(name):
                self.write("questions/en/questions.yaml", "")
                code, _, err = self.import_file(self.draft(body))
                self.assertEqual(code, 1)
                self.assertTrue(err.strip(), "the failure should say why")
                self.assertEqual(self.corpus(), "")

    def test_dry_run_reports_without_touching_the_corpus(self):
        draft = self.draft("# depth: 2\nWhat would be appended?\n")

        code, out, _ = self.import_file(draft, "--dry-run")

        self.assertEqual(code, 0)
        self.assertIn("What would be appended?", out)
        self.assertEqual(self.corpus(), "")


class TestQdb4Contract(unittest.TestCase):
    def fixture(self):
        return {
            "text": "When did you last sing out loud?",
            "depth": 3,
            "tags": ["dark", "sexual", "memory"],
        }

    def test_precise_flags_and_record_layout(self):
        raw = build_bundle.build_bundle_bytes(
            "en",
            "v",
            [self.fixture()],
            ["icebreaker", "reflective", "hypothetical", "memory", "wouldyourather"],
        )
        self.assertEqual(raw[:4], b"QDB4")
        self.assertEqual(raw[11:15], bytes([14, 8, 32, 0]))
        question = build_bundle.parse_bundle(raw)["questions"][0]
        self.assertEqual(question["depth"], 3)
        self.assertTrue(question["dark"] and question["sexual"])
        self.assertEqual(question["forms"], ["memory"])
        for size in range(len(raw)):
            with self.assertRaises(ValueError):
                build_bundle.parse_bundle(raw[:size])
        with self.assertRaises(ValueError):
            build_bundle.parse_bundle(b"QDB3" + raw[4:])

    def test_writer_refuses_retired_metadata(self):
        for legacy in [
            {**self.fixture(), "tags": ["spicy"]},
            {**self.fixture(), "decks": ["wild"]},
        ]:
            with self.assertRaisesRegex(ValueError, "reclassify"):
                build_bundle.build_bundle_bytes("en", "v", [legacy], ["memory"])
