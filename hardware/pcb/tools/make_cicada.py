"""Convert the public cicada SVG into native millimetre stroke geometry.

Only regeneration needs fontTools. Board artwork reads the committed JSON.
"""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from fontTools.pens.basePen import BasePen
from fontTools.svgLib.path import parse_path


class StrokePen(BasePen):
    def __init__(self, centre, scale):
        super().__init__(None)
        self.centre = centre
        self.scale = scale
        self.segments = []
        self.start = None
        self.point = None

    def xy(self, point):
        return [round((value - origin) * self.scale, 6)
                for value, origin in zip(point, self.centre)]

    def _moveTo(self, point):
        self.start = self.point = point

    def _lineTo(self, point):
        if point != self.point:
            self.segments.append({'type': 'line', 'points': [self.xy(self.point), self.xy(point)]})
        self.point = point

    def _curveToOne(self, a, b, point):
        self.segments.append({'type': 'curve',
                              'points': [self.xy(p) for p in (self.point, a, b, point)]})
        self.point = point

    def _closePath(self):
        self._lineTo(self.start)

    def _endPath(self):
        pass


def cicada(source, size=8.0):
    raw = Path(source).read_bytes()
    root = ET.fromstring(raw)
    x, y, width, height = map(float, root.attrib['viewBox'].split())
    group = root.find('{http://www.w3.org/2000/svg}g')
    if group is None or root.attrib.get('fill') != 'none':
        raise ValueError('Expected the public unfilled cicada stroke artwork')
    scale = size / max(width, height)
    pen = StrokePen((x + width / 2, y + height / 2), scale)
    for path in group:
        if path.tag != '{http://www.w3.org/2000/svg}path' or 'transform' in path.attrib:
            raise ValueError('Cicada artwork must contain untransformed SVG paths')
        parse_path(path.attrib['d'], pen)
    return {
        'source': 'website/public/brand/cicala-mark.svg',
        'source_sha256': hashlib.sha256(raw).hexdigest(),
        'size_mm': size,
        'stroke_mm': round(float(group.attrib['stroke-width']) * scale, 6),
        'segments': pen.segments,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('svg', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(cicada(args.svg), indent=2) + '\n')
