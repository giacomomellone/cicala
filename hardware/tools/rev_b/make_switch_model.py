"""Original dimensional reference models from Omron B3F/B32 drawings.

The purchased cap exterior and socket are fit envelopes, not manufacturing CAD.
Requires CadQuery; no supplier CAD is redistributed.
"""
from pathlib import Path
import cadquery as cq

models = Path(__file__).resolve().parents[2] / 'rev_b/pcb/models'
body = cq.Workplane('XY').box(12, 12, 3.5, centered=(True, True, False))
stem = cq.Workplane('XY').circle(3.55).extrude(5.5).union(
    cq.Workplane('XY').box(3.8,3.8,7.3,centered=(True,True,False)))
assembly=cq.Assembly(name='B3F_4050')
assembly.add(body,name='body',color=cq.Color(.12,.13,.12))
assembly.add(stem,name='plunger',color=cq.Color(.88,.85,.73))
for i,x in enumerate((-6.25,6.25)):
    for j,y in enumerate((-2.5,2.5)):
        lead=cq.Workplane('XY').box(.3,1,7,centered=(True,True,False)).translate((x,y,-3.5))
        assembly.add(lead,name=f'lead_{i}_{j}',color=cq.Color(.68,.68,.65))
for i,y in enumerate((-4.5,4.5)):
    assembly.add(cq.Workplane('XY').circle(.8).extrude(-1.5).translate((0,y,0)),name=f'boss_{i}',color=cq.Color(.12,.13,.12))
assembly.save(str(models/'Omron_B3F-4050.step'))
connector = cq.Workplane('XY').box(13.7,4,1.2,centered=(True,False,False)).translate((0,-3.3,0))
cq.exporters.export(connector,str(models/'Molex_503480-2400_envelope.step'))
for path in (models/'Omron_B3F-4050.step',models/'Molex_503480-2400_envelope.step'):
    path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines())+'\n')
