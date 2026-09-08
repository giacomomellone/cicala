# PCB review and candidate tools

The checked `../cicala_rev_a/cicala_rev_a.kicad_pcb` is authoritative. These
scripts support review and isolated routing candidates. The historical
placement pipeline is not an exact replay of the current board; do not run
it over the working source. No script authorizes production release.

## Checks and exports

`just hw-pcb-check` runs KiCad ERC, netlist/pin/land checks, filled DRC,
schematic parity and populated STEP export. `../export_fab.sh` also requires
exact assembly MPNs and creates Gerber/drill ZIPs, BOMs, placement files and
both assembly drawings. `jlc_assembly.py` expands KiCad reference ranges,
converts the headers and requires identical fitted-reference coverage in the
BOM and CPL. It preserves explicit top/bottom placement and the common origin.

Run tool regressions with Python that has NumPy and Shapely 2:

```sh
python3 -m unittest discover -s hardware/pcb/tools -p 'test_*.py'
```

Run the complete source review with `just hw-review`. `CICALA_HW_PYTHON`
selects the analysis interpreter; `KICAD_PYTHON_BIN` selects KiCad's Python.
Pass `--app build/cicala-rev-a/app/zephyr --boot build/cicala-rev-a/mcuboot/zephyr`
after the report-directory argument to compare compiled firmware settings too.
The placement/copper/USB commands return a failing exit status on violations.

KiCad Python and analysis Python may be separate installations. On macOS,
KiCad supplies `pcbnew` in its bundled interpreter. CadQuery and Gerbonara
are optional analysis dependencies; install them in a separate environment.
`requirements.txt` contains the routing dependencies.

| Tool | Input / purpose |
| --- | --- |
| `export_geom.py` | KiCad Python: board → physical pads, drills, courts, tracks, filled polygons |
| `audit_electrical.py` | Netlist and project → critical pin mapping, bypass, feedback, LED and land checks |
| `audit_temperature.py` | Netlist → static NTC/comparator tolerance model |
| `audit_copper.py` | Geometry JSON → continuous clearance, edge, via-in-pad and track-angle audit |
| `audit_placement.py` | Geometry + board/models → courts, through-hole/body interference, fitted models |
| `audit_reference.py` | Candidate USB paths → reference/stitch analysis; not a measurement of current routing |
| `audit_usb.py` | Actual board → path lengths including vias, branch mismatch and In1 reference continuity outside launches |
| `check_rev_a_contract.py` | PCB/case datums and compiled app/MCUboot → connector, GPIO, flash, PSRAM and display agreement |
| `package_prototype.py` | Dated public working-source/export snapshot with archive and per-file SHA256 verification |
| `audit_fabrication.py` | Gerber ZIP + geometry → eleven layers and every drill/slot read back independently; needs Gerbonara |
| `check_assembly_bom.py` | Reject blank, unresolved or multiple MPNs |
| `make_component_models.py` | CadQuery → six original STEP envelopes; drawings in `../cicala_rev_a/models/README.md` |
| `apply_silkscreen.py` | Native cicada/wordmark/service legends and fab references; `--mark-only` preserves existing legends |
| `make_cicada.py` | Public SVG → committed native stroke geometry for the cicada silkscreen |

The fabrication audit accounts for Excellon's 1 µm coordinate rounding.
It also opens the compressed IPC-2581 exchange and checks its XML document;
a plain XML file with a `.zip` extension fails the audit.
The enclosure's `../../case/export_component_bounds.py` exports 87 fitted
component envelopes from STEP and fingerprints placement/model changes.
The case checks reject stale envelopes. Generic capacitor height is bounded
at 1.5 mm; the enclosure contains detailed switch references.

## Candidate routing

`build_candidate.py` creates an isolated candidate directory. Its historical
placement and critical routing stages predate the current top-side display
and temperature guard. Updating those stages requires a new placement and
mechanical review; they must not be represented as a reproducible release.

`route_fanout.py` and `finish_routes.py` route on F.Cu, In2.Cu and B.Cu,
leaving In1 as ground. They model every physical pad, oval drills, no-net
mechanical copper, outlines, keepouts and via drills. Surface escapes stay
on the pad's actual layer. `repair_routes.py` permits explicit scoped rip-up
of ordinary copper while preserving the supplied critical seed. Its displaced
nets must be reconnected and checked.

`usb_pair.py` records the dedicated USB geometry, connector crossover and
inline test points. Its main F.Cu width/gap is 0.32/0.26 mm for the selected
JLC04121H-7628 stack. Short escapes remain 0.29 mm or the explicit fine-pitch
neck width. The current board's connector mismatch including the new barrel
spacing is about 0.041 mm. Generated candidate compensation can differ;
audit the actual board rather than treating a seed report as a measurement.

The recovery keepout is derived from J4's transformed physical contacts,
including rotation. The TC2030 shaded contact region and 0.508 mm foreign
copper rule must accompany candidates. U7's package land-to-land clearance
is 0.15 mm; this exception does not relax routed copper clearance.

`ground_planes.py`, `stitch_islands.py` and `stitch_coverage.py` add checked
plane connections. Filled polygons apply only to the exact track set KiCad
refilled. Never route using stale filled polygons. `stitch_islands.py` rejects
a mismatched input set. Unconnected-zone island removal does not repair an
unconnected pad.

`cleanup_dangling.py` removes only DRC-identified items whose deletion preserves
every physical pad group. Refill and repeat; deleting one stub can expose the
next. Some apparent dangling vias are single-layer copper junctions and need
replacement with proper trace junctions before deletion. Never suppress a
DRC warning merely because the router finished.

`unique_ids.py` assigns deterministic unique IDs to cloned footprint items;
`apply_tracks.py` emits stable IDs for copper. Appearance uses native polygon
artwork from `assets/cicala-wordmark.json` and stroke geometry from
`assets/cicala-mark.json`; their sources and regeneration are documented in
`assets/README.md`.
