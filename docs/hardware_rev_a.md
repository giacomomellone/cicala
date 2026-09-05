# Rev A hardware

Rev A is the first integrated enclosure and PCB study. The files establish a shared coordinate system, component envelopes, captured schematic, recovery access, and validation gates. They are suitable for fit and electrical review. They are not released for PCB fabrication or production tooling.

![CAD model of the Rev A enclosure](assets/images/hardware_rev_a/enclosure_assembly.png)

_Parametric CAD render, 16 August 2026. The colours show warm-paper and charcoal finish intent, not a validated material sample or built hardware._

## Source files

The mechanical source is `hardware/case/cicala_enclosure.scad`. Printable Rev A exports live beside it under `hardware/case/exports/rev_a/`. The KiCad 10 project is `hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pro`.

`check_enclosure.sh` renders every part and asserts the stack: cell clearance, module clearance, the front-opening heights, a positive hold-down pillar at every mounting point, cap travel against the switch drawing, and the nesting of window, lens and rebate. A parameter change that breaks any of them fails `just hw-case-check` instead of exporting a wrong STL.

OpenSCAD is the source of truth for the enclosure. KiCad owns the board outline, stack-up, copper and component placement. Dimensions shared by both files use the case coordinate system below and must be changed together.

## Coordinate contract

All values are millimetres. The origin is the rear-left corner of the case
when viewed from above. X runs right, Y runs toward the front USB edge, and Z
runs up from the table.

| Item                | Contract                                                       |
| ------------------- | -------------------------------------------------------------- |
| Case                | 84 × 56 mm, 3 mm corner radius, 3.37° face incline             |
| Maximum height      | 15.85 mm at the rear, including 1.5 mm feet                    |
| PCB                 | rear-left at (3, 3.5), 78 × 49 × 1.2 mm, 2 mm corner radius    |
| PCB mounting holes  | (13, 7), (77.5, 7), (13, 49), (77.5, 49), Ø2.7 mm NPTH         |
| Base service holes  | (6.5, 40), (77.5, 40); base and steel only, not the board      |
| Module              | underside, 25.5 × 18 × 3.1 mm at (3.5, 14)                     |
| Module antenna zone | X = 3–9.5 over the full board depth; no copper, no metal       |
| Cell volume         | underside, 36 × 30 mm at (32, 6), 5.4 mm nominal height        |
| Category            | centre (27, 12), Ø10.30 mm cap bore                            |
| Next                | centre (53, 12), 18.30 × 11.30 mm cap bore                     |
| Display panel       | centre (42, 37), 59.2 × 29.2 × 1.0 mm                          |
| Visible window      | centre (42, 37), 50.6 × 25.7 mm                                |
| Lens                | centre (42, 37), 55.0 × 30.1 × 0.8 mm                          |
| USB-C               | underside, centred at X = 42, opening 9.4 × 3.8 at Z = 7.55    |
| FPC connector       | underside, centre (26, 47), contacts facing the front edge     |
| Status light        | underside at X = 52, light pipe Ø2.4 centred at Z = 8.70       |

The e-paper panel dimensions use the GDEY0213B74 vendor envelope. Its active
area is 48.55 × 23.7046 mm.

Two of these changed from the first Rev A drawing, and both were errors rather
than preferences.

The board grew from 78 × 45 to 78 × 49 mm and moved 0.5 mm rearward. With the
front edge 6 mm behind the case wall, a USB-C plug could never have reached the
receptacle: the plug's nose is about 6.5 mm long and it would have bottomed out
on the case face with 8 mm still to travel. The board's front edge now ends
1.5 mm from the inside of the wall and the receptacle noses into the wall
aperture, which is also why the board has to be tilted in during assembly
rather than dropped straight down.

Everything except the two switches and the display stack moved to the
underside. The sloped roof leaves between 0.80 mm at the front edge and
3.45 mm at the rear above the board, so nothing 3 mm tall fits on top; the
5.3 mm cavity over the cell does. The USB and light-pipe openings, which had
been drawn at Z = 7.7 without a matching part, are now computed from the board
height and the part height and cannot drift again.

## Mechanical stack

The enclosure uses a deep top shell, a removable base, an optional 1.2 mm
steel weight, and four 8 × 1.5 mm elastomer feet. The steel is cut away over
the antenna keep-out. The PCB sits above a protected 503035 cell. A separate
retainer supports the e-paper around its perimeter and leaves the FPC exit
clear at the front, where the flex folds around the board edge to the
underside connector.

