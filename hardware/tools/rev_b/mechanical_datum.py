"""Check printable PCB geometry against KiCad's right-handed STEP datum."""
from shapely.geometry import Polygon, Point
from shapely.ops import unary_union

from generate_contract import outline_points


def check_mesh_datum(faces, contract):
    top = contract['pcb']['z'] + contract['pcb']['thickness']
    actual = unary_union([
        Polygon([(p[0], p[1]) for p in face]) for face in faces
        if all(abs(p[2] - top) < 1e-4 for p in face)
    ])
    expected = Polygon([(x, -y) for x, y in outline_points(contract)])
    for x, y in contract['mounts']:
        expected = expected.difference(Point(x, -y).buffer(1.35, quad_segs=12))
    if actual.is_empty or actual.symmetric_difference(expected).area > .05:
        raise ValueError('PCB mesh outline/mounts do not match KiCad STEP handedness')
    return {'datum': 'KiCad STEP: X right, Y up; Z translated by pcb.z',
            'outline_and_mounts_difference_mm2': round(actual.symmetric_difference(expected).area, 6)}
