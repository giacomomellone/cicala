"""Read back the Gerber ZIP and compare every drill/slot with KiCad geometry.

Requires gerbonara. Usage: audit_fabrication.py exports geometry.json report.json
"""
from collections import Counter
import json
import math
from pathlib import Path
import sys
from zipfile import BadZipFile, ZipFile
from xml.etree import ElementTree


def check_ipc2581(path):
    try:
        with ZipFile(path) as archive:
            if archive.testzip() is not None:
                raise ValueError('IPC-2581 ZIP CRC failed')
            names = archive.namelist()
            if len(names) != 1 or not names[0].lower().endswith('.xml'):
                raise ValueError('IPC-2581 ZIP must contain one XML document')
            root = ElementTree.fromstring(archive.read(names[0]))
            if root.tag.split('}')[-1] != 'IPC-2581':
                raise ValueError('IPC-2581 XML root is missing')
            return {'compressed': True, 'xml_root': 'IPC-2581',
                    'revision': root.attrib.get('revision')}
    except BadZipFile as error:
        raise ValueError('IPC-2581 output named .zip is not a ZIP archive') from error


def audit(directory, geometry, prefix='cicala_rev_a', origin=(3, 3.5)):
    from gerbonara.layers import LayerStack
    from gerbonara.utils import MM
    directory = Path(directory)
    ipc = check_ipc2581(directory / f'{prefix}_ipc2581.zip')
    stack = LayerStack.open(directory / f'{prefix}_gerbers.zip')
    required = {('top', 'copper'), ('inner_1', 'copper'), ('inner_2', 'copper'),
                ('bottom', 'copper'), ('mechanical', 'outline'),
                *((side, layer) for side in ('top', 'bottom')
                  for layer in ('paste', 'mask', 'silk'))}
    if set(stack.graphic_layers) != required:
        raise ValueError('Fabrication ZIP has missing or unexpected layers')

    def key(x, y, diameter, dx=0, dy=0):
        return tuple(round(v, 4) for v in (x, y, diameter, abs(dx), abs(dy)))

    expected = {'pth': Counter(), 'npth': Counter()}
    # The design's declared drill/placement origin is the PCB rear-left corner.
    for via in geometry['vias']:
        expected['pth'][key(via['at'][0] - origin[0], origin[1] - via['at'][1], via['drill'])] += 1
    for pad in geometry['pads']:
        if not pad['drill']:
            continue
        w, h = pad['drill_w'], pad['drill_h']
        angle = math.radians(-pad['angle'] + (90 if h > w else 0))
        length = abs(w - h)
        expected['npth' if pad['npth'] else 'pth'][key(
            pad['x'] - origin[0], origin[1] - pad['y'], min(w, h),
            length * math.cos(angle), length * math.sin(angle))] += 1
    counts = {}
    for name, layer in [('pth', stack.drill_pth), ('npth', stack.drill_npth)]:
        actual = Counter()
        for obj in layer.objects:
            diameter = obj.aperture.equivalent_width(MM)
            if hasattr(obj, 'x'):
                item = key(obj.x, obj.y, diameter)
            else:
                item = key((obj.x1 + obj.x2) / 2, (obj.y1 + obj.y2) / 2,
                           diameter, obj.x2 - obj.x1, obj.y2 - obj.y1)
            actual[item] += 1
        # KiCad's metric Excellon coordinates are rounded to one micrometre.
        remaining = list(actual.elements())
        for wanted in expected[name].elements():
            match = next((item for item in remaining
                          if max(abs(a - b) for a, b in zip(item, wanted)) <= .00051), None)
            if match is None:
                raise ValueError(f'{name} drill missing or displaced: {wanted}')
            remaining.remove(match)
        if remaining:
            raise ValueError(f'{name} has unexpected drill features: {remaining}')
        counts[name] = sum(actual.values())
    (directory / 'review').mkdir(exist_ok=True)
    for side in ('top', 'bottom'):
        (directory / 'review' / f'gerber_{side}.svg').write_text(
            str(stack.to_pretty_svg(side=side)))
    return {'ipc2581': ipc, 'graphic_layers': len(stack.graphic_layers),
            'matched_plated_drills_and_slots': counts['pth'],
            'matched_nonplated_drills': counts['npth'],
            'comparison_tolerance_mm': .00051,
            'all_drill_positions_sizes_and_slot_axes_match': True}


if __name__ == '__main__':
    result = audit(sys.argv[1], json.loads(Path(sys.argv[2]).read_text()))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
