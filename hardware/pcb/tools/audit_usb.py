"""Measure the USB routes in the actual board, including layer transitions.

Lengths exclude package-internal distances and the resistor bodies. These
geometric checks do not substitute for the manufacturer's impedance solver.
"""
import argparse
from collections import defaultdict
import heapq
import json
import math
from pathlib import Path

from shapely import union_all
from shapely.geometry import LineString, Point, Polygon

import ksexp
from usb_pair import DP_IN, DN_IN, DP_LONG, DN_LONG


def layer_centres(board):
    position = 0
    result = {}
    stack = ksexp.child(ksexp.child(board, 'setup'), 'stackup')
    for layer in ksexp.children(stack, 'layer'):
        thickness = ksexp.child(layer, 'thickness')
        if thickness is None:
            continue
        thickness = float(thickness[1])
        if str(layer[1]).endswith('.Cu'):
            result[str(layer[1])] = position + thickness / 2
        position += thickness
    if set(result) != {'F.Cu', 'In1.Cu', 'In2.Cu', 'B.Cu'}:
        raise ValueError('Expected the four-layer Rev A stack')
    return result


def route_length(geometry, net, start, end, heights):
    segments = [s for s in geometry['segments'] if s['net'] == net]
    vias = [v for v in geometry['vias'] if v['net'] == net]
    graph = defaultdict(list)

    def node(point, layer):
        return (round(point[0], 6), round(point[1], 6), layer)

    def link(a, b, distance):
        graph[a].append((b, distance))
        graph[b].append((a, distance))

    def pad_node(identity):
        pad = next(p for p in geometry['pads'] if (p['ref'], p['pad']) == identity)
        if pad['net'] != net:
            raise ValueError(f'{identity}: wrong USB net')
        return node((pad['x'], pad['y']), 'B.Cu')

    source, target = pad_node(start), pad_node(end)
    points = {node(p, s['layer']) for s in segments for p in (s['start'], s['end'])}
    points.update((source, target))
    for via in vias:
        for layer in heights:
            points.add(node(via['at'], layer))
        layers = sorted(heights, key=heights.get)
        for a, b in zip(layers, layers[1:]):
            link(node(via['at'], a), node(via['at'], b), heights[b] - heights[a])
    for segment in segments:
        line = LineString((segment['start'], segment['end']))
        on_line = sorted((line.project(Point(p[:2])), p) for p in points
                         if p[2] == segment['layer'] and line.distance(Point(p[:2])) < 1e-5)
        for (da, a), (db, b) in zip(on_line, on_line[1:]):
            link(a, b, db - da)
    queue = [(0.0, source)]
    best = {source: 0.0}
    while queue:
        distance, point = heapq.heappop(queue)
        if distance > best[point]:
            continue
        if point == target:
            return distance
        for other, length in graph[point]:
            new = distance + length
            if new < best.get(other, math.inf):
                best[other] = new
                heapq.heappush(queue, (new, other))
    raise ValueError(f'No routed path on {net}: {start} → {end}')


def audit(geometry, board):
    heights = layer_centres(board)
    lengths = {}
    for polarity, incoming, main, pad, protection_in, protection_out, resistor, module in (
        ('dp', DP_IN, DP_LONG, '6', '3', '4', 'R4', '14'),
        ('dn', DN_IN, DN_LONG, '7', '1', '6', 'R3', '13'),
    ):
        lengths[polarity] = sum(route_length(geometry, net, a, b, heights) for net, a, b in (
            (incoming, ('J1', 'B' + pad), ('U4', protection_in)),
            (main, ('U4', protection_out), (resistor, '1')),
            ('USB_' + polarity.upper(), (resistor, '2'), ('U1', module)),
        ))
        lengths[polarity + '_connector_branch'] = route_length(
            geometry, incoming, ('J1', 'A' + pad), ('J1', 'B' + pad), heights)
    ground = union_all([Polygon(z['outline'], z['holes']) for z in geometry['filled_zones']
                        if z['layer'] == 'In1.Cu' and z['net'] == 'GND'])
    uncovered, checked, exempt = {}, {}, {}
    for net in (DP_LONG, DN_LONG):
        segments = [s for s in geometry['segments'] if s['net'] == net and s['layer'] == 'F.Cu']
        if not segments or any(abs(s['width'] - 0.32) > 1e-6 for s in segments):
            raise ValueError(f'{net}: missing main run or changed 0.32 mm width')
        lines = union_all([LineString((s['start'], s['end'])) for s in segments])
        # Signal-via antipads necessarily interrupt the reference at the launch.
        # Limit the exemption to 0.55 mm around this pair's own transition vias.
        launches = union_all([Point(v['at']).buffer(0.55) for v in geometry['vias']
                              if v['net'] in (DP_LONG, DN_LONG)])
        path = lines.difference(launches)
        checked[net] = path.length
        exempt[net] = lines.length - path.length
        uncovered[net] = path.difference(ground.buffer(1e-5)).length
    mismatch = abs(lengths['dp'] - lengths['dn'])
    branch_mismatch = abs(lengths['dp_connector_branch'] - lengths['dn_connector_branch'])
    result = {
        'source': 'actual board segments, vias, pads and published stack construction',
        'route_lengths_mm': lengths,
        'main_mismatch_mm': mismatch,
        'connector_branch_mismatch_mm': branch_mismatch,
        'in1_ground_polygons': len(getattr(ground, 'geoms', [ground])) if not ground.is_empty else 0,
        'front_reference_checked_mm': checked,
        'front_via_launch_exemption_mm': exempt,
        'front_centreline_without_reference_mm': uncovered,
        'copper_layer_centres_mm': heights,
    }
    result['passed'] = (mismatch < 0.1 and branch_mismatch < 0.1
                        and result['in1_ground_polygons'] == 1
                        and all(length < 1e-4 for length in uncovered.values()))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry', type=Path)
    parser.add_argument('board', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = audit(json.loads(args.geometry.read_text()), ksexp.load(args.board))
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['passed'] else 1)
