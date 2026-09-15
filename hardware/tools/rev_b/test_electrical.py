import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parent))
from electrical import JT_RT
from audit_temperature import resistance, verify


class ChargeTemperatureTests(unittest.TestCase):
    def test_selected_sensor_stops_outside_cell_charge_range(self):
        result=verify(rt=JT_RT)
        self.assertGreater(result['model_margins_V']['cold_stop_at_0C_V'],0)
        self.assertGreater(result['model_margins_V']['hot_stop_at_45C_V'],0)
        self.assertGreater(result['model_margins_V']['permission_at_10C_V'],0)
        self.assertGreater(result['model_margins_V']['permission_at_30C_V'],0)

    def test_sensor_selection_does_not_change_rev_a_curve(self):
        self.assertAlmostEqual(resistance(0,JT_RT),27700)
        self.assertAlmostEqual(resistance(0),27280)
        self.assertAlmostEqual(resistance(25,JT_RT),10000)

    def test_unsafe_reference_divider_is_rejected(self):
        with self.assertRaises(ValueError):verify(r35=100000,rt=JT_RT)


if __name__=='__main__':unittest.main()
