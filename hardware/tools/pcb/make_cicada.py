"""Convert the public cicada SVG into millimetre strokes and wing fills.

Only regeneration needs fontTools. Board artwork reads the committed JSON.
"""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from fontTools.pens.basePen import BasePen
from fontTools.svgLib.path import parse_path


FILL_TOLERANCE_MM = 0.002


def flatten_curve(points):
    """Bound the fill polygon's deviation from each cubic by 2 micrometres."""
    start, a, b, end = points
    dx, dy = end[0] - start[0], end[1] - start[1]
    length2 = dx * dx + dy * dy

    def distance2(point):
        t = max(0, min(1, ((point[0] - start[0]) * dx
                          + (point[1] - start[1]) * dy) / length2)) if length2 else 0
        return (point[0] - start[0] - t * dx) ** 2 + (point[1] - start[1] - t * dy) ** 2

    if max(distance2(a), distance2(b)) <= FILL_TOLERANCE_MM ** 2:
        return [end]

    def midpoint(p, q):
        return [(x + y) / 2 for x, y in zip(p, q)]

    ab, bc, cd = midpoint(start, a), midpoint(a, b), midpoint(b, end)
    abc, bcd = midpoint(ab, bc), midpoint(bc, cd)
    middle = midpoint(abc, bcd)
    return (flatten_curve([start, ab, abc, middle])
            + flatten_curve([middle, bcd, cd, end]))


class StrokePen(BasePen):
    def __init__(self, centre, scale):
        super().__init__(None)
        self.centre = centre
        self.scale = scale
        self.segments = []
        self.start = None
        self.point = None
        self.contours = []
        self.contour = []
        self.open_contours = 0

    def xy(self, point):
        return [round((value - origin) * self.scale, 6)
                for value, origin in zip(point, self.centre)]

    def _moveTo(self, point):
        self.start = self.point = point
        self.contour = [self.xy(point)]

    def _lineTo(self, point):
        if point != self.point:
            self.segments.append({'type': 'line', 'points': [self.xy(self.point), self.xy(point)]})
            self.contour.append(self.xy(point))
        self.point = point

    def _curveToOne(self, a, b, point):
        points = [self.xy(p) for p in (self.point, a, b, point)]
        self.segments.append({'type': 'curve', 'points': points})
        self.contour.extend(flatten_curve(points))
        self.point = point

    def _closePath(self):
        self._lineTo(self.start)
        self.contours.append(self.contour)
        self.contour = []

    def _endPath(self):
        self.open_contours += 1


def svg_paths(element, inherited):
    if any(key in element.attrib for key in ('transform', 'style', 'opacity', 'fill-opacity', 'stroke-opacity')):
        raise ValueError('Cicada artwork must use untransformed, opaque SVG attributes')
    tag = element.tag.split('}')[-1]
    attributes = inherited | element.attrib
    if tag == 'path':
        yield attributes
    elif tag in {'svg', 'g'}:
        for child in element:
            yield from svg_paths(child, attributes)
    elif tag not in {'title', 'desc'}:
        raise ValueError(f'Unsupported cicada element: {tag}')


def cicada(source, size=8.0):
    raw = Path(source).read_bytes()
    root = ET.fromstring(raw)
    x, y, width, height = map(float, root.attrib['viewBox'].split())
    if size <= 0 or width <= 0 or height <= 0:
        raise ValueError('Cicada dimensions must be positive')
    scale = size / max(width, height)
    segments, polygons = [], []
    for path in svg_paths(root, {'fill': 'black', 'stroke': 'none', 'stroke-width': '1'}):
        pen = StrokePen((x + width / 2, y + height / 2), scale)
        parse_path(path['d'], pen)
        if path['fill'] != 'none':
            if len(pen.contours) != 1 or pen.open_contours:
                raise ValueError('Filled cicada paths must have one closed contour without holes')
            polygons.append([[round(value, 6) for value in point] for point in pen.contours[0]])
        if path['stroke'] != 'none':
            stroke = round(float(path['stroke-width']) * scale, 6)
            if stroke <= 0 or path.get('stroke-linejoin') != 'round':
                raise ValueError('Cicada strokes must have positive widths and round joins')
            if pen.open_contours and path.get('stroke-linecap') != 'round':
                raise ValueError('Open cicada strokes must have round caps')
            segments.extend(segment | {'stroke_mm': stroke} for segment in pen.segments)
    if not segments and not polygons:
        raise ValueError('Cicada artwork has no visible geometry')
    return {
        'source': 'website/public/brand/cicala-mark.svg',
        'source_sha256': hashlib.sha256(raw).hexdigest(),
        'size_mm': size,
        'fill_tolerance_mm': FILL_TOLERANCE_MM,
        'polygons': polygons,
        'segments': segments,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('svg', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(cicada(args.svg), indent=2) + '\n')
