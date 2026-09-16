"""Check printable STL topology and reject disconnected positive-volume bodies.

An enclosed void has a negative signed volume and is part of a solid, not a
second printable body. Accept both OpenSCAD's ASCII and binary STL exports.
"""
import collections
import struct
import sys
from pathlib import Path


def triangles(path):
    data = Path(path).read_bytes()
    count = struct.unpack_from("<I", data, 80)[0] if len(data) >= 84 else 0
    if len(data) == 84 + 50 * count:
        for i in range(count):
            values = struct.unpack_from("<12f", data, 84 + 50 * i)
            yield [tuple(values[j:j + 3]) for j in (3, 6, 9)]
    else:
        vertices = [tuple(map(float, line.split()[1:]))
                    for line in data.decode().splitlines()
                    if line.strip().startswith("vertex ")]
        if len(vertices) % 3:
            raise ValueError("Incomplete STL triangle")
        for i in range(0, len(vertices), 3):
            yield vertices[i:i + 3]


def check(path, expected_solids=1):
    faces = list(triangles(path))
    if not faces:
        raise ValueError("Empty mesh")
    edges = collections.defaultdict(list)
    parent = list(range(len(faces)))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i, face in enumerate(faces):
        for a, b in zip(face, face[1:] + face[:1]):
            edges[tuple(sorted((a, b)))].append((i, a < b))
    for edge, adjacent in edges.items():
        if len(adjacent) != 2 or adjacent[0][1] == adjacent[1][1]:
            raise ValueError(f"Open, nonmanifold or inconsistently wound edge: {edge}")
        parent[root(adjacent[0][0])] = root(adjacent[1][0])
    volumes = collections.defaultdict(float)
    for i, (a, b, c) in enumerate(faces):
        volumes[root(i)] += (
            a[0] * (b[1] * c[2] - b[2] * c[1])
            + a[1] * (b[2] * c[0] - b[0] * c[2])
            + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6
    solids = sum(v > 1e-6 for v in volumes.values())
    if solids == 0 or (expected_solids is not None and solids != expected_solids):
        raise ValueError(f"Expected {expected_solids or 'positive-volume'} printable body, found {solids}")
    print(f"{Path(path).name}: closed oriented mesh, {solids} solid(s), {sum(volumes.values()):.2f} mm³")


if __name__ == "__main__":
    for argument in sys.argv[1:]:
        check(argument)
