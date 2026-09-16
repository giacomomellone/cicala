# Rev A engineering review — 7 September 2026

The revised design has a complete prototype fabrication/assembly export and
printable enclosure files. It passes schematic, board, mechanical-geometry
and export checks. It has not been built or physically tested. Factory CAM,
component sourcing and placement review precede fabrication; electrical and
mechanical acceptance follows on the prototypes. This is not a production
qualification or certification.

The KiCad project is authoritative. The handoff is described in
[MANUFACTURING.md](MANUFACTURING.md), with print/assembly instructions in
[case/README.md](../case/README.md). Four assembled prototypes are recommended.

## Circuit, symbols and layout

C28/C30's missing ground junction was repaired. D4 now uses the manufacturer's
four-pin red/green LED symbol and land: 1=red anode, 4=red cathode, 2=green
anode, 3=green cathode. U3's DLA0010A land now has its asymmetric PGND terminal
and two paste apertures; there is no exposed central pad. L1/L2's lands match
TDK and Coilcraft drawings. The USB-C and battery connector use their actual
purchasable MPNs. All fitted passives have exact MPNs and ratings.

SW1/SW2 use KSC323GLFG gold contacts and R21/R22 use 22 kΩ pull-ups. At
3V3 −5% and resistance +1%, the external pull-up alone supplies 141 µA,
above the specified 100 µA minimum switching current. The silver-contact
part's 1 mA minimum was not met by the previous 47 kΩ circuit. G-terminal
land dimensions agree with the manufacturer drawing: 12.0 mm outer span,
5.8 mm inner gap, 4 mm row pitch. The cap model uses the actual 3.47 ±0.2 mm
actuator height and includes a separate adjustable downward stop.

The five schematic pages separate the power, controller, display and thermal
circuits. Duplicate overprinted labels were removed, labels crossing wires
were repositioned, and the sheets were visually inspected. ERC is supplemented
by critical pin/net and land checks, since ERC alone missed the floating
bypass capacitors in the starting design.

J2 and the display boost circuit moved to the top so the selected panel tail
can reach the connector. L1/Q1/D3's switch-node connections and the charge-pump
connections are short, direct top-layer copper. U3/L2's switching connections
remain on the bottom. Feedback and bypass placement were reviewed. U5 gained
C34 as its dedicated input bypass. J4 moved to a clear recovery location;
its through-board alignment holes are checked against components on both faces.

The board has 119 footprints, 1949 track segments and 474 vias. Its bottom
legend identifies power blocks, USB, battery polarity and the recovery pinout.
Dense component references live on the fabrication layers. Ground is continuous
on In1 beneath both long USB traces outside the via launches. The main pair
has matched nominal planar lengths of 58.494 mm, or 60.666 mm including the
published stack's layer transitions. These lengths are traced from the actual
board, excluding DNP branches and package-internal distances. The
0.32 mm width / 0.26 mm gap follows the selected
JLC04121H-7628 construction. The short connector branch mismatch including
the revised layer spacing is approximately 0.041 mm. CAM must confirm the
90 Ω differential target; the calculation here is a first-order estimate.

Six original dimensional STEP models restore USB-C, JST, U3, both inductors
and the LED. The populated STEP has every fitted body. These are simplified
models rather than certified vendor mating models; dimensional sources are in
[models/README.md](pcb/models/README.md).

## Charging and capacitance review

The selected protected Adafruit 258 / PKCELL LP503562 pack specifies charging
from 0 to 45°C. BQ25185's native TS threshold with a 10 kΩ NTC permits a higher
hot limit, so U7/Q2 provide an independent window veto at the charger's CE pin.
The native TS connection remains intact. The NTC must be bonded to the cell;
a room-air thermistor is not a cell-temperature measurement.

