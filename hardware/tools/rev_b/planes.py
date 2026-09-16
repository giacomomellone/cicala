"""Add Rev B copper planes and preserve reference corridors from routing JSON.

Run with the routing Python environment after applying the reviewed tracks.
The geometry JSON includes the routing-only reference keepouts. Zone filling
and native DRC remain required after writing the plane definitions.
"""
import argparse
import json
from pathlib import Path
import sys
import uuid
from shapely import union_all
from shapely.geometry import Polygon, Point

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'pcb'))
import ksexp
from ksexp import Sym


def polygon(points):
    return [Sym('polygon'), [Sym('pts'), *[[Sym('xy'), Sym(f'{x:.6f}'), Sym(f'{y:.6f}')] for x,y in points]]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('board', type=Path)
    parser.add_argument('geometry', type=Path)
    args = parser.parse_args()
    board = ksexp.load(args.board)
    geometry = json.loads(args.geometry.read_text())
    for item in list(ksexp.children(board, 'zone')):
        name = ksexp.child(item, 'name')
        if ksexp.child(item, 'keepout') is None or (name and str(name[1]).startswith('REV_B_')):
            board.remove(item)
    def add_zone(name,net,layer,points,priority):
        board.append([Sym('zone'), [Sym('net'), net], [Sym('layer'), layer],
            [Sym('uuid'), str(uuid.uuid4())], [Sym('name'), name],
            [Sym('hatch'), Sym('edge'), Sym('0.5')], [Sym('priority'), Sym(str(priority))],
            [Sym('connect_pads'), [Sym('clearance'), Sym('0.2')]],
            [Sym('min_thickness'), Sym('0.15')], [Sym('filled_areas_thickness'), Sym('no')],
            [Sym('fill'), Sym('yes'), [Sym('island_removal_mode'), Sym('0')],
             [Sym('thermal_gap'), Sym('0.2')], [Sym('thermal_bridge_width'), Sym('0.2')]], polygon(points)])
    for layer in ('F.Cu','In1.Cu','B.Cu'):
        add_zone('REV_B_GND_'+layer,'GND',layer,geometry['outline'],10)
    add_zone('REV_B_3V3','3V3','In2.Cu',geometry['outline'],5)
    usb_regions=[]
    references=[area for area in geometry['keepouts'] if not area['tracks'] and area['vias'] and set(area['layers']).issubset({'In1.Cu','In2.Cu'})]
    for i, area in enumerate(references):
        layer=area['layers'][0]
        board.append([Sym('zone'), [Sym('layer'), layer], [Sym('uuid'), str(uuid.uuid4())],
            [Sym('name'), f'REV_B_REFERENCE_KEEPOUT_{i}'], [Sym('hatch'), Sym('edge'), Sym('0.5')],
            [Sym('connect_pads'), [Sym('clearance'), Sym('0')]], [Sym('min_thickness'), Sym('0.15')],
            [Sym('keepout'), *[[Sym(n), Sym('not_allowed' if n=='tracks' else 'allowed')]
                for n in ('tracks','vias','pads','copperpour','footprints')]], polygon(area['pts'])])
        if layer=='In2.Cu':
            usb_regions.append(Polygon(area['pts']))
    usb_regions.append(Point(76.4,58.9).buffer(.6))
    usb_regions.append(Polygon([(76.7,55.1),(78.35,55.1),(78.35,56.1),(76.7,56.1)]))
    regions=union_all(usb_regions)
    for i, poly in enumerate(getattr(regions,'geoms',[regions])):
        add_zone(f'REV_B_USB_REFERENCE_{i}','GND','In2.Cu',list(poly.exterior.coords),20)
    ksexp.save(args.board,board)
    print('Wrote ground, power and protected USB reference regions')


if __name__=='__main__':
    main()
