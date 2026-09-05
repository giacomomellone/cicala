"""Write the Rev A netclasses and design rules into the project file.

pcbnew rewrites the project when it saves a board, so this runs after every
board-generation step.
"""
import json, collections, sys
p = sys.argv[1]
d = json.load(open(p), object_pairs_hook=collections.OrderedDict)
ns = d['net_settings']
base = ns['classes'][0]
base.update(clearance=0.20, track_width=0.2, via_diameter=0.6, via_drill=0.3)

def cls(name, **kw):
    c = collections.OrderedDict(base); c['name'] = name; c.update(kw); return c

ns['classes'] = [
    base,
    cls('Power', clearance=0.20, track_width=0.4, via_diameter=0.8, via_drill=0.4),
    cls('USB', clearance=0.2, track_width=0.29, via_diameter=0.6, via_drill=0.3,
        diff_pair_width=0.29, diff_pair_gap=0.29, diff_pair_via_gap=0.29),
    # 0.2 mm is the gap between adjacent pins of the panel's own 0.5 mm-pitch
    # connector, so nothing tighter is available on these rails; it is still
    # well above the IPC-2221 minimum for the +/-15 V they carry.
    cls('HV', clearance=0.2, track_width=0.25, via_diameter=0.6, via_drill=0.3),
]
POWER = ['GND', 'VBAT', 'VBAT_CELL', 'VSYS', '3V3', 'EPD_3V3', 'USB_VBUS']
USB = ['USB_DP', 'USB_DN']
HV = ['EPD_VGH', 'EPD_VGL', 'EPD_VSH1', 'EPD_VSH2', 'EPD_VSL', 'EPD_VCOM',
      'EPD_VDD', 'EPD_PUMP', 'EPD_SW', 'EPD_GDR', 'EPD_RESE']
ns['netclass_patterns'] = [collections.OrderedDict(netclass=c, pattern=n)
                           for c, nets in (('Power', POWER), ('USB', USB), ('HV', HV))
                           for n in nets]
r = d['board']['design_settings']['rules']
r.update(min_clearance=0.20, min_track_width=0.15, min_via_diameter=0.45,
         min_copper_edge_clearance=0.3, min_through_hole_diameter=0.2)
def strip(o):
    if isinstance(o, dict):
        o.pop('used_designators', None)
        for v in o.values(): strip(v)
    elif isinstance(o, list):
        for v in o: strip(v)
strip(d)
json.dump(d, open(p, 'w'), indent=2)
open(p, 'a').write('\n')
print('project: rules and %d netclasses applied' % len(ns['classes']))
