# Rev A prototype manufacturing handoff

Prepare four assembled prototypes. If the fabrication service requires five
bare PCBs, assemble four and retain the fifth as a spare. These are engineering
prototypes; no hardware bench test or printed fit test has yet been completed.
No order has been placed and no files have been uploaded to a supplier.

## PCB and assembly settings

| Setting | Requirement |
| --- | --- |
| Finished board | 78 × 49 mm, routed outline, 1.2 mm nominal FR-4 |
| Layers | F.Cu signals; In1.Cu ground; In2.Cu 3V3/signals/local USB ground; B.Cu signals |
| Stack | JLC04121H-7628, outer 1 oz, inner 0.5 oz |
| Published dielectric | 0.2104 mm 7628 prepreg each side; 0.6 mm core |
| Finish | ENIG; lead-free assembly; black mask, white legend |
| Via process | Ordinary plated through vias, minimum drill 0.20 mm; preserve supplied masks and thermal-hole treatment below |
| Assembly | Standard PCBA, both sides; exclude all DNP/bare-contact features |
| USB | 90 Ω differential target; main F.Cu pair 0.32 mm wide, 0.26 mm gap |
| Inspection | AOI; X-ray the BQ25185 bottom-terminated joints and inspect U3's PGND/paste lands |

The layer construction comes from JLCPCB's
[published stackup selector](https://jlcpcb.com/impedance), queried for 4 layers,
1.2 mm, 1 oz outer and 0.5 oz inner on 7 September 2026. Its dielectric figures
are a process construction; finished thickness follows the ordered nominal
thickness and factory tolerance. The enclosure uses a 1.2 mm nominal PCB.

The USB width/gap has been adjusted to this stack. A first-order uncoated
microstrip estimate is about 90 Ω; solder mask and etched conductor shape
require the fabricator's impedance calculation. Specify the 90 Ω target and
have CAM confirm the geometry before approving fabrication. Short connector,
ESD and module escapes retain their fine-pitch geometry. Never change the
layer order or route across In1 ground gaps.

The 474 separately routed vias are tented on both faces. U1 has twelve and
U2 has two intentional Ø0.20 mm thermal holes within their exposed-pad lands.
Those holes are tented on F.Mask and open into the component-side land on
B.Mask. Preserve that mask treatment and review the exposed-pad stencil/joints;
do not convert them to mask openings on both faces. No filled/capped via
process is specified. Include U1/U2 exposed-pad solder quality in inspection.

The board is below the 70 × 70 mm minimum for JLCPCB Standard assembly.
Use **Panel by JLCPCB / edge rails**, with a routed carrier at least 88 × 75 mm.
Have the factory add tooling holes and fiducials on the removable rails.
Keep USB/JST overhang and the left antenna region free. Locate breakaway tabs
away from connector mouths and mounting-hole bearing surfaces; depanelize and
finish tabs flush to the supplied board outline. Do not enlarge the finished
PCB or add holes to it. The factory's panel drawing is part of order review.
See the [assembly service](https://cart.jlcpcb.com/quote/?fromDemo=yes) and
[panelization rules](https://jlcpcb.com/help/article/pcb-panelization).

## Files to provide

Paths below are relative to `hardware/rev_a/pcb/exports/`.

| File | JLCPCB input |
| --- | --- |
| `cicala_rev_a_gerbers.zip` | PCB fabrication upload |
| `assembly/cicala_rev_a_jlc_bom.csv` | Assembly BOM |
| `assembly/cicala_rev_a_jlc_cpl.csv` | Assembly CPL / pick-and-place |
| `assembly/assembly_top.pdf` | Top placement/reference drawing |
| `assembly/assembly_bottom.pdf` | Mirrored bottom placement/reference drawing |
| `review/schematic.pdf` | Circuit reference for assembly review |
| `cicala_rev_a_board.step` | Optional mechanical reference |

The Gerber ZIP includes four copper layers, masks, legends, paste, outline
and separate PTH/NPTH drills. `cicala_rev_a_ipc2581.zip` is an alternative
structured exchange. Unpack the dated prototype handoff first and upload its
inner Gerber ZIP to the fabrication field.

The KiCad and JLCPCB position files use the same drill/placement origin.
Bottom placements remain explicitly marked bottom. Check the supplier's
placement overlay against the mirrored bottom assembly drawing, especially
U1, U2/U3, U7, Q1/Q2, D1–D4 and all three connectors. Confirm each exact MPN;
an LCSC identifier is a lookup aid, not permission for an alternate.
Supplier stock and sourcing acceptance are checked at order preparation.

The display, battery, NTC harness, case and optical/mechanical parts are fitted
after PCBA. Do not send the LiPo or display glass through reflow. Keep the
battery disconnected during inspection and first USB power-up. The complete
prototype assembly and acceptance procedure is in [REVIEW.md](REVIEW.md).

## Submit the prototype order

1. Check `review/verification.json` and the source/checksum record. If the
   design changed, repeat `just hw-review`, `just hw-fab-export` and the
   fabrication read-back before using a dated archive.
2. Start a PCB quote and upload `cicala_rev_a_gerbers.zip`. Confirm the preview
   shows one 78 × 49 mm board, four copper layers and the routed corners. Set
   1.2 mm, ENIG and the construction above. The desired delivery is **four
   individual assembled boards**, with one bare spare if the minimum lot is
   five; panel quantity and finished-board quantity are different fields.
3. Enable Standard assembly on **both sides**. Request the removable routed
   carrier described above, tooling and fiducials on its rails, and return of
   depanelized boards. Have CAM confirm the stack, USB impedance and carrier
   drawing before fabrication. Keep the supplied finished outline unchanged.
4. Upload the two `_jlc_` CSV files to the BOM and CPL fields and process them.
   The BOM columns are `Comment`, `Designator`, `Footprint`, `LCSC Part #`,
   `Manufacturer`, `MPN`, `Quantity`; CPL uses `Designator`, `Mid X`, `Mid Y`,
   `Rotation`, `Layer`. Coordinates are millimetres. These headers follow
   [JLCPCB's KiCad assembly guide](https://jlcpcb.com/help/article/how-to-generate-the-bom-and-centroid-file-from-kicad).
5. Review every matched part against its **exact MPN**. Blank LCSC fields
   require exact-part selection or the service's sourcing/consignment process.
   Do not accept a similar-looking capacitor or switch automatically. SW1/SW2
   are gold-contact **KSC323GLFG** (current Littelfuse orderable alias
   **Y31B13117FPLFG**); the silver version has a different minimum switching
   current. R21/R22 must be 22 kΩ. Confirm connector shell/hold-down soldering
   and the selected mixed SMT/through-hole tabs are included in the quote.
6. Check **89 fitted designators per board** in both BOM and CPL, including
   top-side SW1/SW2 and the display circuit. C1/C2/C31 are DNP. J4, test pads
   and mounting holes are bare features. Inspect the supplier placement preview
   against both PDFs: position, side, pin 1, polarity and connector opening.
   Any supplier rotation correction must be tied to its exact library part.
7. Attach the assembly PDFs and the fabrication note below. Review the final
   CAM image, sourced BOM and assembly overlay before payment/release. Retain
   those files with the order so they can be checked against the arriving boards.

Fabrication note to copy into the order:

```text
Cicala Rev A engineering prototypes; four assembled individual boards.
Finished outline 78 x 49 mm; nominal thickness 1.2 mm; 4 layers.
Stack JLC04121H-7628: outer 1 oz / inner 0.5 oz, ENIG.
F.Cu / In1.Cu GND / In2.Cu / B.Cu. Confirm USB 90-ohm differential.
Main USB F.Cu width/gap 0.32/0.26 mm; confirm with the selected stack.
Double-sided Standard assembly, exact MPNs, 89 fitted parts per board.
C1/C2/C31 DNP; J4/test contacts/mounting holes unpopulated.
Factory routed carrier at least 88 x 75 mm, rail tooling/fiducials.
Submit the carrier drawing and placement overlay before fabrication.
Keep connector overhangs and X=3..9.5 mm antenna strip clear.
Depanelize and finish tabs flush; preserve the finished board outline.
Inspect fine-pitch/polarized parts and USB/JST hold-down solder joints.
X-ray U2; inspect U3 asymmetric PGND and its two paste apertures.
Preserve thermal-hole masks: U1 (12) and U2 (2), tented on F.Mask.
Battery, NTC harness, display glass and enclosure are fitted after PCBA.
```

## After delivery

Complete the electrical acceptance sequence in [REVIEW.md](REVIEW.md), then
follow [Rev A build and flash](../../docs/firmware_rev_a.md). The case print,
button-stop adjustment, display/harness assembly and German/European printing
options are in [case/README.md](../case/README.md). Keep the first assembly
accessible until USB recovery, JTAG, display, buttons and charging pass.
