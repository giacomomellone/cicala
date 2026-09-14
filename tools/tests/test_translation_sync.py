"""Synchronization across accepted edits, generated drafts and subsequent merges."""

import copy
import io
import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(TOOLS))

import translate_question as translation  # noqa: E402
import validate  # noqa: E402


class TestTranslationSync(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.schema = json.loads((TOOLS.parent / "questions/schema.json").read_text())
        self.config = self.schema["x-cicala"]
        for language in ("en", "de", "it"):
            self.config["languages"][language]["google"] = {
                "source": language,
                "target": language,
            }
        self.entries = {
            "en": [{"id": "q-11111111", "text": "What changed your mind?"}],
            "de": [
                {
                    "id": "q-22222222",
                    "text": "Was hat deine Meinung geändert?",
                    "origin": "q-11111111",
                    "translated_by": "google",
                }
            ],
            "it": [
                {
                    "id": "q-33333333",
                    "text": "Che cosa ti ha fatto cambiare idea?",
                    "origin": "q-11111111",
                    "author": "Ada",
                }
            ],
        }
        for language, entries in self.entries.items():
            directory = self.root / "questions" / language
            directory.mkdir(parents=True)
            (directory / "STYLE.md").write_text("# Style\n")
            (directory / "denylist.txt").write_text("# Empty test denylist\n")
            for entry in entries:
                entry.update(depth=2, added="2026-01-01")
        (self.root / "questions/schema.json").write_text(json.dumps(self.schema))
        self.write()

    def write(self):
        for language, entries in self.entries.items():
            (self.root / "questions" / language / "questions.yaml").write_text(
                validate.format_file(language, entries, self.config["keyOrder"]), encoding="utf-8"
            )

    def git(self, *args):
        return subprocess.run(
            ["git", *args], cwd=self.root, check=True, capture_output=True, text=True
        ).stdout.strip()

    def commit(self):
        self.git("add", "questions")
        self.git("commit", "-qm", "fixture")
        return self.git("rev-parse", "HEAD")

    def initialize_git(self):
        self.git("init", "-q")
        self.git("config", "user.name", "test")
        self.git("config", "user.email", "test@example.invalid")
        return self.commit()

    def run_tool(self, main, *args):
        with redirect_stdout(io.StringIO()) as out, redirect_stderr(io.StringIO()) as err:
            code = main(["--root", str(self.root), *args])
        self.assertEqual(code, 0, err.getvalue())
        return out.getvalue()

    def test_original_edit_targets_all_other_shipped_languages(self):
        before = copy.deepcopy(self.entries)
        self.entries["en"][0]["text"] = "What changed your mind most recently?"
        plan = translation.translation_plan(
            translation.changed_questions(before, self.entries), self.entries, self.config
        )
        self.assertEqual(set(plan), {"de", "it"})

    def test_translation_edit_targets_siblings_and_preserves_human_original(self):
        before = copy.deepcopy(self.entries)
        self.entries["de"][0]["text"] = "Was hat dich umgestimmt?"
        plan = translation.translation_plan(
            translation.changed_questions(before, self.entries), self.entries, self.config
        )
        self.assertEqual(set(plan), {"it"})
        self.assertEqual(plan["it"][0].language, "de")
        self.assertEqual(plan["it"][0].origin, "q-11111111")

    def test_paused_targets_skip_api_and_resume_from_final_english_revision(self):
        before = self.initialize_git()
        enabled = copy.deepcopy(self.config)
        for language in ("de", "it"):
            del self.config["languages"][language]["google"]
        (self.root / "questions/schema.json").write_text(json.dumps(self.schema))
        self.entries["en"][0]["depth"] = 3
        self.write()
        self.commit()
        self.entries["en"][0]["text"] = "What changed your mind most recently?"
        self.write()
        after = self.commit()

        self.assertEqual(translation.build_plan(self.root, before, after, self.config), {})
        with patch.object(translation, "GoogleTranslateClient") as client:
            for language in ("de", "it"):
                target = self.root / "questions" / language / "questions.yaml"
                original = target.read_bytes()
                self.run_tool(
                    translation.main,
                    "apply",
                    "--before-ref",
                    before,
                    "--after-ref",
                    after,
                    "--target",
                    language,
                )
                self.assertEqual(target.read_bytes(), original)
            client.assert_not_called()

        plan = translation.build_plan(self.root, before, after, enabled)
        self.assertEqual(set(plan), {"de", "it"})
        for sources in plan.values():
            self.assertEqual(len(sources), 1)
            self.assertEqual(sources[0].entry, self.entries["en"][0])
            self.assertTrue(sources[0].text_changed)

    def test_human_translation_is_updated_in_place(self):
        source = translation.SourceQuestion("de", self.entries["de"][0])
        target = self.entries["it"]
        translation.apply_translations(target, [source], [target[0]["text"]])
        self.assertEqual(len(target), 1)
        self.assertEqual(target[0]["id"], "q-33333333")
        self.assertEqual(target[0]["author"], "Ada")
        self.assertEqual(target[0]["added"], "2026-01-01")
        self.assertEqual(target[0]["origin"], "q-11111111")
        self.assertEqual(target[0]["translated_by"], "google")
        self.assertEqual(target[0]["translation_sync"]["source"], "q-22222222")

    def test_apply_refuses_to_write_into_the_original_corpus(self):
        source = translation.SourceQuestion("de", self.entries["de"][0])
        original = copy.deepcopy(self.entries["en"])
        with self.assertRaisesRegex(translation.TranslationError, "human-written original"):
            translation.apply_translations(
                self.entries["en"], [source], ["What changed your mind?"]
            )
        self.assertEqual(self.entries["en"], original)

    def test_generated_updates_do_not_retrigger_but_review_edits_do(self):
        before = copy.deepcopy(self.entries)
        source = translation.SourceQuestion("en", self.entries["en"][0])
        translation.apply_translations(self.entries["de"], [source], ["Was hat dich umgestimmt?"])
        self.assertEqual(translation.changed_questions(before, self.entries), [])
        generated = copy.deepcopy(self.entries)
        self.entries["de"][0]["depth"] = 3
        changed = translation.changed_questions(generated, self.entries)
        self.assertEqual([entry.language for entry in changed], ["de"])
        self.assertFalse(changed[0].text_changed)

    def test_new_translations_and_provenance_only_edits_do_not_retrigger(self):
        before = copy.deepcopy(self.entries)
        before["de"] = []
        self.entries["it"][0]["translated_by"] = "google"
        self.assertEqual(translation.changed_questions(before, self.entries), [])

    def test_tag_order_and_empty_tags_are_not_edits(self):
        before = copy.deepcopy(self.entries)
        before["en"][0]["tags"] = ["reflective", "memory"]
        self.entries["en"][0]["tags"] = ["memory", "reflective"]
        self.entries["it"][0]["tags"] = []
        self.assertEqual(translation.changed_questions(before, self.entries), [])

    def test_competing_edits_fail_and_can_be_resolved_explicitly(self):
        before = self.initialize_git()
        self.entries["en"][0]["depth"] = 1
        self.entries["de"][0]["depth"] = 3
        self.write()
        after = self.commit()
        with self.assertRaisesRegex(translation.TranslationError, "conflicting edits"):
            translation.build_plan(self.root, before, after, self.config)
        plan = translation.build_plan(self.root, before, after, self.config, "q-22222222")
        self.assertEqual(set(plan), {"it"})
        self.assertEqual(plan["it"][0].entry["depth"], 3)
        with self.assertRaisesRegex(translation.TranslationError, "no human edit"):
            translation.build_plan(self.root, before, after, self.config, "q-33333333")

    def test_multiple_translations_in_one_language_are_ambiguous(self):
        self.entries["it"].append({**self.entries["it"][0], "id": "q-44444444"})
        source = translation.SourceQuestion("en", self.entries["en"][0])
        with self.assertRaisesRegex(translation.TranslationError, "multiple translations"):
            translation.translation_plan([source], self.entries, self.config)

    def test_git_edit_apply_validate_and_next_plan(self):
        before = self.initialize_git()
        self.entries["de"][0]["text"] = "Was hat dich umgestimmt?"
        self.write()
        after = self.commit()
        original = (self.root / "questions/en/questions.yaml").read_bytes()
        with patch.object(translation.GoogleTranslateClient, "translate") as translate:
            translate.return_value = [self.entries["it"][0]["text"]]
            with patch.dict("os.environ", {"GOOGLE_TRANSLATE_API_KEY": "test-only"}):
                self.run_tool(
                    translation.main,
                    "apply",
                    "--before-ref",
                    before,
                    "--after-ref",
                    after,
                    "--target",
                    "it",
                )
            translate.assert_called_once_with(["Was hat dich umgestimmt?"], "de", "it")
        self.run_tool(validate.main, "--fix")
        self.run_tool(validate.main)
        self.assertEqual((self.root / "questions/en/questions.yaml").read_bytes(), original)
        generated = self.commit()
        plan = self.run_tool(
            translation.main, "plan", "--before-ref", after, "--after-ref", generated
        )
        self.assertEqual(json.loads(plan), [])

    def test_metadata_only_apply_needs_no_api_key_and_preserves_reviewed_text(self):
        before = self.initialize_git()
        self.entries["en"][0].update(depth=3, tags=["dark"])
        self.write()
        after = self.commit()
        with patch.object(translation, "GoogleTranslateClient") as client:
            self.run_tool(
                translation.main,
                "apply",
                "--before-ref",
                before,
                "--after-ref",
                after,
                "--target",
                "it",
            )
            client.assert_not_called()
        self.run_tool(validate.main, "--fix")
        self.run_tool(validate.main)
        target = translation.load_yaml((self.root / "questions/it/questions.yaml").read_text())[0]
        self.assertEqual(target["text"], self.entries["it"][0]["text"])
        self.assertEqual(target["depth"], 3)
        self.assertEqual(target["tags"], ["dark"])
        self.assertNotIn("translated_by", target)
        self.assertEqual(
            translation.changed_questions(self.entries, {**self.entries, "it": [target]}), []
        )

    def test_validator_rejects_missing_unrelated_and_same_language_sync_sources(self):
        source = translation.SourceQuestion("en", self.entries["en"][0])
        translation.apply_translations(self.entries["de"], [source], ["Was hat dich umgestimmt?"])
        self.entries["en"].append(
            {**self.entries["en"][0], "id": "q-44444444", "text": "What did you learn today?"}
        )
        for source_id, message in [
            ("q-00000000", "is missing"),
            ("q-44444444", "same origin"),
            ("q-22222222", "another language"),
        ]:
            with self.subTest(source=source_id):
                self.entries["de"][0]["translation_sync"]["source"] = source_id
                self.write()
                with redirect_stderr(io.StringIO()) as err, redirect_stdout(io.StringIO()):
                    code = validate.main(["--root", str(self.root)])
                self.assertEqual(code, 1)
                self.assertIn(message, err.getvalue())
