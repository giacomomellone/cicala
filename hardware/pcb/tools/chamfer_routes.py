"""Replace isolated right-angle bends with checked 45-degree chamfers.

Critical seeds, pad entries, via junctions and copper branches are unchanged.
Refill and rerun DRC after this geometric edit.
"""
import json
import math
import sys
from collections import defaultdict

from shapely.geometry import Point

from copper_geometry import Copper,pad_shape,segment_shape,via_shape
from usb_pair import USB_NETS


def chamfer(geom,tracks,seed):
    tracks={**tracks,'segments':list(tracks['segments']),'vias':list(tracks['vias'])}
    copper=Copper(geom)
    for s in tracks['segments']:copper.add_segment(s)
    for v in tracks['vias']:copper.add_via(v)
    nodes=defaultdict(list)
    for s in tracks['segments']:
        for at in (s['start'],s['end']):nodes[(s['net'],s['layer'],tuple(at))].append(s)
    changed=0
    for (net,layer,at),edges in nodes.items():
        if net in USB_NETS or len(edges)!=2:continue
        a,b=edges
        if any(s in seed['segments'] or s not in tracks['segments'] for s in edges):continue
        if a['width']!=b['width']:continue
        point=Point(at)
        if any(point.distance(pad_shape(p))<0.5 for p in geom['pads'] if layer in p['layers']):continue
        if any(point.distance(via_shape(v))<0.3 for v in tracks['vias']):continue
        if any(s['net']==net and s['layer']==layer and s not in edges and
               segment_shape(s).intersects(point) for s in tracks['segments']):continue
        far=[s['end'] if tuple(s['start'])==at else s['start'] for s in edges]
        length=[math.dist(p,at) for p in far]
        if min(length)<0.45:continue
        vec=[((p[0]-at[0])/d,(p[1]-at[1])/d) for p,d in zip(far,length)]
        if abs(sum(x*y for x,y in zip(*vec)))>1e-6:continue
        cut=min(0.3,min(length)/3)
        ends=[[round(at[0]+v[0]*cut,6),round(at[1]+v[1]*cut,6)] for v in vec]
        replacement=[{**a,'start':far[0],'end':ends[0]},
                     {**a,'start':ends[0],'end':ends[1]},
                     {**b,'start':ends[1],'end':far[1]}]
        if not all(copper.clear(segment_shape(s),net,(layer,)) for s in replacement):continue
        tracks['segments'].remove(a);tracks['segments'].remove(b)
        tracks['segments'].extend(replacement)
        changed+=1
    tracks['chamfered_corners']=changed
    print('chamfered',changed,'isolated right-angle corners')
    return tracks


if __name__=='__main__':
    geom,tracks,seed=[json.load(open(p)) for p in sys.argv[1:4]]
    json.dump(chamfer(geom,tracks,seed),open(sys.argv[4],'w'),indent=2)
