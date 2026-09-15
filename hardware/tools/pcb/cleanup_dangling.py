"""Remove DRC-identified stubs only when pad connectivity is unchanged.

Use an exact, refilled geometry export and its DRC report. Refill and check
again after cleanup; a shortened branch can expose another dangling segment.
"""
import json
import sys

from finish_routes import physical_groups
from copper_geometry import pad_shape


def pad_groups(net, geom, tracks):
    identities = {}
    for index,p in enumerate(geom['pads']):
        if p['net']==net:
            key = (f"{p['ref']}.{p['pad']}",pad_shape(p).wkb)
            identities.setdefault(key,set()).add(p.get('uuid',str(index)))
    return {frozenset(identity for s,ls,k in group
                      for identity in identities.get((k,s.wkb),()))
            for group in physical_groups(net,geom,tracks)} - {frozenset()}


def clean(geom, tracks, drc):
    # Keep exact serialized coordinates and object identities from the board.
    tracks = {**tracks,'segments':list(geom['segments']),'vias':list(geom['vias'])}
    removed, retained = [], []
    for violation in drc['violations']:
        kind = {'track_dangling':'segments','via_dangling':'vias'}.get(violation['type'])
        if kind is None:
            continue
        identity = violation['items'][0]['uuid']
        item = next((x for x in tracks[kind] if x['uuid']==identity),None)
        if item is None:
            raise ValueError('DRC does not match this geometry export')
        before = pad_groups(item['net'],geom,tracks)
        candidate = {**tracks,kind:[x for x in tracks[kind] if x is not item]}
        if pad_groups(item['net'],geom,candidate) != before:
            retained.append(item)
            continue
        tracks = candidate
        removed.append(item)
    tracks['dangling_cleanup'] = {'removed':removed,'retained':retained}
    print('Removed',len(removed),'redundant items; retained',len(retained),'junctions')
    return tracks


if __name__=='__main__':
    data = clean(*[json.load(open(p)) for p in sys.argv[1:4]])
    json.dump(data,open(sys.argv[4],'w'),indent=2)
