"""Measure Rev B USB paths and their adjacent ground references from filled copper."""
import argparse
from collections import defaultdict
import heapq
import json
import math
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'pcb'))
from shapely import union_all
from shapely.geometry import LineString,Point,Polygon
from copper_geometry import pad_shape
import ksexp


def heights(board):
    z=0.;out={}
    for layer in ksexp.children(ksexp.child(ksexp.child(board,'setup'),'stackup'),'layer'):
        t=ksexp.child(layer,'thickness')
        if t:
            t=float(t[1])
            if str(layer[1]).endswith('.Cu'):out[str(layer[1])]=z+t/2
            z+=t
    return out


def length(g,net,start,end,z):
    tracks=[s for s in g['segments']if s['net']==net];vias=[v for v in g['vias']if v['net']==net]
    pads=[p for p in g['pads']if p['net']==net];nodes=set();graph=defaultdict(list)
    def key(pt,layer):return(round(pt[0],6),round(pt[1],6),layer)
    def join(a,b,d):graph[a].append((b,d));graph[b].append((a,d))
    for s in tracks:nodes.update(key(p,s['layer'])for p in (s['start'],s['end']))
    for v in vias:
        ns=[key(v['at'],l)for l in z];nodes.update(ns)
        for a,b in zip(ns,ns[1:]):join(a,b,abs(z[a[2]]-z[b[2]]))
    for p in pads:nodes.update(key((p['x'],p['y']),l)for l in p['layers'])
    for s in tracks:
        line=LineString([s['start'],s['end']]);points=sorted((line.project(Point(p[:2])),p)for p in nodes if p[2]==s['layer']and line.distance(Point(p[:2]))<1e-5)
        for (da,a),(db,b)in zip(points,points[1:]):join(a,b,db-da)
    for v in vias:
        for layer in z:
            center=key(v['at'],layer)
            for n in nodes:
                if n[2]==layer and math.dist(center[:2],n[:2])<=v['size']/2+0.15:
                    join(center,n,math.dist(center[:2],n[:2]))
    for p in pads:
        shape=pad_shape(p).buffer(1e-5)
        for l in p['layers']:
            center=key((p['x'],p['y']),l)
            for n in nodes:
                if n[2]==l and shape.covers(Point(n[:2])):join(center,n,math.dist(center[:2],n[:2]))
    def terminal(identity):
        p=next(p for p in pads if (p['ref'],p['pad'])==identity);return key((p['x'],p['y']),p['layers'][0])
    source,target=terminal(start),terminal(end);queue=[(0,source)];best={source:0}
    while queue:
        dist,n=heapq.heappop(queue)
        if dist>best[n]:continue
        if n==target:return dist
        for other,cost in graph[n]:
            value=dist+cost
            if value<best.get(other,math.inf):best[other]=value;heapq.heappush(queue,(value,other))
    raise ValueError(f'USB path missing: {net} {start} {end}')


def audit(g,board):
    z=heights(board);rows={}
    for pol,usbpin,uin,uout,r,m in [('DP','6','3','4','R4','14'),('DN','7','1','6','R3','13')]:
        incoming=f'Net-(J1-D{"+"if pol=="DP"else"-"}-PadA{usbpin})';main=f'Net-({r}-Pad1)'
        paths=[(incoming,('J1','B'+usbpin),('U4',uin)),(main,('U4',uout),(r,'1')),('USB_'+pol,(r,'2'),('U1',m))]
        rows[pol]=[length(g,n,a,b,z)for n,a,b in paths]
    refs={l:union_all([Polygon(a['outline'],a['holes'])for a in g['filled_zones']if a['net']=='GND'and a['layer']==l])for l in ('In1.Cu','In2.Cu')}
    missing={};checked={}
    usb={'USB_DP','USB_DN','Net-(J1-D+-PadA6)','Net-(J1-D--PadA7)','Net-(R3-Pad1)','Net-(R4-Pad1)'}
    launches=union_all([Point(v['at']).buffer(.55)for v in g['vias']if v['net']in usb])
    for layer,reference in [('F.Cu','In1.Cu'),('B.Cu','In2.Cu')]:
        for n in usb:
            lines=union_all([LineString([s['start'],s['end']])for s in g['segments']if s['net']==n and s['layer']==layer]);path=lines.difference(launches);key=n+' / '+layer
            checked[key]=path.length;missing[key]=path.difference(refs[reference].buffer(1e-5)).length
    return {'path_sections_mm':rows,'total_path_mm':{p:sum(v)for p,v in rows.items()},'pair_skew_mm':abs(sum(rows['DP'])-sum(rows['DN'])),'reference_checked_mm':checked,'centreline_without_ground_mm':missing,'limitations':'Geometric audit only. Factory impedance confirmation and USB signal testing required.'}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('geometry',type=Path);p.add_argument('board',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    result=audit(json.loads(a.geometry.read_text()),ksexp.load(a.board));a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
