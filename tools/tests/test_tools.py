"""Tests for tools/validate.py, build_site_data.py, build_bundle.py.

Run from the repo root:  python -m unittest discover tools/tests
Uses only the stdlib (unittest) per the tools dependency policy.
"""

import gzip
import hashlib
import io
import json
import os
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
import build_firmware_manifest  # noqa: E402
import build_site_data  # noqa: E402
import promote_issue  # noqa: E402
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

    def question(self, text, decks="[new_people]", depth=1, extra=""):
        return f'- text: "{text}"\n  decks: {decks}\n  depth: {depth}\n{extra}'


class TestIdAssignment(TmpDb):
    def test_fix_assigns_expected_id_and_date(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "When did you last change your mind about something important?",
                "[new_people, close]",
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
            self.question("What matters most to you today?", "[close]", 2),
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
            "  decks: [close]\n"
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
            + self.question("What  made you LAUGH today,   honestly?", "[close]", 2),
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
            self.question("Tell me about your best day ever.", "[close]", 2),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)
        self.assertIn("must end with", err)

    def test_terminator_followed_by_closing_quote_passes(self):
        self.write(
            "questions/en/questions.yaml",
            "- text: 'When did you last ask yourself \"why not me?\"'\n"
            "  decks: [close]\n"
            "  depth: 2\n",
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)

    def test_too_short_fails(self):
        self.write(
            "questions/en/questions.yaml",
            self.question("Why not?", "[close]", 2),
        )
        code, _, _ = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 1)


class TestOrigin(TmpDb):
    def test_dangling_origin_warns_but_passes(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                "[close]",
                2,
                "  origin: q-deadbeef\n",
            ),
        )
        code, _, err = run_quiet(validate.main, ["--fix", "--root", str(self.tmp)])
        self.assertEqual(code, 0)
        self.assertIn("q-deadbeef", err)
        self.assertIn("warning", err)


class TestSiteData(TmpDb):
    def test_incubator_excluded_and_fields_stripped(self):
        self.write(
            "questions/en/questions.yaml",
            self.question(
                "What belief have you outgrown lately?",
                "[new_people, close]",
                2,
                '  author: "seed"\n',
            ),
        )
        (self.tmp / "questions/incubator/fr").mkdir(parents=True)
        self.write(
            "questions/incubator/fr/questions.yaml",
            self.question("Quelle certitude as-tu abandonnée récemment?", "[close]", 2),
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
            set(entry), {"id", "text", "decks", "depth", "tags"}
        )  # author/added/origin stripped
        recent = json.loads((out / "recent.en.json").read_text())
        self.assertEqual(recent[0]["decks"], ["new_people", "close"])
        self.assertEqual(recent[0]["depth"], 2)
        index = json.loads((out / "index.json").read_text())
        self.assertEqual(list(index.values()), ["en"])


class TestBundle(unittest.TestCase):
    def test_round_trip_against_real_database(self):
        # Round-trip both shipped languages.
        with tempfile.TemporaryDirectory() as tmp:
            code, _, _ = run_quiet(
                build_bundle.main, ["--root", str(REPO), "--out", tmp, "--version", "test.1"]
            )
            self.assertEqual(code, 0)
            manifest = json.loads((Path(tmp) / "manifest.json").read_text())
            self.assertEqual(manifest["schema"], 3)
            for lang in ("en", "de"):
                blob = (Path(tmp) / f"bundle-{lang}-test.1.qdb").read_bytes()
                self.assertEqual(len(blob), manifest["languages"][lang]["size"])
                parsed = build_bundle.parse_bundle(blob)
                self.assertEqual(parsed["lang"], lang)
                self.assertEqual(parsed["version"], "test.1")
                self.assertEqual(len(parsed["questions"]), manifest["languages"][lang]["count"])
                for question in parsed["questions"]:
                    self.assertTrue(question["decks"])
                    self.assertIn(question["depth"], (1, 2, 3))
                    if question["spicy"] or question["dark"]:
                        self.assertEqual(question["decks"], ["wild"])

    def test_manifest_describes_the_raw_bundle_a_device_downloads(self):
        """schema 3: the device takes the .qdb, and size/sha256/sig cover it.

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
                self.assertEqual(raw[:4], b"QDB2")

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
            published = (out / "kveld-0.2.0.bin").read_bytes()

            self.assertEqual(manifest["schema"], 1)
            self.assertEqual(manifest["version"], "0.2.0")
            self.assertTrue(manifest["url"].endswith("kveld-0.2.0.bin"))
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
    return json.loads((REPO / "questions" / "schema.json").read_text(encoding="utf-8"))["x-kveld"]


class TestIssueFormVocabulary(unittest.TestCase):
    """The issue form is the only place a contributor picks these values, and
    the website prefills it by option string. A value that is not declared
    here is dropped by GitHub, leaving a required field blank."""

    def setUp(self):
        self.options = issue_form_options()
        self.cfg = schema_config()

    def test_deck_options_follow_the_schema_order(self):
        self.assertEqual(self.options["decks"], self.cfg["decks"])

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
    decks="new_people, close",
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
        f"### Decks\n\n{decks}\n\n"
        f"### Depth\n\n{depth}\n\n"
        f"### Question\n\n{question}\n\n"
        f"### Tags (optional)\n\n{tags}\n\n"
        f"### Name for credit (optional)\n\n{credit}\n\n"
        f"### Public domain dedication\n\n{cc0}\n"
    )


class PromoteCase(TmpDb):
    """A temp database plus a way to run promote_issue.py against it."""

    def setUp(self):
        super().setUp()
        self.addCleanup(setattr, promote_issue, "ROOT", promote_issue.ROOT)
        promote_issue.ROOT = self.tmp
        self.write("questions/en/questions.yaml", "")

    def promote(self, body, mode="apply", issue="7"):
        env = {"ISSUE_BODY": body, "ISSUE_NUMBER": issue}
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
        self.assertEqual(entry["decks"], ["new_people", "close"])
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

    def test_dark_questions_outside_wild_are_refused(self):
        code, _, err = self.promote(rendered_issue(decks="new_people, close", tags="dark"))
        self.assertEqual(code, 1)
        self.assertIn("wild", err)

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

    def test_unknown_tags_are_dropped_rather_than_written(self):
        code, _, _ = self.promote(rendered_issue(tags="memory, sneaky-injected-tag"))
        self.assertEqual(code, 0)
        self.assertEqual(self.corpus()[0]["tags"], ["memory"])

    def test_parse_mode_reads_the_issue_without_touching_the_database(self):
        code, out, _ = self.promote(rendered_issue(), mode="parse")
        self.assertEqual(code, 0)
        self.assertIn("lang: en", out)
        self.assertEqual(self.corpus(), None)


class TestCheckSubmission(PromoteCase):
    """`check` mode answers the submitter while the issue is still open, using
    the same validator that decides at merge time. It writes nothing."""

    def test_a_good_submission_passes_and_writes_nothing(self):
        code, out, _ = self.promote(rendered_issue(), mode="check")
        self.assertEqual(code, 0)
        self.assertIn("passes every automatic check", out)
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
