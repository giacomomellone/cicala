import unittest

from jlc_assembly import references


class ReferenceRangeTests(unittest.TestCase):
    def test_kicad_grouped_ranges(self):
        self.assertEqual(references('C12,C17-C20,C27'),
                         ['C12', 'C17', 'C18', 'C19', 'C20', 'C27'])

    def test_invalid_ranges_and_duplicates(self):
        for value in ['C3-C1', 'C1-R4', 'C1-C3,C2']:
            with self.subTest(value=value), self.assertRaises(ValueError):
                references(value)


if __name__ == '__main__':
    unittest.main()
