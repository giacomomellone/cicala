"""Export aligned orthographic board views and readable native silkscreen plots."""
from datetime import date
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from check_contract import BOARD
from generate_contract import ROOT
from study import run, tool


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    kicad = tool('KICAD_CLI', 'kicad-cli', '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    output = BOARD.parent / 'renders'
    output.mkdir(exist_ok=True)
    report = ROOT / 'build/hardware-rev-b'
    report.mkdir(parents=True, exist_ok=True)
    for side, layer in [('top', 'F'), ('bottom', 'B')]:
        run([kicad, 'pcb', 'render', '--output', output / f'{side}.png',
             '--width', '2400', '--height', '1560', '--side', side, '--rotate', '0,0,0',
             '--zoom', '1.4', '--background', 'transparent', '--quality', 'basic', BOARD],
            report / f'render-pcb-{side}.log')
        run([kicad, 'pcb', 'export', 'svg', '--layers', f'{layer}.Mask,{layer}.SilkS,Edge.Cuts',
             *(['--mirror'] if side == 'bottom' else []), '--black-and-white',
             '--subtract-soldermask', '--fit-page-to-board', '--exclude-drawing-sheet',
             '--mode-single', '--output', output / f'silkscreen_{side}.svg', BOARD],
            report / f'plot-silkscreen-{side}.log')
    sources = [BOARD, BOARD.with_suffix('.kicad_pro'), *sorted(BOARD.parent.glob('models/*.step')),
               Path(__file__)]
    outputs = [output / name for name in ('top.png', 'bottom.png', 'silkscreen_top.svg', 'silkscreen_bottom.svg')]
    size = struct.unpack('>II', (output / 'top.png').read_bytes()[16:24])
    provenance = {
        'generated': str(date.today()),
        'generator': 'KiCad ' + subprocess.check_output([kicad, 'version'], text=True).strip(),
        'projection': 'orthographic', 'rotation_degrees': [0, 0, 0],
        'zoom': 1.4,
        'background': 'transparent', 'quality': 'basic',
        'requested_viewport_px': [2400, 1560], 'output_size_px': list(size),
        'inputs': {str(p.relative_to(ROOT)): sha(p) for p in sources},
        'outputs': {p.name: sha(p) for p in outputs},
        'model_libraries': 'Installed KiCad 10 models plus project-local STEP envelopes; standard library files are not vendored or fingerprinted here.',
        'limitations': 'Digital board render, not a physical prototype; some component models are bounding envelopes.',
    }
    (output / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    print(f'Exported orthographic board views and silkscreen plots: {output.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
