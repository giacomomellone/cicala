# Rev A enclosure

`cicala_enclosure.scad` is the mechanical source. The revised enclosure is
84 × 56 × 24.05 mm with a flat face, a removable base and an internal battery.
The selected display folds into the top-side PCB connector. Sources use
[CERN-OHL-S-2.0](../../../LICENSE-HARDWARE).

![Rev A enclosure CAD](renders/assembly.png)

This is a CAD render. No physical print or assembled-fit test has been done.
The files are prepared for an FDM fit prototype; print the coupons first.

## Printing

The thirteen STL/3MF parts are in `exports/`, in millimetres at Z=0.
The shell and two caps are exported with their visible faces on the bed.
The base prints floor-down, with its board posts pointing up. The retainer
prints on its flat perimeter. The separate `button_stop` prints flat, PCB
contact face down; use solid infill for this support. Do not scale any part.

Start with PETG, a 0.4 mm nozzle, 0.15 mm layers, four perimeters and five
solid top/bottom layers. Use 25–35% infill in the shell/base and solid caps.
Inspect the slicer preview for unsupported ledges; place local supports under
internal ledges if the printer cannot bridge them. Keep supports off cap
bores, lens seats and the display-glass ledge. Use the same material and
orientation for coupons and the final parts. These are starting settings;
printer calibration and shrinkage determine the actual fits.

Print `coupon_buttons`, `category_cap`, `next_cap`, `button_stop`, `coupon_usb`,
`coupon_lens` and `coupon_boss` first. Check the nominal 0.15 mm radial cap
clearance, cable overmold, lens rebate and screw pilots.

The KSC323GLFG actuator is 3.47 ±0.2 mm above its mounting plane; solder and
FDM tolerances add to that stack. The downward stop is a separate plate resting
on the component-free top PCB area near H1/H2. It clears the switch lands,
roof pillars and J4 alignment holes. The cap's lower brim contacts this plate
after 0.85 mm of travel from the CAD datum. The cap stem has a nominal 0.10 mm
relief. Actual resting height and electrical actuation must be checked with
the assembled switches, rather than assuming the nominal CAD height.

Fit each cap without preloading its switch. Check continuity with a meter:
open at rest, closed on press, then open on release. Trim/reprint the stem with
`cap_tip_relief` as needed. Adjust `cap_overload_gap` or add insulating shims on
the stop's contact lands so the brim meets the plate just after electrical
actuation, before a hard press loads the switch further. Recheck both caps
after fastening the case. Do not force a cap that bottoms on the actuator
before reaching the plate. This is an adjustable travel stop; its overload
force and cycle life have not been qualified. The button coupon checks bore
and flange fits; use the assembled PCB and plate for the stroke check.

