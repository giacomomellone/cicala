"""Apply Rev B identity, functional boundaries and service legends to native KiCad.

Only silkscreen graphics and the obsolete drawing-layer status note are changed.
The committed native artwork can be regenerated without KiCad or website fonts.
"""
import argparse
import json
from pathlib import Path
import re
import sys
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'pcb'))
import ksexp as k
from apply_silkscreen import keyhole, node, number

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / 'hardware/rev_b/pcb/cicala_rev_b.kicad_pcb'
ART = Path(__file__).resolve().parents[1] / 'pcb/assets'
NAMESPACE = uuid.UUID('22b35b21-40f0-454a-b938-5fcb2ec45a77')
GROUP = 'Rev B identity and service guide'
MIN_STROKE = .15

# Label aliases are checked against native nets before any artwork is changed.
PROBES = {
    'TP1': ('BOOT_IO0', 'BOOT', 49, 59, .8),
    'TP2': ('CHIP_EN', 'EN', 53.5, 50.1, .8),
    'TP3': ('GND', 'GND', 61.5, 56.5, .8),
    'TP4': ('UART0_TX', '4 TX', 16.8, 45.2, .8),
    'TP5': ('UART0_RX', '5 RX', 56.5, 45.5, .8),
    'TP6': ('FACTORY_RESET_OD', '6 FACTORY OD', 24, 49.5, .8),
    'TP7': ('3V3', '3V3', 60, 48.5, .8),
    'TP8': ('VBAT', 'BAT', 61, 50.5, .8),
    'TP9': ('VSYS', 'SYS', 34.5, 58.5, .8),
    'TP10': ('USB_VBUS', '5V', 50.1, 54.3, .8),
    'TP11': ('REG_PG', '11 PGOOD', 71, 48.3, .8),
    'TP14': ('CHG_STAT1', 'S1', 50.5, 51.8, .8),
    'TP15': ('CHG_STAT2', '15 STAT2', 21, 61.3, .8),
    'TP16': ('PNL_CLK', 'CLK', 49, 62, .8),
    'TP17': ('PNL_MOSI', 'MOSI', 59, 59.5, .8),
    'TP18': ('PNL_CS', 'CS', 60, 54, .8),
    'TP19': ('PNL_DC', 'DC', 44, 58.5, .8),
    'TP20': ('PNL_RESET', '20 RESET', 88, 33.3, .8),
    'TP21': ('EPD_BUSY', 'BUSY', 60.5, 52.3, .8),
    'TP22': ('VBAT_CELL', '22 CELL+', 75, 30.5, .8),
}
CONNECTORS = {
    'J3': {'1': 'VBAT_CELL', '2': 'BATT_NTC', '3': 'GND'},
    'J4': {'1': '3V3', '2': 'GND', '3': 'CHIP_EN', '4': 'BOOT_IO0',
           '5': 'UART0_TX', '6': 'UART0_RX'},
}


def check_signals(board):
    fps = {k.ref_of(f): f for f in k.children(board, 'footprint')}
    actual = {r for r in fps if r.startswith('TP')}
    if actual != set(PROBES):
        raise ValueError(f'Test-point coverage changed: {actual ^ set(PROBES)}')
    for ref, pins in {**CONNECTORS, **{r: {'1': p[0]} for r, p in PROBES.items()}}.items():
        nets = {str(p[1]): str(k.child(p, 'net')[1]) for p in k.children(fps[ref], 'pad')
                if k.child(p, 'net')}
        if any(nets.get(pin) != net for pin, net in pins.items()):
            raise ValueError(f'Silkscreen pinout no longer matches {ref}: {nets}')
    return fps


