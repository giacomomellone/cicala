import contextlib
import io
import struct
import tempfile
import unittest
from pathlib import Path

from check_mesh import check


def tetrahedron(offset=(0, 0, 0), scale=1):
    points = [tuple(offset[i] + scale * vertex[i] for i in range(3))
              for vertex in [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]]
    return [[points[i] for i in face]
            for face in [(0, 2, 1), (0, 1, 3), (0, 3, 2), (1, 2, 3)]]


class MeshTests(unittest.TestCase):
    def check_faces(self, faces, binary=False):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "part.stl"
            if binary:
                data = b"solid binary".ljust(80, b"\0") + struct.pack("<I", len(faces))
                for face in faces:
                    data += struct.pack("<12fH", 0, 0, 0,
                                        *(v for point in face for v in point), 0)
                path.write_bytes(data)
            else:
                text = "solid part\n"
                for face in faces:
                    text += "facet normal 0 0 0\nouter loop\n"
                    text += "".join("vertex " + " ".join(map(str, p)) + "\n" for p in face)
                    text += "endloop\nendfacet\n"
                path.write_text(text + "endsolid part\n")
            with contextlib.redirect_stdout(io.StringIO()):
                check(path)

    def test_closed_ascii_and_binary(self):
        for binary in (False, True):
            with self.subTest(binary=binary):
                self.check_faces(tetrahedron(), binary)

    def test_enclosed_void_is_not_a_second_printed_solid(self):
        cavity = [list(reversed(face)) for face in tetrahedron((.1, .1, .1), .1)]
        self.check_faces(tetrahedron() + cavity)

    def test_detached_standoff(self):
        with self.assertRaisesRegex(ValueError, "found 2"):
            self.check_faces(tetrahedron() + tetrahedron((2, 0, 0)))

    def test_open_surface(self):
        with self.assertRaisesRegex(ValueError, "Open, nonmanifold"):
            self.check_faces(tetrahedron()[:-1])

    def test_reversed_face(self):
        faces = tetrahedron()
        faces[0].reverse()
        with self.assertRaisesRegex(ValueError, "inconsistently wound"):
            self.check_faces(faces)

    def test_empty_export(self):
        with self.assertRaisesRegex(ValueError, "Empty mesh"):
            self.check_faces([])


if __name__ == "__main__":
    unittest.main()
