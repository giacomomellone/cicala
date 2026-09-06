"""Write the router's output into the board as segments and vias."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import collections
import json, sys, uuid
import ksexp
from ksexp import Sym

board_path, tracks_path = sys.argv[1], sys.argv[2]
board = ksexp.load(board_path)
data = json.load(open(tracks_path))
namespace = uuid.UUID('bf044dcb-47ad-43e7-849a-7f0f7cb011a9')
occurrences = collections.Counter()


def identity(kind, value):
    key = json.dumps([kind,value],sort_keys=True,separators=(',',':'))
    index = occurrences[key]
    occurrences[key] += 1
    return str(uuid.uuid5(namespace,f'{key}:{index}'))


def point(p):
    return [round(float(v),6) for v in p]

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
                  [Sym('uuid'), identity('segment',[s['net'],s['layer'],round(s['width'],6),
                      sorted([point(s['start']),point(s['end'])])])]])

for v in data['vias']:
    size, drill = VIA.get(v['net'], (0.6, 0.3))
    size, drill = v.get('size', size), v.get('drill', drill)
    board.append([Sym('via'),
                  [Sym('at'), Sym(str(v['at'][0])), Sym(str(v['at'][1]))],
                  [Sym('size'), Sym(str(size))],
                  [Sym('drill'), Sym(str(drill))],
                  [Sym('layers'), 'F.Cu', 'B.Cu'],
                  [Sym('net'), v['net']],
                  [Sym('uuid'), identity('via',[v['net'],point(v['at']),size,drill])]])

ksexp.save(board_path, board)
print('applied %d segments and %d vias' % (len(data['segments']), len(data['vias'])))
