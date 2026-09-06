"""Independent continuous-geometry audit and optional routing plot.

python audit_copper.py geometry.json [--tracks tracks.json] [--plot image.png]
Requires numpy, Shapely 2, and matplotlib for plots. Does not replace KiCad DRC.
"""
import argparse
import json
import math
from collections import Counter, defaultdict

from shapely import STRtree
from shapely.geometry import Polygon

from copper_geometry import Copper, LAYERS, pad_shape, segment_shape, via_shape


def audit(geom, tracks):
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    shapes = [o[2] for o in copper.objects]
    tree = STRtree(shapes)
    conflicts = []
    minimum = None
    for i, (net, layers, poly, label) in enumerate(copper.objects):
        for j in tree.query(poly, predicate='dwithin', distance=0.508):
            if j <= i:
                continue
            other, olayers, shape, olabel = copper.objects[j]
            if other == net or not set(layers).intersection(olayers):
                continue
            d = poly.distance(shape)
            minimum = d if minimum is None else min(minimum,d)
            recovery = ((label.startswith('J4.') and olabel in {'track','via'})
                        or (olabel.startswith('J4.') and label in {'track','via'}))
            clearance = 0.508 if recovery else 0.2
            if d < clearance-0.00001:
                conflicts.append({'a': [net, label, list(poly.centroid.coords)[0]],
                                  'b': [other, olabel, list(shape.centroid.coords)[0]],
                                  'clearance': round(d, 6), 'required':clearance})
    edge = []
    inner = copper.outline.buffer(-0.29999)
    for net, layers, poly, label in copper.objects:
        if not inner.covers(poly):
            edge.append([net, label, list(poly.centroid.coords)[0]])
    lengths = defaultdict(float)
    for s in tracks['segments']:
        lengths[s['net']] += math.dist(s['start'], s['end'])
    invalid_angles = [s for s in tracks['segments']
                      if abs(s['end'][0]-s['start'][0]) > 1e-5
                      and abs(s['end'][1]-s['start'][1]) > 1e-5
                      and abs(abs(s['end'][0]-s['start'][0])-abs(s['end'][1]-s['start'][1])) > 1e-5]
    via_in_pad = [v for v in tracks['vias']
                  if len(copper.pad_tree.query(via_shape(v),predicate='intersects'))]
    return {'segments': len(tracks['segments']), 'vias': len(tracks['vias']),
            'vias_by_net': dict(Counter(v['net'] for v in tracks['vias'])),
            'clearance_conflicts': conflicts, 'edge_conflicts': edge,
            'minimum_pad_track_via_clearance_mm':round(minimum,6) if minimum is not None else None,
            'via_in_pad':via_in_pad,
            'non_45_segments': invalid_angles,
            'total_copper_length_by_net': {n: round(v, 4) for n, v in lengths.items()}}


def plot(geom, tracks, path, bounds=None):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon as Patch
    fig, axs = plt.subplots(1, 3, figsize=(24, 7))
    colors = {'F.Cu': '#ba3446', 'In2.Cu': '#ad821e', 'B.Cu': '#246faf'}
    for ax, layer in zip(axs, LAYERS):
        ax.add_patch(Patch(geom['outline'], facecolor='#f6f4ea', edgecolor='black'))
        for s in tracks['segments']:
            if s['layer'] == layer:
                ax.add_patch(Patch(segment_shape(s).exterior.coords, color=colors[layer], linewidth=0))
        for p in geom['pads']:
            if layer in p['layers'] or p['npth']:
                poly = pad_shape(p)
                ax.add_patch(Patch(poly.exterior.coords, facecolor='white' if p['npth'] else '#b49b62', edgecolor='#615841', linewidth=0.2))
        for v in tracks['vias']:
            ax.add_patch(Patch(via_shape(v).exterior.coords, facecolor=colors[layer], linewidth=0))
            ax.add_patch(plt.Circle(v['at'], v['drill']/2, facecolor='white', linewidth=0))
        for fp in geom.get('footprints', []):
            if fp['layer'] == layer:
                ax.text(fp['x'], fp['y'], fp['ref'], fontsize=5, ha='center', va='center', color='#111', clip_on=True)
        ax.set(xlim=(2,82), ylim=(53,2), title=layer)
        if bounds:
            ax.set(xlim=(bounds[0], bounds[2]), ylim=(bounds[3], bounds[1]))
        ax.set_aspect('equal')
    fig.tight_layout()
    fig.savefig(path, dpi=200)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry')
    parser.add_argument('--tracks')
    parser.add_argument('--plot')
    parser.add_argument('--output')
    parser.add_argument('--bounds', nargs=4, type=float)
    args = parser.parse_args()
    geom = json.load(open(args.geometry))
    tracks = json.load(open(args.tracks)) if args.tracks else geom
    result = audit(geom, tracks)
    if args.output:
        json.dump(result, open(args.output, 'w'), indent=2)
    print(json.dumps(result, indent=2))
    if args.plot:
        plot(geom, tracks, args.plot, args.bounds)


if __name__ == '__main__':
    main()
