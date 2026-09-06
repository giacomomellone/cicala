"""Measure filled reference-plane continuity and ground-stitch coverage."""
import json
import math
import sys

import numpy as np
from shapely import union_all
from shapely.geometry import LineString,Point,Polygon


def audit(geom,tracks):
    ground=union_all([Polygon(z['outline'],z['holes']) for z in geom['filled_zones']
                      if z['layer']=='In1.Cu' and z['net']=='GND'])
    count=0 if ground.is_empty else len(getattr(ground,'geoms',[ground]))
    paths=tracks.get('usb_reference_paths',[])
    uncovered=[LineString(p).difference(ground.buffer(0.00001)).length for p in paths]
    stitches=[v['at'] for v in tracks['vias'] if v['net']=='GND']
    stitches += [[p['x'],p['y']] for p in geom['pads'] if p['drill'] and p['net']=='GND']
    maximum,worst=None,None
    if stitches and not ground.is_empty:
        maximum=0
        points=np.array(stitches)
        x0,y0,x1,y1=ground.bounds
        for x in np.arange(x0,x1,0.5):
            for y in np.arange(y0,y1,0.5):
                if not ground.covers(Point(x,y)):continue
                distance=float(np.sqrt(((points-[x,y])**2).sum(axis=1)).min())
                if distance>maximum:maximum,worst=distance,[float(x),float(y)]
    return {'in1_ground_polygons':count,
            'usb_coupled_paths_checked':len(paths),
            'usb_centreline_without_in1_reference_mm':uncovered,
            'max_ground_stitch_distance_mm_on_0_5_mm_samples':
                round(maximum,4) if maximum is not None else None,
            'worst_stitch_sample':worst}


if __name__=='__main__':
    result=audit(json.load(open(sys.argv[1])),json.load(open(sys.argv[2])))
    json.dump(result,open(sys.argv[3],'w'),indent=2)
    print(json.dumps(result,indent=2))