def apply(board):
    fps = check_signals(board)
    old_ids = set()
    for group in list(k.children(board, 'group')):
        if str(group[1]) == GROUP:
            old_ids.update(str(v) for v in k.child(group, 'members')[1:])
            board.remove(group)
    for item in list(board):
        if not isinstance(item, list):
            continue
        identity = k.child(item, 'uuid')
        if identity and str(identity[1]) in old_ids:
            board.remove(item)
        elif item[0] == 'gr_text' and (item[1] == 'REV B FIT ONLY - UNROUTED - DO NOT FABRICATE' or (str(item[1]).startswith('REV B.') and 'ENGINEERING PROTOTYPE' in str(item[1]))):
            item[1] = 'REV B.05 ENGINEERING PROTOTYPE - PHYSICAL QUALIFICATION PENDING'

    members = []

    def add(item, key):
        identity = str(uuid.uuid5(NAMESPACE, key))
        item.append(node('uuid', identity))
        board.append(item)
        members.append(identity)

    def text(label, x, y, size=1, angle=0, side='F', key=None):
        effects = node('effects', node('font', node('size', number(size), number(size)),
                                       node('thickness', number(MIN_STROKE))))
        if side == 'B':
            effects.append(node('justify', k.Sym('mirror')))
        add(node('gr_text', label, node('at', number(x), number(y), number(angle)),
                 node('layer', side + '.SilkS'), effects), key or f'{side}:{label}')

    def line(points, key, side='F', width=.15):
        for i, (a, b) in enumerate(zip(points, points[1:])):
            add(node('gr_line', node('start', *map(number, a)), node('end', *map(number, b)),
                     node('stroke', node('width', number(width)), node('type', k.Sym('default'))),
                     node('layer', side + '.SilkS')), f'{side}:{key}:{i}')

    def brand(x, y, mark_x, mark_y, side):
        sign = -1 if side == 'B' else 1
        art = json.loads((ART / 'cicala-wordmark.json').read_text())
        for i, polygon in enumerate(art['polygons']):
            points = keyhole(polygon['outline'], polygon['holes'])
            add(node('gr_poly', node('pts', *[node('xy', number(x + sign * px), number(y + py))
                                             for px, py in points]),
                     node('stroke', node('width', number(0)), node('type', k.Sym('default'))),
                     node('fill', k.Sym('solid')), node('layer', side + '.SilkS')),
                f'{side}:wordmark:{i}')
        art = json.loads((ART / 'cicala-mark.json').read_text())
        scale = .7
        for i, segment in enumerate(art['segments']):
            points = [(number(mark_x + sign * scale * px), number(mark_y + scale * py))
                      for px, py in segment['points']]
            if segment['type'] == 'curve':
                item = node('gr_curve', node('pts', *[node('xy', *p) for p in points]))
            else:
                item = node('gr_line', node('start', *points[0]), node('end', *points[1]))
            item += [node('stroke', node('width', number(segment['stroke_mm'] * scale)),
                          node('type', k.Sym('default'))), node('layer', side + '.SilkS')]
            add(item, f'{side}:cicada:{i}')

    brand(84.5, 6, 110, 7, 'F')
    brand(83, 8.3, 100, 9, 'B')
    front = [
        ('01 DISPLAY / BOOST', 70, 18, 1, 90),
        ('U6', 73, 12, .8, 0), ('L1', 83, 11.8, .8, 0),
        ('J2 E-PAPER', 98.2, 33, 1, 90),
        ('CONTACTS DOWN', 101, 33, .8, 90),
        ('SW1 FILTERS', 108, 25.5, .8, 0),
        ('SW2 NEXT', 108, 44.3, .8, 0),
        ('J3 BAT', 69.8, 29, .8, 0),
        ('+', 73.3, 31.8, .8, 0),
        ('T', 73.3, 33, .8, 0),
        ('-', 73.3, 34.2, .8, 0),
        ('02 NTC GUARD', 67.2, 40.5, .8, 90),
        ('U7', 84, 35.5, .8, 0),
        ('03 POWER', 95.5, 43.5, 1, 0),
        ('U3 3V3', 69.8, 53.6, .8, 0),
        ('U2 CHARGE', 94, 50.5, .8, 90),
        ('100mA', 89, 58.5, .8, 0),
        ('USB 5V / DATA', 67.5, 62, .8, 0),
        ('D4 STATUS', 89, 60, .8, 0),
        ('J4', 52, 45.3, .8, 0),
        ('DEBUG', 61, 46, .8, 0),
        ('B.05', 110, 61.5, 1, 0),
    ]
    for row in front:
        text(*row)
    for ref, (_, label, x, y, size) in PROBES.items():
        side = str(k.child(fps[ref], 'layer')[1])[0]
        text(label, x, y, size, side=side, key='probe:' + ref)

    line([(68, 10), (68, 27.5), (72, 27.5)], 'display-boundary')
    line([(96.8, 25.5), (96.8, 11.5), (93, 11.5)], 'display-boundary-right')
    line([(72.5, 37), (72.5, 39.7), (70, 39.7)], 'temperature-bracket')
    line([(91.5, 37), (91.5, 44.8), (90, 44.8)], 'temperature-bracket-right')
    line([(66.7, 47.5), (66.7, 57.5), (72.7, 57.5)], 'power-bracket')
    line([(96.5, 47.5), (96.5, 58), (91, 58)], 'power-bracket-right')
    line([(63.5, 47.5), (63.5, 60.5), (53, 60.5)], 'debug-bracket')

    back = [
        ('REV B.05 / ESP32-S3 / 16MB', 85, 14, 1),
        ('4 LAYERS / 1.2mm / PROTOTYPE', 85, 16.5, 1),
        ('POWER: USB 5V -> U2 -> VSYS', 86, 21, 1),
        ('LiPo <-> U2 / VSYS -> U3 -> 3V3', 86, 23.3, 1),
        ('U2 BQ25185: 100mA CHARGE', 86, 25.6, 1),
        ('U7 + Q2: NTC CHARGE GUARD', 86, 27.9, 1),
        ('TOP TEST POINTS', 91, 37, 1),
        ('1 BOOT  2 EN  3 GND', 91, 39.3, 1),
        ('7 3V3  8 BAT  9 SYS', 91, 41.6, 1),
        ('10 5V  14 STAT1', 91, 43.9, 1),
        ('16 CLK  17 MOSI', 91, 46.2, 1),
        ('18 CS  19 DC  21 BUSY', 91, 48.5, 1),
        ('J2 UC8253 / CONTACTS DOWN', 90, 51.5, 1),
        ('J3 1:+  2:NTC  3:GND', 90, 53.8, 1),
        ('PROTECTED 1S LiPo ONLY', 90, 56.1, 1),
        ('NTC 10k B3435 ON CELL', 94, 58.4, .8),
        ('J4 RECOVERY / 3V3 LOGIC', 53, 51, 1),
        ('1 3V3  2 GND  3 EN', 53, 53.3, 1),
        ('4 BOOT  5 TX  6 RX', 53, 55.6, 1),
        ('BOOT LOW + PULSE EN', 53, 58, 1),
        ('3V3 REF ONLY / NO POWER', 53, 60.3, .8),
        ('U1 ESP32-S3 / 16MB', 22, 52.5, 1),
        ('TX / RX FROM MCU', 22, 54.8, 1),
        ('CERN-OHL-S-2.0', 22, 57.1, 1),
        ('ANTENNA / KEEP CLEAR', 10.5, 54, .8, 90),
    ]
    for row in back:
        text(*row, side='B')
    line([(68, 18.8), (104, 18.8)], 'identity-rule', 'B', .2)
    line([(68, 35.2), (104, 35.2)], 'service-rule', 'B', .2)
    line([(64.5, 49.8), (64.5, 61.6), (42.5, 61.6), (42.5, 49.8)], 'recovery-frame', 'B')
    line([(34, 51), (34, 59), (12, 59)], 'core-bracket', 'B')

    board.append(node('group', GROUP, node('uuid', str(uuid.uuid5(NAMESPACE, 'group'))),
                      node('members', *members)))
    for fp in fps.values():
        for shape in fp:
            if not isinstance(shape, list):
                continue
            layer = k.child(shape, 'layer')
            stroke = k.child(shape, 'stroke')
            if layer and layer[1] in ('F.SilkS', 'B.SilkS') and stroke:
                width = k.child(stroke, 'width')
                if float(width[1]) < MIN_STROKE:
                    width[1] = number(MIN_STROKE)
    # Shorten L1's Q1-facing line without moving ink toward the inductor lands.
    for shape in k.children(fps['L1'], 'fp_line'):
        if k.child(shape, 'layer')[1] != 'F.SilkS':
            continue
        start, end = k.child(shape, 'start'), k.child(shape, 'end')
        if float(start[2]) > 2 and float(end[2]) > 2:
            start[2] = end[2] = number(2.25)
            end[1] = number(.8)


def save_preserving(path, original, board):
    """Keep unedited KiCad blocks byte-for-byte, including filled-zone polygons."""
    blocks = {}
    depth, start = 0, None
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|[()]', original):
        if token[0] == '(':
            depth += 1
            if depth == 2:
                start = token.start()
        elif token[0] == ')':
            if depth == 2:
                block = original[start:token.end()]
                blocks[k.dumps(k.parse(block))] = block
            depth -= 1
    chunks = []
    for item in board[1:]:
        serialized = k.dumps(item)
        chunks.append('\t' + blocks[serialized] if serialized in blocks else k.dumps(item, 1))
    path.write_text('(kicad_pcb\n' + '\n'.join(chunks) + '\n)\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('board', type=Path, nargs='?', default=BOARD)
    args = parser.parse_args()
    original = args.board.read_text()
    board = k.parse(original)
    apply(board)
    save_preserving(args.board, original, board)
    print(f'Applied Rev B silkscreen: {args.board}')


if __name__ == '__main__':
    main()
