# PCB — Rev A

The KiCad 10 project is `cicala_rev_a/cicala_rev_a.kicad_pro`. Open the project
so that the local symbols, footprints and USB-C STEP model resolve. Hardware
sources use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The revised board is a prototype design with 119 footprints on a
78 × 49 × 1.2 mm, four-layer PCB. The display connector and boost circuit are
on top; the controller, USB and battery power circuits are underneath.
The selected battery sits below the board. The enclosure is 84 × 56 × 24.05 mm.

The checked board has 1949 track segments and 474 vias, zero DRC violations,
zero unconnected items, and no independent copper or placement conflicts.
The schematic has five pages, including an independent charge-temperature
guard. Physical operation and printed fit still need prototype testing.
Read [REVIEW.md](REVIEW.md) for the evidence and acceptance procedure.

## Sources and outputs

| Path under `cicala_rev_a/` | Purpose |
| --- | --- |
| `cicala_rev_a.kicad_sch`, `sheets/` | System diagram, power/USB, controller, display, temperature guard |
| `cicala_rev_a.kicad_pcb` | Authoritative placement, copper and mechanical outline |
| `cicala_rev_a.kicad_sym`, `cicala.pretty/` | Local symbols and manufacturer-specific lands |
| `models/` | Six original dimensional STEP models and their sources |
| `exports/assembly/` | Assembly BOM, placement files and both assembly drawings |
| `exports/cicala_rev_a_gerbers.zip` | Gerbers and plated/non-plated drills for fabrication |
| `exports/cicala_rev_a_board.step` | Fitted board for mechanical integration |
| `exports/review/` | Schematic, board renders and validation reports |
| `pin_contract.csv` | Firmware GPIO allocation |

[BOM.md](BOM.md) records the selected circuit and off-board parts.
[MANUFACTURING.md](MANUFACTURING.md) specifies the JLCPCB prototype handoff.
The enclosure contract is [docs/hardware_rev_a.md](../../docs/hardware_rev_a.md).

Run `just hw-pcb-check` to check ERC, critical pin mappings and lands, DRC,
schematic parity and STEP export. Run `bash hardware/pcb/export_fab.sh` to
regenerate the manufacturing files. The export rejects unresolved assembly
MPNs. Manufacturer component availability and the placement preview must be
reviewed when preparing an order.

The [routing tools](tools/README.md) can produce experimental candidates.
They are not an exact replay of this board; keep the checked KiCad source
and never regenerate it in place with the historical placement pipeline.