R35/R36/R37 are 820 kΩ / 210 kΩ / 82.5 kΩ. The static model uses the Semitec
103AT-2 R/T table, ±5% sensor allowance, 1% resistors, 3V3 ±5%, 36.5–39.5 µA
TS bias and ±5 mV comparator allowance. It predicts a nominal 4.17–37.11°C
permission window, 16.4 mV cold-stop margin at 0°C and 19.6 mV hot-stop margin
at 45°C. Charging is permitted throughout 10–30°C under this model. The ladder
draws 2.97 µA. R13/R39 keep CE below 0.252 V when the MCU pin floats and the
guard permits charging. R38/C33 filter permission; no hysteresis or guaranteed
power-on-reset behavior is assumed. Threshold chatter, sensor lag and USB
insertion behavior require bench testing. See `tools/audit_temperature.py`.

Samsung's published DC-bias curves were checked at the operating voltages.
The following estimates additionally multiply by negative capacitance
tolerance, 0.85 for temperature and 0.95 as an aging allowance:

| Position | Selection / bias | Estimated effective capacitance | Required |
| --- | --- | ---: | ---: |
| C8 regulator input | 47 µF 10 V 0805 at 4.59 V | 10.9 µF | ≥4 µF |
| C9+C10 regulator output | 22 µF + 47 µF at 3.465 V | 20.3 µF combined | ≥7 µF |
| C4 charger input | 10 µF 10 V 0603 at 5.5 V | 1.83 µF | ≥1 µF |
| C6 charger BAT | 10 µF 10 V 0603 at 4.2 V | 2.51 µF | ≥1 µF |
| C34 divider-switch input | Same 10 µF part at 4.2 V | 2.51 µF | Local bypass |

These are design estimates from typical curves, not manufacturer-guaranteed
minimums. The larger values provide margin; substitute capacitors only after
checking the replacement's DC-bias data. The panel's 25 V reservoir and pump
capacitors follow the display reference circuit. Verify actual rail peaks and
3V3 droop while the radio and display operate together.

## Mechanical fit

The case is 84 × 56 × 24.05 mm. PCB bottom/top are Z=16.5/17.7. The protected
pack reserve is 63 × 36 × 6.3 mm plus adhesive, with a separate NTC reserve.
The mated JST projection and right-edge wire corridor clear the modeled shell
and components. Keep the harness out of the antenna strip and mounting posts.

The panel is rotated 180° in its plane. Its glass centre is (45.075,37),
active-area/window centre (42,37), and the flex exits right toward J2. The
14.298 mm nominal loop uses a 2 mm bend radius and 3.4 mm connector insertion.
The 6 mm stiffener stays straight. Glass X adjustment of ±0.35 mm accommodates
the drawing's flex/stiffener tolerances; the worst modeled straight margin
is approximately 0.165 mm. Fit the actual tail before bonding the glass.

Three M2.5 × 20 countersunk screws hold the base through H1/H2/H4 into tapped
blind roof pilots; H3 is a locating peg. Lower posts are integral to the base.
The display retainer supports the glass perimeter, independently of the caps.
The shell and caps export face-down for FDM. The USB aperture and coupon use
a 12 × 6 mm cable overmold and the HRO drawing's 3.26 mm body height.

The cap brims contact a separate printed plate on the supported PCB face
during downward travel. This corrects the earlier roof-facing brim, which
moved away from its supposed stop when pressed. Nominal travel is 0.85 mm;
fit the stem and stop against the actual switch, solder and print tolerances
using the case instructions. Electrical actuation must precede stop contact,
and neither switch may be preloaded at rest. No overload-force or cycle-life
qualification is claimed.

The fourteen nominal collision checks cover PCB, glass, retainer, shell,
USB plug, folded flex, fitted components, battery/NTC and the mated harness
corridor, plus the button support and cap motion. All are empty intersections.
Only the intended PCB/plate mating surface has a 0.01 mm numerical relief in
the collision test. A positive-volume test separately proves downward contact
with the stop. The component envelopes are fingerprinted
against the checked PCB so placement changes invalidate the fit result.
Print tolerances, adhesives, real wire bends and assembly force remain physical
checks. No ingress-protection rating is claimed.

## Flash, debug and recovery

Use a USB-C **data** cable. The ESP32-S3's native USB reaches GPIO19/20 through
ESD protection and series resistors. ROM download and built-in USB Serial/JTAG
require no external programmer. Build with the Rev A hardware selection:

