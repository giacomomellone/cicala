"""Check Rev B engineering files and export printable parts from native CAD."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

from generate_contract import CONTRACT, ROOT, OUTPUT, render
from check_contract import BOARD, BOUNDS, check
from button_tolerances import audit as button_audit
from print_layout import placement, role, rotation, check_flat_face

sys.path.insert(0, str(ROOT / 'hardware/tools/case'))
from check_mesh import check as check_mesh, triangles

SOURCE = ROOT / 'hardware/rev_b/case/cicala_enclosure.scad'
PARTS = ['base','top_shell','display_frame','display_support_bar','carrier','carrier_cover',
         'flex_former','wedge','pcb_reference','cell_reference','panel_reference',
         'category_cap_reference','next_cap_reference','switch_reference']
FIT = ['components_case','components_cell','pcb_cell','panel_case','flex_case',
       'flex_components','harness','carrier','switches_case','carrier_fasteners']


def tool(variable, name, mac):
    candidate = os.environ.get(variable) or shutil.which(name)
    if not candidate and Path(mac).is_file():
        candidate = mac
    if not candidate:
        raise RuntimeError(f'Install {name} or set {variable}; see docs/hardware_rev_b.md')
    return candidate


def run(command, log):
    result = subprocess.run(list(map(str, command)), cwd=ROOT, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError(f'{command[0]} exited {result.returncode}; see {log}\n'
                           + (result.stdout + result.stderr)[-2500:])
    return result


def netlist_parity(path, geometry):
    import ksexp as s
    netlist = s.load(path)
    expected = set()
    for net in s.children(s.child(netlist, 'nets'), 'net'):
        name = s.child(net, 'name')[1]
        for node in s.children(net, 'node'):
            expected.add((s.child(node, 'ref')[1], s.child(node, 'pin')[1], name))
    actual = {(pad['ref'],pad['pad'],pad['net']) for pad in geometry['pads']
              if pad['net'] and pad['pad']}
    if actual != expected:
        raise ValueError('Schematic/PCB connectivity differs: ' + str(sorted(actual ^ expected)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', action='store_true')
    parser.add_argument('--mechanical-only', action='store_true', help='Skip electrical checks; outputs are for enclosure fit only')
    parser.add_argument('--renders', action='store_true', help='Needs an OpenGL display; use xvfb-run on Linux')
    parser.add_argument('--report', type=Path, default=ROOT / 'build/hardware-rev-b')
    args = parser.parse_args()
    report = args.report.resolve(); report.mkdir(parents=True, exist_ok=True)
    kicad = tool('KICAD_CLI', 'kicad-cli', '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    kpython = tool('KICAD_PYTHON', 'python3', '/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3')
    if platform.system() == 'Darwin' and not os.environ.get('KICAD_PYTHON'):
        kpython = '/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3'
    models = os.environ.get('KICAD10_3DMODEL_DIR',
        '/Applications/KiCad/KiCad.app/Contents/SharedSupport/3dmodels' if platform.system() == 'Darwin'
        else '/usr/share/kicad/3dmodels')
    scad = [tool('OPENSCAD_BIN', 'openscad', '/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD')]
    if platform.system() == 'Darwin' and platform.machine() == 'arm64':
        if subprocess.run(['arch','-x86_64',*scad,'--version'],capture_output=True).returncode == 0:
            scad = ['arch','-x86_64',*scad]
    run([kpython, ROOT/'hardware/tools/rev_b/export_geometry.py', BOARD, report/'geometry.json'], report/'geometry.log')
    geometry = json.loads((report/'geometry.json').read_text())
    contract = json.loads(CONTRACT.read_text())
    if args.export:
        OUTPUT.write_text(render(contract))
        run([kicad,'pcb','export','step','--force','--subst-models','--no-dnp','--no-unspecified',
             '-o',report/'board.step',BOARD],report/'step.log')
        cad_python = os.environ.get('CAD_PYTHON',sys.executable)
        run([cad_python, ROOT/'hardware/tools/case/export_component_bounds.py',BOARD,BOUNDS,
             '--step',report/'board.step','--geometry',report/'geometry.json',
             '--pcb-z',contract['pcb']['z']],report/'bounds.log')
    result = check(geometry, Path(models))
    (report/'button_tolerances.json').write_text(json.dumps(button_audit(contract),indent=2)+'\n')
    print('Native board: concave outline, placement, models, RF clearance and source fingerprints passed', flush=True)
    if not args.mechanical_only:
        run([kicad,'sch','erc','--severity-all','--exit-code-violations','--format','json',
             '-o',report/'erc.json',BOARD.with_suffix('.kicad_sch')],report/'erc.log')
        run([kicad,'sch','export','netlist','-o',report/'netlist.net',BOARD.with_suffix('.kicad_sch')],report/'netlist.log')
        netlist_parity(report/'netlist.net',geometry)
        run([kicad,'pcb','drc','--severity-all','--format','json','-o',report/'drc.json',BOARD],report/'drc.log')
        drc = json.loads((report/'drc.json').read_text())
        if drc['violations'] or drc.get('schematic_parity'):
            raise ValueError('Unexpected DRC violations; inspect ' + str(report/'drc.json'))
        result['unconnected_items'] = len(drc['unconnected_items'])
        if result['unconnected_items']:
            raise ValueError('PCB routing incomplete; inspect ' + str(report/'drc.json'))
        from electrical import audit as electrical_audit
        (report/'temperature_audit.json').write_text(json.dumps(electrical_audit(report/'netlist.net',BOARD),indent=2)+'\n')
        from usb_review import audit as usb_audit
        import ksexp
        usb=usb_audit(geometry,ksexp.load(BOARD))
        (report/'usb_audit.json').write_text(json.dumps(usb,indent=2)+'\n')
        if any(v > 1e-4 for v in usb['centreline_without_ground_mm'].values()):
            raise ValueError('USB centreline lacks its adjacent ground reference')
        if usb['pair_skew_mm'] >= .1:
            raise ValueError('USB geometric path mismatch exceeds 0.1 mm')
        print('ERC, netlist parity and DRC passed with zero unconnected items',flush=True)
    output = ROOT/'hardware/rev_b/case/exports' if args.export else report/'meshes'
    output.mkdir(parents=True,exist_ok=True)
    jobs = [(part,part,[]) for part in PARTS]
    print_jobs={}
    for name,part,defines in jobs:
        model_dir=output/('reference' if role(part)=='reference' else 'assembly') if args.export else output
        model_dir.mkdir(parents=True,exist_ok=True)
        options=['-D',f'part="{part}"']
        for define in defines: options += ['-D',define]
        for ext in (['stl','3mf'] if args.export else ['stl']):
            target=model_dir/f'{name}.{ext}'
            run([*scad,'--hardwarnings',*options,'-o',target,SOURCE],report/f'{name}-{ext}.log')
            if ext=='stl':
                check_mesh(target, expected_solids=5 if part=='switch_reference' else 1)
                if part in ('category_cap_reference','next_cap_reference'):
                    index=0 if part=='category_cap_reference' else 1
                    b=contract['buttons']
                    check_flat_face(triangles(target),contract['pcb']['z']+contract['pcb']['thickness']+b['assembled_height'],b['cap_sizes'][index]**2-1)
                if role(part)=='core':
                    points=[p for face in triangles(target)for p in face]
                    for axis,limit in enumerate((contract['case']['width'],contract['case']['depth'],contract['case']['height'])):
                        extra=0
                        lower=contract['case']['origin'][axis]
                        if min(p[axis]for p in points)<lower-.0001 or max(p[axis]for p in points)+extra>lower+limit+.0001:
                            raise ValueError(f'{name} exceeds the core envelope on axis {axis}')
        if args.export and role(part)!='reference':
            layout=placement(triangles(model_dir/f'{name}.stl'),rotation(part))
            directory=output/'print'/role(part);directory.mkdir(parents=True,exist_ok=True)
            orient=['-D','print_rotation='+json.dumps(layout['rotation_degrees']),
                    '-D','print_shift='+json.dumps(layout['translation_mm'])]
            for ext in ('stl','3mf'):
                target=directory/f'{name}.{ext}'
                run([*scad,'--hardwarnings',*options,*orient,'-o',target,SOURCE],report/f'print-{name}-{ext}.log')
                if ext=='stl':
                    check_mesh(target)
                    minimum=min(p[2]for face in triangles(target)for p in face)
                    if abs(minimum)>.0001:raise ValueError(f'{name} is not seated on the print bed')
            print_jobs[str((directory/f'{name}.stl').relative_to(output))]=layout
    failures=[]
    poses = [(name,0,[]) for name in FIT]
    poses += [(name,s,[]) for name in ('caps_motion','caps_switch_body') for s in (0,.1,.2,.3,.4,.5)]
    poses += [('cap_insert',s,[]) for s in (0,1,2,3,4,5,6)]
    play=contract['buttons']['alignment_tolerance']
    for x,y in ((-play,-play),(-play,play),(play,-play),(play,play)):
        for stroke in (0,.5):
            poses.append(('caps_motion',stroke,[f'side_shift=[{x},{y},0]']))
    for pose_index,(name,stroke,defines) in enumerate(poses):
        target=report/f'fit_{pose_index}_{name}_{stroke}.stl'
        target.unlink(missing_ok=True)
        command=[*scad,'--hardwarnings','-D',f'part="fit_{name}"','-D',f'stroke={stroke}']
        for define in defines:command += ['-D',define]
        command += ['-o',target,SOURCE]
        process=subprocess.run(list(map(str,command)),cwd=ROOT,capture_output=True,text=True)
        log=process.stdout+process.stderr
        (report/f'fit_{pose_index}_{name}_{stroke}.log').write_text(log)
        if (process.returncode==0 or 'Current top level object is empty.' not in log
                or 'WARNING:' in log or 'ERROR:' in log): failures.append(f'{name} at {stroke}')
    result['mechanical_poses']=len(poses)
    if failures:raise ValueError('Mechanical intersections: '+', '.join(failures))
    result['mesh_pairs']=len(jobs)
    result['printable_parts']=sum(role(part)!='reference' for _,part,_ in jobs)
    result['button_clearance_audit']=button_audit(contract)
    result['status']=('mechanical files checked; electrical checks not run' if args.mechanical_only else
                      'engineering prototype files checked; first-article physical measurements pending')
    (report/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    if args.export:
        w,h = contract['display']['glass'][:2]
        (output/'patterns').mkdir(exist_ok=True)
        (output/'patterns'/'lens_cut.svg').write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}mm" height="{h}mm" viewBox="0 0 {w} {h}">\n'
            '<title>Rev B 0.8 mm lens blank; nominal outline, no kerf compensation</title>\n'
            f'<path d="M0 0H{w}V{h}H0Z" fill="none" stroke="black" stroke-width="0.01"/>\n</svg>\n')
        (output/'print-layout.json').write_text(json.dumps({'jobs':print_jobs,'notes':'Individually oriented meshes. Slicer supports, material and fit require qualification; purchased caps and switch are reference meshes, never print jobs.'},indent=2)+'\n')
        source_paths=[CONTRACT,SOURCE,OUTPUT,BOARD,BOUNDS,BOARD.with_suffix('.kicad_pro'),
                      BOARD.with_suffix('.kicad_dru'),*BOARD.parent.rglob('*.kicad_sch'),
                      *BOARD.parent.glob('models/*.step'),*BOARD.parent.glob('cicala.pretty/*.kicad_mod'),
                      *Path(__file__).parent.glob('*.py'),ROOT/'hardware/tools/case/export_component_bounds.py']
        manifest={'status':result['status'],'inputs':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
                  'outputs':{str(p.relative_to(output)):hashlib.sha256(p.read_bytes()).hexdigest() for p in output.rglob('*') if p.suffix in ('.stl','.3mf','.svg') or p.name=='print-layout.json'}}
        (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    if args.renders:
        renders=ROOT/'hardware/rev_b/case/renders';renders.mkdir(parents=True,exist_ok=True)
        for part in ['assembly','inside','exploded','section','carrier','wedge','buttons_exploded']:
            run([*scad,'--hardwarnings','-D',f'part="{"carrier_assembly" if part == "carrier" else part}"','--imgsize=1600,1000','--viewall','--autocenter',
                 '--render','--colorscheme=Tomorrow',
                 '--camera='+('125,115,55,54,25,4' if part=='section' else '148,-28,70,108,35,11' if part=='buttons_exploded' else '130,-110,130,54,33,3'),
                 '-o',renders/f'{part}.png',SOURCE],report/f'render-{part}.log')
        docs_image = ROOT/'docs/assets/images/hardware_rev_b/assembly.png'
        docs_image.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(renders/'assembly.png',docs_image)
    print('Printable meshes, purchased-cap clearances and empty collision checks passed. Physical tests remain open.',flush=True)


if __name__=='__main__':
    main()
