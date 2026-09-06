"""Route from checked fine-pitch escapes using continuous copper obstacles.

Usage: python route_fanout.py geometry.json tracks.json [passes]
Requires numpy and Shapely 2. Board and project files are never modified.
"""
import json
import math
import sys
import time
from collections import defaultdict

import numpy as np
from shapely.geometry import LineString, Point

import route as maze
from copper_geometry import Copper, LAYERS, pad_shape, raster, segment_shape, via_shape


def rules(net):
    if net in maze.USB:
        return 0.29, 0.6
    if net in maze.POWER:
        return 0.4, 0.6
    if net in maze.HV:
        return 0.25, 0.6
    return 0.2, 0.6


def cell_at(xy):
    x, y = maze.to_cell(*xy)
    return y * maze.W + x


def cell_xy(c):
    return maze.to_mm(c % maze.W, c // maze.W)


def pad_cells(p):
    mask = np.zeros((maze.H, maze.W), dtype=bool)
    raster(pad_shape(p), mask, maze.GRID, maze.ORIGIN)
    ys, xs = np.nonzero(mask)
    return {(int(y) * maze.W + int(x), l) for y, x in zip(ys, xs)
            for l, name in enumerate(LAYERS) if name in p['layers']}


def segment(net, a, b, width, layer='B.Cu'):
    return {'net': net, 'start': list(a), 'end': list(b), 'width': width, 'layer': layer}


def escape_candidates(p):
    """Exact pad-axis stubs followed by a 45-degree snap onto the maze grid."""
    if p['ref'] == 'U2' and p['pad'] == '10':
        yield [(p['x'], p['y']), (73.85, 14.8), (74.55, 15.5)]
    if p['ref'] in {'U2', 'U3'}:
        cy = p.get('fp_y',14.0 if p['ref'] == 'U2' else 20.0)
        direction = -1 if p['x'] < 72 else 1
        for factor in (2.25, 2.0, 2.5, 1.8, 3.0):
            for distance in (1.8, 2.1, 2.4, 2.7, 3.0):
                end = cell_xy(cell_at((p['x'] + direction * distance, cy + (p['y'] - cy) * factor)))
                bend = (end[0] - direction * abs(end[1] - p['y']), p['y'])
                yield [(p['x'], p['y']), bend, end]
    ang = math.radians(-p['angle'])
    axis = (math.cos(ang), math.sin(ang)) if p['w'] >= p['h'] else (-math.sin(ang), math.cos(ang))
    axis = tuple(round(v) for v in axis)
    half = max(p['w'], p['h']) / 2
    for extra in np.arange(0.55, 3.6, 0.15):
        for sign in (-1, 1):
            dx, dy = (sign * a for a in axis)
            x, y = p['x'] + dx * (half + extra), p['y'] + dy * (half + extra)
            end = cell_xy(cell_at((x, y)))
            if dx:
                bend = (end[0] - dx * abs(end[1] - p['y']), p['y'])
            else:
                bend = (p['x'], end[1] - dy * abs(end[0] - p['x']))
            yield [(p['x'], p['y']), bend, end]
    for extra in np.arange(0.3, 1.6, 0.05):
        for offset in (0.15, -0.15, 0.3, -0.3, 0.45, -0.45, 0.6, -0.6):
            for sign in (-1, 1):
                dx, dy = (sign * a for a in axis)
                end = cell_xy(cell_at((p['x'] + dx*(half+extra) - dy*offset,
                                      p['y'] + dy*(half+extra) + dx*offset)))
                bend = ((end[0] - dx*abs(end[1]-p['y']), p['y']) if dx else
                        (p['x'], end[1] - dy*abs(end[0]-p['x'])))
                yield [(p['x'], p['y']), bend, end]


def fanout(copper, by_net):
    segments, vias, terminals = [], [], {}
    physical = {}
    candidates = [p for ps in by_net.values() for p in ps
                  if p['ref'] in {'J1', 'J2', 'U2', 'U3'} and p['drill'] == 0
                  and min(p['w'], p['h']) <= 0.6]
    candidates.sort(key=lambda p: (min(p['w'], p['h']), p['ref'], p['x'], p['y']))
    for p in candidates:
        net = p['net']
        if any(other == net and label == 'track' and shape.intersects(pad_shape(p))
               for other, layers, shape, label in copper.objects):
            continue
        physical_key = (net, p['x'], p['y'], tuple(p['layers']))
        if physical_key in physical:
            terminals[(p['ref'], p['uuid'])] = physical[physical_key]
            continue
        width = 0.15 if min(p['w'], p['h']) <= 0.3 else 0.2
        for points in escape_candidates(p):
            ss = [segment(net, a, b, width) for a, b in zip(points, points[1:]) if math.dist(a, b) > 1e-6]
            v = {'net': net, 'at': list(points[-1]), 'size': 0.45, 'drill': 0.2}
            if not all(copper.clear(segment_shape(s), net, ('B.Cu',)) for s in ss):
                continue
            if not copper.clear(via_shape(v), net, LAYERS, via=True):
                continue
            for s in ss:
                copper.add_segment(s)
            copper.add_via(v)
            segments.extend(ss)
            vias.append(v)
            terminals[(p['ref'], p['uuid'])] = {(cell_at(v['at']), l) for l in range(3)}
            physical[physical_key] = terminals[(p['ref'], p['uuid'])]
            break
        else:
            print('escape failed:', p['ref'], p['pad'], net, flush=True)
    print(f'fanout: {len(vias)}/{len(candidates)} pads escaped', flush=True)
    return segments, vias, terminals


def plane_taps(copper, by_net, terminals):
    segments, vias = [], []
    for net in ('GND', '3V3'):
        seen = set()
        drilled = [pad_shape(p) for p in by_net[net] if p['drill']]
        for p in by_net[net]:
            key = (p['x'], p['y'], tuple(p['layers']))
            if key in seen or (p['ref'], p['uuid']) in terminals:
                continue
            seen.add(key)
            if p['drill'] or any(pad_shape(p).intersects(poly) for poly in drilled):
                continue
            side = next((name for name in ('B.Cu', 'F.Cu') if name in p['layers']), None)
            if side is None:
                continue
            width = min(0.3, min(p['w'], p['h']))
            for points in escape_candidates(p):
                ss = [segment(net, a, b, width, side) for a, b in zip(points, points[1:]) if math.dist(a, b) > 1e-6]
                v = {'net': net, 'at': list(points[-1]), 'size': 0.45, 'drill': 0.2}
                if not all(copper.clear(segment_shape(s), net, (side,)) for s in ss):
                    continue
                if not copper.clear(via_shape(v), net, LAYERS, via=True):
                    continue
                for s in ss:
                    copper.add_segment(s)
                copper.add_via(v)
                segments.extend(ss)
                vias.append(v)
                break
            else:
                print('plane tap failed:', p['ref'], p['pad'], net, flush=True)
    print(f'plane taps: {len(vias)}', flush=True)
    return segments, vias


def escape_stubs(copper, by_net, terminals):
    segments = []
    for net, pads in by_net.items():
        if net in {'GND', '3V3'}:
            continue
        width, _ = rules(net)
        for p in pads:
            key = (p['ref'], p['uuid'])
            if key in terminals or p['drill']:
                continue
            side = next((name for name in ('B.Cu', 'F.Cu') if name in p['layers']), None)
            if side is None:
                continue
            for points in escape_candidates(p):
                ss = [segment(net, a, b, width, side) for a, b in zip(points, points[1:]) if math.dist(a, b) > 1e-6]
                if not all(copper.clear(segment_shape(s), net, (side,)) for s in ss):
                    continue
                for s in ss:
                    copper.add_segment(s)
                segments.extend(ss)
                terminals[key] = {(cell_at(points[-1]), LAYERS.index(side))}
                break
    print(f'reserved {len(segments)} pad escape segments', flush=True)
    return segments


def components(ps, terminals):
    comps = []
    for p in ps:
        cells = terminals.get((p['ref'], p['uuid']), pad_cells(p))
        poly = pad_shape(p)
        joined = [c for c in comps if any(poly.intersects(shape) and set(p['layers']).intersection(layers)
                                          for shape, layers in c['pads'])]
        c = {'cells': cells.copy(), 'keys': [f"{p['ref']}.{p['pad']}"], 'pads': [(poly, p['layers'])]}
        for other in joined:
            c['cells'] |= other['cells']
            c['keys'] += other['keys']
            c['pads'] += other['pads']
            comps.remove(other)
        comps.append(c)
    return comps


def route_net(net, ps, copper, terminals, initial=None, heuristic_weight=1.0):
    width, via_size = rules(net)
    bt, bv = copper.masks(net, width, via_size, maze.GRID, maze.ORIGIN, (maze.H, maze.W))
    comps = components(ps, terminals) if initial is None else initial
    for comp in comps:
        comp['cells'] = {(c, l) for c, l in comp['cells'] if not bt[l][c]}
    segments, vias, failed_pairs = [], [], set()
    while len(comps) > 1:
        choices = []
        for i, a in enumerate(comps):
            if not a['cells']:
                continue
            ax = sum(c % maze.W for c, _ in a['cells']) / len(a['cells'])
            ay = sum(c // maze.W for c, _ in a['cells']) / len(a['cells'])
            for j in range(i + 1, len(comps)):
                if (i, j) in failed_pairs or not comps[j]['cells']:
                    continue
                c, _ = min(comps[j]['cells'], key=lambda t: abs(t[0] % maze.W - ax) + abs(t[0] // maze.W - ay))
                choices.append((abs(c % maze.W - ax) + abs(c // maze.W - ay), i, j, c))
        if not choices:
            break
        _, i, j, tc = min(choices)
        path = maze.astar(bt, bv, sorted(comps[i]['cells']), comps[j]['cells'], (tc % maze.W, tc // maze.W),
                          heuristic_weight=heuristic_weight)
        if path is None:
            failed_pairs.add((i, j))
            continue
        path = maze.simplify(bt, maze.merge_collinear(path))
        ss, vs = maze.path_to_items(path, net, width)
        for v in vs:
            v.update(size=via_size, drill=0.4 if via_size == 0.8 else 0.3)
        if not all(copper.clear(segment_shape(s), net, (s['layer'],)) for s in ss):
            raise RuntimeError(f'Continuous clearance audit rejected {net}')
        if not all(copper.clear(via_shape(v), net, LAYERS, via=True) for v in vs):
            raise RuntimeError(f'Continuous via audit rejected {net}')
        segments.extend(ss)
        vias.extend(vs)
        for s in ss:
            copper.add_segment(s)
        for v in vs:
            copper.add_via(v)
        comps[i]['cells'] |= comps[j]['cells'] | set(maze.densify(path))
        comps[i]['keys'] += comps[j]['keys']
        comps.pop(j)
        failed_pairs.clear()
        if vs:
            # The next branch must also respect this net's existing holes.
            bt, bv = copper.masks(net, width, via_size, maze.GRID, maze.ORIGIN, (maze.H, maze.W))
    return segments, vias, [c['keys'] for c in comps] if len(comps) > 1 else []


def main():
    geom = json.load(open(sys.argv[1]))
    by_net = defaultdict(list)
    for p in geom['pads']:
        if p['net'] and not p['net'].startswith('unconnected-') and p['layers']:
            by_net[p['net']].append(p)
    bias = set()
    best = None
    seed = json.load(open(sys.argv[4])) if len(sys.argv) > 4 else {}
    for attempt in range(int(sys.argv[3]) if len(sys.argv) > 3 else 1):
        copper = Copper(geom)
        ss, vs = list(seed.get('segments', [])), list(seed.get('vias', []))
        for s in ss:
            copper.add_segment(s)
        for v in vs:
            copper.add_via(v)
        active = {n: ps for n, ps in by_net.items() if n not in seed.get('complete_nets', [])}
        fs, fv, terminals = fanout(copper, active)
        ss.extend(fs)
        vs.extend(fv)
        for t in seed.get('terminals', []):
            for p in geom['pads']:
                if p['ref'] == t['ref'] and p['pad'] == t['pad']:
                    terminals[(p['ref'], p['uuid'])] = {(cell_at(t['at']), l) for l in range(3)}
        failed = {}
        order = sorted((n for n in active if n not in {'GND', '3V3'}),
                       key=lambda n: (n not in bias, n not in maze.USB, len(by_net[n])))
        for net in order:
            t0 = time.monotonic()
            from finish_routes import connected_components
            initial = connected_components(net, {**geom, 'filled_zones': []},
                                           {'segments':ss, 'vias':vs})
            s, v, f = route_net(net, by_net[net], copper, terminals, initial=initial)
            ss.extend(s)
            vs.extend(v)
            if f:
                failed[net] = f
            print(f'{net}: {len(s)} segments, {len(v)} vias, {len(f)} components, {time.monotonic()-t0:.1f}s', flush=True)
        data = {**seed, 'segments': ss, 'vias': vs, 'failed': failed}
        score = sum(len(cs)-1 for cs in failed.values())
        print(f'pass {attempt+1}: {len(ss)} segments, {len(vs)} vias, {score} failed connections', flush=True)
        if best is None or score < best[0]:
            best = score, data
            json.dump(data, open(sys.argv[2], 'w'), indent=2)
        if not failed:
            break
        bias.update(failed)


if __name__ == '__main__':
    main()
