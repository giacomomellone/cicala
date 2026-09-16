import unittest
from audit_temperature import resistance, verify


class TemperatureTests(unittest.TestCase):
    def test_interpolation_retains_manufacturer_points(self):
        self.assertAlmostEqual(resistance(0), 27280)
        self.assertAlmostEqual(resistance(25), 10000)
        self.assertAlmostEqual(resistance(40), 5827)

    def test_unknown_temperatures_are_not_extrapolated(self):
        with self.assertRaises(ValueError):
            resistance(-10)

    def test_selected_values_stop_at_both_battery_limits(self):
        result = verify()
        self.assertTrue(all(v > 0 for v in result['model_margins_V'].values()))

    def test_hot_reference_shift_cannot_silently_widen_charge_window(self):
        with self.assertRaises(ValueError):
            verify(r37=47000)

    def test_cold_reference_shift_cannot_silently_widen_charge_window(self):
        with self.assertRaises(ValueError):
            verify(r35=470000)
