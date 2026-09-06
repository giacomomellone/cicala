# Board generation tools

The Rev A board is generated from the schematic rather than dragged by hand,
so it can be regenerated after a schematic change and reviewed as a diff.
The working board is not production-ready. Do not regenerate it in place
while reviewing a routing experiment.

## Checked candidate workflow

The current candidate workflow needs NumPy and Shapely 2 in system Python;
`requirements.txt` records the dependencies. KiCad's Python remains separate.
From `hardware/pcb/cicala_rev_a/`, run:

```sh
python3 ../tools/build_candidate.py /absolute/path/to/a/new/candidate-directory
```

The destination must not already exist. The command copies the board, project,
schematic hierarchy and local libraries, applies placement refinements and
critical routing seeds, routes the remaining nets, adds plane connections,
refills, and reports DRC and an independent copper audit. It does not replace
the working board or export fabrication files. It exits unsuccessfully if
DRC, connectivity or parity is nonzero. Review is still required after a pass.

`KICAD_CLI_BIN` and `KICAD_PYTHON_BIN` override discovery. macOS defaults use
the installed KiCad application; on Linux, choose a Python that imports
`pcbnew`. Matplotlib is optional for `audit_copper.py --plot`.

The candidate stages are `refine_placement.py`, `apply_project.py`, `apply_silkscreen.py`, `zones.py`,
`export_geom.py`, `critical_routes.py` (including `usb_pair.py`),
`route_fanout.py`, `finish_routes.py`, `ground_planes.py`, `apply_tracks.py`,
KiCad refill, geometry export and `stitch_islands.py`. `repair_routes.py` is
an explicit, scoped rip-up tool, not part of the automatic candidate command;
its output lists the other nets that must be reconnected.

Run the model regression tests with system Python:

```sh
python3 -m unittest discover -s ../tools -p 'test_*.py'
```

## Original placement pipeline

Run them in order from `hardware/pcb/cicala_rev_a/`:

| Step | Script | Interpreter | What it does |
| ---- | ------ | ----------- | ------------ |
| 1 | `kicad-cli sch export netlist` | — | the netlist every later step reads |
| 2 | `place.py` | KiCad's Python | places all footprints and sets the fields DRC's parity check compares |
| 3 | `apply_project.py` | system Python | writes net classes and design rules into the project |
| 4 | `zones.py` | system Python | writes the copper pours |
| 5 | `export_geom.py` | KiCad's Python | dumps pads, outline and rule areas as JSON |
| 6 | `route.py` | system Python (needs numpy) | legacy routing experiment; use the candidate workflow for new routes |
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

The continuous-geometry candidate router additionally accounts for:

- every physical pad independently, including duplicate pin numbers;
- oval pads as capsules, not ellipses;
- emitted via diameter and drill, including explicit fine-pitch overrides;
- all four outline edges, even when the grid stops at the outline;
- existing drills when adding another via, including same-net drills;
- actual pad layers and existing copper connectivity when joining components.

`unique_ids.py` assigns deterministic, unique UUIDs to cloned footprint items.
Duplicate UUIDs were the cause of misleading DRC object references in the
starting board. Continue to use the independent geometry audit; UUID repair
does not make a DRC result an electrical-layout review.

Filled-zone geometry is valid only for the exact tracks that KiCad refilled.
`stitch_islands.py` rejects a mismatched track set. Ordinary routing and the
first plane-tap pass ignore stale filled polygons. Pours retain isolated
islands (`island_removal_mode 1`); none are automatically deleted to clear a
report. The full zone outlines are retained.

## Electrical placement and routing seeds

`refine_placement.py` records all candidate moves. U1, J1, J2, J3, the switches,
U2, U6 and L1 retain their original anchors. U4 rotates 90 degrees and
moves from Y=43 to 42.4 so its USB input/output banks face their respective
routes, with courtyard clearance to J1. Q1 moves +0.25 mm in X to leave a
legal location for R20 beside its source. D3 rotates 180 degrees to put its
switch-node terminal toward Q1. U3 moves from (72,20) to (72,21.5), and L2
from (72,25) to (77.55,21.5), rotated 90 degrees. Its two terminals now face
the switch pins and both switch nets stay on B.Cu without vias. These anchor
changes are electrical changes, not enclosure changes.

The USB series resistors, shunt capacitors and inline test pads move beside
the module. Several nearby passives and test pads move to maintain courtyards;
R21/R22/C25/C26 move to B.Cu. The feedback divider moves beside U3. Charger
caps move toward U2, output caps toward U3, C15 toward L1 and R20 toward Q1.
The eight panel-rail capacitors form an ordered bank immediately above J2.
U6's input, output and slew capacitors move beside U6, with dedicated plane
connections. Test points and ordinary passives displaced by these changes
are recorded in the same placement table. No component values change.
These changes do not constitute completed regulator-loop or decoupling review.

