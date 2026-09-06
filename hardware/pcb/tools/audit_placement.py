"""Check footprint grouping constraints independently of the routing grid."""
import argparse
import json
from pathlib import Path

from shapely import union_all
from shapely.geometry import Polygon, box

import ksexp


def audit(geom, board_path=None, models_root=None):
    footprints = geom['footprints']
    courts = [(f,union_all([Polygon(p) for p in f['courtyards']]))
              for f in footprints if f['courtyards']]
    overlaps = []
    for i,(a,pa) in enumerate(courts):
        for b,pb in courts[i+1:]:
            if a['layer']==b['layer'] and pa.intersection(pb).area>0.00001:
                overlaps.append([a['ref'],b['ref']])
    cell = box(32,6,68,36)
    result = {'footprints':len(footprints), 'courtyard_overlaps':overlaps,
              'cell_courtyard_intrusions':[f['ref'] for f,p in courts
                  if f['layer']=='B.Cu' and p.intersection(cell).area>0.00001],
              'unexpected_front_parts':[f['ref'] for f in footprints
                  if f['layer']=='F.Cu' and f['ref'] not in {'SW1','SW2'}
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
