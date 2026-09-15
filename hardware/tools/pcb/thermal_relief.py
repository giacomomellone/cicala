"""Orient SW1's thermal and use U5's explicit via-backed ground return.

U5 requires the copper in finish_power_taps.u5_return before this is applied.
No pour outline, minimum gap, spoke width or DRC severity is reduced.
"""
import sys

import ksexp
from ksexp import Sym


def apply(board, angle):
    for fp in ksexp.children(board,'footprint'):
        ref = next(str(p[2]) for p in ksexp.children(fp,'property') if str(p[1])=='Reference')
        if ref not in {'U5','SW1'}:
            continue
        for pad in ksexp.children(fp,'pad'):
            net = ksexp.child(pad,'net')
            if net is None or str(net[-1])!='GND':
                continue
            if ref=='SW1' and float(ksexp.child(pad,'at')[1])>0:
                continue
            if ref=='U5':
                def is_ground(item):
                    net=ksexp.child(item,'net')
                    return net is not None and str(net[-1])=='GND'
                def point(item,name):
                    return tuple(round(float(x),6) for x in ksexp.child(item,name)[1:3])
                segments=[s for s in ksexp.children(board,'segment') if is_ground(s)]
                expected={((70.8625,30.0),(70.7125,29.85)),
                          ((70.7125,29.85),(69.65,29.85))}
                present={(point(s,'start'),point(s,'end')) for s in segments}
                vias=[v for v in ksexp.children(board,'via') if is_ground(v)
                      and point(v,'at')==(69.65,29.85)]
                if not expected.issubset(present) or not vias:
                    raise ValueError('Route U5 to its dedicated ground via before removing automatic thermals')
                connection=ksexp.child(pad,'zone_connect')
                if connection is None:
                    pad.append([Sym('zone_connect'),Sym('0')])
                else:
                    connection[1]=Sym('0')
                continue
            field = ksexp.child(pad,'thermal_bridge_angle')
            if field is None:
                pad.append([Sym('thermal_bridge_angle'),Sym(str(angle))])
            else:
                field[1]=Sym(str(angle))


if __name__=='__main__':
    board=ksexp.load(sys.argv[1])
    apply(board,float(sys.argv[3]) if len(sys.argv)>3 else 45)
    ksexp.save(sys.argv[2],board)