If workplace FDM cannot achieve the cap fit, use PA12 SLS/MJF for the shell,
base, retainer, button stop and caps. [Rapidobject in Leipzig](https://www.rapidobject.com/)
offers FDM, SLS and MJF; ask for an unscaled PA12 fit set and retain the coupons.
[Materialise](https://www.materialise.com/en/industrial/3d-printing-services/online-3d-printing?origin=imaterialise)
is another European ordering option. Obtain a quote from the final files;
no files have been sent to either service.

## Sheet parts, fasteners and adhesives

`lens_cut.svg` and `steel_cut.svg` are full-size cutting outlines. Use the
following specifications with the files; a printed lens/steel surrogate is
only a fit gauge.

| Part | Prototype specification |
| --- | --- |
| Lens | Clear PMMA, 0.8 mm, 55.0 × 30.1 mm, R1 corners; XY ±0.10 mm, deburred edges; no frosting over the display |
| Steel skin | 1.2 mm steel, cut to SVG, XY ±0.15 mm, deburred and corrosion protected; retain the antenna cutout |
| Steel countersinks | At H1/H2/H4: 90° countersink to Ø4.72 mm, after cutting the Ø3.0 mm clearance holes; flush screw heads |
| Base screws | Three ISO 7046 / DIN 965 M2.5 × 20 countersunk machine screws |
| Roof pilots | Printed Ø2.1 mm blind pilots, carefully tapped M2.5; nominal thread engagement 3.8 mm and 1.75 mm roof-tip clearance |
| Feet | Four Ø8 × 1.5 mm self-adhesive elastomer disks |
| Light pipe | Clear Ø2.0 × 3.5 mm PMMA rod, polished ends; the exported part is a fit reference |
| Display support | 0.10 mm compliant transfer adhesive on the glass perimeter ledge; keep the active area and flex free |
| Battery attachment | Removable 0.2 mm adhesive tabs under the pack; never clamp or screw through it |

For the first unweighted fit, print `steel_skin.stl` as a spacer so the screw
and base stack remains the same. The functional steel part can follow after
fit and radio testing. Tap and test the boss coupon before tapping the shell.
Do not run a power driver against the blind pilots.

## Battery and display assembly

Use the protected Adafruit 258 / PKCELL LP503562 pack and Semitec 103AT-2 NTC
specified in [the BOM](../BOM.md). Keep the factory PH2 connector intact.
Make an adapter from the mating end of an
[Adafruit 1131 extension](https://www.adafruit.com/product/1131), shortened to
suit the right-edge cable channel, to a JST PHR-3 housing with SPH-002T-P0.5S
contacts. Use AWG28 wire with insulation OD no greater than 1.0 mm at J3.
Verify contact retention and continuity before connecting the pack.

J3 is **1=protected pack positive, 2=NTC, 3=pack negative**. The second NTC lead
goes to pin 3. Bond the insulated NTC head to the pouch near its centre using
thin polyimide tape, and insulate every splice. Keep solder at least 5 mm
from the sensor head and lead bends at least 3 mm away, per the Semitec drawing.
Do not solder to the pouch cell or bypass its PCM. Check actual connector
polarity with a meter; wire colour alone is insufficient.

The case reserves a 63 × 36 × 6.3 mm pack envelope at (10.2,10), plus adhesive,
against a maximum new pack size of 62.3 × 35.3 × 5.3 mm. The NTC has a separate
head reserve. Dress excess cable in the right edge channel, away from the
antenna and the mounting posts. The mated JST projection and cable corridor
are included in the collision checks. Real strain relief and wire bends
still need inspection during assembly.

Fit the lens and compliant display support first. Rotate the GDEY0213B74
glass 180° in its plane: glass centre (45.075,37), visible-area centre (42,37).
The flex exits to the right. Form a smooth 180° loop with nominal 2.0 mm
radius; never crease the tail or bend the 6 mm stiffener. Insert 3.4 mm into
J2 with the exposed contacts facing the PCB, then close the latch. The
connector is at (67.66,38.58), facing right. The glass pocket allows ±0.35 mm
X adjustment to accommodate the tail/stiffener tolerances before bonding.

Place the caps in the shell, fit the display/retainer and put `button_stop`
over the switches with its flat face against the PCB. Keep this printed
insulating plate free of debris; it spreads button load over the supported
board face. Connect the flex and install the PCB on the four base posts.
H3 uses a locating peg; only H1/H2/H4
receive screws. Fit the battery after electrical bring-up, connect J3, dress
the wires, and close the base without force. Reopen it if any cap binds or
anything presses on the pouch or glass. This enclosure has no verified IP
rating and must not be described as waterproof.

## Reproduction and checks

Use OpenSCAD 2026.06.12 or a compatible version:

```sh
just hw-case-check
just hw-case-export
```

The check renders 18 selectors, verifies thirteen closed, oriented, single-solid
meshes, and tests fourteen nominal intersections, including cap travel at
rest, half stroke and just before the stop. A separate positive-volume contact
check confirms that a downward press reaches the plate. It also checks the placement
fingerprint of all 87 fitted component envelopes, with separate native switch
references. The SCAD assertion stack checks the flex length, stiffener reserve,
board and battery heights, mounting points, buttons and optical opening.

After PCB placement or local STEP-model changes, regenerate the populated
board STEP, run `hardware/tools/pcb/export_geom.py` using KiCad Python, then
run `export_component_bounds.py` with CadQuery Python and the `--step` and
`--geometry` paths. See `--help`. Regenerate the exports and repeat the checks.
The shared dimensions are in [docs/hardware_rev_a.md](../../../docs/hardware_rev_a.md).
