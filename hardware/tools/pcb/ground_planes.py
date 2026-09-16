"""Add local plane connections and checked ground stitching to routed copper."""
import json
import sys
from collections import defaultdict

from copper_geometry import Copper, LAYERS, via_shape
from finish_routes import connected_components
from route_fanout import plane_taps


def main():
    geom = json.load(open(sys.argv[1]))
    geom['filled_zones'] = []
    tracks = json.load(open(sys.argv[2]))
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    by_net = defaultdict(list)
    terminals = {}
    for p in geom['pads']:
        if p['net'] and p['layers']:
            by_net[p['net']].append(p)
    for net in ('GND', '3V3'):
        connected = {key for c in connected_components(net, geom, tracks) if c['has_via'] for key in c['keys']}
        for p in by_net[net]:
            if f"{p['ref']}.{p['pad']}" in connected:
                terminals[(p['ref'], p['uuid'])] = set()
    ss, vs = plane_taps(copper, by_net, terminals)
    tracks['segments'].extend(ss)
    tracks['vias'].extend(vs)
    count = 0
    for y in range(6, 52, 4):
        for x in range(7, 80, 4):
            v = {'net': 'GND', 'at': [x, y], 'size': 0.6, 'drill': 0.3}
            if copper.clear(via_shape(v), 'GND', LAYERS, via=True):
                tracks['vias'].append(v)
                copper.add_via(v)
                count += 1
    print('added', count, 'ground stitches on a 4 mm grid')
    json.dump(tracks, open(sys.argv[3], 'w'), indent=2)


if __name__ == '__main__':
    main()
