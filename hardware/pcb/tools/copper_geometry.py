"""Continuous copper geometry for routing audits (requires Shapely 2).

Distances are in mm. Mechanical copper, NPTH holes and every physical pad
remain separate objects, including repeated numbers in a footprint.
"""
import math

import numpy as np
from shapely import STRtree, contains_xy
from shapely.affinity import rotate, translate
from shapely.geometry import LineString, Point, Polygon, box

LAYERS = ('F.Cu', 'In2.Cu', 'B.Cu')


def pad_shape(pad, drill=False):
    w = pad.get('drill_w', pad['drill']) if drill else pad['w']
    h = pad.get('drill_h', pad['drill']) if drill else pad['h']
    shape = 'oval' if drill else pad['shape']
    if shape in ('circle', 'oval'):
        r = min(w, h) / 2
        dx, dy = max(0, w / 2 - r), max(0, h / 2 - r)
        poly = LineString([(-dx, -dy), (dx, dy)]).buffer(r, quad_segs=32)
    elif shape == 'roundrect' and pad['r']:
        r = pad['r']
        poly = box(-w / 2 + r, -h / 2 + r, w / 2 - r, h / 2 - r).buffer(r, quad_segs=32)
    else:
        poly = box(-w / 2, -h / 2, w / 2, h / 2)
    return translate(rotate(poly, -pad['angle'], origin=(0, 0)), pad['x'], pad['y'])


def segment_shape(segment):
    return LineString([segment['start'], segment['end']]).buffer(segment['width'] / 2, quad_segs=32)


def via_shape(via):
    return Point(via['at']).buffer(via['size'] / 2, quad_segs=32)


def raster(poly, mask, grid, origin):
    """OR polygon-covered cell centres into a mask, clipping to its bounds."""
    if poly.is_empty:
        return
    x0, y0, x1, y1 = poly.bounds
    ix0 = max(0, math.floor((x0 - origin[0]) / grid))
    iy0 = max(0, math.floor((y0 - origin[1]) / grid))
    ix1 = min(mask.shape[1], math.ceil((x1 - origin[0]) / grid) + 1)
    iy1 = min(mask.shape[0], math.ceil((y1 - origin[1]) / grid) + 1)
    if ix1 <= ix0 or iy1 <= iy0:
        return
    xs = origin[0] + np.arange(ix0, ix1)[None, :] * grid
    ys = origin[1] + np.arange(iy0, iy1)[:, None] * grid
    mask[iy0:iy1, ix0:ix1] |= contains_xy(poly, xs, ys)


class Copper:
    def __init__(self, geom):
        self.geom = geom
        self.outline = Polygon(geom['outline'], geom['outline_holes'])
        self.inner = self.outline.buffer(-0.3)
        self.objects = []
        self.holes = []
        self.pads = []
        from recovery_keepout import polygon as contact_keepout
        self.keepouts = list(geom['keepouts'])
        recovery = contact_keepout(geom['pads'])
        if recovery is not None:
            self.keepouts.append({'layers':['B.Cu'],'tracks':False,'vias':False,
                                  'pts':list(recovery.exterior.coords)})
        self.areas = [(ko,Polygon(ko['pts'])) for ko in self.keepouts]
        self.object_tree = self.hole_tree = None
        for p in geom['pads']:
            if p['npth']:
                self.holes.append(('', pad_shape(p, drill=True)))
                continue
            layers = tuple(l for l in LAYERS if l in p['layers'])
            if not layers:
                continue
            poly = pad_shape(p)
            self.objects.append((p['net'], layers, poly, f"{p['ref']}.{p['pad']}"))
            self.pads.append(poly)
            if p['drill']:
                self.holes.append((p['net'], pad_shape(p, drill=True)))
        self.pad_tree = STRtree(self.pads)

    def add_segment(self, s):
        self.objects.append((s['net'], (s['layer'],), segment_shape(s), 'track'))
        self.object_tree = None

    def add_via(self, v):
        self.objects.append((v['net'], LAYERS, via_shape(v), 'via'))
        self.holes.append((v['net'], Point(v['at']).buffer(v['drill']/2, quad_segs=32)))
        self.object_tree = self.hole_tree = None

    def clear(self, poly, net, layers, via=False):
        if not self.inner.covers(poly):
            return False
        for ko, area in self.areas:
            if (not ko['vias'] if via else not ko['tracks']) and set(layers).intersection(ko['layers']):
                if area.intersects(poly):
                    return False
        if self.object_tree is None:
            self.object_tree = STRtree([o[2] for o in self.objects])
        for i in self.object_tree.query(poly,predicate='dwithin',distance=0.508):
            other, olayers, shape, _label = self.objects[i]
            clearance = 0.508 if _label.startswith('J4.') else 0.2
            if other != net and set(layers).intersection(olayers) and poly.distance(shape) < clearance-0.00001:
                return False
        if self.hole_tree is None:
            self.hole_tree = STRtree([h[1] for h in self.holes])
        for i in self.hole_tree.query(poly,predicate='dwithin',distance=0.25):
            hole_net, hole = self.holes[i]
            if hole_net == net and not via:
                continue
            if poly.distance(hole) < 0.24999:
                return False
        if via and len(self.pad_tree.query(poly,predicate='intersects')):
            return False
        return True

    def masks(self, net, width, via_size, grid, origin, shape):
        slack = grid / math.sqrt(2) + 0.002
        masks = []
        for diameter, is_via in ((width, False), (via_size, True)):
            radius = diameter / 2
            inside = np.zeros(shape, dtype=bool)
            raster(self.outline.buffer(-0.3 - radius - slack), inside, grid, origin)
            result = [~inside.copy() for _ in LAYERS]
            for ko in self.keepouts:
                if ko['vias'] if is_via else ko['tracks']:
                    continue
                poly = Polygon(ko['pts']).buffer(radius + slack)
                for l, name in enumerate(LAYERS):
                    if name in ko['layers']:
                        raster(poly, result[l], grid, origin)
            for other, layers, poly, label in self.objects:
                if other == net and not (is_via and label != 'track'):
                    continue
                clearance = 0.508 if label.startswith('J4.') else 0.2
                expanded = poly.buffer(radius + clearance + slack)
                for l, name in enumerate(LAYERS):
                    if name in layers:
                        raster(expanded, result[l], grid, origin)
            for hole_net, hole in self.holes:
                if hole_net == net and not is_via:
                    continue
                expanded = hole.buffer(radius + 0.25 + slack)
                for mask in result:
                    raster(expanded, mask, grid, origin)
            masks.append([m.tobytes() for m in result])
        return masks
