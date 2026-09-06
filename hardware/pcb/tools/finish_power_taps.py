"""Make room for R9's local 3V3 tap without moving its component anchor.

A short CHG_STAT1 dogleg makes a 0.45 mm via site with clearance margin;
the 3V3 branch supplies a 10 kilohm status pull-up, not the regulator load.
"""
import json
import sys

from copper_geometry import Copper, LAYERS, segment_shape, via_shape
from usb_pair import require_pad


def u5_return(geom, tracks):
    require_pad(geom,'U5','2',(70.8625,30),'GND')
    points=[(70.8625,30),(70.7125,29.85),(69.65,29.85)]
    segments=[{'net':'GND','layer':'B.Cu','width':0.2,'start':a,'end':b}
              for a,b in zip(points,points[1:])]
    via={'net':'GND','at':[69.65,29.85],'size':0.45,'drill':0.2}
    copper=Copper(geom)
    for s in tracks['segments']:copper.add_segment(s)
    for v in tracks['vias']:copper.add_via(v)
    if not all(copper.clear(segment_shape(s),'GND',('B.Cu',)) for s in segments):
        raise ValueError('U5 ground return obstructed')
    if not copper.clear(via_shape(via),'GND',LAYERS,via=True):
        raise ValueError('U5 ground via obstructed')
    tracks['segments'].extend(segments)
    tracks['vias'].append(via)
    return tracks


def finish(geom, tracks):
    require_pad(geom,'R9','1',(69.5,9.925),'3V3')
    old = [s for s in tracks['segments'] if s['net']=='CHG_STAT1'
           and s['layer']=='B.Cu' and s['start'] in [[72.9,9.125],[70.425,9.125]]]
    if len(old)!=2:
        raise ValueError('R9 corridor changed; review its local escape before applying')
    tracks['segments'] = [s for s in tracks['segments'] if s not in old]
    added = []
    for net,points in (
        ('CHG_STAT1',[(72.9,9.125),(71.0,9.125),(70.925,9.05),(70.325,9.05),(69.9,8.625)]),
        ('3V3',[(69.5,9.925),(70.25,9.925),(70.55,9.625),(70.75,9.625)]),
    ):
        added.extend({'net':net,'layer':'B.Cu','width':0.2,'start':a,'end':b}
                     for a,b in zip(points,points[1:]))
    via = {'net':'3V3','at':[70.75,9.625],'size':0.45,'drill':0.2}
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    for s in added:
        if not copper.clear(segment_shape(s),s['net'],(s['layer'],)):
            raise ValueError(f'Power tap obstructed: {s}')
        copper.add_segment(s)
    if not copper.clear(via_shape(via),'3V3',LAYERS,via=True):
        conflicts = [(n,k,ls,list(p.centroid.coords),via_shape(via).distance(p)) for n,ls,p,k in copper.objects
                     if n!='3V3' and via_shape(via).distance(p)<0.19999]
        raise ValueError(f'R9 via site obstructed: {conflicts}')
    tracks['segments'].extend(added)
    tracks['vias'].append(via)
    return tracks


if __name__=='__main__':
    operation = u5_return if '--u5-only' in sys.argv else finish
    data = operation(json.load(open(sys.argv[1])),json.load(open(sys.argv[2])))
    json.dump(data,open(sys.argv[3],'w'),indent=2)
