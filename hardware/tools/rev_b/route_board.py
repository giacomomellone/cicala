"""Route remaining prototype nets around reviewed copper; operates on JSON exports.

The native KiCad file remains the authoritative layout. This helper writes a
candidate routing JSON which must pass native DRC and connectivity audits.
"""
import argparse
from collections import defaultdict
import json
import math
from pathlib import Path
import sys
import time

import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'pcb'))
import route as maze
import route_fanout as fan
from copper_geometry import Copper, LAYERS, pad_shape, segment_shape, via_shape
from finish_routes import connected_components


def configure(grid=.075):
    maze.GRID = grid
    maze.ORIGIN = (3., 2.)
    maze.W, maze.H = math.ceil(103/grid)+1, math.ceil(62/grid)+1
    maze.XS = (np.arange(maze.W)*grid+3)[None,:]
    maze.YS = (np.arange(maze.H)*grid+2)[:,None]


def escapes(p):
    point = (p['x'], p['y'])
    if p['ref'] in {'U2','U3'}:
        cx,cy = p['fp_x'],p['fp_y']
        sign = -1 if p['x'] < cx else 1
        if abs(p['x']-cx) > .6:
            for factor in (2,2.5,3,3.5):
                for distance in (1.8,2.1,2.4,2.7,3,3.3):
                    end=fan.cell_xy(fan.cell_at((p['x']+sign*distance,cy+(p['y']-cy)*factor)))
                    bend=(end[0]-sign*abs(end[1]-p['y']),p['y'])
                    yield [point,bend,end]
    angle=math.radians(-p['angle'])
    axis=(math.cos(angle),math.sin(angle)) if p['w']>=p['h'] else (-math.sin(angle),math.cos(angle))
    half=max(p['w'],p['h'])/2
    for extra in np.arange(.4,4.1,.15):
        for sign in (-1,1):
            dx,dy=(sign*round(a) for a in axis)
            end=fan.cell_xy(fan.cell_at((p['x']+dx*(half+extra),p['y']+dy*(half+extra))))
            bend=(end[0]-dx*abs(end[1]-p['y']),p['y']) if dx else (p['x'],end[1]-dy*abs(end[0]-p['x']))
            yield [point,bend,end]
    # Circular test contacts and recovery contacts also need transverse exits.
    for extra in np.arange(.55, 3.6, .15):
        for dx,dy in ((1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)):
            end=fan.cell_xy(fan.cell_at((p['x']+dx*extra,p['y']+dy*extra)))
            delta=(end[0]-p['x'],end[1]-p['y'])
            diagonal=min(abs(delta[0]),abs(delta[1]))
            bend=(p['x']+math.copysign(diagonal,delta[0]),
                  p['y']+math.copysign(diagonal,delta[1]))
            yield [point,bend,end]


