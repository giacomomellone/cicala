"""Reconnect selected nets, allowing scoped rip-up of non-critical copper.

Existing pads, holes and the explicit seed remain hard obstacles. Crossed
ordinary tracks are removed and their nets are reported for reconnection.
"""
import json
import sys

import numpy as np
from shapely.geometry import Point

import route as maze
from copper_geometry import Copper, LAYERS, raster, segment_shape, via_shape
from finish_routes import connected_components
from route_fanout import rules, route_net


def prune_orphans(geom,tracks,nets):
    from finish_routes import physical_groups
    for net in nets:
        if net in {'GND','3V3'}:continue
        orphan = {s.wkb for group in physical_groups(net,geom,tracks)
                  if all(k in {'track','via','zone'} for s,ls,k in group)
                  for s,ls,k in group}
        tracks['segments']=[s for s in tracks['segments'] if s['net']!=net or segment_shape(s).wkb not in orphan]
        tracks['vias']=[v for v in tracks['vias'] if v['net']!=net or via_shape(v).wkb not in orphan]


def repair(geom, tracks, seed, nets):
    geom = {**geom,'filled_zones':[]}
    maze.GRID,maze.W,maze.H = 0.025,3120,1962
    affected = set()
    for net in nets:
        comps = connected_components(net,geom,tracks)
        if len(comps)<2:
            tracks['failed'].pop(net,None)
            continue
        print(net,'repair components',[c['keys'] for c in comps],flush=True)
        hard = Copper(geom)
        for s in tracks['segments']:
            if s['net']==net or s in seed['segments']:
                hard.add_segment(s)
        for v in tracks['vias']:
            if v['net']==net or v in seed['vias']:
                hard.add_via(v)
        width,size = rules(net)
        bt,bv = hard.masks(net,width,size,maze.GRID,maze.ORIGIN,(maze.H,maze.W))
        soft = [np.zeros((maze.H,maze.W),dtype=bool) for _ in LAYERS]
        for s in tracks['segments']:
            if s['net']!=net and s not in seed['segments']:
                raster(segment_shape(s).buffer(width/2+0.23),soft[LAYERS.index(s['layer'])],maze.GRID,maze.ORIGIN)
        for v in tracks['vias']:
            if v['net']!=net and v not in seed['vias']:
                for mask in soft:
                    raster(via_shape(v).buffer(width/2+0.23),mask,maze.GRID,maze.ORIGIN)
        costs = [(m.astype(np.uint8)*100).tobytes() for m in soft]
        for comp in comps:
            comp['cells'] = {(c,l) for c,l in comp['cells'] if not bt[l][c]}
        new_s,new_v = [],[]
        while len(comps)>1:
            options = []
            for i,a in enumerate(comps):
                if not a['cells']:continue
                ac,_=min(a['cells'])
                for j,b in enumerate(comps[i+1:],i+1):
                    if not b['cells']:continue
                    bc,_=min(b['cells'],key=lambda t:abs(t[0]%maze.W-ac%maze.W)+abs(t[0]//maze.W-ac//maze.W))
                    options.append((abs(bc%maze.W-ac%maze.W)+abs(bc//maze.W-ac//maze.W),i,j,bc))
            joined=False
            for _,i,j,tc in sorted(options):
                path=maze.astar(bt,bv,sorted(comps[i]['cells']),comps[j]['cells'],
                                (tc%maze.W,tc//maze.W),cost=costs,heuristic_weight=2,
                                max_expansions=900000)
                if not path:continue
                path=maze.merge_collinear(path)
                ss,vs=maze.path_to_items(path,net,width)
                for v in vs:v.update(size=size,drill=0.3)
                if not all(hard.clear(segment_shape(s),net,(s['layer'],)) for s in ss):
                    raise ValueError('Repair crossed a hard copper obstacle')
                if not all(hard.clear(via_shape(v),net,LAYERS,via=True) for v in vs):
                    raise ValueError('Repair via crossed a hard obstacle')
                new_s.extend(ss);new_v.extend(vs)
                comps[i]['cells'] |= comps[j]['cells'] | set(maze.densify(path))
                comps[i]['keys'] += comps[j]['keys']
                comps.pop(j)
                joined=True
                break
            if not joined:break
        paths=[(segment_shape(s),{s['layer']}) for s in new_s]+[(via_shape(v),set(LAYERS)) for v in new_v]
        def crossed(item,via=False):
            if item['net']==net:return False
            shape=via_shape(item) if via else segment_shape(item)
            layers=set(LAYERS) if via else {item['layer']}
            if any(ls & layers and p.distance(shape)<0.19999 for p,ls in paths):return True
            if via:
                hole=Point(item['at']).buffer(item['drill']/2)
                if any(via_shape(v).distance(hole)<0.24999 for v in new_v):return True
            return False
        removed_s=[s for s in tracks['segments'] if crossed(s)]
        removed_v=[v for v in tracks['vias'] if crossed(v,True)]
        if any(s in seed['segments'] for s in removed_s) or any(v in seed['vias'] for v in removed_v):
            raise ValueError('Repair attempted to rip up critical seed')
        affected.update(i['net'] for i in removed_s+removed_v)
        tracks['segments']=[s for s in tracks['segments'] if s not in removed_s]+new_s
        tracks['vias']=[v for v in tracks['vias'] if v not in removed_v]+new_v
        if len(comps)>1:tracks['failed'][net]=[c['keys'] for c in comps]
        else:
            tracks['failed'].pop(net,None)
            seed['segments'].extend(s for s in tracks['segments'] if s['net']==net and s not in seed['segments'])
            seed['vias'].extend(v for v in tracks['vias'] if v['net']==net and v not in seed['vias'])
        print(net,'added',len(new_s),len(new_v),'removed',len(removed_s),len(removed_v),flush=True)
    prune_orphans(geom,tracks,affected)
    for net in affected:
        if net in {'GND','3V3'}:
            tracks['plane_repair_needed']=True
            continue
        comps=connected_components(net,geom,tracks)
        if len(comps)>1:tracks['failed'][net]=[c['keys'] for c in comps]
    print('Affected nets:',sorted(affected),flush=True)
    return tracks


if __name__=='__main__':
    geom,tracks,seed=[json.load(open(p)) for p in sys.argv[1:4]]
    data=repair(geom,tracks,seed,sys.argv[5].split(','))
    json.dump(data,open(sys.argv[4],'w'),indent=2)
