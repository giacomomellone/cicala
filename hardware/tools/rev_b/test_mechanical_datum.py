import json
import unittest
import sys
from generate_contract import CONTRACT, ROOT
from mechanical_datum import check_mesh_datum
sys.path.insert(0, str(ROOT/'hardware/tools/case'))
from check_mesh import triangles


class MechanicalDatumTests(unittest.TestCase):
    def setUp(self):
        self.contract = json.loads(CONTRACT.read_text())
        self.faces = triangles(ROOT/'hardware/rev_b/case/exports/reference/pcb_reference.stl')

    def test_right_handed_outline_and_mounts_match(self):
        self.assertLess(check_mesh_datum(self.faces, self.contract)['outline_and_mounts_difference_mm2'], .05)

    def test_mirrored_print_is_rejected(self):
        mirrored = [[(x, -y, z) for x, y, z in f] for f in self.faces]
        with self.assertRaisesRegex(ValueError, 'handedness'):
            check_mesh_datum(mirrored, self.contract)

    def test_displaced_mount_is_rejected(self):
        self.contract['mounts'][0][0] += 1
        with self.assertRaisesRegex(ValueError, 'handedness'):
            check_mesh_datum(self.faces, self.contract)


if __name__ == '__main__':
    unittest.main()
