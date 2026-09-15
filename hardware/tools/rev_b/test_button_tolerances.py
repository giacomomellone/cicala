import copy
import json
import unittest
from button_tolerances import audit
from generate_contract import CONTRACT

class ButtonToleranceTests(unittest.TestCase):
    def setUp(self):self.contract=json.loads(CONTRACT.read_text())
    def test_worst_case_clearances_remain_positive(self):
        result=audit(self.contract)
        for key in ('minimum_radial_clearance_mm','minimum_pressed_cap_proud_mm','minimum_lead_base_clearance_mm'):
            self.assertGreater(result[key],0)
    def test_tight_aperture_rejects_binding(self):
        self.contract['buttons']['radial_clearance']=.1
        with self.assertRaisesRegex(ValueError,'bind'):audit(self.contract)
    def test_high_panel_rejects_inaccessible_pressed_cap(self):
        self.contract['case']['control_ledge_z']=15.5
        with self.assertRaisesRegex(ValueError,'sink'):audit(self.contract)
    def test_low_board_rejects_lead_collision(self):
        self.contract['pcb']['z']=4
        self.contract['case']['control_ledge_z']=13
        with self.assertRaisesRegex(ValueError,'lead'):audit(self.contract)

if __name__=='__main__':unittest.main()
