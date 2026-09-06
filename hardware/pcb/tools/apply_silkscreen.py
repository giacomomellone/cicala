"""Apply native underside artwork without changing placements or copper.

Run with system Python: apply_silkscreen.py input.kicad_pcb output.kicad_pcb.
Dense passive references stay on the assembly fab layer.
"""
import json
import math
import sys
import uuid
from pathlib import Path

import ksexp
from ksexp import Sym


NAMESPACE = uuid.UUID('43d8319c-81b4-4451-9a87-4e99fe66b0aa')
GROUP_NAME = 'Cicala identity and service legends'
OLD_LEGENDS = {'RECOVERY', '1 3V3', '2 GND', '3 EN', '4 BOOT', '5 TX',
               '6 RX', 'USB 5V', 'LiPo / NTC', '+', 'T', '-', 'CICALA A0'}
LABELS = [
    ('REV A0 / 4 LAYERS', 50, 25.7, 1.0, 0),
    ('PROTECTED LiPo + NTC', 50, 28.1, 0.8, 0),
    ('J4  1:3V3  2:GND  3:EN', 50, 31.0, 0.8, 0),
    ('4:BOOT  5:TX  6:RX', 50, 32.3, 0.8, 0),
    ('CELL AREA / NO COMPONENTS', 50, 8.0, 0.8, 0),
    ('CHARGE', 65.5, 12.0, 0.8, 270),
    ('3V3', 65.5, 22.0, 0.8, 270),
    ('BAT SENSE', 65.5, 31.0, 0.8, 270),
    ('J4 RECOVERY', 58.0, 43.4, 0.8, 0),
    ('USB 5V', 49.2, 46.1, 0.8, 0),
    ('J3 LiPo / NTC', 71.0, 47.0, 0.8, 0),
    ('+', 79.5, 42.0, 0.8, 0),
    ('NTC', 79.3, 40.5, 0.8, 0),
    ('1:+  2:NTC  3:-', 70.9, 50.7, 0.8, 0),
    ('J2 EPD', 35.5, 49.0, 0.8, 270),
]


def node(name, *values):
    return [Sym(name), *values]


def number(value):
    return Sym(f'{value:.6f}'.rstrip('0').rstrip('.') if value else '0')


def keyhole(outline, holes):
    """Bridge glyph counters with zero-width slits for KiCad's flat polygons."""
    points = outline[:-1]
    for hole in holes:
        ring = hole[:-1]
        oi, hi = min(((i, j) for i in range(len(points)) for j in range(len(ring))),
                     key=lambda ij: math.dist(points[ij[0]], ring[ij[1]]))
        loop = ring[hi:] + ring[:hi+1]
        points = points[:oi+1] + loop + points[oi:]
    return points


def apply(board):
    remove_ids = set()
    for group in list(ksexp.children(board, 'group')):
        if str(group[1]) == GROUP_NAME:
            remove_ids.update(str(i) for i in ksexp.child(group, 'members')[1:])
            board.remove(group)
    for item in list(board):
        if not isinstance(item, list):
            continue
        identity = ksexp.child(item, 'uuid')
        if ((identity and str(identity[1]) in remove_ids)
                or (str(item[0]) == 'gr_text' and str(item[1]) in OLD_LEGENDS)):
            board.remove(item)
    members = []

    def add(item, key):
        identity = str(uuid.uuid5(NAMESPACE, key))
        item.append(node('uuid', identity))
        board.append(item)
        members.append(identity)

    for label, x, y, size, angle in LABELS:
        add(node('gr_text', label, node('at', number(x), number(y), number(angle)),
                 node('layer', 'B.SilkS'),
                 node('effects', node('font', node('size', number(size), number(size)),
                                      node('thickness', number(0.15))),
                      node('justify', Sym('mirror')))), 'label:' + label)

    for x, y, dx, dy in ((32.5, 6.5, 1, 1), (67.5, 6.5, -1, 1),
                         (32.5, 35.5, 1, -1), (67.5, 35.5, -1, -1)):
        for end in ((x+2*dx, y), (x, y+2*dy)):
            add(node('gr_line', node('start', number(x), number(y)),
                     node('end', *map(number, end)),
                     node('stroke', node('width', number(0.15)), node('type', Sym('default'))),
                     node('layer', 'B.SilkS')), f'cell:{x},{y}:{end}')

    artwork = json.loads((Path(__file__).parent / 'assets/cicala-wordmark.json').read_text())
    for i, polygon in enumerate(artwork['polygons']):
        points = keyhole(polygon['outline'], polygon['holes'])
        add(node('gr_poly', node('pts', *[node('xy', number(50-x), number(19.5+y))
                                         for x, y in points]),
                 node('stroke', node('width', number(0)), node('type', Sym('default'))),
                 node('fill', Sym('solid')), node('layer', 'B.SilkS')), f'wordmark:{i}')

    board.append(node('group', GROUP_NAME, node('uuid', str(uuid.uuid5(NAMESPACE, 'group'))),
                      node('members', *members)))
    for fp in ksexp.children(board, 'footprint'):
        field = next(p for p in ksexp.children(fp, 'property') if str(p[1]) == 'Reference')
        if str(field[2]) in {'J2', 'J3', 'J4'}:
            ksexp.child(field, 'layer')[1] = 'B.Fab'
        on_fab=str(ksexp.child(field,'layer')[1])=='B.Fab'
        if on_fab:
            for item in list(ksexp.children(fp,'fp_text')):
                if str(item[2]) in {'${REFERENCE}','%R'}:
                    fp.remove(item)
        font = ksexp.child(ksexp.child(field, 'effects'), 'font')
        size,stroke=(0.6,0.1) if on_fab else (0.8,0.15)
        ksexp.child(font, 'size')[1:] = [number(size),number(size)]
        ksexp.child(font, 'thickness')[1] = number(stroke)
        at = ksexp.child(field, 'at')
        if on_fab and str(field[2]) not in {'D1','D2','D3'}:
            at[1:3]=[number(0),number(0)]
            fp_at=ksexp.child(fp,'at')
            body_angle=float(fp_at[3])%180 if len(fp_at)>3 else 0
            angle=270 if body_angle==90 or str(field[2]).startswith('TP') else 0
            at[3:]=[number(angle)]
        if len(at) > 3:
            angle=float(at[3])%180
            if angle==90 and str(ksexp.child(field,'layer')[1]).startswith('B.'):
                angle=270
            at[3] = number(angle)


if __name__ == '__main__':
    board = ksexp.load(sys.argv[1])
    apply(board)
    ksexp.save(sys.argv[2], board)
