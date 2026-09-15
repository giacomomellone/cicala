"""Original dimensional envelope; requires CadQuery, no supplier CAD redistribution."""
from pathlib import Path

import cadquery as cq

models = Path(__file__).resolve().parents[2] / 'rev_b/pcb/models'
body = cq.Workplane('XY').box(6.2, 6.2, 2.5, centered=(True, True, False))
stem = cq.Workplane('XY').circle(1).extrude(3.65)
cq.exporters.export(body.union(stem), str(models / 'Alps_SKRABCE010_envelope.step'))
# KiCad's footprint Y axis is opposite the STEP model Y axis.
connector = cq.Workplane('XY').box(13.7, 4, 1.2, centered=(True, False, False)).translate((0, -3.3, 0))
cq.exporters.export(connector, str(models / 'Molex_503480-2400_envelope.step'))
for name in ('Alps_SKRABCE010_envelope.step', 'Molex_503480-2400_envelope.step'):
    path = models / name
    path.write_text('\n'.join(line.rstrip() for line in path.read_text().splitlines()) + '\n')
