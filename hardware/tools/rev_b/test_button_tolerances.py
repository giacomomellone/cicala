import copy
import json
import unittest

from button_tolerances import audit, select
from generate_contract import CONTRACT


class ButtonToleranceTests(unittest.TestCase):
    def setUp(self):
        self.contract = json.loads(CONTRACT.read_text())

    def test_current_parts_cover_declared_corners_only_after_selection(self):
        result = audit(self.contract)
        self.assertEqual(result['screened_corner_combinations'],192)
        self.assertLess(result['fixed_nominal_cap_gap_range'][0],0)
        self.assertFalse(result['fixed_parts_guarantee_actuation'])

    def test_short_stop_cannot_be_called_an_actuating_switch(self):
        self.assertIsNone(select([.1],[0],.5,0,0,.6,[.05,.15],[.05,.15]))

    def test_preloaded_button_is_rejected(self):
        self.assertIsNone(select([.1],[0],.65,.2,0,.4,[.05,.15],[.05,.15]))

    def test_insufficient_retaining_overlap_is_rejected(self):
        c = copy.deepcopy(self.contract)
        c['buttons']['retention_extension'] = .8
        with self.assertRaisesRegex(ValueError,'overlap'):
            audit(c)

    def test_single_nominal_pair_cannot_hide_stack_tolerance(self):
        c = copy.deepcopy(self.contract)
        c['buttons']['coupon_reliefs'] = [.1]
        with self.assertRaisesRegex(ValueError,'corners'):
            audit(c)


if __name__ == '__main__':
    unittest.main()
