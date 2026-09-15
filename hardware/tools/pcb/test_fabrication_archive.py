from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile

from audit_fabrication import check_ipc2581
import package_prototype


class SnapshotSourceTests(unittest.TestCase):
    def test_new_website_artwork_is_included_without_dependencies_or_backups(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            (root / '.gitignore').write_text('website/node_modules/\n')
            for name in ('website/public/brand/mark.svg', 'website/src/Logo.astro',
                         'website/node_modules/dependency/index.js', 'hardware/pcb/board.bak'):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('fixture')
            with patch.object(package_prototype, 'ROOT', root):
                names = {str(path.relative_to(root)) for path in package_prototype.snapshot_files()}
            self.assertEqual(names, {'website/public/brand/mark.svg', 'website/src/Logo.astro'})


class IpcArchiveTests(unittest.TestCase):
    def test_plain_xml_with_zip_extension_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'board.zip'
            path.write_text('<IPC-2581 revision="C"/>')
            with self.assertRaisesRegex(ValueError, 'not a ZIP archive'):
                check_ipc2581(path)

    def test_compressed_ipc_document(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'board.zip'
            with ZipFile(path, 'w') as archive:
                archive.writestr('board.xml', '<IPC-2581 xmlns="urn:ipc" revision="C"/>')
            self.assertEqual(check_ipc2581(path)['revision'], 'C')

    def test_unrelated_xml_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'board.zip'
            with ZipFile(path, 'w') as archive:
                archive.writestr('board.xml', '<not-a-board/>')
            with self.assertRaisesRegex(ValueError, 'root is missing'):
                check_ipc2581(path)
