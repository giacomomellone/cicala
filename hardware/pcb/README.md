# PCB — Rev A schematic baseline

The KiCad 10 project is under `kveld_rev_a/`. It contains the first complete hierarchical schematic and a validated mechanical constraint board. The schematic is an electrical review baseline, not a fabrication release. Hardware files use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The shared coordinate, stack-up, pin and validation contracts are in [docs/hardware_rev_a.md](../../docs/hardware_rev_a.md). `BOM.md` records the selected parts and the gates that remain before an assembly quote.

## What exists

- `kveld_rev_a.kicad_pro`: KiCad project metadata;
- `kveld_rev_a.kicad_sch`: root hierarchy with three functional sheets;
- `kveld_rev_a.kicad_pcb`: 78 × 45 × 1.2 mm four-layer board skeleton;
- `kveld_rev_a.kicad_sym`: project symbols for parts absent from the KiCad library;
- `pin_contract.csv`: firmware-to-schematic signal allocation;
- `renders/constraint_map.svg`: vector plot of mechanical datums and keep-outs;
- `renders/board_top.png`: empty-board 3D check;
- `exports/kveld_rev_a_board.step`: board outline for enclosure fit checks.

The PCB skeleton carries four Ø2.7 mm mechanical holes, a 2 mm corner radius, and user-layer envelopes for the panel, FPC, USB-C, switches, cap bores, status light, cell, gasket, ESP32-S3 module and service holes. Keep-out zones prohibit all copper and metal below the module antenna and prohibit bottom-side components in the 503035 cell volume.

All component outlines are mechanical datums. They are deliberately not electrical footprints. Replace each with a footprint reviewed against the current manufacturer drawing before placing copper.

## Hierarchy

| Sheet                   | Captured circuit                                                                 |
| ----------------------- | -------------------------------------------------------------------------------- |
| Power and USB           | USB-C protection and sensing, BQ25185 charger, protected cell path and TPS63802 |
| Controller and user I/O | ESP32-S3, EN/IO0, buttons, status LED, recovery header and production test pads |
| E-paper display         | GDEY0213B74 FPC, SSD1680 boost network and switched panel power                  |

Short local connections are drawn directly. Named global labels are reserved for sheet boundaries, controller or connector fan-out, shared rails and production test access.

Start capture from the power tree and recovery path. A battery-powered USB device needs both IO0 and EN access: plugging USB into a running unit is not a reliable reset action. Keep these pads concealed from normal use and reachable with the enclosure open.

## Layout constraints

- Retain the current firmware pins recorded in `docs/hardware_rev_a.md`.
- Prefer WROOM-1 for the close-range captive-portal use case if placement clears the module antenna keep-out. Keep WROOM-1U as the assembly fallback if the display, cell, buttons, copper or enclosure metal prevent that placement. Either variant requires assembled radio testing.
- Keep the steel ballast, cell, display and button hardware out of the antenna volume. Test the final assembled radio; a drawing cannot qualify it.
- Route GPIO19/20 as native USB over continuous ground. Set impedance from the selected four-layer fabricator stack before routing.
- Keep charger, cell and e-paper boost current loops short. Do not route their switching returns through the USB or antenna reference path.
- Put all development pads and current links on the underside. Maintain tool access with the base removed and prevent contact with the cell pouch.
- Keep the preliminary FPC connector centred at (42, 40) and USB-C at the front datum only until reviewed footprints replace the envelopes. Their current drawings have 0.5 mm between them, which is not a manufacturing clearance.
- Use a side-fire or light-guide-coupled bi-colour LED at the X=52 front-edge datum. Both firmware outputs low must leave no standing LED load.

## Validation

KiCad 10.0.5 currently reports zero schematic ERC violations and zero constraint-board DRC violations. STEP export also succeeds. Schematic-to-PCB parity is deliberately excluded until the open footprints are reviewed and components are placed; the current board contains only mechanical datums.

Run the same checks with:

```sh
just hw-pcb-check
```

Do not produce Gerbers until the schematic review, custom footprints, routed board, manufacturer stack, BOM, assembly drawing and enclosure interference check all pass their gates.
