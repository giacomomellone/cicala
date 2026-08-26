# PCB — Rev A schematic baseline

The KiCad 10 project is under `cicala_rev_a/`. It contains the first complete hierarchical schematic and a validated mechanical constraint board. The schematic is an electrical review baseline, not a fabrication release. Hardware files use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The shared coordinate, stack-up, pin and validation contracts are in [docs/hardware_rev_a.md](../../docs/hardware_rev_a.md). `BOM.md` records the selected parts and the gates that remain before an assembly quote.

## What exists

- `cicala_rev_a.kicad_pro`: KiCad project metadata;
- `cicala_rev_a.kicad_sch`: root hierarchy with three functional sheets;
- `cicala_rev_a.kicad_pcb`: 78 × 45 × 1.2 mm four-layer board skeleton;
- `cicala_rev_a.kicad_sym`: project symbols for parts absent from the KiCad library;
- `pin_contract.csv`: firmware-to-schematic signal allocation;
- `renders/constraint_map.svg`: vector plot of mechanical datums and keep-outs;
- `renders/board_top.png`: empty-board 3D check;
- `exports/cicala_rev_a_board.step`: board outline for enclosure fit checks.

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

## Schematic review TODO

The following items were found during the Rev A functional and readability review. Checked items are resolved design decisions; unchecked items remain gates for schematic approval.

### Clock decision

- [x] Do not add an external 40 MHz crystal. The selected ESP32-S3-WROOM-1/1U module contains its required 40 MHz crystal.
- [x] Do not fit a 32.768 kHz RTC crystal by default. It is optional for this product; revisit only if deep-sleep timing accuracy or measured sleep power requires it.

### Electrical blockers

- [ ] Correct the TPS22917 soft-start capacitor C14: connect its CT-side capacitor return to U6 VIN/3V3, not GND, following the device reference circuit.
- [ ] Correct the VBUS-sense divider so R5, from VBUS to the sense node, is 100 kΩ and R6, from the sense node to GND, is 150 kΩ. The present 150 kΩ/100 kΩ arrangement produces only 2.0 V from 5 V, below the ESP32-S3 guaranteed high-input threshold at 3.3 V.
- [ ] Prevent an unpowered e-paper panel from being back-powered through its logic pins. Drive or park every MCU-driven panel input low or high-impedance before deasserting `EPD_PWR_EN`, and preserve that state while `EPD_3V3` is off; add isolation if firmware cannot guarantee the sequence.
- [ ] Decide and document the USB-C shield connection near the receptacle. Review a direct, 0 Ω configurable, or RC connection to board ground against the enclosure and EMC/ESD plan instead of leaving SHIELD explicitly unconnected without rationale.

### Hardware and firmware integration

- [ ] Add a Rev A firmware board configuration that controls GPIO2 `VBAT_SENSE_EN` and GPIO5 `EPD_PWR_EN`, including safe reset and deep-sleep states.
- [ ] Update battery-voltage conversion for the 1 MΩ/470 kΩ divider to the exact multiplier 147/47 (approximately 3.1277), replacing the breadboard 2/1 value.
- [ ] After enabling the battery divider, allow its 100 nF filter to settle before sampling. Its approximately 32 ms time constant requires about 160 ms for 1% settling unless the filter or measurement method is changed.
- [ ] Define and test the e-paper power-up and power-down sequence, including panel reset, bus-pin parking, and discharge/restart behaviour.

### Testability and drawing readability

- [ ] Complete the production-test contract in `docs/hardware_rev_a.md`: expose USB D+/D−, the display bus, charger status, and removable current-measurement links in addition to the existing power, UART, IO0/EN, and regulator-status access.
- [ ] Replace generic connector J2 with a project-specific GDEY0213B74 symbol carrying semantic signal names and correct electrical pin types so ERC can catch interface errors.
- [ ] Turn the root sheet into a real system block diagram with hierarchical sheet pins for power, USB, SPI, control, and status. Show the power flow explicitly as USB/BAT → BQ25185 SYS → TPS63802 → 3V3 → panel load switch, and reduce passive global-label fan-out.
- [ ] Tighten excess whitespace on the controller sheet and visually associate each decoupling group with the device or rail it serves.
- [ ] Hide visible `#FLG`/`PWR_FLAG` references and add a consistent `PRELIMINARY — NOT FOR FABRICATION` title-block note and sheet date on every page.
- [ ] Consider reserving 0 Ω or 22–47 Ω series-tuning footprints on e-paper CLK and MOSI near the controller for signal-integrity and EMC adjustment.

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