def add_escape(copper,p,data):
    net=p['net'];width=.15 if min(p['w'],p['h'])<.4 else .2
    for points in escapes(p):
        ss=[fan.segment(net,a,b,width,'F.Cu') for a,b in zip(points,points[1:]) if math.dist(a,b)>1e-6]
        via={'net':net,'at':list(points[-1]),'size':.6,'drill':.3}
        if not all(copper.clear(segment_shape(s),net,('F.Cu',)) for s in ss):continue
        if not copper.clear(via_shape(via),net,LAYERS,via=True):continue
        for s in ss:copper.add_segment(s)
        copper.add_via(via);data['segments'].extend(ss);data['vias'].append(via)
        return True
    return False


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry',type=Path);parser.add_argument('output',type=Path)
    parser.add_argument('--seed',type=Path);parser.add_argument('--grid',type=float,default=.075)
    parser.add_argument('--nets',help='Comma-separated subset; existing copper is preserved')
    parser.add_argument('--use-filled-zones', action='store_true', help='Use filled plane connectivity for final power repairs')
    parser.add_argument('--priority', type=Path, help='Prior pass whose incomplete nets route first')
    parser.add_argument('--local-inner-crossings', action='store_true', help='Allow reviewed local crossings on In1.Cu during a repair pass')
    parser.add_argument('--branch-width', type=float, help='Width for selected short branch connections; requires --nets')
    args=parser.parse_args();configure(args.grid)
    if args.branch_width is not None:
        if not args.nets or args.branch_width < .15:
            parser.error('Branch routing needs explicit nets and at least 0.15 mm copper')
        fan.rules=lambda net:(args.branch_width,.6)
    if args.local_inner_crossings:
        import copper_geometry, finish_routes
        global LAYERS
        LAYERS=(*LAYERS,'In1.Cu')
        copper_geometry.LAYERS=fan.LAYERS=finish_routes.LAYERS=LAYERS
        maze.NL=len(LAYERS);maze.LAYER_NAME=dict(enumerate(LAYERS))
    geom=json.loads(args.geometry.read_text());data=json.loads(args.seed.read_text()) if args.seed else {'segments':[],'vias':[]}
    by=defaultdict(list);fps={f['ref']:f for f in geom['footprints']}
    for p in geom['pads']:
        p['fp_x']=fps[p['ref']]['x'];p['fp_y']=fps[p['ref']]['y']
        if p['net'] and not p['net'].startswith('unconnected-') and p['layers']:by[p['net']].append(p)
    copper=Copper(geom,edge_clearance=.5)
    for s in data['segments']:copper.add_segment(s)
    for v in data['vias']:copper.add_via(v)
    if not args.nets:
        candidates=[p for ps in by.values() for p in ps if not p['drill'] and
                    (p['ref'].startswith('U') or p['ref'] in {'J2','J4','J1'} or p['net'] in {'GND','3V3'})]
        candidates.sort(key=lambda p:(p['ref']!='J4',p['ref']!='U1',min(p['w'],p['h']),p['ref'],p['x'],p['y']))
        seen=set()
        for p in candidates:
            key=(p['net'],p['x'],p['y'])
            if key in seen:continue
            seen.add(key)
            if any(n==p['net'] and label=='track' and shape.intersects(pad_shape(p)) for n,ls,shape,label in copper.objects):continue
            if not add_escape(copper,p,data):print('escape pending',p['ref'],p['pad'],p['net'],flush=True)
        args.output.write_text(json.dumps(data,indent=2))
        print('Escapes:',len(data['segments']),'segments',len(data['vias']),'vias',flush=True)
    selected=args.nets.split(',') if args.nets else [n for n in by if n not in {'GND','3V3'} and n not in data.get('complete_nets',[])]
    priority={'Net-(U3-FB)','Net-(U3-L1)','Net-(U3-L2)','EPD_RESE','EPD_GDR','EPD_SW','EPD_PUMP'}
    bias = set(json.loads(args.priority.read_text()).get('failed', {})) if args.priority else set()
    def span(net):
        ps=by[net]
        return max(p['x'] for p in ps)-min(p['x'] for p in ps)+max(p['y'] for p in ps)-min(p['y'] for p in ps)
    order=sorted(selected,key=lambda n:(n not in priority,n not in bias,-span(n)))
    failed={}
    for net in order:
        start=time.monotonic();initial=connected_components(net,geom if args.use_filled_zones else {**geom,'filled_zones':[]},data)
        if args.nets:
            for comp in initial:
                if comp['has_via']:continue
                keys=set(comp['keys'])
                for p in by[net]:
                    if f"{p['ref']}.{p['pad']}" in keys and add_escape(copper,p,data):break
            initial=connected_components(net,geom if args.use_filled_zones else {**geom,'filled_zones':[]},data)
        try:
            ss,vs,errors=fan.route_net(net,by[net],copper,{},initial=initial,heuristic_weight=1.5)
        except RuntimeError as error:
            # A discretized path may fail the continuous geometry check. Discard
            # that net's tentative copper before trying subsequent nets.
            ss,vs,errors=[],[],[str(error)]
            copper=Copper(geom,edge_clearance=.5)
            for s in data['segments']:copper.add_segment(s)
            for v in data['vias']:copper.add_via(v)
        data['segments'].extend(ss);data['vias'].extend(vs)
        if errors:failed[net]=errors
        data['failed']=failed;args.output.write_text(json.dumps(data,indent=2))
        print(net,len(ss),'segments',len(vs),'vias',len(errors),'groups',round(time.monotonic()-start,1),'s',flush=True)
    print('Pending:',json.dumps(failed),flush=True)


if __name__=='__main__':main()
