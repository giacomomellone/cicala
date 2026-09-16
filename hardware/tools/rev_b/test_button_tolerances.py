import json
import unittest
from button_tolerances import audit
from generate_contract import CONTRACT


class ButtonToleranceTests(unittest.TestCase):
    def setUp(self):
        self.contract = json.loads(CONTRACT.read_text())

    def test_worst_case_clearances_remain_positive(self):
        result = audit(self.contract)
        for key in ('minimum_radial_clearance_mm', 'minimum_unpressed_recess_mm',
                    'minimum_access_probe_clearance_mm', 'minimum_lead_base_clearance_mm'):
            self.assertGreater(result[key], 0)

    def test_tight_aperture_rejects_binding(self):
        self.contract['buttons']['radial_clearance'] = .1
        with self.assertRaisesRegex(ValueError, 'bind'):
            audit(self.contract)

    def test_low_panel_rejects_exposed_cap(self):
        self.contract['case']['control_ledge_z'] = 16
        with self.assertRaisesRegex(ValueError, 'protrude'):
            audit(self.contract)

    def test_excessive_recess_rejects_inaccessible_button(self):
        self.contract['case']['control_ledge_z'] = 18
        self.contract['buttons']['nominal_recess'] = 2.2
        with self.assertRaisesRegex(ValueError, 'deeply'):
            audit(self.contract)

    def test_low_board_rejects_lead_collision(self):
        self.contract['pcb']['z'] = 4
        self.contract['case']['control_ledge_z'] = 16.2
        with self.assertRaisesRegex(ValueError, 'lead'):
            audit(self.contract)

    def test_small_unflared_well_rejects_probe(self):
        self.contract['buttons']['well_flare'] = 0
        with self.assertRaisesRegex(ValueError, 'probe'):
            audit(self.contract)


if __name__ == '__main__':
    unittest.main()
