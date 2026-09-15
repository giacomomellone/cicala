"""Connect retained islands to a reference plane; never remove pour polygons.

Geometry MUST come from this exact track set after KiCad zone refill. Refill
and re-audit the output: adding holes can change plane connectivity elsewhere.
"""
import argparse
import json
import math
import sys

import numpy as np
from shapely import union_all
from shapely.geometry import Point

import route as maze
from copper_geometry import Copper, LAYERS, raster, segment_shape, via_shape
from finish_routes import physical_groups


def connect(geom, tracks, pads_only=False, radius=2.0):
    def signature(data):
        def point(p):
            return tuple(round(x,6) for x in p)
        return (sorted((s['net'],s['layer'],round(s['width'],6),
                        tuple(sorted((point(s['start']),point(s['end']))))) for s in data['segments']),
                sorted((v['net'],point(v['at']),round(v['size'],6),round(v['drill'],6))
                       for v in data['vias']))
    def equal(a, b):
        if isinstance(a, (tuple,list)) and isinstance(b, (tuple,list)):
            return len(a)==len(b) and all(equal(x,y) for x,y in zip(a,b))
        if isinstance(a,(int,float)) and isinstance(b,(int,float)):
            return math.isclose(a,b,rel_tol=0,abs_tol=0.00000101)
        return a==b
    # KiCad quantizes coordinates to integer nanometres on serialization.
    if not equal(signature(geom),signature(tracks)):
        raise ValueError('Stale filled geometry: write these tracks, refill, and export again')
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    report = {}
    for net, reference in (('GND','In1.Cu'),('3V3','In2.Cu')):
        groups = physical_groups(net,geom,tracks)
        if not groups:
            continue
        main = max(groups,key=lambda g:sum(s.area for s,ls,k in g if reference in ls))
        reference_shape = union_all([s for s,ls,k in main if reference in ls])
        added, failed = 0, []
        for group in sorted((g for g in groups if g is not main),
                            key=lambda g:-sum(s.area for s,ls,k in g)):
            if pads_only and not any(k not in {'track','via','zone'} for s,ls,k in group):
                continue
            layers = {l:union_all([s for s,ls,k in group if l in ls]) for l in LAYERS}
            total = union_all(list(layers.values()))
            # Test a centre and a fine grid before constructing a maze search.
            point = total.representative_point()
            x0,y0,x1,y1 = total.buffer(0.2).bounds
            candidates = [(point.x,point.y)]
            candidates += [(float(x),float(y)) for x in np.arange(x0,x1+0.01,0.1)
                           for y in np.arange(y0,y1+0.01,0.1)]
            done = False
            for at in candidates:
                v = {'net':net,'at':at,'size':0.45,'drill':0.2}
                poly = via_shape(v)
                if (poly.intersects(total) and reference_shape.covers(Point(at))
                        and copper.clear(poly,net,LAYERS,via=True)):
                    tracks['vias'].append(v)
                    copper.add_via(v)
                    added += 1
                    done = True
                    break
            if done:
                continue
            # A bounded local escape may reach a via site outside a pad field.
            width, size = 0.2, 0.45
            bt,bv = copper.masks(net,width,size,maze.GRID,maze.ORIGIN,(maze.H,maze.W))
            x0,y0,x1,y1 = total.buffer(radius).bounds
            ix0,iy0 = maze.to_cell(x0,y0)
            ix1,iy1 = maze.to_cell(x1,y1)
            region = np.zeros((maze.H,maze.W),dtype=bool)
            region[max(0,iy0):min(maze.H,iy1+1),max(0,ix0):min(maze.W,ix1+1)] = True
            sources,targets = set(),set()
            plane = np.zeros_like(region)
            raster(reference_shape,plane,maze.GRID,maze.ORIGIN)
            via_free = region & plane
            for layer in bv:
                via_free &= ~np.frombuffer(layer,dtype=bool).reshape(region.shape)
            ys,xs = np.nonzero(via_free)
            targets = {(int(y)*maze.W+int(x),l) for y,x in zip(ys,xs) for l in range(3)}
            for l,name in enumerate(LAYERS):
                mask = np.zeros_like(region)
                raster(layers[name],mask,maze.GRID,maze.ORIGIN)
                blocked = np.frombuffer(bt[l],dtype=bool).reshape(region.shape).copy() | ~region
                ys,xs = np.nonzero(mask & ~blocked)
                sources.update((int(y)*maze.W+int(x),l) for y,x in zip(ys,xs))
                bt[l] = blocked.tobytes()
            if sources and targets:
                sc,_ = min(sources)
                tc,_ = min(targets,key=lambda t:abs(t[0]%maze.W-sc%maze.W)+abs(t[0]//maze.W-sc//maze.W))
                path = maze.astar(bt,bv,sorted(sources),targets,(tc%maze.W,tc//maze.W))
                if path:
                    path = maze.merge_collinear(path)
                    ss,vs = maze.path_to_items(path,net,width)
                    for v in vs:
                        v.update(size=size,drill=0.2)
                    cell,layer = path[-1]
                    v = {'net':net,'at':maze.to_mm(cell%maze.W,cell//maze.W),'size':size,'drill':0.2}
                    vs.append(v)
                    if (all(copper.clear(segment_shape(s),net,(s['layer'],)) for s in ss)
                            and all(copper.clear(via_shape(v),net,LAYERS,via=True) for v in vs)):
                        tracks['segments'].extend(ss)
                        tracks['vias'].extend(vs)
                        for s in ss:
                            copper.add_segment(s)
                        for v in vs:
                            copper.add_via(v)
                        added += 1
                        continue
            failed.append({'area_mm2':round(total.area,4),'at':[point.x,point.y],
                           'pads':[k for s,ls,k in group if k not in ('track','via','zone')]})
        report[net] = {'connected_components':added,'unresolved_components':failed}
        print(net,'connected',added,'unresolved',len(failed),flush=True)
    tracks['island_stitch_report'] = report
    return tracks


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry')
    parser.add_argument('tracks')
    parser.add_argument('output')
    parser.add_argument('--pads-only', action='store_true')
    parser.add_argument('--radius', type=float, default=2.0)
    parser.add_argument('--fine-grid', action='store_true')
    args = parser.parse_args()
    if args.fine_grid:
        maze.GRID, maze.W, maze.H = 0.025, 3120, 1962
    data = connect(json.load(open(args.geometry)),json.load(open(args.tracks)),
                   pads_only=args.pads_only,radius=args.radius)
    json.dump(data,open(args.output,'w'),indent=2)