The board is captured by geometry rather than by screws. Four bosses rise
5.3 mm from the case seam to the underside of the board, and four Ø5 mm
pillars come down from the roof to its top face; the 1.2 mm gap between them
is the board. The four M2.5 thread-forming screws enter from below through the
steel and the base and thread into the bosses, clamping those two parts to the
shell. Earlier drawings had the bosses stop 0.9 mm short of the board, which
left it resting on nothing.

The modeled 0.35 mm base recess leaves 0.25 mm nominal clearance between a
5.4 mm cell keep-out and the PCB. That is a feasibility warning, not a release
clearance: the selected cell drawing, protection tab, wire exit, adhesive,
swelling allowance and base process tolerance must either validate the stack
or force a height change. The keep-out is now 36 × 30 mm, which clears the
published LP503035 drawing that the earlier 35 mm figure did not.

The display retainer starts 0.05 mm above the nominal PCB top and overlaps
only the panel's perimeter support region. That clearance also requires a
tolerance study and a component keep-out below the frame; it is not suitable
for release as drawn.

### Buttons

The cap geometry is derived from the C&K KSC3 drawing rather than assumed. A
KSC321G is 6.2 mm square, 2.9 mm to the top of its body and 3.5 mm to the top
of its actuator, with 0.2 +0.3/−0 mm of electrical travel at 2 ± 0.4 N.

With the board top at 10.40 mm the actuator top is at 13.90 mm, which is
inside the cap bore rather than under the roof. The bore passes through a
locally thinned 1.0 mm roof and opens into a counterbore, leaving a shoulder
at 14.32 mm. A cap rests on the actuator, is held up against that shoulder by
the switch's own return spring, and carries a brim 0.70 mm below the roof's
inner surface, so a hard press lands on the shell instead of on the actuator.
The 0.70 mm gap clears the 0.5 mm worst-case travel.

The default presentation is Category 0.2 mm sub-flush and Next flush, with
0.25 mm nominal XY print clearance at both bores. Those are starting values.
`coupon_buttons` now reproduces the real bore, counterbore and shoulder, and
must be printed with `coupon_lens`, `coupon_usb` and `coupon_boss` before the
complete shell. Record the printer, material, layer orientation and measured
correction.

The lens uses a 55.4 × 30.5 mm rebate around a 50.6 × 25.7 mm through-window.
Each switch bore has its own gasket loop with a low-point drain break. A sealed
switch does not seal the USB or light-pipe openings, so no enclosure ingress
rating is claimed.

## PCB foundation

The KiCad project starts with a four-layer 1.2 mm FR-4 stack:

| Layer  | Role                                                        |
| ------ | ----------------------------------------------------------- |
| F.Cu   | the two switches, signals, ground pour                      |
| In1.Cu | uninterrupted ground reference                              |
| In2.Cu | 3V3 plane, signals, ground island under the USB pair        |
| B.Cu   | almost every component, signals, ground pour                |

Four layers are what the ESP32-S3 RF return path, native USB and the density
of 105 parts on 78 × 49 mm need. Two signal layers were not enough: the third
came from letting In2.Cu carry signals through the 3V3 plane, which is a
distribution plane rather than a reference. In1.Cu stays unbroken.

The USB pair runs on B.Cu with a ground island poured beneath it on In2.Cu, so
it is referenced to ground rather than to the rail. The pair is 0.29 mm wide
with a 0.29 mm gap, which computes to 90 Ω differential over the declared
0.18 mm prepreg at εr 4.5. The final stack and the controlled-impedance
geometry must still come from the selected fabricator; those numbers change
with the real dielectric.

Net classes are Default (0.20 mm, 0.15 mm clearance), Power (0.30 mm), USB
(0.29 mm, 0.20 mm) and HV for the SSD1680 pump rails (0.25 mm, 0.20 mm). The
HV clearance is 0.20 mm because that is the gap between adjacent pins of the
panel's own 0.5 mm-pitch connector; nothing tighter is available on those
rails, and it is still well above the IPC-2221 minimum for ±15 V. Nets that
have to leave a 0.4 or 0.5 mm-pitch package neck down to 0.15 mm, which is the
widest track that clears 0.15 mm on both sides at that pitch.

The root schematic contains three captured hierarchy sheets: Power and USB, Controller and user I/O, and E-paper display. Short local connections are drawn directly; named global nets remain at functional boundaries, dense controller and connector fan-out, shared rails and production test access so the power and firmware contracts remain visible during review.

