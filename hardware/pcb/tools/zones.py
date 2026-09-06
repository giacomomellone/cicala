"""Copper pours, written as S-expressions and filled by kicad-cli.

Layer plan: F.Cu and B.Cu carry signals with a ground pour in the space left
over, In1.Cu is a solid ground reference, In2.Cu is the 3V3 plane with a
ground reference regions beneath the short B.Cu USB escapes. The coupled
F.Cu run references In1.Cu.
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sys, uuid
import ksexp
from ksexp import Sym

path = sys.argv[1]
board = ksexp.load(path)

# KiCad 10 references a net by name, both on pads and on zones.


for z in list(ksexp.children(board, 'zone')):
    name = ksexp.child(z, 'name')
    if name is not None and str(name[1]) == 'TC2030_CONTACT_KEEPOUT':
        board.remove(z)
        continue
    if name is not None and 'KEEPOUT' in str(name[1]):
        continue
    board.remove(z)

def zone(name, net, layers, pts, priority):
    node = [Sym('zone'), [Sym('net'), net]]
    if len(layers) == 1:
        node.append([Sym('layer'), layers[0]])
    else:
        node.append([Sym('layers')] + list(layers))
    node += [
        [Sym('uuid'), str(uuid.uuid4())],
        [Sym('name'), name],
        [Sym('hatch'), Sym('edge'), Sym('0.5')],
        [Sym('priority'), Sym(str(priority))],
        [Sym('connect_pads'), [Sym('clearance'), Sym('0.2')]],
        [Sym('min_thickness'), Sym('0.2')],
        [Sym('filled_areas_thickness'), Sym('no')],
        [Sym('fill'), Sym('yes'),
         [Sym('island_removal_mode'), Sym('1')],
         [Sym('thermal_gap'), Sym('0.2')],
         [Sym('thermal_bridge_width'), Sym('0.2')]],
        [Sym('polygon'), [Sym('pts')] +
         [[Sym('xy'), Sym(str(x)), Sym(str(y))] for x, y in pts]],
    ]
    board.append(node)

X0, Y0, X1, Y1 = 3.0, 3.5, 81.0, 52.5
FULL = [(X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1)]
zone('GND_F', 'GND', ['F.Cu'], FULL, 10)
zone('GND_IN1', 'GND', ['In1.Cu'], FULL, 10)
zone('GND_B', 'GND', ['B.Cu'], FULL, 10)
zone('RAIL_IN2', '3V3', ['In2.Cu'], FULL, 5)
zone('GND_IN2_USB', 'GND', ['In2.Cu'],
     [(28.0, 36.0), (52.0, 36.0), (52.0, Y1), (28.0, Y1)], 20)
zone('GND_IN2_USB_MODULE', 'GND', ['In2.Cu'],
     [(22.0, Y0), (31.0, Y0), (31.0, 15.5), (22.0, 15.5)], 20)

from recovery_keepout import polygon as contact_keepout
j4 = next(fp for fp in ksexp.children(board,'footprint')
          if any(str(p[1])=='Reference' and str(p[2])=='J4'
                 for p in ksexp.children(fp,'property')))
at = ksexp.child(j4,'at')
assert [float(x) for x in at[1:3]] == [61,46.5] and (len(at)<4 or float(at[3])==0), \
    'Update the TC2030 keepout transform after moving its reviewed anchor'
shape = contact_keepout([{'ref':'J4','pad':str(i),'x':x,'y':y,'w':0.7874}
                        for i,(x,y) in enumerate([(59.73,45.865),(59.73,47.135),
                            (61,45.865),(61,47.135),(62.27,45.865),(62.27,47.135)],1)])
board.append([Sym('zone'),[Sym('layer'),'B.Cu'],[Sym('uuid'),str(uuid.uuid4())],
    [Sym('name'),'TC2030_CONTACT_KEEPOUT'],[Sym('hatch'),Sym('edge'),Sym('0.5')],
    [Sym('connect_pads'),[Sym('clearance'),Sym('0')]],
    [Sym('min_thickness'),Sym('0.15')],
    [Sym('keepout')]+[[Sym(n),Sym('allowed' if n in ('pads','footprints') else 'not_allowed')]
                      for n in ('tracks','vias','pads','copperpour','footprints')],
    [Sym('polygon'),[Sym('pts')]+[[Sym('xy'),Sym(f'{x:.6f}'),Sym(f'{y:.6f}')]
                                for x,y in list(shape.exterior.coords)[:-1]]]])

ksexp.save(path, board)
print('zones written: 3 ground pours, the 3V3 plane and 2 USB escape references; islands retained')
