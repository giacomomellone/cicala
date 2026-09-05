# Board generation tools

The Rev A board is generated from the schematic rather than dragged by hand,
so it can be regenerated after a schematic change and reviewed as a diff.
These scripts are the source of the layout; `cicala_rev_a.kicad_pcb` is their
output.

Run them in order from `hardware/pcb/cicala_rev_a/`:

| Step | Script | Interpreter | What it does |
| ---- | ------ | ----------- | ------------ |
| 1 | `kicad-cli sch export netlist` | — | the netlist every later step reads |
| 2 | `place.py` | KiCad's Python | places all footprints and sets the fields DRC's parity check compares |
| 3 | `apply_project.py` | system Python | writes net classes and design rules into the project |
| 4 | `zones.py` | system Python | writes the copper pours |
| 5 | `export_geom.py` | KiCad's Python | dumps pads, outline and rule areas as JSON |
| 6 | `route.py` | system Python (needs numpy) | routes the signals |
| 7 | `stitch.py` | system Python | ground stitching vias |
| 8 | `apply_tracks.py` | system Python | writes the tracks and vias into the board |

Two interpreters are needed because KiCad's bundled Python has `pcbnew` but no
numpy, and the routing is numpy work. `apply_project.py` runs after any step
that saves the board through `pcbnew`, because saving a board rewrites the
project file.

## Placement

Anchors — the module, the connectors, the switches, the power column, the
e-paper boost loop and the pins that can only leave their package in one
direction — are positioned by hand in `place.py`. Everything else is packed
into free regions, each part taking the free slot nearest the pads it already
shares a net with. Legality is checked against the same polygons DRC uses:
courtyard collision, board outline, rule areas and hole spacing.

## Routing

`route.py` is a grid maze router on a 0.075 mm grid over F.Cu, In2.Cu and
B.Cu; In1.Cu is left as an unbroken ground plane. It routes each net by
joining its pads closest-pair-first, with hard blocking, so clearance is a
property of the search rather than something checked afterwards. Several
things it has to get right and did not at first, recorded because they are
easy to get wrong again:

- copper is modelled as grid cells, so its edge can be half a cell beyond the
  outermost cell centre: every clearance radius carries that slack;
- DRC resolves a clearance as the larger of the two objects' rules, so the
  router plans for the widest rule on the board, not the net's own;
- a pad that exists only on B.Cu is not reached by arriving above it on F.Cu,
  so targets carry their layer;
- a shortcut may not reach across a layer change, or it deletes the via;
- mechanical and shield pads carry no net but are still copper.

The router is not finished: see the status note in `../README.md`.
