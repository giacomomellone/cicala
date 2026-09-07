# Rev A hardware

Rev A integrates the ESP32-S3, USB-C flashing/debugging, protected battery
power and the GDEY0213B74 display on a four-layer PCB. The revised schematic
and board pass ERC, DRC and parity, and the enclosure has printable fit files.
The manufacturing handoff targets four assembled engineering prototypes.
Physical electrical operation, radio performance and printed fit remain to
be measured on those prototypes.

![Rev A enclosure CAD](assets/images/hardware_rev_a/enclosure_assembly.png)

_Parametric CAD, 7 September 2026; this is not a photograph of built hardware._

## Files and commands

Open `hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pro` in KiCad 10. The five
schematic pages separate power/USB, controller/recovery, display and the charge
temperature guard beneath a system diagram. Project-local models restore
USB-C, JST, regulator, inductors and the status LED in 3D. Test contacts and
mounting holes intentionally have no fitted component bodies.

The red circles in KiCad's project tree indicate files modified in Git, not
electrical-check failures. See [KiCad Git integration](https://docs.kicad.org/10.0/en/kicad/kicad.html#_git_integration).
ERC and DRC have their own reports. Saving a file does not commit it to Git.

The mechanical source is `hardware/case/cicala_enclosure.scad`; STL/3MF and
sheet-cutting outlines are under `hardware/case/exports/rev_a/`. OpenSCAD owns
the enclosure; KiCad owns PCB placement and copper. Change shared dimensions
together and regenerate the component-envelope fingerprint.

```sh
just hw-check
just hw-review
just hw-case-export
bash hardware/pcb/export_fab.sh
just hw=rev_a fw-build debug
just hw=rev_a fw-flash debug
just hw=rev_a fw-debugserver
```

The default `hw=breadboard` target remains available. Rev A enables the battery
measurement divider, uses the 1 MΩ/470 kΩ scale and 200 ms settling time, and
controls panel power. It parks the physical display bus low for deep sleep
and performs a full refresh after panel power is restored. USB Serial/JTAG
provides console and debugging; UART0 recovery is available at J4.
Use the [Rev A build and flash guide](firmware_rev_a.md) for toolchain setup,
first download, diagnostics, charger indications and normal operation.

## Mechanical coordinate contract

All dimensions are millimetres. Viewed from above, the case origin is the
rear-left corner: X right, Y toward the USB front edge, Z up from the table.

| Item | Contract |
| --- | --- |
| Case | 84 × 56 × 24.05, flat face, R3 outside corners |
| PCB | Origin (3,3.5,16.5), 78 × 49 × 1.2, R2 corners |
| PCB top | Z=17.7 |
| Mounts | H1 (13,7), H2 (77.5,7), H3 (13,49), H4 (79,49), Ø2.7 NPTH |
| Retention | Three M2.5 × 20 countersunk screws at H1/H2/H4; H3 locating peg |
| Battery reserve | (10.2,10), 63 × 36 × 6.3, plus 0.2 adhesive; top Z=10.05 |
| Battery | Protected Adafruit 258 / PKCELL LP503562, 1200 mAh |
| Category / Next | Centres (27,12) and (53,12); 0.15 radial cap clearance |
| Button travel stop | Separate insulating plate on PCB top; nominal 0.85 travel, tune on fitted gold-contact switches |
| Display glass | Centre (45.075,37), 59.2 × 29.2 × 1.0, rotated 180° |
| Visible window | Centre (42,37), 50.6 × 25.7 |
| Lens | Centre (42,37), 55.0 × 30.1 × 0.8, R1 corners |
| USB-C | J1 underside at (42,50.1), opening 12.6 × 6.4 |
| Display connector | J2 top at (67.66,38.58), 90°, mouth faces +X, contacts down |
| Display flex | 14.3 ±0.3 free length, 2.0 nominal bend radius, 3.4 insertion |
| Battery connector | J3 underside at (73.6,40), 90°; 1=+, 2=NTC, 3=− |
| Recovery | J4 underside at (39,20), project-specific TC2030 pinout |
| Antenna exclusion | X=3…9.5 over the full board depth; no PCB copper or metal ballast |

The display's 48.55 × 23.7046 mm active area is offset from the glass centre.
Rotating the glass brings the short tail to the right; the case and firmware
follow that orientation. The modeled flex path is 14.298 mm. The glass pocket
allows ±0.35 mm X adjustment; even the short-tail/long-stiffener combination
retains a small straight-length margin in the nominal tolerance model.
The actual tail must be fitted without tension, creases or adhesive on its bend.

The base owns all lower posts. Three blind roof pillars capture the board;
the glass occupies the fourth pillar location. The retainer supports the glass
perimeter, independently of button loads. A separate plate below the cap brims
limits downward travel and spreads force over the supported PCB face. Its
stroke must be adjusted on the real switches; no overload rating is claimed.
The USB model uses the HRO drawing's
3.26 mm body height and 7.35 mm depth. Its recessed mouth is checked with a
12 × 6 mm cable overmold. Larger plugs require the USB coupon.

## Electrical and assembly contract

The fitted BOM has 89 components with exact MPNs. The board contains 119
footprints including bare contacts, mounting holes and three DNP capacitors.
U3, L1/L2 and the red/green LED use corrected manufacturer land patterns.
The long USB pair uses 0.32 mm width and 0.26 mm gap on the selected
JLC04121H-7628 construction; the manufacturer confirms the 90 Ω target at CAM.

The charger is set to 4.2 V / approximately 250 mA, with a 500 mA USB input
limit. A dual comparator and MOSFET independently veto charging outside a
conservative temperature window. The protected pack's bonded 10 kΩ NTC is
mandatory. The static tolerance model stops charging at 0°C and 45°C; actual
thermal lag and startup behavior remain bench measurements.

Native USB supports ROM download, serial and built-in JTAG. If USB recovery
is unavailable, remove the base and disconnect the battery to access J4.
Its pins are 1=3V3 reference, 2=GND, 3=EN, 4=BOOT, 5=UART TX, 6=UART RX.
Use a TC2030 breakout wired to this table and a 3.3 V UART adapter; this is
not an ARM debugger pinout. Hold BOOT low while releasing EN to enter ROM
recovery. Do not power the board through J4's reference pin.

## Prototype acceptance

The source and export checks establish geometry, connectivity and file
consistency. They do not establish electrical performance or manufacturing
yield. Print the coupons, inspect the assembled PCBs, and record first power,
USB/JTAG/UART recovery, battery/NTC charge behavior, display refresh, sleep
current, button fit and assembled radio results before accepting the prototypes.

Detailed BOM, manufacturing settings and electrical acceptance steps are in
`hardware/pcb/BOM.md`, `hardware/pcb/MANUFACTURING.md` and
`hardware/pcb/REVIEW.md`. Print settings, fasteners, cutting tolerances,
harness construction and assembly order are in `hardware/case/README.md`.
No IP rating or production certification is claimed.

![Exploded Rev A CAD](assets/images/hardware_rev_a/enclosure_exploded.png)
