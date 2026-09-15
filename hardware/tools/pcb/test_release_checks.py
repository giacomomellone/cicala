import tempfile
import unittest
from pathlib import Path

from audit_electrical import check_duplicate_labels, check_switches
from check_assembly_bom import check
from ksexp import parse, load, children, ref_of


class AssemblyBomTests(unittest.TestCase):
    def check_csv(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bom.csv"
            path.write_text(text)
            check(path)

    def test_selected_part(self):
        self.check_csv('Refs,MPN\nU3,TPS63802DLAR\n')

    def test_unselected_or_multiple_parts(self):
        for value in ("", "TBD", "custom harness", "PART1; PART2", "PART1 alt PART2"):
            with self.subTest(value=value), self.assertRaisesRegex(SystemExit, "U3"):
                self.check_csv(f'Refs,MPN\nU3,{value}\n')

    def test_empty_bom(self):
        with self.assertRaisesRegex(SystemExit, "empty"):
            self.check_csv('Refs,MPN\n')

    def test_wrong_export_columns(self):
        with self.assertRaisesRegex(SystemExit, "columns"):
            self.check_csv('Reference,Value\nU3,TPS63802\n')

    def test_missing_reference(self):
        with self.assertRaisesRegex(SystemExit, "references"):
            self.check_csv('Refs,MPN\n,TPS63802DLAR\n')

    def test_truncated_row(self):
        with self.assertRaisesRegex(SystemExit, "U3"):
            self.check_csv('Refs,MPN\nU3\n')


class SchematicLabelTests(unittest.TestCase):
    def test_repeated_net_at_different_connections(self):
        check_duplicate_labels(parse('''(kicad_sch
            (hierarchical_label "3V3" (at 10 20 0))
            (hierarchical_label "3V3" (at 30 20 0)))'''))

    def test_overprinted_label_with_distinct_uuid(self):
        with self.assertRaisesRegex(ValueError, "Duplicate hierarchical_label USB_VBUS"):
            check_duplicate_labels(parse('''(kicad_sch
                (hierarchical_label "USB_VBUS" (at 46.99 38.1 0) (uuid "a"))
                (hierarchical_label "USB_VBUS" (at 46.990 38.100 0) (uuid "b")))'''))


class SwitchContactTests(unittest.TestCase):
    def test_silver_contact_substitution_requires_current_review(self):
        board = load(Path(__file__).resolve().parents[2] / 'rev_a/pcb/cicala_rev_a.kicad_pcb')
        check_switches(board)
        switch = next(fp for fp in children(board, 'footprint') if ref_of(fp) == 'SW1')
        mpn = next(p for p in children(switch, 'property') if p[1] == 'MPN')
        mpn[2] = 'KSC321GLFS'
        with self.assertRaisesRegex(ValueError, 'minimum switching current'):
            check_switches(board)


if __name__ == "__main__":
    unittest.main()
