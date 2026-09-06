"""Convert the public Literata wordmark to millimetre polygon artwork.

Only regeneration needs fontTools and Brotli. The committed polygon asset is
used by KiCad's Python without a font installation or website dependency.
"""
import argparse
import json
from pathlib import Path

from fontTools.pens.basePen import BasePen
from fontTools.ttLib import TTFont
from shapely import affinity, union_all
from shapely.geometry import Polygon


class FlattenPen(BasePen):
    def __init__(self, glyphs):
        super().__init__(glyphs)
        self.contours = []
        self.points = []

    def _moveTo(self, p):
        self.points = [p]

    def _lineTo(self, p):
        self.points.append(p)

    def _curveToOne(self, a, b, end):
        start = self.points[-1]
        for i in range(1, 25):
            t = i / 24
            self.points.append(tuple((1-t)**3*start[j] + 3*(1-t)**2*t*a[j]
                                     + 3*(1-t)*t*t*b[j] + t**3*end[j]
                                     for j in (0, 1)))

    def _qCurveToOne(self, control, end):
        start = self.points[-1]
        for i in range(1, 25):
            t = i / 24
            self.points.append(tuple((1-t)**2*start[j] + 2*(1-t)*t*control[j]
                                     + t*t*end[j] for j in (0, 1)))

    def _closePath(self):
        self.contours.append(self.points)
        self.points = []

    _endPath = _closePath


def wordmark(font_path, width=22.0):
    font = TTFont(font_path)
    glyphs, cmap = font.getGlyphSet(), font.getBestCmap()
    cursor, shapes = 0, []
    tracking = -0.035 * font['head'].unitsPerEm
    for char in 'cicala':
        glyph = glyphs[cmap[ord(char)]]
        pen = FlattenPen(glyphs)
        glyph.draw(pen)
        shape = Polygon()
        for contour in pen.contours:
            shape = shape.symmetric_difference(Polygon(contour))
        shapes.append(affinity.translate(shape, xoff=cursor))
        cursor += glyph.width + tracking
    ink = union_all(shapes)
    x0, y0, x1, y1 = ink.bounds
    scale = width / (x1-x0)
    ink = affinity.translate(ink, xoff=-(x0+x1)/2, yoff=-(y0+y1)/2)
    ink = affinity.scale(ink, xfact=scale, yfact=-scale, origin=(0, 0))
    polygons = list(ink.geoms) if ink.geom_type == 'MultiPolygon' else [ink]
    return {'source': 'Literata Latin 400 normal; public cicala wordmark',
            'width_mm': width, 'tracking_em': -0.035,
            'polygons': [{'outline': list(p.exterior.coords),
                          'holes': [list(h.coords) for h in p.interiors]}
                         for p in polygons]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(wordmark(args.font), indent=2) + '\n')
