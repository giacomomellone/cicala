# Rev B engineering validation

Validated locally on 16 September 2026 with KiCad 10.0.5, OpenSCAD 2026.06.12,
CadQuery 2.8.0, Shapely 2.1.2 and Gerbonara 1.6.3. This record covers the digital
engineering prototype. No integrated Rev B device has been built or powered.

## Results

- Native schematic ERC: zero violations. Schematic/PCB netlist parity matches.
- Filled-board DRC: **zero violations and zero unconnected items**. No DRC exclusions were added to clear the routing.
- Native front/back silkscreen includes Cicala branding, functional brackets, all 20 probe signals, connector polarity and a recovery/power guide. Text is at least 0.8 mm with 0.15 mm strokes and silk clearance. Pin labels match native nets; front text clears the fitted component envelopes. The right arm and switch footprints now follow the B3F-4050 drilling diagram; unrelated circuitry and the USB pair are retained.
- 115 footprints; 89 fitted electronic components: 87 SMT parts and two through-hole switches installed from above and soldered below. Bare diagnostic contacts also use the underside. The L-shaped board has 4360.22 mm² area.
- USB geometric D+ and D− paths are both approximately 71.806 mm, including board-layer transitions and excluding package interiors. Their computed difference is below 0.001 mm. Every checked outer data-trace centreline has adjacent ground, outside the defined 0.55 mm launch allowance around USB vias. Factory impedance confirmation and USB testing remain required.
- The selected 103JT thermistor table passes the static charge-veto tolerance model: nominal permission window 4.44–37.05 °C, positive stop margins at 0 and 45 °C. Actual cell temperature and bonded-sensor lag require measurement.
- 87 STEP component envelopes plus two explicit switch models. Placement/model, courtyard, mounting, concave outline, RF reserve and copper-to-edge checks pass.
- Purchased B32-1200/B32-1320 caps have 9 × 9 / 12 × 12 mm flat faces and direct B3F-4050 plunger seating. All 37 mechanical poses pass, covering assembly insertion, 0–0.5 mm travel, lateral alignment, switch leads, electronics, glass/flex and carrier clearances.
- The declared worst-case cap/print/assembly screen leaves 0.075 mm radial aperture clearance, 0.17 mm cap protrusion at maximum specified pretravel and 0.17 mm lead-to-base clearance. These are modeled margins; physical cap seating, solder fillets, force and return remain unmeasured.
- Export contains 14 native STL/3MF pairs: eight printed parts and six purchased-part references. Eight additional pairs are oriented for printing. Printed parts are closed, oriented single solids; the switch reference intentionally has a body and four separate terminal solids. Both cap reference meshes pass their flat-face area checks.
- Fabrication readback confirms the expected 11 graphic layers, including all four copper layers and the outline. It matches every drilled hole and slot to native geometry within 0.00051 mm. Full native BOM/positions cover 89 electronic references. JLC BOM/CPL cover the same 87 SMT references; the manual assembly CSV lists both switches and both caps. Gerber ZIP, Excellon files, assembly drawings, schematic PDF, IPC-2581 and STEP are packaged with source/output hashes.
- 22 Rev B helper tests and the required product-tool/website tests pass. The public website build and strict documentation build pass.
- The device page passes nine viewport/language combinations, with all eight CAD images loaded, both full-resolution PCB PNG links and both silkscreen SVG links checked. PCB views are straight orthographic exports with matching scale.
- Rev A still passes its enclosure, ERC, filled-board DRC, schematic parity and STEP-export checks after the folder reorganization.

The detailed electrical and fabrication reports are in `pcb/exports/review/`.
`pcb/exports/manifest.json` and `case/exports/manifest.json` identify the checked
sources and output hashes. The ZIP checksum is beside the prototype package.

## First-article qualification

The flex entry height and R1.55 bend remain assembly assumptions. The calculated
0.276 mm tail-length remainder does not establish fold durability. Check the
actual panel revision, insertion and latch access, battery pulse limits, NTC
bonding, USB operation, panel rails/refresh and radio behavior. The UC8253 firmware
port is still required before display bring-up.

Printed fits, component/PCB tolerances, lens supports, screw engagement, cap
travel/force/wear and battery clearance need physical checks. The magnetic
carrier is a made-to-drawing accessory concept that fails the prior iPhone 16 Plus dimensional fit;
retention, optical clearance, cases and attached RF are unqualified. Factory CAM
must confirm the stack, impedance, stencil and supported tab panel before the
prototype order. These are remaining physical/order checks, not missing digital
routing. Nothing was ordered or published by this work.

The purchased controls and soldering sequence are detailed in the [assembly guide](../../docs/hardware_rev_b.md#buttons-and-print-preparation). No custom printed caps, silicone strips, keepers or keeper screws are installed or exported as print jobs.