`usb_pair.py` defines the coupled F.Cu path, the connector crossover and the
short B.Cu escapes. Its report separates planar main-path lengths from barrel
lengths; connector branch matching includes the two B.Cu-to-In2.Cu barrel
lengths. It excludes component interiors and does not model propagation delay.
The declared 0.29 mm
width/gap assumes 0.18 mm prepreg at er=4.5; a fabricator must confirm it.
The test pads lie on the signal path. In1.Cu is the long pair's reference;
separate In2.Cu ground regions reference the B.Cu escapes.

`critical_routes.py` reserves U3 feedback and the panel switch node on B.Cu
without vias, plus charger pad escapes and capacitor connections. The seeds
are audited against continuous copper geometry before maze routing. Generic
maze routes still need bend, stub, width, loop-area and via-count review.

Layout requirements were checked against the
[BQ25185 datasheet](https://www.ti.com/lit/ds/symlink/bq25185.pdf),
[TPS63802 datasheet](https://www.ti.com/lit/ds/symlink/tps63802.pdf), and
[Espressif PCB guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html).
Close capacitor placement, short feedback routing and a continuous USB
reference are requirements; a passing router does not verify their completion.

## Working-board checkpoint and local edits

The checked candidate has replaced the working `.kicad_pcb`. Its exact current
counts and limitations are in `../README.md` and `../REVIEW.md`. Full automatic
regeneration is still a candidate-producing experiment, not an exact replay
of the working board. Never run the legacy pipeline over it in place.

The final connections required scoped `repair_routes.py` calls followed by
`finish_routes.py`, including the EPD_VSL and USB_VBUS corridors and displaced
charger/panel signals. These connections are preserved in the board snapshot;
their maze-search history is not a placement contract. Ground taps and retained
islands were handled after refill. `stitch_islands.py --pads-only --fine-grid
--radius 5` concentrates on disconnected pads without trying to connect every
tiny island. Its input must match the filled board to within serialization's
one-nanometre quantization.

`recovery_routes.py` records the TC2030 contact-side rework. Its partial escapes
also enter the automatic critical seed. The shaded contact-area keepout and
0.508 mm foreign-copper clearance follow the manufacturer's drawing; do not
replace them with ordinary 0.2 mm signal clearance. Its rule file is part of
the KiCad project and must accompany any candidate copy.

`finish_power_taps.py` records the short CHG_STAT1 corridor adjustment needed
for R9's 3V3 via. Its `--u5-only` operation adds U5's dedicated ground return.
`thermal_relief.py` turns SW1's crowded thermal spokes 45 degrees without
changing width/gap; U5 uses a checked 0.2 mm trace to its own ground via
instead of an automatic pour thermal. That override refuses to apply without
the explicit return copper. `refine_fpc_junctions.py` replaces two single-layer FPC via
junctions with direct copper. These are placement-guarded local corrections,
not generic optimizers; re-review them after changing the named components.
`cleanup_dangling.py` uses DRC item UUIDs and independently checks every
physical pad's connectivity before deleting a reported stub. Refill and
repeat the check after cleanup; never delete a via solely because it is
reported as connected on only one layer.

## Appearance and independent review

`apply_silkscreen.py input.kicad_pcb output.kicad_pcb` applies native polygon
wordmark artwork and service legends using system Python. It is idempotent,
changes no copper/placement, and requires no installed font. The artwork is
grouped in KiCad. `assets/README.md` records its public font source and optional
regeneration dependencies. Do not replace it with an invented icon or raster.

`audit_placement.py` checks courtyards, the cell reserve and front-side part
restrictions. With `--board` and `--models-root`, it also identifies missing
3D bodies; pads without models are not missing schematic components.
`audit_copper.py` checks continuous copper clearance and track orientation.
`audit_reference.py filled.json routes.json report.json` checks the ground
reference under the explicit USB paths and samples ground-stitch coverage.
None of these replace DRC or the unfinished electrical/mechanical reviews.

`stitch_coverage.py` adds a bounded number of off-grid ground vias at measured
coverage gaps. Refill afterwards. It stops rather than violating pad/track
clearance, and a remaining distance above its target is not a passing result.
Track/via UUIDs emitted by `apply_tracks.py` are deterministic, so subsequent
local corrections do not needlessly change every other copper item's ID.