```sh
just hw=rev_a fw-build debug
just hw=rev_a fw-flash debug
just hw=rev_a fw-monitor debug
just hw=rev_a fw-debugserver
```

The debug ELF is `build/cicala-rev-a-debug/zephyr/zephyr.elf`; OpenOCD accepts
GDB on port 3333. Use `just hw=rev_a fw-flash` for the release/MCUboot image.
The Rev A firmware controls GPIO2's divider and GPIO5's panel switch, applies
200 ms ADC settling, disables the divider even after conversion failure,
and parks the physical display signals low before removing the panel rail.
The first refresh after power restoration is full.

Both application and MCUboot select 16 MiB flash without PSRAM. GPIO6/7 are
read together to decode BQ25185 charging, idle/disabled and both fault states.
High/high does not prove charge completion. Faults produce a red pulse and
are not automatically reset. Power-state access is synchronized across threads;
ADC work stops before the LED and switched rails are shut down. Full setup,
flash, diagnostic profiles and LED meanings are in
[Rev A firmware](../../docs/firmware_rev_a.md). The accepted question corpus
is empty at this review; the existing charset profile supports display bring-up.

J4 is an unpopulated TC2030-IDC-NL contact pattern at (39,20), accessible after
opening the base and disconnecting/moving the battery. A cable needs a breakout
wired to this project-specific pinout:

| Pin | Signal | Connection |
| --- | --- | --- |
| 1 | 3V3 | Target voltage reference only; do not back-power the regulator |
| 2 | GND | Adapter ground |
| 3 | CHIP_EN | Pull low for reset |
| 4 | BOOT_IO0 | Hold low while releasing reset for ROM download |
| 5 | UART0_TX | To the adapter's RX |
| 6 | UART0_RX | From the adapter's 3.3 V TX |

A standard ARM debugger pinout is incompatible. A 3.3 V USB/UART adapter can
recover through UART0: hold BOOT low, assert EN low, release EN, then release
BOOT. Do not apply 5 V to the service signals or change USB/JTAG eFuses during
prototype bring-up. TP1/TP2 expose BOOT/EN if temporary test leads are preferred.

## Prototype acceptance procedure

1. Inspect assembly orientation, fine-pitch joints, USB tabs, connector latches
   and pin-1 marks. Check for a 3V3/GND short with power disconnected. Keep the
   cell and display disconnected for the initial current-limited 5 V input.
2. Measure USB_VBUS, VSYS and 3V3 at TP10/TP9/TP7. Start with a low current limit
   for short detection, then allow up to the designed 500 mA USB input for boot.
   Verify native USB enumeration, flash, console, a JTAG breakpoint and UART0
   recovery before installing the board in the enclosure.
3. Emulate the NTC using a resistor box, keeping the cell at room temperature.
   Check nominal 10 kΩ permission and open/short inhibition; check 27.28 kΩ
   (0°C) and approximately 4.91 kΩ (45°C) inhibit charging. Sweep both thresholds
   and cycle USB with firmware halted. Then verify the bonded sensor and
   actual charge current/termination on the protected pack. Do not heat or
   freeze a connected pouch cell merely to test the thresholds.
4. Connect the display and exercise full/partial refresh while Wi-Fi is active.
   Scope 3V3 and panel rails, check brownout margin, off-state bus voltage and
   leakage, and repeat refresh after deep sleep and USB insertion/removal.
5. Lift R34 and measure whole-device battery sleep current across its pads.
   The target is below 30 µA; this has not been measured. Account for meter
   burden and bypass the meter for large transient tests if needed.
6. Print the coupons, fit the real cable/display/harness, check cap return and
   stop contact after actuation, and close the enclosure without pressure on glass or pouch.
   Test radio performance with the case, battery and optional steel installed.
   Record temperatures and current under combined charge/radio/display load.

## Verification record

