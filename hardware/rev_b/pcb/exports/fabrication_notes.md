# Cicala Rev B.05 — engineering prototype fabrication notes

Reviewed 16 September 2026. This package supports a first-article fabrication and
assembly review. It is not a production release. No factory has accepted the
stack, tooling, component sourcing or assembly process, and no integrated unit
has been powered. Do not substitute parts or change copper without approval.

## Board order

| Setting | Required specification |
| --- | --- |
| Board | One design; routed L outline, 113 × 62 mm bounding box; dimensions from Edge.Cuts |
| Material | FR-4, four copper layers, nominal 1.2 mm finished board; maximum 1.33 mm reserved |
| Copper | 1 oz finished outer; 1 oz inner; no blind/buried vias or filled via-in-pad |
| Finish | ENIG, lead-free process; green solder mask, white legend on both sides |
| Quality | IPC Class 2 acceptance; electrical open/short test on every bare board; AOI after SMT |
| Geometry | 0.20 mm general track/space; 0.15 mm fine-pitch escape width; local 0.15 mm U7 pad spacing |
| Holes | Standard vias Ø0.30/0.60 mm drill/land; use separate plated/non-plated drill files |
| Switch holes | SW1/SW2: Ø1.20 ±0.05 mm **finished PTH**, Ø1.80 ±0.05 mm NPTH; preserve the 12.5 × 5 mm terminal grid and 9 mm boss pitch |
| Edge | Routed, R1 concave corner, 0.50 mm minimum copper clearance; no V scoring through the L |
| Mask | Follow supplied masks; keep probe contacts exposed; do not fill or cap test-contact vias |
| Panel | Factory-designed tab-routed support, ≥5 mm handling rails and 2 mm route channels; approval drawing before manufacture |
| Tooling | Fiducials and tooling holes on removable rails; support both arms during assembly and depanelization |
| Marking | Keep factory order marks on removable rails; preserve Cicala and all diagnostic labels |

