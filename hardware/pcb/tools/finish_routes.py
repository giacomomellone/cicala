"""Connect remaining copper components on a finer grid without ripping up routes.

Usage: python finish_routes.py geometry.json tracks.json output.json [net,...]
The geometry must describe the candidate placement. Requires numpy and Shapely.
"""
import json
import sys
from collections import defaultdict

import numpy as np
from shapely import STRtree
from shapely.geometry import Polygon

import route as maze
from copper_geometry import Copper, LAYERS, pad_shape, raster, segment_shape, via_shape
from route_fanout import route_net, escape_candidates, segment, rules


def physical_groups(net, geom, tracks):
    objects = []
    for p in geom['pads']:
        if p['net'] == net and p['layers']:
            objects.append((pad_shape(p), set(p['layers']), f"{p['ref']}.{p['pad']}"))
    for s in tracks['segments']:
        if s['net'] == net:
            objects.append((segment_shape(s), {s['layer']}, 'track'))
    for v in tracks['vias']:
        if v['net'] == net:
            objects.append((via_shape(v), set(LAYERS) | {'In1.Cu'}, 'via'))
    for z in geom.get('filled_zones', []):
        if z['net'] == net:
            objects.append((Polygon(z['outline'], z['holes']), {z['layer']}, 'zone'))
    parent = list(range(len(objects)))
    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    tree = STRtree([o[0] for o in objects])
    for i, (shape, layers, key) in enumerate(objects):
        for j in tree.query(shape, predicate='dwithin', distance=0.00001):
            if layers.intersection(objects[j][1]):
                parent[root(j)] = root(i)
    groups = defaultdict(list)
    for i, item in enumerate(objects):
        groups[root(i)].append(item)
    return list(groups.values())


def connected_components(net, geom, tracks):
    components = []
    for items in physical_groups(net, geom, tracks):
        cells, keys = set(), []
        for shape, layers, key in items:
            if key == 'zone':
                for x, y in shape.exterior.coords:
                    ix, iy = maze.to_cell(x, y)
                    if 0 <= ix < maze.W and 0 <= iy < maze.H:
                        cells.update((iy*maze.W+ix, l) for l, name in enumerate(LAYERS) if name in layers)
                continue
            mask = np.zeros((maze.H, maze.W), dtype=bool)
            raster(shape, mask, maze.GRID, maze.ORIGIN)
            ys, xs = np.nonzero(mask)
            cells.update((int(y)*maze.W+int(x), l) for x, y in zip(xs, ys)
                         for l, name in enumerate(LAYERS) if name in layers)
            if key not in {'track', 'via', 'zone'}:
                keys.append(key)
        components.append({'cells': cells, 'keys': keys,
                           'has_via': any(key == 'via' or 'In1.Cu' in layers for _, layers, key in items)})
    return components


def main():
    geom = json.load(open(sys.argv[1]))
    tracks = json.load(open(sys.argv[2]))
    nets = sys.argv[4].split(',') if len(sys.argv) > 4 else list(tracks['failed'])
    maze.GRID = 0.025
    maze.W, maze.H = 3120, 1962
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    for net in nets:
        comps = connected_components(net, geom, tracks)
        print(net, 'initial components', [c['keys'] for c in comps], flush=True)
        ss, vs, failed = route_net(net, [], copper, {}, initial=comps, heuristic_weight=2.0)
        tracks['segments'].extend(ss)
        tracks['vias'].extend(vs)
        if failed:
            for group in failed:
                if len(group) != 1:
                    continue
                for p in geom['pads']:
                    if p['net'] != net or f"{p['ref']}.{p['pad']}" != group[0]:
                        continue
                    width = min(0.2, rules(net)[0])
                    layer = next(name for name in LAYERS if name in p['layers'])
                    for points in escape_candidates(p):
                        if sum(np.linalg.norm(np.array(b)-a) for a, b in zip(points, points[1:])) > 2.0:
                            continue
                        necks = [segment(net, a, b, width, layer) for a, b in zip(points, points[1:]) if a != b]
                        via = {'net': net, 'at': list(points[-1]), 'size': 0.45, 'drill': 0.2}
                        if not all(copper.clear(segment_shape(s), net, (layer,)) for s in necks):
                            continue
                        if not copper.clear(via_shape(via), net, LAYERS, via=True):
                            continue
                        tracks['segments'].extend(necks)
                        tracks['vias'].append(via)
                        for s in necks:
                            copper.add_segment(s)
                        copper.add_via(via)
                        print('local escape:', group[0], points, flush=True)
                        break
            comps = connected_components(net, geom, tracks)
            ss, vs, failed = route_net(net, [], copper, {}, initial=comps, heuristic_weight=2.0)
            tracks['segments'].extend(ss)
            tracks['vias'].extend(vs)
        if failed:
            tracks['failed'][net] = failed
        else:
            tracks['failed'].pop(net, None)
        print(net, 'added', len(ss), 'segments and', len(vs), 'vias; residue', failed, flush=True)
        json.dump(tracks, open(sys.argv[3], 'w'), indent=2)


if __name__ == '__main__':
    main()
