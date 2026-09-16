import copy
import json
import unittest

from check_contract import check_outline
from generate_contract import CONTRACT, outline_points


class BoardOutlineTests(unittest.TestCase):
    def setUp(self):
        self.contract = json.loads(CONTRACT.read_text())
        self.geometry = {'outline': outline_points(self.contract), 'outline_holes': [],
                         'outline_count': 1, 'outline_valid': True}

    def test_bounding_rectangle_cannot_hide_a_missing_battery_bay(self):
        check_outline(self.geometry, self.contract)
        self.geometry['outline'] = [[3,2],[104,2],[104,64],[3,64]]
        with self.assertRaisesRegex(ValueError, 'L-shaped contract'):
            check_outline(self.geometry, self.contract)

    def test_second_island_and_open_outline_are_rejected(self):
        for field, value in [('outline_count',2),('outline_valid',False)]:
            geometry = copy.deepcopy(self.geometry)
            geometry[field] = value
            with self.assertRaisesRegex(ValueError, 'one closed outline'):
                check_outline(geometry, self.contract)

    def test_unrecorded_hole_cannot_disappear_from_enclosure_export(self):
        self.geometry['outline_holes'] = [[[80,50],[82,50],[82,52],[80,52]]]
        with self.assertRaisesRegex(ValueError, 'L-shaped contract'):
            check_outline(self.geometry, self.contract)


if __name__ == '__main__':
    unittest.main()
