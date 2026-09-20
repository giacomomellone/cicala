"""The public core archive must be deterministic and carry its standalone inputs."""

import hashlib
import json
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import package_core  # noqa: E402


class CorePackageTest(unittest.TestCase):
    def test_deterministic_package_and_provenance(self):
        root = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory() as directory:
            one, two = [Path(directory) / name for name in ("one.tar.gz", "two.tar.gz")]
            digest = package_core.package(root, one)
            self.assertEqual(digest, package_core.package(root, two))
            self.assertEqual(one.read_bytes(), two.read_bytes())
            with tarfile.open(one) as archive:
                names = archive.getnames()
                self.assertIn("src/vendor/tweetnacl.c", names)
                self.assertIn("tests/fixtures/selection.json", names)
                self.assertIn("include/trusted_key.h", names)
                self.assertIn("assets/LICENSE-QUESTIONS", names)
                self.assertFalse(any("__pycache__" in name for name in names))
                provenance = json.load(archive.extractfile("package-provenance.json"))
                corpus = archive.extractfile("assets/en.qdb").read()
                self.assertEqual(provenance["corpus_sha256"], hashlib.sha256(corpus).hexdigest())
                self.assertTrue(corpus.startswith(b"QDB4"))
