"""Export a checked Rev B engineering prototype package from native KiCad files."""
import csv
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from zipfile import ZIP_DEFLATED, ZipFile

from generate_contract import ROOT
from check_contract import BOARD
from study import run, tool
sys.path.insert(0, str(ROOT/'hardware/tools/pcb'))
from audit_fabrication import audit
from check_assembly_bom import check as check_bom
from jlc_assembly import convert
import ksexp


def main():
    kicad = tool('KICAD_CLI','kicad-cli','/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
    report=ROOT/'build/hardware-rev-b'
    # This checks native ERC/DRC, parity, mechanical geometry and stale envelopes.
    subprocess.run([sys.executable, str(Path(__file__).with_name('study.py'))],cwd=ROOT,check=True)
    geometry=json.loads((report/'geometry.json').read_text())
    prefix=BOARD.stem
    with tempfile.TemporaryDirectory(prefix='rev-b-fab-',dir=report) as temp:
        out=Path(temp)
        for directory in ('assembly','gerbers','drill','review'): (out/directory).mkdir()
        def cli(args,log):return run([kicad,*args],report/log)
        bom=out/'assembly'/f'{prefix}_bom.csv'
        cli(['sch','export','bom','--exclude-dnp','--fields','Reference,Value,Footprint,MPN,Manufacturer,LCSC,QUANTITY,DNP',
             '--labels','Refs,Value,Footprint,MPN,Manufacturer,LCSC,Qty,DNP','--group-by','Value,Footprint,MPN',
             '-o',bom,BOARD.with_suffix('.kicad_sch')],'fab-bom.log')
        check_bom(bom)
        cli(['pcb','export','gerbers','--no-protel-ext','--subtract-soldermask','--use-drill-file-origin',
             '--layers','F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts',
             '-o',str(out/'gerbers')+'/',BOARD],'fab-gerber.log')
        cli(['pcb','export','drill','--format','excellon','--drill-origin','plot','--excellon-separate-th',
             '--generate-map','--map-format','gerberx2','-o',str(out/'drill')+'/',BOARD],'fab-drill.log')
        pos=out/'assembly'/f'{prefix}_pos.csv'
        cli(['pcb','export','pos','--format','csv','--units','mm','--side','both','--exclude-dnp',
             '--use-drill-file-origin','-o',pos,BOARD],'fab-position.log')
        # The two B3F switches are installed from the top and soldered separately.
        manual_refs={'SW1','SW2'}
        smt_bom=out/'assembly'/f'{prefix}_smt_bom.csv'
        smt_pos=out/'assembly'/f'{prefix}_smt_pos.csv'
        from jlc_assembly import references
        for source,target,field in ((bom,smt_bom,'Refs'),(pos,smt_pos,'Ref')):
            with source.open(newline='') as stream:
                reader=csv.DictReader(stream); fields=reader.fieldnames; rows=list(reader)
            seen=set()
            with target.open('w',newline='') as stream:
                writer=csv.DictWriter(stream,fieldnames=fields,lineterminator='\n');writer.writeheader()
                for row in rows:
                    refs=set(references(row[field]))
                    if refs & manual_refs:
                        if not refs <= manual_refs:raise ValueError('Mixed SMT/through-hole BOM group')
                        seen |= refs
                    else:writer.writerow(row)
            if seen != manual_refs:raise ValueError('Through-hole assembly coverage differs')
        convert(smt_bom,smt_pos,out/'assembly',prefix)
        (out/'assembly'/'manual_assembly.csv').write_text(
            'Refs,MPN,Qty,Method\n'
            '"SW1,SW2",B3F-4050,2,"Through-hole; body seated on PCB; solder after SMT; no wash"\n'
            'SW1 cap,B32-1200,1,"Ivory 9 mm; press onto plunger after soldering"\n'
            'SW2 cap,B32-1320,1,"Orange 12 mm; press onto plunger after soldering"\n')
        for side,layer in [('top','F'),('bottom','B')]:
            cli(['pcb','export','pdf','--layers',f'{layer}.Fab,{layer}.SilkS,Edge.Cuts',
                 *(['--mirror'] if side=='bottom' else []),'--mode-single','--scale','0','--black-and-white',
                 '--exclude-value','-o',out/'assembly'/f'assembly_{side}.pdf',BOARD],f'fab-{side}.log')
        cli(['sch','export','pdf','-o',out/'review'/'schematic.pdf',BOARD.with_suffix('.kicad_sch')],'fab-schematic.log')
        cli(['pcb','export','ipc2581','--units','mm','--compress','--bom-col-mfg-pn','MPN',
             '--bom-col-mfg','Manufacturer','--bom-col-dist-pn','LCSC','--bom-col-dist','LCSC',
             '-o',out/f'{prefix}_ipc2581.zip',BOARD],'fab-ipc2581.log')
        with ZipFile(out/f'{prefix}_gerbers.zip','w',ZIP_DEFLATED) as archive:
            for directory in ('gerbers','drill'):
                for path in sorted((out/directory).iterdir()): archive.write(path,path.relative_to(out))
        setup=ksexp.child(ksexp.load(BOARD),'setup')
        origin=tuple(float(v)for v in ksexp.child(setup,'aux_axis_origin')[1:])
        (out/'review'/'fabrication_audit.json').write_text(json.dumps(audit(out,geometry,prefix,origin),indent=2)+'\n')
        cli(['pcb','export','step','--force','--subst-models','--no-unspecified','--no-dnp',
             '-o',out/f'{prefix}_board.step',BOARD],'fab-step.log')
        step = out/f'{prefix}_board.step'
        step.write_text('\n'.join(line.rstrip() for line in step.read_text().splitlines()) + '\n')
        for name in ('erc.json','drc.json','geometry.json','report.json','temperature_audit.json','usb_audit.json','button_tolerances.json'):shutil.copy2(report/name,out/'review'/name)
        sources=[BOARD,BOARD.with_suffix('.kicad_pro'),BOARD.with_suffix('.kicad_dru'),
                 *BOARD.parent.rglob('*.kicad_sch'),*BOARD.parent.glob('*.kicad_sym'),
                 *BOARD.parent.glob('cicala.pretty/*.kicad_mod'),ROOT/'hardware/rev_b/contract.json']
        manifest={'status':'engineering prototype; physical qualification pending',
                  'inputs':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()for p in sources},
                  'outputs':{str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest()for p in out.rglob('*')if p.is_file()}}
        (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        destination=BOARD.parent/'exports';destination.mkdir(exist_ok=True)
        for path in out.iterdir():
            target=destination/path.name
            if path.is_dir():shutil.copytree(path,target,dirs_exist_ok=True)
            else:shutil.copy2(path,target)
        bundle=destination/f'{prefix}_engineering_prototype.zip'
        with ZipFile(bundle,'w',ZIP_DEFLATED) as archive:
            for path in sorted(out.rglob('*')):
                if path.is_file():archive.write(path,path.relative_to(out))
        (destination/f'{bundle.name}.sha256').write_text(hashlib.sha256(bundle.read_bytes()).hexdigest()+'  '+bundle.name+'\n')
        print(f'Exported checked engineering prototype: {bundle.relative_to(ROOT)}')


if __name__=='__main__':main()
