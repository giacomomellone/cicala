"""Build an isolated review candidate; never overwrite the working board.

python tools/build_candidate.py /absolute/path/to/new-directory
Requires KiCad 10 CLI, pcbnew Python, and tools/requirements.txt in this Python.
KICAD_CLI_BIN and KICAD_PYTHON_BIN override tool discovery.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

TOOLS = Path(__file__).resolve().parent
SOURCE = TOOLS.parent/'cicala_rev_a'
MAC_KICAD = Path('/Applications/KiCad/KiCad.app/Contents')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination',type=Path)
    parser.add_argument('--stitch-rounds',type=int,default=1)
    args = parser.parse_args()
    cli = os.environ.get('KICAD_CLI_BIN') or shutil.which('kicad-cli')
    cli = cli or str(MAC_KICAD/'MacOS/kicad-cli')
    kpython = os.environ.get('KICAD_PYTHON_BIN')
    kpython = kpython or (str(MAC_KICAD/'Frameworks/Python.framework/Versions/3.9/bin/python3')
                         if MAC_KICAD.exists() else sys.executable)
    import numpy, shapely
    subprocess.run([kpython,'-c','import pcbnew'],check=True)
    out = args.destination.resolve()
    out.mkdir(parents=True,exist_ok=False)
    for name in ('cicala_rev_a.kicad_pcb','cicala_rev_a.kicad_pro','cicala_rev_a.kicad_sch','cicala_rev_a.kicad_dru',
                 'cicala_rev_a.kicad_sym','fp-lib-table','sym-lib-table'):
        shutil.copy2(SOURCE/name,out/name)
    for name in ('sheets','cicala.pretty'):
        shutil.copytree(SOURCE/name,out/name)
    board = out/'cicala_rev_a.kicad_pcb'
    project = out/'cicala_rev_a.kicad_pro'
    def script(name,*arguments,kicad=False):
        subprocess.run([kpython if kicad else sys.executable,str(TOOLS/name),
                        *map(str,arguments)],check=True)
    def refill(index):
        subprocess.run([cli,'pcb','drc','--schematic-parity','--refill-zones','--save-board',
                        '--format','json','-o',str(out/f'drc-{index}.json'),str(board)],check=True)
        script('apply_project.py',project)
        script('export_geom.py',board,out/'filled-geometry.json',kicad=True)
    script('refine_placement.py',board,board,kicad=True)
    script('apply_project.py',project)
    script('apply_silkscreen.py',board,board)
    script('zones.py',board)
    script('export_geom.py',board,out/'placement.json',kicad=True)
    script('critical_routes.py',out/'placement.json',out/'seed.json')
    script('route_fanout.py',out/'placement.json',out/'routes.json',1,out/'seed.json')
    if json.loads((out/'routes.json').read_text())['failed']:
        script('finish_routes.py',out/'placement.json',out/'routes.json',out/'routes.json')
    script('ground_planes.py',out/'placement.json',out/'routes.json',out/'routes.json')
    script('apply_tracks.py',board,out/'routes.json')
    script('thermal_relief.py',board,board)
    refill(0)
    for index in range(1,args.stitch_rounds+1):
        script('stitch_islands.py',out/'filled-geometry.json',out/'routes.json',out/'routes.json')
        script('apply_tracks.py',board,out/'routes.json')
        refill(index)
    script('audit_copper.py',out/'filled-geometry.json','--output',out/'copper-audit.json')
    report = json.loads((out/f'drc-{args.stitch_rounds}.json').read_text())
    counts = {key:len(report.get(key,[])) for key in
              ('violations','unconnected_items','schematic_parity')}
    print('Candidate only:',out,counts)
    if any(counts.values()):
        raise SystemExit(1)
    print('DRC passed. Electrical, mechanical and fabrication reviews remain required.')


if __name__ == '__main__':
    main()
