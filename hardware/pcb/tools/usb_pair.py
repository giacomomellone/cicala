"""Explicit USB channel geometry, including connector crossover and test pads.

Requires refined placement. Outputs a routing seed; all copper is checked
against the continuous pad geometry. Width/gap are 0.32/0.26 mm on F.Cu (JLC04121H-7628).
"""
import json
import math
import sys

from shapely.geometry import LineString

from copper_geometry import Copper, LAYERS, segment_shape, via_shape

DP_IN = 'Net-(J1-D+-PadA6)'
DN_IN = 'Net-(J1-D--PadA7)'
DP_LONG = 'Net-(R4-Pad1)'
DN_LONG = 'Net-(R3-Pad1)'
USB_NETS = {DP_IN, DN_IN, DP_LONG, DN_LONG, 'USB_DP', 'USB_DN'}
B_TO_IN2 = 0.2104 + (0.035 + 0.0152) / 2
F_TO_B = 1.2 - 0.035


def length(points):
    return sum(math.dist(a, b) for a, b in zip(points, points[1:]))


def require_pad(geom,ref,number,point,net):
    if not any(p['ref']==ref and p['pad']==number and p['net']==net
               and 'B.Cu' in p['layers'] and math.dist((p['x'],p['y']),point)<1e-5
               for p in geom['pads']):
        raise ValueError(f'Revise routing seed after changing {ref}.{number} ({net})')