### Firmware pin contract

Existing firmware pins stay fixed unless a hardware review changes both sides in one revision.

| Function                |         GPIO | Hardware rule                                      |
| ----------------------- | -----------: | -------------------------------------------------- |
| Battery ADC             |            1 | 1 MΩ / 470 kΩ divider; switch the complete divider |
| Battery-divider enable  |            2 | off during sleep                                   |
| Category                |            4 | active-low RTC wake input                          |
| Panel power enable      |            5 | panel and boost off between refreshes              |
| Charger STAT1 / STAT2   |        6 / 7 | inputs; truth table belongs in the charger sheet   |
| Display reset / busy    |        8 / 9 | preserve current firmware mapping                  |
| Display CS / MOSI / CLK | 10 / 11 / 12 | preserve current firmware mapping                  |
| Charger enable          |           14 | active-low; 100 kΩ pull-down enables by default    |
| Status red / green      |      15 / 16 | both low means no LED sleep load                   |
| Next                    |           17 | active-low RTC wake input                          |
| Display D/C             |           18 | preserve current firmware mapping                  |
| Native USB D− / D+      |      19 / 20 | route as a matched pair over solid ground          |
| VBUS sense              |           21 | boot/poll input; optional EXT0 measurement path    |
| Factory reset control   |           38 | open-drain compatible; final use remains gated     |

IO0 and EN both need concealed service pads. IO0 alone does not recover a battery-powered board unless reset can also be asserted. Expose ground, 3V3, VBUS, battery, UART0, USB, the display bus, charger status and removable current links on the underside.

## Electrical baseline

The captured USB-C port is a 5 V USB 2.0 sink without USB-PD. Its data pair connects through USBLC6-2SC6 ESD protection and 22 Ω series resistors to the ESP32-S3 native USB pins. The port is the normal flashing, serial-console and JTAG path. Concealed IO0, EN and UART0 access remains available for recovery.

BQ25185 provides the single-cell power path and charger. The schematic sets a 500 mA USB input limit and 250 mA charge current for a protected 500 mAh pack, pulls `/CE` low by default, and exposes both open-drain status outputs to firmware. The pack specification requires a PCM and a 10 kΩ, B=3435 K NTC on a three-wire keyed harness.

TPS63802 replaces the earlier LDO candidate. Its buck-boost topology holds 3.3 V across the useful protected-LiPo range instead of dropping out as the cell approaches 3.3 V. A second TPS22917 switches the complete 1 MΩ/470 kΩ battery divider, and another removes power from the e-paper panel and boost network between refreshes.

GDEY0213B74 remains the panel and the boost circuit follows its reference design. The board-side connector candidate is a bottom-contact Hirose FH12-24S-0.5SH. Its contact orientation, insertion motion and folded FPC route remain mechanical review items.

WROOM-1-N16 is the preferred assembly for the expected close-range captive-portal use. WROOM-1U-N16 remains an alternative on the same module pad pattern if the onboard-antenna placement cannot clear the display, cell, buttons, copper and enclosure metal. Close range reduces the required link budget; it does not remove Espressif's keep-out or the assembled radio test. The two modules are assembly alternatives, not parts to fit at once.

The FPC connector and the USB receptacle are both on the underside and no
longer share a datum: the receptacle sits at the front edge centred on X = 42
and the connector sits at (26, 47) with its contacts facing the front, where
the panel flex folds around the board edge to reach it. The flex exit position
on the panel itself comes from a vendor drawing this repository does not have,
so the fold, its bend radius and the insertion motion are still review items.

A published protected LP503035 pack is approximately 36 × 30 × 5 mm before the
wire exit and swelling allowance. The keep-out is now 36 × 30 × 5.4 mm, so the
published drawing fits; an exact pack drawing with the wire exit and swelling
allowance is still needed before ordering either the cell or a complete shell.

KSC321G LFS is specified at 2 ±0.4 N, at least 15% tactile ratio, and electrical travel of 0.2 mm with +0.3/−0 mm tolerance. Cap geometry must accommodate that range. A fixed 0.25 mm travel or a 50% snap ratio is not a design input.

## Validation gates

Run `just hw-check` for file-level checks: it renders every enclosure part and
asserts the stack, and it runs schematic ERC, schematic-to-board parity, board
DRC and the STEP export. `hardware/pcb/export_fab.sh` writes the fabrication
and assembly outputs.

