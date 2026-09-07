import unittest

from audit_placement import audit


class ThroughHolePlacementTests(unittest.TestCase):
    def test_alignment_hole_under_opposite_face_component(self):
        geom = {
            'footprints': [
                {'ref': 'J4', 'layer': 'B.Cu', 'courtyards': []},
                {'ref': 'L1', 'layer': 'F.Cu',
                 'courtyards': [[[9, 9], [11, 9], [11, 11], [9, 11]]]},
            ],
            'pads': [{'ref': 'J4', 'npth': True, 'drill': 1, 'w': 1,
                      'h': 1, 'x': 10, 'y': 10, 'angle': 0}],
        }
        self.assertEqual(audit(geom)['through_hole_component_intrusions'],
                         [['J4', 'L1']])
        geom['pads'][0]['x'] = 12
        self.assertEqual(audit(geom)['through_hole_component_intrusions'], [])


if __name__ == '__main__':
    unittest.main()
