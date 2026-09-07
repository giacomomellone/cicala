"""Check footprint grouping constraints independently of the routing grid."""
import argparse
import json
from pathlib import Path

from shapely import union_all
from shapely.geometry import Polygon

import ksexp
from copper_geometry import pad_shape

FRONT_PARTS = {'SW1', 'SW2', 'J2', 'U6', 'L1', 'Q1', 'D1', 'D2', 'D3',
               *(f'C{i}' for i in range(12, 25)), 'R18', 'R19', 'R20',
               *(f'R{i}' for i in range(27, 33))}


def audit(geom, board_path=None, models_root=None):
    footprints = geom['footprints']
    courts = [(f,union_all([Polygon(p) for p in f['courtyards']]))
              for f in footprints if f['courtyards']]
    overlaps = []
    for i,(a,pa) in enumerate(courts):
        for b,pb in courts[i+1:]:
            if a['layer']==b['layer'] and pa.intersection(pb).area>0.00001:
                overlaps.append([a['ref'],b['ref']])
    # Alignment holes pass through both assembly faces. Same-side courtyard
    # checks alone miss a probe hole underneath a component on the other face.
    hole_intrusions = []
    for pad in geom.get('pads', []):
        if not pad['npth']:
            continue
        hole = pad_shape(pad, drill=True).buffer(0.2)
        for fp, court in courts:
            if fp['ref'] != pad['ref'] and hole.intersection(court).area > 0.00001:
                hole_intrusions.append([pad['ref'], fp['ref']])
    result = {'footprints':len(footprints), 'courtyard_overlaps':overlaps,
              'through_hole_component_intrusions':hole_intrusions,
              'unexpected_front_parts':[f['ref'] for f in footprints
                  if f['layer']=='F.Cu' and f['ref'] not in FRONT_PARTS
                  and not f['ref'].startswith('H')]}
    if board_path:
        missing = []
        board = ksexp.load(board_path)
        for fp in ksexp.children(board,'footprint'):
            props = {str(p[1]):str(p[2]) for p in ksexp.children(fp,'property')}
            ref = props['Reference']
            if ref.startswith(('TP','H')) or ref=='J4':
                continue
            models = list(ksexp.children(fp,'model'))
            if not models:
                missing.append({'ref':ref,'reason':'no model assigned'})
            elif models_root:
                for m in models:
                    path = str(m[1]).replace('${KICAD10_3DMODEL_DIR}',str(models_root))
                    path = path.replace('${KIPRJMOD}',str(Path(board_path).resolve().parent))
                    if not Path(path).is_file():
                        missing.append({'ref':ref,'reason':'model file unavailable','model':str(m[1])})
        result['missing_component_models'] = missing
    return result


if __name__=='__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('geometry')
    parser.add_argument('--board')
    parser.add_argument('--models-root')
    parser.add_argument('--output')
    args = parser.parse_args()
    result = audit(json.load(open(args.geometry)),args.board,args.models_root)
    print(json.dumps(result,indent=2))
    if args.output:
        Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    raise SystemExit(int(any(result[key] for key in result if key != 'footprints')))