What those checks cover today, in KiCad 10.0.5 and OpenSCAD 2026.06.12: the
schematic passes ERC with zero errors and zero warnings and matches the board
with zero parity issues; the enclosure renders every selector without warnings
and satisfies its geometry assertions.

The board itself is placed, poured, stitched and mostly routed, and is not
finished. The last run left 36 connections unrouted, 65 unconnected pads and
about 50 DRC violations, most of them pour artifacts around the unrouted
areas. Board DRC and connectivity are the outstanding gate; everything above
them — schematic, rules, layer plan, placement, pours — is settled.

They do not replace electrical, layout or physical review. ERC accepts
electrically valid but topologically wrong wiring — it saw none of the eight
circuit errors listed below — and DRC checks manufacturability and
connectivity, not whether the circuit is right or the loops are tight.

Before ordering a PCB or complete shell, close every applicable physical gate:

1. Print and measure all four enclosure coupons in the intended materials.
2. Import manufacturer 3D models and clear the display glass, FPC fold, cell, antenna, switches, USB shell and service tool access. The USB-C receptacle overhangs the board edge into the wall aperture, so confirm the board can be tilted into the shell during assembly.
3. Confirm the drafted project land patterns for U3, L1, L2 and D4, and the J1 silkscreen variant, against current manufacturer drawings.
4. Review the routed board against the fabricator's real stack: USB return current, the antenna keep-out, the e-paper boost loop, battery protection and the charge path. The 90 Ω pair geometry assumes the declared 0.18 mm prepreg.
5. Build an unweighted fit prototype before fitting the steel. Verify stable button presses, cap stops, lens support, FPC bend radius of at least 2.5 mm, antenna clearance, drainage and serviceability.
6. Measure whole-device sleep current at the cell through the R34 link, e-paper refresh brownout margin, radio performance, charging temperature and LED leakage.
7. Run drop, cap-overload, button-life, lint and small-spill tests. Keep ingress claims out of product material until a complete assembly passes a defined test method.

Eight schematic errors have been found and corrected, none of which ERC could
see. The TPS63802 feedback divider was shorted — a straight wire tied FB to
VOUT and bypassed R14, so the part would have regulated the 3.3 V rail down to
its 0.5 V reference. The BQ25185 TS/MR pin and the pack NTC wire (J3.2) were
tied straight to VBAT, which the charger reads as an out-of-range temperature
and suspends charging; TS/MR is now the `BATT_NTC` net carrying only J3.2,
while BAT stays on VBAT. The charger sources ~38 µA into TS, so the 10 kΩ
B=3435 pack NTC to GND needs no external resistors and sets an approximate
1.5 °C to 59 °C charge window. The USB data pair was first shorted to CC2 and
then crossed, so the connector's D− reached GPIO20; it now runs straight. Q1
carried a `Q_NMOS_SGD` symbol while the Si1308EDL is pin 1 = G, 2 = S, 3 = D,
which put the gate driver on the sense node. The VGL charge pump had D2's
anode on the wrong node, leaving C16 with no charging path. C21 was 1 nF where
the panel reference gives VSH1 1 µF/25 V. The VBUS-sense divider was the wrong
way round: 150 k over 100 k gives 2.0 V from a 5 V bus, below the ESP32-S3
guaranteed high-input threshold at 3.3 V, and it is now 100 k over 150 k for
3.0 V. The TPS22917 slew capacitor C14 returned to ground, where the device
requires it to return to VIN. Every fix still needs bench confirmation on an
assembled board.

## Primary references

- [Good Display GDEY0213B74](https://www.good-display.com/product/391.html)
- [C&K KSC3 series data sheet](https://www.ckswitches.com/media/1969/ksc3.pdf)
- [Espressif PCB layout guidance](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html)
- [TI BQ25185 data sheet](https://www.ti.com/document-viewer/BQ25185/datasheet/GUID-038472CB-EA03-4FB2-B894-ED0D9E1E6481)
- [TI TPS63802 data sheet](https://www.ti.com/lit/ds/symlink/tps63802.pdf)
- [TI TPS22917 product page](https://www.ti.com/product/TPS22917)
- [ST USBLC6-2SC6 data sheet](https://www.st.com/resource/en/datasheet/usblc6-2.pdf)
- [Hirose FH12-24S-0.5SH](https://www.hirose.com/product/p/CL0528-0015-4-98)
- [Lipo Battery LP503035 protected-pack drawing](https://www.lipolbattery.com/LiPo-Battery-Datahseet/LiPo_Battery_LP503035_3.7V_500mAh.pdf)
