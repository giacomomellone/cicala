"""Add bounded off-grid GND stitching at the worst measured coverage gaps.

The nominal 4 mm grid loses sites to components and tracks. This pass targets
at most 4 mm nearest-via/PTH distance on 0.5 mm samples; refill and re-audit.
"""
import json
import sys

from audit_reference import audit
from copper_geometry import Copper, LAYERS, via_shape


def improve(geom,tracks):
    copper=Copper(geom)
    for s in tracks['segments']:copper.add_segment(s)
    for v in tracks['vias']:copper.add_via(v)
    added=[]
    offsets=sorted(((dx/4,dy/4) for dx in range(-8,9) for dy in range(-8,9)),
                   key=lambda p:p[0]*p[0]+p[1]*p[1])
    for _ in range(12):
        report=audit(geom,tracks)
        if report['max_ground_stitch_distance_mm_on_0_5_mm_samples']<=4:
            break
        x,y=report['worst_stitch_sample']
        chosen=None
        for dx,dy in offsets:
            via={'net':'GND','at':[round(x+dx,4),round(y+dy,4)],'size':0.6,'drill':0.3}
            if copper.clear(via_shape(via),'GND',LAYERS,via=True):
                chosen=via
                break
        if chosen is None:
            break
        tracks['vias'].append(chosen)
        copper.add_via(chosen)
        added.append(chosen)
    tracks['coverage_stitches']=added
    tracks['coverage_report_before_refill']=audit(geom,tracks)
    print('Added',len(added),'off-grid ground stitches;',tracks['coverage_report_before_refill'])
    return tracks


if __name__=='__main__':
    data=improve(json.load(open(sys.argv[1])),json.load(open(sys.argv[2])))
    json.dump(data,open(sys.argv[3],'w'),indent=2)
