# PCB — Rev A foundation

The KiCad 10 project is under `kveld_rev_a/`. It contains a validated mechanical board and empty hierarchical schematic sheets. It is a capture starting point, not a circuit or fabrication release. Hardware files use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The shared coordinate, stack-up, pin and validation contracts are in [docs/hardware_rev_a.md](../../docs/hardware_rev_a.md). `BOM.md` is the planning list for parts that will populate the hierarchy.

## What exists

- `kveld_rev_a.kicad_pro`: KiCad project metadata;
- `kveld_rev_a.kicad_sch`: root hierarchy with seven subsystem sheets;
- `kveld_rev_a.kicad_pcb`: 78 × 45 × 1.2 mm four-layer board skeleton;
- `pin_contract.csv`: firmware-to-schematic signal allocation;
- `renders/constraint_map.svg`: vector plot of mechanical datums and keep-outs;
- `renders/board_top.png`: empty-board 3D check;
- `exports/kveld_rev_a_board.step`: board outline for enclosure fit checks.

The PCB skeleton carries four Ø2.7 mm mechanical holes, a 2 mm corner radius, and user-layer envelopes for the panel, FPC, USB-C, switches, cap bores, status light, cell, gasket, ESP32-S3 module and service holes. Keep-out zones prohibit all copper and metal below the module antenna and prohibit bottom-side components in the 503035 cell volume.

All component outlines are mechanical datums. They are deliberately not electrical footprints. Replace each with a footprint reviewed against the current manufacturer drawing before placing copper.

## Hierarchy

| Sheet                | First capture pass                                                           |
| -------------------- | ---------------------------------------------------------------------------- |
| USB and protection   | receptacle, CC pull-downs, ESD, VBUS input and sense                         |
| Battery charger      | BQ25185 candidate, cell connector, NTC, limits, STAT1/2                      |
| 3V3 regulation       | XC6220, rail measurement link and decoupling                                 |
| ESP32-S3 core        | WROOM-1U baseline or gated WROOM-1 option, EN, IO0, native USB and strapping |
| E-paper interface    | FPC, SSD1680 boost network, power gate and discharge state                   |
| Controls and status  | KSC321G switches, side-fire bi-colour LED and leakage rules                  |
| Programming and test | IO0/EN, UART0, rails, display bus and current links                          |

Start capture from the power tree and recovery path. A battery-powered USB device needs both IO0 and EN access: plugging USB into a running unit is not a reliable reset action. Keep these pads concealed from normal use and reachable with the enclosure open.

## Layout constraints

- Retain the current firmware pins recorded in `docs/hardware_rev_a.md`.
- Treat the full WROOM-1 rectangle as a conflict study, not an accepted placement. Its PCB-antenna clearance overlaps the current display envelope. WROOM-1U is the baseline after its external antenna, connector and cable route are qualified. WROOM-1 stays open only for a placement that clears copper, display, cell, button hardware and enclosure metal by the module guidance.
- Keep the steel ballast, cell, display and button hardware out of the antenna volume. Test the final assembled radio; a drawing cannot qualify it.
- Route GPIO19/20 as native USB over continuous ground. Set impedance from the selected four-layer fabricator stack before routing.
- Keep charger, cell and e-paper boost current loops short. Do not route their switching returns through the USB or antenna reference path.
- Put all development pads and current links on the underside. Maintain tool access with the base removed and prevent contact with the cell pouch.
- Keep the preliminary FPC connector centred at (42, 40) and USB-C at the front datum only until reviewed footprints replace the envelopes. Their current drawings have 0.5 mm between them, which is not a manufacturing clearance.
- Use a side-fire or light-guide-coupled bi-colour LED at the X=52 front-edge datum. Both firmware outputs low must leave no standing LED load.

## Validation

KiCad 10.0.5 currently reports zero ERC violations, zero PCB DRC violations, zero unconnected items, and zero schematic-parity issues. Those results mean the hierarchy and constraint board are structurally valid. The empty sheets do not prove an electrical design.

Run the same checks with:

```sh
just hw-pcb-check
```

Do not produce Gerbers until the schematic, reviewed footprints, routed board, manufacturer stack, BOM, assembly drawing and enclosure interference check all pass their gates.
