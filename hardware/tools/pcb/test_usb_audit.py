import unittest

from audit_usb import route_length


class RouteMeasurementTests(unittest.TestCase):
    def geometry(self):
        return {
            'pads': [dict(ref='J1', pad='A', net='USB', x=0, y=0),
                     dict(ref='U1', pad='B', net='USB', x=5, y=0)],
            'segments': [dict(net='USB', layer='B.Cu', start=[0, 0], end=[2, 0]),
                         dict(net='USB', layer='F.Cu', start=[2, 0], end=[4, 0]),
                         dict(net='USB', layer='B.Cu', start=[4, 0], end=[5, 0]),
                         dict(net='USB', layer='F.Cu', start=[3, 0], end=[3, 10])],
            'vias': [dict(net='USB', at=[2, 0]), dict(net='USB', at=[4, 0])],
        }

    def length(self, geom):
        return route_length(geom, 'USB', ('J1', 'A'), ('U1', 'B'),
                            {'F.Cu': 0, 'In1.Cu': .2, 'In2.Cu': .8, 'B.Cu': 1})

    def test_barrels_count_and_test_stub_is_excluded(self):
        self.assertAlmostEqual(self.length(self.geometry()), 7)

    def test_missing_transition_fails_instead_of_reporting_partial_length(self):
        geom = self.geometry()
        geom['vias'].pop()
        with self.assertRaisesRegex(ValueError, 'No routed path'):
            self.length(geom)

    def test_actual_route_change_affects_measurement(self):
        geom = self.geometry()
        geom['segments'][1:2] = [
            dict(net='USB', layer='F.Cu', start=[2, 0], end=[3, 1]),
            dict(net='USB', layer='F.Cu', start=[3, 1], end=[4, 0])]
        self.assertAlmostEqual(self.length(geom), 5 + 2 ** 1.5)