def make_pair(geom,validate_placement=True):
    segments, vias, terminals = [], [], []
    paths = {}
    def run(net, points, layer='B.Cu', width=None):
        width = (0.32 if layer == 'F.Cu' else 0.29) if width is None else width
        points = [[round(x, 6), round(y, 6)] for x, y in points]
        paths.setdefault(net, []).append(points)
        segments.extend({'net': net, 'layer': layer, 'width': width, 'start': a, 'end': b}
                        for a, b in zip(points, points[1:]) if a != b)
    def via(net, point, size=0.45, drill=0.2):
        vias.append({'net': net, 'at': list(point), 'size': size, 'drill': drill})

    if validate_placement:
        for ref,pad,point,net in (
            ('J1','A6',(42.25,46.055),DP_IN),('J1','B6',(41.25,46.055),DP_IN),
            ('J1','A7',(41.75,46.055),DN_IN),('J1','B7',(42.75,46.055),DN_IN),
            ('U4','3',(41.05,43.5375),DP_IN),('U4','1',(42.95,43.5375),DN_IN),
            ('U4','4',(41.05,41.2625),DP_LONG),('U4','6',(42.95,41.2625),DN_LONG),
            ('R4','1',(28.2,8.175),DP_LONG),('R3','1',(25.8,8.175),DN_LONG),
            ('R4','2',(28.2,9.825),'USB_DP'),('R3','2',(25.8,9.825),'USB_DN'),
            ('TP12','1',(28.2,11.9),'USB_DP'),('TP13','1',(25.8,11.9),'USB_DN'),
            ('C2','1',(30.3,12.175),'USB_DP'),('C1','1',(23.7,12.175),'USB_DN'),
            ('U1','13',(26.23,14.25),'USB_DN'),('U1','14',(27.5,14.25),'USB_DP'),
        ):
            require_pad(geom,ref,pad,point,net)

    # B6/B7 enter the protection device symmetrically. A6 uses an inner
    # crossover; the 0.15 mm section of A7 clears the staggered crossover vias.
    dp_feed = [(41.25,46.055),(41.25,44.45),(41.05,44.25),(41.05,43.5375)]
    dn_feed = [(42.75,46.055),(42.75,44.45),(42.95,44.25),(42.95,43.5375)]
    run(DP_IN, dp_feed[:2], width=0.2)
    run(DP_IN, dp_feed[1:])
    run(DN_IN, dn_feed[:2], width=0.2)
    run(DN_IN, dn_feed[1:])
    dp_a = [(42.25,46.055),(42.25,46.9),(42.1,47.05),(42.1,47.35)]
    dp_b = [(41.25,46.055),(41.25,46.85),(41.0,47.1),(41.0,47.35)]
    dn_a = [(41.75,46.055),(41.75,46.95),(41.55,47.15),(41.55,47.9)]
    dn_b = [(42.75,46.055),(42.75,47.9)]
    run(DP_IN, dp_a, width=0.15)
    run(DP_IN, dp_b, width=0.15)
    via(DP_IN, (41.0,47.35))
    via(DP_IN, (42.1,47.35))
    run(DN_IN, dn_a, width=0.15)
    run(DN_IN, dn_b, width=0.15)
    dn_bridge = [(41.55,47.9),(41.75,48.1),(42.55,48.1),(42.75,47.9)]
    run(DN_IN, dn_bridge)
    # Equalise the two orientation-dependent connector branches with four
    # 45-degree bends in the inner crossover, clear of the shell drills.
    required = length(dn_a)+length(dn_b)+length(dn_bridge)-length(dp_a)-length(dp_b)-2*B_TO_IN2
    drop = (required - 0.5 - 0.6*math.sqrt(2))/2
    dp_bridge = [(41.0,47.35),(41.0,47.35+drop),(41.3,47.65+drop),
                 (41.8,47.65+drop),(42.1,47.35+drop),(42.1,47.35)]
    run(DP_IN, dp_bridge, 'In2.Cu')

    dp_launch = [(41.05,41.2625),(41.05,40.45),(41.6,39.9)]
    dn_launch = [(42.95,41.2625),(42.95,40.45),(42.4,39.9)]
    run(DP_LONG, dp_launch)
    run(DN_LONG, dn_launch)
    via(DP_LONG,(41.6,39.9))
    via(DN_LONG,(42.4,39.9))
    centre = LineString([(42,39.4),(42,29.5),(27.3,14.8),(27.3,5.5),
                         (26.3,4.5),(25.4,4.5),(24.4,5.5),(24.4,7.0)])
    dp_coupled = list(centre.offset_curve(-0.29, join_style=2).coords)
    dn_coupled = list(centre.offset_curve(0.29, join_style=2).coords)
    # In board coordinates the initial travel is toward decreasing Y.
    assert dp_coupled[0][0] < dn_coupled[0][0]
    run(DP_LONG,[(41.6,39.9),(41.71,39.79),dp_coupled[0]],'F.Cu')
    run(DN_LONG,[(42.4,39.9),(42.29,39.79),dn_coupled[0]],'F.Cu')
    run(DP_LONG, dp_coupled, 'F.Cu')
    run(DN_LONG, dn_coupled, 'F.Cu')
    run(DP_LONG,[dp_coupled[-1],(24.69,7.39),(24.8,7.5)],'F.Cu')
    run(DN_LONG,[dn_coupled[-1],(24.11,7.39),(24.0,7.5)],'F.Cu')
    via(DP_LONG,(24.8,7.5))
    via(DN_LONG,(24.0,7.5))
    run(DP_LONG,[(24.8,7.5),(25.0,7.3),(28.0,7.3),(28.2,7.5),(28.2,8.175)])
    run(DN_LONG,[(24.0,7.5),(24.0,7.6),(24.575,8.175),(25.8,8.175)])

    dp_end = [(28.2,9.825),(28.2,11.9),(28.2,12.9),(27.5,13.6),(27.5,14.25)]
    dn_end = [(25.8,9.825),(25.8,11.9),(25.8,12.9),(26.23,13.33),(26.23,14.25)]
    dp_total = sum(length(p) for p in paths[DP_LONG])+length(dp_end)
    dn_total = sum(length(p) for p in paths[DN_LONG])+length(dn_end)
    delta = dp_total-dn_total
    amplitude = abs(delta)/(2*(math.sqrt(2)-1))
    if amplitude > 0.65:
        raise ValueError(f'USB tuning requires excessive excursion: {amplitude}')
    if delta > 0:
        dn_end = [(25.8,9.825),(25.8,10.3),(25.8+amplitude,10.3+amplitude),
                  (25.8+amplitude,10.6+amplitude),(25.8,10.6+2*amplitude)] + dn_end[1:]
    else:
        dp_end = [(28.2,9.825),(28.2,10.3),(28.2-amplitude,10.3+amplitude),
                  (28.2-amplitude,10.6+amplitude),(28.2,10.6+2*amplitude)] + dp_end[1:]
    run('USB_DP', dp_end)
    run('USB_DN', dn_end)
    run('USB_DP',[(28.2,11.9),(30.025,11.9),(30.3,12.175)])
    run('USB_DN',[(25.8,11.9),(23.975,11.9),(23.7,12.175)])

    run('USB_VBUS',[(42,41.2625),(42,42.4)],width=0.2)
    via('USB_VBUS',(42,42.4))
    terminals.append({'ref':'U4','pad':'5','at':[42,42.4]})
    run('GND',[(42,43.5375),(42,44.65)],width=0.2)
    via('GND',(42,44.65))
    terminals.append({'ref':'U4','pad':'2','at':[42,44.65]})
    for point in ((40.55,39.9),(43.45,39.9),(23.1,7.5),(24.4,9.0)):
        via('GND',point,0.6,0.3)

    copper = Copper(geom)
    errors = []
    for s in segments:
        if not copper.clear(segment_shape(s),s['net'],(s['layer'],)):
            errors.append(s)
        copper.add_segment(s)
    for v in vias:
        if not copper.clear(via_shape(v),v['net'],LAYERS,via=True):
            errors.append(v)
        copper.add_via(v)
    if errors:
        print(json.dumps(errors,indent=2))
        raise ValueError(f'{len(errors)} USB geometry conflicts')
    return {'segments':segments,'vias':vias,'complete_nets':sorted(USB_NETS),
            'usb_reference_paths':[dp_coupled,dn_coupled],
            'terminals':terminals,'failed':{},
            'usb_metrics':{'dp_copper_mm':sum(length(p) for p in paths[DP_LONG])+length(dp_end)+length(dp_feed),
                           'dn_copper_mm':sum(length(p) for p in paths[DN_LONG])+length(dn_end)+length(dn_feed),
                           'connector_dp_branch_mm':length(dp_a)+length(dp_b)+length(dp_bridge)+2*B_TO_IN2,
                           'connector_dn_branch_mm':length(dn_a)+length(dn_b)+length(dn_bridge),
                           'connector_dp_barrel_mm':2*B_TO_IN2,
                           'main_channel_barrel_mm':2*F_TO_B,
                           'test_point_stub_mm':0.0,'coupled_width_mm':0.32,'coupled_gap_mm':0.26}}


if __name__ == '__main__':
    result = make_pair(json.load(open(sys.argv[1])))
    json.dump(result,open(sys.argv[2],'w'),indent=2)
    print(json.dumps(result['usb_metrics'],indent=2))
    if len(sys.argv)>3:
        if sys.argv[3]!='--update-routes':raise ValueError('Expected --update-routes input.json output.json')
        routes=json.load(open(sys.argv[4]))
        for key in ('segments','vias'):
            routes[key]=[i for i in routes[key] if i['net'] not in USB_NETS]+[i for i in result[key] if i['net'] in USB_NETS]
        routes['usb_metrics']=result['usb_metrics']
        routes['usb_reference_paths']=result['usb_reference_paths']
        from audit_copper import audit
        checked=audit(json.load(open(sys.argv[1])),routes)
        if checked['clearance_conflicts'] or checked['edge_conflicts']:
            raise ValueError('Updated USB channel conflicts with existing copper')
        json.dump(routes,open(sys.argv[5],'w'),indent=2)
