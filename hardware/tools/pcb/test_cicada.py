import tempfile
import unittest
from pathlib import Path

from shapely.geometry import LineString, Point, Polygon

from make_cicada import FILL_TOLERANCE_MM, cicada, flatten_curve


class CicadaTests(unittest.TestCase):
    def convert(self, body, attributes=''):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'mark.svg'
            source.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" '
                              f'viewBox="0 0 8 8" fill="none" {attributes}>{body}</svg>')
            return cicada(source)

    def test_group_fill_and_root_head_keep_their_own_stroke_widths(self):
        art = self.convert(
            '<g fill="currentColor" stroke="currentColor" stroke-width="0.3" '
            'stroke-linejoin="round"><path d="M1 1L3 1L2 4Z"/></g>'
            '<path d="M6 1C4 1 4 3 6 3" fill="none" stroke="currentColor" '
            'stroke-width="0.5" stroke-linecap="round" stroke-linejoin="round"/>')
        self.assertEqual(len(art['polygons']), 1)
        self.assertAlmostEqual(Polygon(art['polygons'][0]['outline']).area, 3)
        self.assertEqual([s['stroke_mm'] for s in art['segments']], [.3, .3, .3, .5])
        self.assertEqual(art['segments'][-1]['points'], [[2, -3], [0, -3], [0, -1], [2, -1]])

    def test_group_fill_can_be_overridden_by_an_open_path(self):
        art = self.convert(
            '<g fill="currentColor" stroke="currentColor" stroke-width="0.3" '
            'stroke-linejoin="round" stroke-linecap="round">'
            '<path d="M1 1L3 1L2 4Z"/><path d="M4 1L6 1" fill="none"/></g>')
        self.assertEqual(len(art['polygons']), 1)
        self.assertEqual(len(art['segments']), 4)

    def test_fill_only_path_does_not_add_a_stroke(self):
        art = self.convert('<path fill="currentColor" d="M1 1L3 1L2 4Z"/>')
        self.assertEqual(len(art['polygons']), 1)
        self.assertEqual(art['segments'], [])

    def test_counters_are_nested_inside_the_outline_they_cut(self):
        art = self.convert('<path fill="currentColor" fill-rule="evenodd" '
                           'd="M1 1L7 1L7 7L1 7Z M3 3L5 3L5 5L3 5Z"/>')
        self.assertEqual(len(art['polygons']), 1)
        polygon = art['polygons'][0]
        self.assertEqual(len(polygon['holes']), 1)
        self.assertAlmostEqual(Polygon(polygon['outline'], polygon['holes']).area, 32)

    def test_disjoint_contours_stay_separate_outlines(self):
        art = self.convert('<path fill="currentColor" fill-rule="evenodd" '
                           'd="M1 1L3 1L3 3L1 3Z M5 5L7 5L7 7L5 7Z"/>')
        self.assertEqual(len(art['polygons']), 2)
        self.assertEqual([len(polygon['holes']) for polygon in art['polygons']], [0, 0])

    def test_unsupported_geometry_is_rejected_instead_of_dropped(self):
        for body, attributes in [
            ('<g transform="translate(1 1)"><path d="M0 0L1 1"/></g>', ''),
            ('<path fill="black" d="M0 0L1 1L2 0Z"/>', 'transform="scale(2)"'),
            ('<circle cx="4" cy="4" r="2"/>', ''),
            ('<path style="fill:black" d="M0 0L1 1L2 0Z"/>', ''),
            ('<path fill="black" d="M0 0L1 1L2 0"/>', ''),
            ('<path fill="black" d="M0 0L4 0L4 4Z M1 1L2 1L2 2Z"/>', ''),
        ]:
            with self.subTest(body=body, attributes=attributes), self.assertRaises(ValueError):
                self.convert(body, attributes)

    def test_fill_curve_stays_within_declared_tolerance(self):
        controls = [(0, 0), (-3, 8), (7, -4), (4, 2)]
        line = LineString([controls[0], *flatten_curve(controls)])
        for i in range(1001):
            t = i / 1000
            point = tuple((1-t)**3*controls[0][j] + 3*(1-t)**2*t*controls[1][j]
                          + 3*(1-t)*t*t*controls[2][j] + t**3*controls[3][j]
                          for j in (0, 1))
            self.assertLessEqual(line.distance(Point(point)), FILL_TOLERANCE_MM)


if __name__ == '__main__':
    unittest.main()
