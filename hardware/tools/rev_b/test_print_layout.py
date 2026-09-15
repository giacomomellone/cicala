import unittest

from print_layout import placement, role, transform


class PrintLayoutTests(unittest.TestCase):
    def test_rotation_then_translation_places_all_vertices_on_or_above_bed(self):
        faces = [[(100,10,6),(104,10,8),(100,20,6)]]
        result = placement(faces,[0,150,0])
        points = [transform(p,result['rotation_degrees']) for p in faces[0]]
        shifted = [[a+b for a,b in zip(p,result['translation_mm'])] for p in points]
        for axis in range(3):
            self.assertAlmostEqual(min(p[axis] for p in shifted),0)

    def test_sheet_and_component_references_are_never_print_jobs(self):
        for part in ('cap_pad','next_pad','pcb_reference','cell_reference','panel_reference'):
            self.assertEqual(role(part),'reference')


if __name__ == '__main__':
    unittest.main()