PCBWay's published standard PTH tolerance is ±0.08 mm, which is wider than the
switch drawing. Obtain written acceptance of the specified **±0.05 mm** switch
holes. NPTH tolerance is also explicit. A nominal drill file does not request a
tighter finished-hole tolerance by itself.
[PCBWay capabilities](https://www.pcbway.com/capabilities.html).

Keep tabs away from the module antenna, USB mouth, switch apertures, mounting
holes and reentrant corner. Factory-depanelize and deburr before shipment; no
remaining tabs may intrude into the enclosure fit. Return the panel drawing for
review. [PCBWay assembly panel requirements](https://www.pcbway.com/pcb_prototype/Panel_Requirements_for_Assembly.html).

## Controlled impedance

Target **90 Ω differential, ±10%**, for the USB main pair on B.Cu, referenced to
the designated In2.Cu ground corridor. Main geometry is 0.28 mm trace width and
0.24 mm edge gap. Local launches differ; use the complete native board for review.
Keep the reference corridor continuous and the return vias intact.

Calculation basis: PCBWay's published four-layer 1.2 mm entry 17, 0.1855 mm pressed
outer dielectric, Dk 4.74, 1 oz inner/outer copper. The published entry lists
0.630 mm core and 1.21 mm finished thickness. KiCad's normalized 0.689 mm core only
makes the CAD substrate total 1.2 mm; it is **not** a factory stack instruction.
Confirm the actual laminate, pressed thicknesses, solder mask and required width/gap
before releasing CAM. Request an impedance coupon and measured report. If the
proposed stack requires different geometry, return it for layout review instead
of scaling or silently editing the tracks.
[PCBWay stack table](https://www.pcbway.com/multi-layer-laminated-structure.html).

## Assembly scope and files

Install **87 SMT components on the top face and two Omron B3F-4050 through-hole
switches from the top**. The full native BOM/position files cover all 89 electronic
references. `cicala_rev_b_smt_bom.csv` and `cicala_rev_b_smt_pos.csv` isolate the SMT
stage; the JLC BOM/CPL also covers only these 87 parts. `manual_assembly.csv`
specifies the switches and two caps. Include its operations in the quote; an SMT
upload alone is not a complete assembly order.

Use exact manufacturer part numbers. Blank LCSC fields mean sourcing is required,
not that a part is optional. The caps, display, protected cell, NTC harness, lens,
plastics and fasteners are additional device parts, not SMT placements. Request
B32-1200 ivory and B32-1320 orange caps packed separately for enclosure assembly.

- Inspect USB connector position, Molex FPC pin 1/entry direction, diode polarity,
  module antenna overhang and U2/U3 exposed pads against the assembly drawings.
- Use a 0.10 mm laser-cut stainless stencil as the initial process proposal. The
  assembler must approve aperture volumes, paste and reflow profile for the mixed
  package sizes. Inspect hidden U2/U3 joints by X-ray; ask for the inspection record.
- Complete SMT reflow and cleaning **before** adding B3F switches. The switches
  are unsealed and must not be washed. Follow Omron's permitted soldering process.
- Seat switch bodies on the PCB; support the board during soldering and later cap
  fitting. The total lead/fillet projection below B.Cu must be **≤2.60 mm**. Trim
  where needed, remove clippings and inspect for shorts. This preserves at least
  0.40 mm modeled clearance to the base under the declared assembly assumptions.
- SW1 is Filters at upper right: ivory **9 × 9 mm B32-1200**. SW2 is Next at lower
  right: orange **12 × 12 mm B32-1320**. Fit caps only after soldering. Released
  contacts must be open and pressed contacts closed; inspect both duplicate pairs.

[Omron B3F process restrictions](https://omronfs.omron.com/en_US/ecb/products/pdf/en-b3f.pdf),
[Omron B32 cap drawing](https://omronfs.omron.com/en_US/ecb/products/pdf/en-b32.pdf).

No programming or functional-test binary is included. The Rev B UC8253 firmware
port remains outstanding. Optical/X-ray inspection and an unpowered continuity
test do not certify operation of the device.

## Enclosure print order

Use the four parts in `case/exports/print/core/`: base, top shell, display
frame and support bar. Select either STL or 3MF for each part, not both.
Optional accessories and the temporary flex-forming jig
are separate. Never print `reference/` files. STL/3MF units are mm; do not scale,
mirror or auto-thicken parts. Native assembly meshes use X right, Y up, matching
KiCad STEP after translating the PCB upward by 4.6 mm. Print files are separately
rotated and placed on the bed.

For the first dimensional sample, request **SLA Somos Ledo tough resin, natural
colour, no paint or dimensional coating**, with support removal and full cure.
Ask for explicit acceptance of the 1.4 mm shell walls, 1.2 mm base and 0.8 mm display
supports, and an inspection of the button openings and PCB seats. Local dimensional
assumptions are tighter than a generic whole-part percentage tolerance; they must
be confirmed, measured or compensated. Thin supports and pilot threads need a
sample before selecting a final process/material. SLS/MJF is not approved merely
because it is stronger; the service's wall and tolerance rules must be met.
[PCBWay SLA service](https://www.pcbway.com/rapid-prototyping/3d-printing/3d-printing-SLA.html),
[JLC3DP design guidelines](https://jlc3dp.com/help/article/3d-printing-design-guideline).

Home fit trials may use PETG, 0.4 mm nozzle, 0.15 mm layers, three or more walls;
inspect slicer paths through thin features. Keep support scars off the PCB seats,
lens seat and button wells. Do not force a screw into an undersized printed pilot.

The housing is **128 × 66 × 16.8 mm**. Cap faces are nominally at z=15.8 mm,
**1.0 mm recessed**, in 10 × 10 and 13 × 13 mm throats with 1.4 mm flared mouths.
Four M2.5 × 10 mm countersunk screws close the core; check thread engagement and
head seating on the empty sample. Electronics, lens and cell are installed later.

## Release conditions

CAM acceptance must include the stack/impedance, switch-hole tolerances, panel,
exact BOM/placement, stencil, no-wash switch sequence and printed critical fits.
The first articles then need power/recovery, display/flex, battery/NTC/current,
USB/RF, enclosure and button force/return measurements. The optional phone carrier
fails the iPhone 16 Plus dimensional screen and is not part of a validated product.
