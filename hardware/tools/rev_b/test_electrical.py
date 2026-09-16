import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parent))
from electrical import JT_RT, check_programming_values
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

    def test_charge_voltage_and_usb_termination_values_are_checked(self):
        values = {'R7':'18k','R8':'3.00k','R14':'511k 1%','R15':'91k 1%',
                  'R1':'5.1k','R2':'5.1k','R21':'22k','R22':'22k'}
        self.assertEqual(check_programming_values(values)['charger_regulation_v'], 4.2)
        for ref, wrong in [('R7','75k'),('R8','300'),('R14','806k 1%'),('R2','0')]:
            with self.subTest(ref=ref), self.assertRaisesRegex(ValueError, ref):
                check_programming_values({**values,ref:wrong})


if __name__=='__main__':unittest.main()
