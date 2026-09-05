"""Copper pours, written as S-expressions and filled by kicad-cli.

Layer plan: F.Cu and B.Cu carry signals with a ground pour in the space left
over, In1.Cu is a solid ground reference, In2.Cu is the 3V3 plane with a
ground island under the USB pair so that pair, which runs on B.Cu, is
referenced to ground rather than to the rail.
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
        [Sym('connect_pads'), [Sym('clearance'), Sym('0.3')]],
        [Sym('min_thickness'), Sym('0.2')],
        [Sym('filled_areas_thickness'), Sym('no')],
        [Sym('fill'), Sym('yes'),
         [Sym('thermal_gap'), Sym('0.3')],
         [Sym('thermal_bridge_width'), Sym('0.4')]],
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

ksexp.save(path, board)
print('zones written: 3 ground pours, the 3V3 plane and a ground island under the USB pair')
