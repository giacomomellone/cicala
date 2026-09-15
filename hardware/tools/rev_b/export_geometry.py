"""Run with KiCad Python; retain outline-count evidence for the concave board."""
import hashlib
import json
from pathlib import Path
import runpy
import sys

import pcbnew

runpy.run_path(str(Path(__file__).resolve().parents[1] / 'pcb/export_geom.py'),
               run_name='__main__')
board = pcbnew.LoadBoard(sys.argv[1])
outlines = pcbnew.SHAPE_POLY_SET()
valid = board.GetBoardPolygonOutlines(outlines, False)
path = Path(sys.argv[2])
geometry = json.loads(path.read_text())
geometry['outline_count'] = outlines.OutlineCount()
geometry['outline_valid'] = bool(valid)
geometry['source_sha256'] = hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest()
path.write_text(json.dumps(geometry, indent=2) + '\n')
