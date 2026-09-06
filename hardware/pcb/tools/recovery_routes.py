"""Rework only the TC2030 pad field for the connector's 0.020 inch rule.

The four displaced signal connections must then pass finish_routes and DRC.
This step keeps the reviewed connector anchor and pin mapping unchanged.
"""
import json
import sys

from shapely.geometry import box

from copper_geometry import Copper, LAYERS, segment_shape, via_shape
from usb_pair import require_pad


def escape(geom):
    for pin, net, x, y in [('1','3V3',59.73,45.865), ('2','GND',59.73,47.135),
                          ('3','CHIP_EN',61,45.865), ('4','BOOT_IO0',61,47.135),
                          ('5','UART0_TX',62.27,45.865), ('6','UART0_RX',62.27,47.135)]:
        require_pad(geom,'J4',pin,(x,y),net)
    segments, vias = [], []
    for net, points in (
        ('GND',[(59.73,47.135),(59.73,48.25)]),
        ('CHIP_EN',[(61,45.865),(61,44.7)]),
        ('UART0_TX',[(62.27,45.865),(62.27,44.7)]),
        ('BOOT_IO0',[(61,47.135),(61,47.45),(60.4,48.05)]),
        ('UART0_RX',[(62.27,47.135),(62.27,47.73),(61.95,48.05)]),
    ):
        segments.extend({'net':net,'layer':'B.Cu','width':0.2,'start':a,'end':b}
                        for a,b in zip(points,points[1:]))
        if net != 'UART0_RX':
            vias.append({'net':net,'at':points[-1],'size':0.45,'drill':0.2})
    return {'segments':segments,'vias':vias}


def rework(geom, tracks):
    nets = {'CHIP_EN','BOOT_IO0','UART0_TX','UART0_RX'}
    field = box(58.5,44.4,63.3,48.1)
    old_vias = {('CHIP_EN',(60.225,45.05)),('BOOT_IO0',(58.875,47.6)),
                ('UART0_TX',(63,46.475))}
    tracks['segments'] = [s for s in tracks['segments'] if not
                          (s['net'] in nets and s['layer']=='B.Cu'
                           and segment_shape(s).intersects(field))]
    tracks['vias'] = [v for v in tracks['vias'] if (v['net'],tuple(v['at'])) not in old_vias]
    copper = Copper(geom)
    for s in tracks['segments']:
        copper.add_segment(s)
    for v in tracks['vias']:
        copper.add_via(v)
    seed = escape(geom)
    for s in seed['segments']:
        if not copper.clear(segment_shape(s),s['net'],(s['layer'],)):
            raise ValueError(f'Recovery segment is obstructed: {s}')
        copper.add_segment(s)
    for v in seed['vias']:
        if not copper.clear(via_shape(v),v['net'],LAYERS,via=True):
            raise ValueError(f'Recovery via is obstructed: {v}')
        copper.add_via(v)
    tracks['segments'].extend(seed['segments'])
    tracks['vias'].extend(seed['vias'])
    tracks['recovery_escape'] = seed
    for net in nets:
        tracks['failed'][net] = ['Reconnect displaced recovery escape']
    return tracks


if __name__ == '__main__':
    data = rework(json.load(open(sys.argv[1])),json.load(open(sys.argv[2])))
    json.dump(data,open(sys.argv[3],'w'),indent=2)
