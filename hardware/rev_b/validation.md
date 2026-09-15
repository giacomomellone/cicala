# Rev B engineering validation

Validated locally on 15 September 2026 with KiCad 10.0.5, OpenSCAD 2026.06.12,
CadQuery 2.8.0, Shapely 2.1.2 and Gerbonara 1.6.3. This record covers the digital
engineering prototype. No integrated Rev B device has been built or powered.

## Results

- Native schematic ERC: zero violations. Schematic/PCB netlist parity matches.
- Filled-board DRC: **zero violations and zero unconnected items**. No DRC exclusions were added to clear the routing.
- 115 footprints; 89 fitted components, all assembled on top. Bare diagnostic contacts also use the underside. The L-shaped board has 3740.22 mm² area.
- USB geometric D+ and D− paths are both approximately 71.806 mm, including board-layer transitions and excluding package interiors. Their computed difference is below 0.001 mm. Every checked outer data-trace centreline has adjacent ground, outside the defined 0.55 mm launch allowance around USB vias. Factory impedance confirmation and USB testing remain required.
- The selected 103JT thermistor table passes the static charge-veto tolerance model: nominal permission window 4.44–37.05 °C, positive stop margins at 0 and 45 °C. Actual cell temperature and bonded-sensor lag require measurement.
- 87 STEP component envelopes plus two explicit switch models. Placement/model, courtyard, mounting, concave outline, RF reserve and copper-to-edge checks pass.
- Flat 8 × 14 / 8 × 18 mm caps have 1.4 mm rounded end tabs, screwed keepers and captured 0.5 mm silicone strips. Motion and insertion checks include the nominal stroke, extreme relief/stop variants, lateral-play corners, keeper screws and both retention directions. Lens/flex clearance and carrier fasteners are included. The contact-area check rejects sloping button faces. All exported STL meshes must be closed, oriented single solids; the print variants are translated to the bed.
- The button displacement audit covers 192 combinations of stack, stop and switch-travel assumptions. Each has a graded cap/keeper selection; fixed nominal parts do not guarantee actuation. Actual force, pad compression, fit and wear remain physical checks.
- Fabrication readback confirms the expected 11 graphic layers, including all four copper layers and the outline. It matches every drilled hole and slot to native geometry within 0.00051 mm. The generated assembly BOM and CPL cover exactly the same 89 fitted references. Gerber ZIP, Excellon files, assembly drawings, schematic PDF, IPC-2581 and STEP are packaged with source/output hashes.
- 15 Rev B helper tests, 54 shared PCB helper tests, 97 product-tool tests and 99 website tests pass. The public website build and strict documentation build pass.
- The device page passes nine viewport/language combinations, with all eight CAD images loaded and both full-resolution PCB PNG links checked. The enclosure export contains 65 native STL/3MF pairs and 60 additional print-oriented pairs, including graded coupons.
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
carrier is a made-to-drawing accessory concept with a nominal large-phone fit;
retention, optical clearance, cases and attached RF are unqualified. Factory CAM
must confirm the stack, impedance, stencil and supported tab panel before the
prototype order. These are remaining physical/order checks, not missing digital
routing. Nothing was ordered or published by this work.

The graded parts and selection procedure are detailed in the [print and assembly guide](../../docs/hardware_rev_b.md#buttons-and-print-preparation). The former thin-flange design has been replaced; factory acceptance and physical durability are still unqualified.
