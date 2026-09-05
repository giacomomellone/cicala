"""Write the router's output into the board as segments and vias."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import json, sys, uuid
import ksexp
from ksexp import Sym

board_path, tracks_path = sys.argv[1], sys.argv[2]
board = ksexp.load(board_path)
data = json.load(open(tracks_path))

for name in ('segment', 'via', 'arc'):
    for n in list(ksexp.children(board, name)):
        board.remove(n)

VIA = {'GND': (0.8, 0.4), 'VBAT': (0.8, 0.4), 'VBAT_CELL': (0.8, 0.4),
       'VSYS': (0.8, 0.4), '3V3': (0.8, 0.4), 'EPD_3V3': (0.8, 0.4),
       'USB_VBUS': (0.8, 0.4)}

for s in data['segments']:
    board.append([Sym('segment'),
                  [Sym('start'), Sym(str(s['start'][0])), Sym(str(s['start'][1]))],
                  [Sym('end'), Sym(str(s['end'][0])), Sym(str(s['end'][1]))],
                  [Sym('width'), Sym(str(s['width']))],
                  [Sym('layer'), s['layer']],
                  [Sym('net'), s['net']],
                  [Sym('uuid'), str(uuid.uuid4())]])

for v in data['vias']:
    size, drill = VIA.get(v['net'], (0.6, 0.3))
    board.append([Sym('via'),
                  [Sym('at'), Sym(str(v['at'][0])), Sym(str(v['at'][1]))],
                  [Sym('size'), Sym(str(size))],
                  [Sym('drill'), Sym(str(drill))],
                  [Sym('layers'), 'F.Cu', 'B.Cu'],
                  [Sym('net'), v['net']],
                  [Sym('uuid'), str(uuid.uuid4())]])

ksexp.save(board_path, board)
print('applied %d segments and %d vias' % (len(data['segments']), len(data['vias'])))
