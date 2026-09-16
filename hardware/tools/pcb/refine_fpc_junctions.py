"""Replace two single-layer FPC via junctions with direct 45-degree copper."""
import json
import sys

from cleanup_dangling import pad_groups
from copper_geometry import Copper, segment_shape


def refine(geom, tracks):
    before = {n:pad_groups(n,geom,tracks) for n in ('EPD_VGL','PNL_CS')}
    replacements = {
        'EPD_VGL': (
            [(20.75,43.95),(20.75,45.15),(21.675,43.025)],
            [((21.675,43.025),(21.125,43.575),0.25),
             ((21.125,43.575),(20.75,43.95),0.15),
             ((20.75,43.95),(20.75,45.15),0.15)], (20.775,43.925)),
        'PNL_CS': (
            [(27,46.925),(26.25,45.15),(26.475,46.4)],
            [((26.25,45.15),(26.25,46.175),0.15),
             ((26.25,46.175),(26.45,46.375),0.15),
             ((26.45,46.375),(27,46.925),0.2)], (26.25,46.325)),
    }
    additions = []
    for net,(starts,new,at) in replacements.items():
        old = [s for s in tracks['segments'] if s['net']==net and s['layer']=='B.Cu'
               and tuple(s['start']) in starts]
        if len(old)!=3:
            raise ValueError(f'{net} junction changed; review before replacing it')
        tracks['segments'] = [s for s in tracks['segments'] if s not in old]
        tracks['vias'] = [v for v in tracks['vias'] if (v['net'],tuple(v['at']))!=(net,at)]
        additions.extend({'net':net,'layer':'B.Cu','start':a,'end':b,'width':w} for a,b,w in new)
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    for s in additions:
        if not copper.clear(segment_shape(s),s['net'],(s['layer'],)):
            raise ValueError(f'FPC refinement obstructed: {s}')
        copper.add_segment(s)
    tracks['segments'].extend(additions)
    if before != {n:pad_groups(n,geom,tracks) for n in before}:
        raise ValueError('FPC refinement changed pad connectivity')
    return tracks


if __name__=='__main__':
    data = refine(json.load(open(sys.argv[1])),json.load(open(sys.argv[2])))
    json.dump(data,open(sys.argv[3],'w'),indent=2)
