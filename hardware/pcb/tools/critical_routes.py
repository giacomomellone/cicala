"""Reserve local regulator feedback, charger and panel-boost copper."""
import json
import sys

from copper_geometry import Copper, LAYERS, segment_shape, via_shape
from usb_pair import make_pair, require_pad


def make_seed(geom):
    data = make_pair(geom)
    from recovery_routes import escape
    service = escape(geom)
    data['segments'].extend(service['segments'])
    data['vias'].extend(service['vias'])
    from finish_power_taps import u5_return
    u5_return(geom,data)
    for ref,pad,point,net in (
        ('U3','4',(71.075,21),'Net-(U3-FB)'),('R14','2',(69.2,20.575),'Net-(U3-FB)'),
        ('R15','1',(71.175,18.4),'Net-(U3-FB)'),
        ('U3','7',(72.925,21),'Net-(U3-L2)'),('L2','2',(77.55,20),'Net-(U3-L2)'),
        ('U3','9',(72.925,22),'Net-(U3-L1)'),('L2','1',(77.55,23),'Net-(U3-L1)'),
        ('U3','1',(71.075,22.5),'VSYS'),('C8','1',(69.2,22.25),'VSYS'),
        ('U3','6',(72.925,20.5),'3V3'),('C9','1',(74.5,19.925),'3V3'),
        ('Q1','2',(18.8625,34.35),'EPD_RESE'),('R20','1',(17.35,33.825),'EPD_RESE'),
        ('Q1','1',(18.8625,35.65),'EPD_GDR'),('R19','1',(18.6,37.825),'EPD_GDR'),
        ('Q1','3',(20.6375,35),'EPD_SW'),('L1','2',(15.5,35),'EPD_SW'),
        ('D3','2',(22.35,35.5),'EPD_SW'),('C16','2',(30.45,39),'EPD_SW'),
        ('C15','1',(12.05,38.3),'EPD_3V3'),('L1','1',(12.5,35),'EPD_3V3'),
    ):
        require_pad(geom,ref,pad,point,net)
    def run(net, points, width=0.4):
        data['segments'].extend({'net':net, 'layer':'B.Cu', 'width':width, 'start':a, 'end':b}
                                for a,b in zip(points,points[1:]))
    run('Net-(U3-FB)',[(71.075,21),(70.5,21),(70.075,20.575),(69.2,20.575)],0.2)
    run('Net-(U3-FB)',[(71.175,18.4),(70.3,19.275),(70.3,20.8)],0.2)
    run('Net-(U3-L2)',[(72.925,21),(74.0,21)],0.15)
    run('Net-(U3-L2)',[(74.0,21),(75.4,21),(76.4,20),(77.55,20)])
    run('Net-(U3-L1)',[(72.925,22),(74.0,22)],0.15)
    run('Net-(U3-L1)',[(74.0,22),(75.8,22),(76.8,23),(77.55,23)])
    run('VSYS',[(69.2,22.25),(70.1,22.25)])
    run('VSYS',[(70.1,22.25),(70.35,22.5),(71.075,22.5),(72.925,22.5)],0.2)
    run('3V3',[(72.925,20.5),(73.65,20.5)],0.15)
    run('3V3',[(73.65,20.5),(74.225,19.925),(74.5,19.925)])
    run('GND',[(72.925,21.5),(73.6,21.5)],0.15)
    data['vias'].append({'net':'GND','at':[73.6,21.5],'size':0.45,'drill':0.2})
    run('GND',[(74.5,18.375),(75.7,18.375)])
    data['vias'].append({'net':'GND','at':[75.7,18.375],'size':0.6,'drill':0.3})
    data['complete_nets'] += ['Net-(U3-L1)','Net-(U3-L2)']
    run('Net-(U6-CT)',[(14.1375,40.05),(15.775,40.05),(16.05,39.775)],0.2)
    data['complete_nets'].append('Net-(U6-CT)')
    run('3V3',[(11.8625,41.95),(11.8625,42.95),(11.2,43.6125),(11.2,43.625)])
    run('EPD_3V3',[(14.1375,41.95),(14.85,41.95),(15.975,43.075)])
    run('EPD_3V3',[(15.975,43.075),(17.5,43.075),(18.325,42.25),(18.325,41.4)])
    for net,points in (
        ('3V3',[(11.2,43.625),(10.25,43.625)]),
        ('3V3',[(16.05,38.225),(17.1,38.225)]),
        ('GND',[(11.2,45.175),(10.25,45.175)]),
        ('GND',[(11.8625,41),(10.8,41)]),
        ('GND',[(15.975,44.625),(14.9,44.625)]),
    ):
        run(net,points)
        data['vias'].append({'net':net,'at':points[-1],'size':0.45,'drill':0.2})
    run('EPD_RESE',[(18.8625,34.35),(18.3375,33.825),(17.35,33.825)],0.25)
    run('EPD_GDR',[(18.8625,35.65),(18.8625,37.5625),(18.6,37.825)],0.25)
    run('EPD_SW',[(15.5,35),(15.5,33.6),(16.15,32.95),(20,32.95),
                  (20.6375,33.5875),(20.6375,35),(21.1375,35.5),(22.35,35.5)])
    run('EPD_SW',[(22.35,35.5),(22.35,36.4),(23.15,37.2),(29.7,37.2),
                  (30.45,37.95),(30.45,39)])
    run('EPD_3V3',[(12.05,38.3),(12.05,37.3),(12.5,36.85),(12.5,35)])
    run('VBAT',[(70.9,14.4),(69.65,14.4)],0.15)
    run('VBAT',[(69.65,14.4),(67.8,14.4),(67.5,14.1),(67.5,13.925),
                 (67.8,13.625),(68.85,13.625)])
    run('VSYS',[(70.9,14.8),(70.6,14.8),(70.0,15.4)],0.15)
    run('VSYS',[(70.0,15.4),(69.2,15.4)])
    run('USB_VBUS',[(73.1,14.8),(73.9,14.8),(74.1,15.0)],0.15)
    run('USB_VBUS',[(74.1,15.0),(74.325,15.225),(74.9,15.225)])
    for net,points in (
        ('GND',[(70.9,13.2),(70.5,13.2),(70.3,13),(70.3,11.3),(70.75,10.85)]),
        ('CHG_ENABLE',[(70.9,13.6),(70.15,13.6),(69.95,13.4),(69.95,11.3),(69.9,11.25),(69.9,10.85)]),
        ('CHG_STAT2',[(70.9,14),(70,14),(69.6,13.6),(69.6,11.5),(69.05,10.95)]),
        ('CHG_STAT1',[(73.1,14.4),(76,14.4),(76.4,14.8),(76.4,15.3)]),
        ('Net-(U2-ISET)',[(73.1,14),(74.4,14),(74.6,13.8)]),
    ):
        run(net,points,0.15)
        data['vias'].append({'net':net,'at':points[-1],'size':0.45,'drill':0.2})
    run('CHG_ILIM_VSET',[(73.1,13.6),(73.9,13.6),(74.7,12.8),
                          (75.2,12.8),(75.675,13.275),(75.675,13.6)],0.15)
    data['complete_nets'].append('CHG_ILIM_VSET')
    data['complete_nets'] += ['Net-(U3-FB)', 'EPD_SW']
    copper = Copper(geom)
    conflicts = []
    for s in data['segments']:
        if not copper.clear(segment_shape(s),s['net'],(s['layer'],)):
            conflicts.append({**s, 'conflicts':[(n,k,round(segment_shape(s).distance(p),5))
                for n,ls,p,k in copper.objects if n != s['net'] and s['layer'] in ls
                and segment_shape(s).distance(p) < 0.19999]})
        copper.add_segment(s)
    for v in data['vias']:
        if not copper.clear(via_shape(v),v['net'],LAYERS,via=True):
            conflicts.append(v)
        copper.add_via(v)
    if conflicts:
        print(json.dumps(conflicts,indent=2))
        raise ValueError(f'{len(conflicts)} critical copper conflicts')
    return data


if __name__ == '__main__':
    data = make_seed(json.load(open(sys.argv[1])))
    json.dump(data,open(sys.argv[2],'w'),indent=2)