KiCad 10.0.5: zero ERC violations, zero DRC violations, zero unconnected pads and
zero schematic parity differences. Independent copper audit: no clearance or
edge conflicts, no separately routed vias overlapping footprint pads and no
non-45° segments. U1/U2 retain their twelve/two intentional footprint thermal
holes, tented on the opposite face; preserve the supplied masks. Placement audit:
no courtyard or through-hole/component collisions and no missing models.
The KiCad reports list the project's ignored rule categories in their footers;
the zero counts use those configured rules. Separate pin/land, placement and
copper audits cover checks that ERC/DRC alone cannot establish.
Both main USB runs have uninterrupted In1 ground reference outside explicit
0.55 mm transition-via launch exemptions. The audit reads the actual routes.

The assembly BOM/CPL cover exactly 89 fitted parts. Gerbonara independently
read all eleven Gerber layers and matched all 492 plated drill/slot features
and nine NPTH features against KiCad, within Excellon's 1 µm coordinate
resolution. Its syntax/macro warnings concern KiCad's accepted export formatting;
no layer or drill mismatch was found.

OpenSCAD 2026.06.12: 18 selectors, thirteen closed oriented single-solid meshes,
fourteen empty nominal collision checks and a positive-volume stop contact.
Host tests: 84 Python and 101 website tests; PCB tools: 45 tests; mesh checker:
six tests. Rev A debug, charset and release/MCUboot firmware builds succeed.
All 226 executed firmware cases pass across 16 QEMU configurations, with
three platform-specific skips. This includes 15 base and 16 Rev A integration
cases, charger fault/idle behavior, ADC failure cleanup and stopping pending
ADC/LED work before sleep. Compiled flash, GPIO and display settings match the
PCB/case contract.
No bench, physical flash, JTAG or physical print result is claimed.

## Primary drawing and data sources

- [Kingbright APBA2006](https://www.kingbrightusa.com/images/catalog/SPEC/APBA2006SURKCGKC.pdf)
- [Littelfuse KSC3 switch drawing and ratings](https://www.littelfuse.com/assetdocs/littelfuse-ck-tactile-ksc3-series-datasheet?assetguid=846c4254-b69f-4cfb-8b4d-abd3d115c59a)
- [TI TPS63802 and DLA0010A land](https://www.ti.com/lit/ds/symlink/tps63802.pdf)
- [TI BQ25185](https://www.ti.com/lit/ds/symlink/bq25185.pdf), [TLV9022](https://www.ti.com/lit/ds/symlink/tlv9022.pdf), [TPS22917](https://www.ti.com/lit/ds/symlink/tps22917.pdf)
- [Coilcraft XFL4015](https://www.coilcraft.com/getmedia/84927b8b-f089-421b-a7f4-a0fa23afe908/xfl4015.pdf), [TDK VLS4012CX](https://product.tdk.com/en/search/inductor/inductor/smd/info?part_no=VLS4012CX-470M-1)
- [JST PH catalogue](https://www.jst-mfg.com/product/pdf/eng/ePH.pdf), [Hirose exact 24-pin connector](https://www.hirose.com/product/p/CL0586-0521-0-55)
- [HRO TYPE-C-31-M-12 drawing](https://datasheet.lcsc.com/datasheet/pdf/9e56b777c022540fcce7c7f67825f55e.pdf?productCode=C165948)
- [Good Display W2 drawing, page 7](https://files.seeedstudio.com/wiki/Other_Display/213-epaper/GDEY0213B74.pdf)
- [Adafruit 258 protected pack drawing](https://cdn-shop.adafruit.com/product-files/258/C101-_Li-Polymer_503562_1200mAh_3.7V_with_PCM_APPROVED_8.18.pdf), [Semitec AT thermistors](https://www.semitec-global.com/uploads/2022/01/P12-13-AT-Thermistor.pdf)
- Samsung DC-bias data: [47 µF](https://product.samsungsem.com/mlcc/CL21A476MPYNNN.do), [22 µF](https://product.samsungsem.com/mlcc/CL10A226MQ8NRN.do), [10 µF 0603](https://product.samsungsem.com/mlcc/CL10A106KP8NNN.do)
- [Espressif PCB guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html), [TC2030 contact drawing](https://www.tag-connect.com/wp-content/uploads/bsk-pdf-manager/2019/12/TC2030-IDC-NL-Datasheet-Rev-B.pdf)
