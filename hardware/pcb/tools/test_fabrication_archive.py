from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile

from audit_fabrication import check_ipc2581


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
