"""Export aligned orthographic board views and readable native silkscreen plots."""
from datetime import date
import hashlib
import json
import os
import tempfile
from pathlib import Path
import struct
import subprocess

from check_contract import BOARD
from generate_contract import ROOT
from study import tool


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    kicad = tool('KICAD_CLI', 'kicad-cli', '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    output = BOARD.parent / 'renders'
    output.mkdir(exist_ok=True)
    report = ROOT / 'build/hardware-rev-b'
    report.mkdir(parents=True, exist_ok=True)
    # A local viewer may hide through-hole parts. Isolate render preferences.
    with tempfile.TemporaryDirectory(prefix='cicala-pcb-render-') as config:
        settings=Path(config)/'10.0';settings.mkdir()
        (settings/'3d_viewer.json').write_text(json.dumps({
            'render': {'show_footprints_normal':True,'show_footprints_insert':True,
                       'show_footprints_dnp':False,'show_footprints_virtual':False},
            'current_layer_preset':'follow_plot_settings'}))
        environment=dict(os.environ,KICAD_CONFIG_HOME=config)
        export_views(kicad,output,report,environment)
    sources = [BOARD, BOARD.with_suffix('.kicad_pro'), *sorted(BOARD.parent.glob('models/*.step')),
               Path(__file__)]
    outputs = [output / name for name in ('top.png', 'bottom.png', 'silkscreen_top.svg', 'silkscreen_bottom.svg')]
    size = struct.unpack('>II', (output / 'top.png').read_bytes()[16:24])
    provenance = {
        'generated': str(date.today()),
        'generator': 'KiCad ' + subprocess.check_output([kicad, 'version'], text=True).strip(),
        'projection': 'orthographic', 'rotation_degrees': [0, 0, 0],
        'zoom': 1.4, 'models': 'SMT and through-hole; DNP and virtual models hidden',
        'background': 'transparent', 'quality': 'basic',
        'requested_viewport_px': [2400, 1560], 'output_size_px': list(size),
        'inputs': {str(p.relative_to(ROOT)): sha(p) for p in sources},
        'outputs': {p.name: sha(p) for p in outputs},
        'model_libraries': 'Installed KiCad 10 models plus project-local STEP envelopes; standard library files are not vendored or fingerprinted here.',
        'limitations': 'Digital board render, not a physical prototype; some component models are bounding envelopes.',
    }
    (output / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    print(f'Exported orthographic board views and silkscreen plots: {output.relative_to(ROOT)}')


def export_views(kicad,output,report,environment):
    def run_view(command, log):
        process=subprocess.run(list(map(str,command)),cwd=ROOT,env=environment,capture_output=True,text=True)
        log.write_text(process.stdout+process.stderr)
        if process.returncode:raise RuntimeError(f'KiCad render/export failed: {log}')
    for side,layer in [('top','F'),('bottom','B')]:
        run_view([kicad, 'pcb', 'render', '--output', output / f'{side}.png',
             '--width', '2400', '--height', '1560', '--side', side, '--rotate', '0,0,0',
             '--zoom', '1.4', '--background', 'transparent', '--quality', 'basic', BOARD],
            report / f'render-pcb-{side}.log')
        run_view([kicad, 'pcb', 'export', 'svg', '--layers', f'{layer}.Mask,{layer}.SilkS,Edge.Cuts',
             *(['--mirror'] if side == 'bottom' else []), '--black-and-white',
             '--subtract-soldermask', '--fit-page-to-board', '--exclude-drawing-sheet',
             '--mode-single', '--output', output / f'silkscreen_{side}.svg', BOARD],
            report / f'plot-silkscreen-{side}.log')
        plot = output / f'silkscreen_{side}.svg'
        plot.write_text('\n'.join(line.rstrip() for line in plot.read_text().splitlines()) + '\n')


if __name__ == '__main__':
    main()
