"""Tests for tools/validate.py, build_site_data.py, build_bundle.py.

Run from the repo root:  python -m unittest discover tools/tests
Uses only the stdlib (unittest) per the tools dependency policy.
"""

import io
import json
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))

import build_bundle  # noqa: E402
import build_site_data  # noqa: E402
import validate  # noqa: E402


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


class TestIdAssignment(TmpDb):
    def test_fix_assigns_expected_id_and_date(self):
        self.write("questions/en/deep.yaml",
                   '- text: "When did you last change your mind about something important?"\n'
                   "  tags: [reflective]\n")
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        content = (self.tmp / "questions/en/deep.yaml").read_text()
        # id must equal the documented hash construction
        expected = validate.compute_id(
            "en", "deep", "When did you last change your mind about something important?")
        self.assertIn(f"id: {expected}", content)
        self.assertIn("added: ", content)

    def test_missing_id_without_fix_fails(self):
        self.write("questions/en/deep.yaml", '- text: "What matters most to you today?"\n')
        code, _, err = run_quiet(validate.main, ["--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("--fix", err)

    def test_existing_id_is_preserved_after_text_edit(self):
        # ids are stable forever: a typo fix must not change or invalidate the id
        self.write("questions/en/deep.yaml",
                   "- id: q-00000000\n"
                   '  text: "What matters most to you todayy?"\n'
                   '  added: "2026-01-01"\n')
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        self.assertIn("id: q-00000000",
                      (self.tmp / "questions/en/deep.yaml").read_text())

    def test_fix_is_idempotent(self):
        path = self.write("questions/en/party.yaml",
                          '- text: "What was the best part of your week so far?"\n')
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        first = path.read_text()
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(first, path.read_text())


class TestDedup(TmpDb):
    def test_duplicate_across_categories_same_language_fails(self):
        self.write("questions/en/party.yaml", '- text: "What made you laugh today, honestly?"\n')
        self.write("questions/en/deep.yaml",
                   '- text: "What  made you LAUGH today,   honestly?"\n')  # normalizes equal
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("duplicate question text", err)

    def test_same_text_in_other_language_is_fine(self):
        self.write("questions/en/party.yaml", '- text: "What made you laugh today, honestly?"\n')
        (self.tmp / "questions/de").mkdir()
        (self.tmp / "questions/de/STYLE.md").write_text("# stil\n")
        (self.tmp / "questions/de/denylist.txt").write_text("egal\n")
        self.write("questions/de/party.yaml", '- text: "What made you laugh today, honestly?"\n')
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)


class TestDenylist(TmpDb):
    def test_denylist_hit_fails_with_review_message(self):
        self.write("questions/en/party.yaml", '- text: "Who said the badword at dinner?"\n')
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("review", err)

    def test_denylist_matches_whole_words_only(self):
        # "badwording" must NOT match the term "badword" (no Scunthorpe failures)
        self.write("questions/en/party.yaml",
                   '- text: "What does badwording mean to you?"\n')
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)

    def test_denylist_matches_phrases(self):
        self.write("questions/en/party.yaml",
                   '- text: "Is a naughty phrase always funny to you?"\n')
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)


class TestTextRules(TmpDb):
    def test_missing_question_mark_fails(self):
        self.write("questions/en/deep.yaml", '- text: "Tell me about your best day ever."\n')
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("must end with", err)

    def test_terminator_followed_by_closing_quote_passes(self):
        self.write("questions/en/deep.yaml",
                   "- text: 'When did you last ask yourself \"why not me?\"'\n")
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)

    def test_too_short_fails(self):
        self.write("questions/en/deep.yaml", '- text: "Why not?"\n')
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)


class TestOrigin(TmpDb):
    def test_dangling_origin_warns_but_passes(self):
        self.write("questions/en/deep.yaml",
                   '- text: "What belief have you outgrown lately?"\n'
                   "  origin: q-deadbeef\n")
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        self.assertIn("q-deadbeef", err)
        self.assertIn("warning", err)


class TestSiteData(TmpDb):
    def test_incubator_excluded_and_fields_stripped(self):
        self.write("questions/en/deep.yaml",
                   '- text: "What belief have you outgrown lately?"\n'
                   '  author: "seed"\n')
        (self.tmp / "questions/incubator/fr").mkdir(parents=True)
        self.write("questions/incubator/fr/deep.yaml",
                   '- text: "Quelle certitude as-tu abandonnée récemment?"\n')
        run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        out = self.tmp / "site_data"
        code, _, _ = run_quiet(build_site_data.main,
                               ["--root", str(self.tmp), "--out", str(out)])
        self.assertEqual(code, 0)
        self.assertTrue((out / "questions.en.json").exists())
        self.assertFalse((out / "questions.fr.json").exists())
        langs = json.loads((out / "languages.json").read_text())
        self.assertEqual([l["code"] for l in langs], ["en"])
        payload = json.loads((out / "questions.en.json").read_text())
        entry = payload["categories"]["deep"][0]
        self.assertEqual(set(entry), {"id", "text", "tags"})  # author/added/origin stripped
        recent = json.loads((out / "recent.en.json").read_text())
        self.assertEqual(recent[0]["category"], "deep")
        index = json.loads((out / "index.json").read_text())
        self.assertEqual(list(index.values()), ["en"])


class TestBundle(unittest.TestCase):
    def test_round_trip_against_real_database(self):
        # phase C acceptance: bundle round-trip for en and de
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(build_bundle.main,
                                   ["--root", str(REPO), "--out", tmp, "--version", "test.1"])
            self.assertEqual(code, 0)
            manifest = json.loads((Path(tmp) / "manifest.json").read_text())
            self.assertEqual(manifest["schema"], 1)
            for lang in ("en", "de"):
                blob = (Path(tmp) / f"bundle-{lang}-test.1.tkb").read_bytes()
                self.assertEqual(len(blob), manifest["languages"][lang]["size"])
                parsed = build_bundle.parse_bundle(blob)
                self.assertEqual(parsed["lang"], lang)
                self.assertEqual(parsed["version"], "test.1")
                total = sum(len(v) for v in parsed["categories"].values())
                self.assertEqual(total, manifest["languages"][lang]["count"])
                # spicy flag must never appear in family
                self.assertFalse(any(q["spicy"] for q in parsed["categories"]["family"]))
                for items in parsed["categories"].values():
                    for q in items:
                        self.assertTrue(0 <= q["display"] <= 9999)

    def test_display_id_construction(self):
        # decimal of first 2 hash bytes mod 10000: q-8f3a2c1d -> 0x8f3a = 36666 -> 6666
        self.assertEqual(build_bundle.display_id("q-8f3a2c1d"), 6666)


if __name__ == "__main__":
    unittest.main()
