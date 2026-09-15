# Rev B component selection evidence

Reviewed 15 September 2026. The selections in `hardware/rev_b/component_selection.csv` are applied to the native Rev B design. The assembled board has not been powered or measured. [The engineering notes](hardware_rev_b.md) describe the current geometry; this page records the source evidence and the decisions that need first-article measurements.

## Controller, display and boost

Retain **ESP32-S3-WROOM-1-N16**, native USB, recovery contacts and 16 MB flash. Use **Waveshare SKU 22609, 3.52-inch V1.1, UC8253**, with a 360 × 240 landscape image. The [V1.1 panel drawing](https://files.waveshare.com/wiki/3.52inch%20e-Paper%20HAT/3.52inch%20e-Paper%20V1.1.pdf) specifies pins 1 and 4 NC and pin 5 VDHR. The [linked HAT schematic](<https://files.waveshare.com/wiki/3.52inch%20e-Paper%20HAT%20(B)/3.52inch_e-Paper_HAT.pdf>) uses older names: pin 1 is unconnected; pins 4 and 5 each have a capacitor to ground. It does not externally drive pin 5 as a gate voltage.

The implementation follows the V1.1 pin table: **pins 1/4 NC, pin 5 VDHR with 1 µF to ground**. The PCB net name `EPD_VSH2` at pin 5 is the inherited host-side name for that reservoir. Pin 4 has no fitted or optional capacitor. This reconciles conflicting documents; it is not a vendor-issued correction. Compare actual rail waveforms with a reference HAT during bring-up.

| Circuit role | Applied MPN / value                                                       |
| ------------ | ------------------------------------------------------------------------- |
| L1           | TDK VLS4012CX-680M-1, 68 µH                                               |
| Q1           | Diodes BSS138-7-F, SOT-23: gate 1, source 2, drain 3                      |
| D1–D3        | Diodes MBR0530-7-F                                                        |
| R20 / R19    | Yageo RC0603FR-073RL / RC0603FR-0710KL, 3 Ω / 10 kΩ                       |
| C15 / C16    | Murata GRM31CR71H475KA12L, 4.7 µF, 50 V, X7R, 1206; 1.8 mm maximum height |
| C17–C24      | Samsung CL21B105KBFNNNE, 1 µF, 50 V, X7R, 0805                            |

The boost topology follows the HAT. C15 is input decoupling; C16 is the flying capacitor. The HAT input regulator and level translator are unnecessary with a 3.3 V host. Direct logic, panel supply switching and GPIO parking are retained. Check gate drive, rail startup, MLCC capacitance under bias and inductor current on the prototype. [TDK](https://product.tdk.com/en/search/inductor/inductor/smd/info?part_no=VLS4012CX-680M-1) specifies 1.625 Ω maximum DCR and 320 mA minimum saturation current at its 30% inductance-drop criterion. Sources: [BSS138](https://www.diodes.com/datasheet/download/BSS138.pdf), [Samsung capacitor](https://product.samsungsem.com/mlcc/CL21B105KBFNNN.do), [Murata capacitor](https://search.murata.co.jp/Ceramy/image/img/A01X/G101/ENG/GRM31CR71H475KA12-01A.pdf).

### Firmware interface

Zephyr [PR 113230](https://github.com/zephyrproject-rtos/zephyr/pull/113230) added UC8253 support, merged 4 August 2026 as `4420c8ce85d91ac7f309cb5fb1b13e75b843a844`. Cicala still pins Zephyr 4.4.1, whose checked-out driver lacks that support. The selected bring-up path is a reviewed project-owned backport; do not edit `deps/`. The [binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/display/ultrachip%2Cuc8253.html) uses active-low BUSY. Address the controller as **240 × 360**, then rotate the UI. A monochrome framebuffer occupies 10,800 bytes. Start with full refresh and the panel waveform settings. This hardware revision does not implement or validate that firmware port.

## Display connector and fold

**Molex 503480-2400** has dual contacts, a 0.30 ±0.05 mm FPC interface, 1 mm nominal body height and **1.58 mm insertion**. Its [manufacturer drawing](https://www.molex.com/content/dam/molex/molex-dot-com/products/automated/en-us/salesdrawingpdf/503/503480/5034802400_sd.pdf) requires at least 2 mm exposed contacts and 3 mm reinforcement. The panel drawing gives 3.15 ±0.30 mm contacts and 3.65 ±0.30 mm reinforcement, which meet those lengths.

The earlier FH12 selection was rejected: its [Hirose family drawing](https://www.hirose.com/en/product/series/FH12) calls for a longer reinforced end and exposed contact region than this panel supplies. Connector pitch and cable thickness alone did not establish compatibility.

The custom land pattern follows Molex drawing 503480 rev J/J1: 24 lands at 0.5 mm pitch, each 0.30 × 0.70 mm; two 0.30 × 1.00 mm retention lands at ±6.54 mm. Pin 1 is the left rear contact in the footprint drawing. The board rotates the footprint 90°, with cable entry toward the right. Inspect pin 1 and the dual-contact latch in the assembly viewer before ordering.

The supported static fold uses **R1.55 mm inside radius**. With maximum flex thickness 0.13 mm, minimum 9.70 mm tail, maximum 3.95 mm stiffener and 0.40 mm straight at the glass bond:

`9.70 − 3.95 − π × (1.55 + 0.13/2) − 0.40 = 0.276 mm`

That is the remaining length allowance, not a physical measurement. The nominal folded centerline fits the 10 mm tail with the reinforced end straight. The CAD entry height of **4.15 mm** is an assembly assumption based on the connector envelope, not a dimensioned contact-plane tolerance. Check insertion, contact side, latch access and bond loading on the first article. Use the rounded former as an assembly jig; remove it after forming. [Minco's static-flex guidance](https://www.minco.com/wp-content/uploads/Designing-a-Flex-Circuit-for-Flexibility.pdf) supports a starting bend ratio; it does not qualify this particular panel construction. Do not force the fold or bend the stiffener.

## Battery and harness

Select **Renata ICP303450PA-02 / 100701**, protected, 500 mAh minimum / 510 mAh typical, maximum 52 × 34.5 × 3.6 mm. Its [datasheet](https://www.renata.com/en-us/downloads/?fileid=362eb77e217c713a5b0570752b&product=icp303450pa-02) specifies 510 mA continuous discharge; its 1.02 A noncontinuous figure has no usable pulse-duration qualification. Use **Semitec 103JT-025**, an insulated 10 kΩ B3435 film NTC, maximum 0.5 mm thick, following the [sensor drawing](https://www.semitec-global.com/uploads/2022/01/P9-JT-Thermistor.pdf).

Retain BQ25185DLHR, TPS63802DLAR, TPS22917DBVR and the TLV9022DGKR charge-temperature veto. R8 = 3.00 kΩ selects nominal **100 mA charging**, with 4.2 V regulation. The JT-table tolerance model gives a nominal permission window of 4.44–37.05 °C and positive static stop margins at 0 and 45 °C; sensor bonding and thermal lag require physical measurements. Charging must remain inside the pack's 0–45 °C range including circuit tolerances. Hardware protection, the NTC veto and firmware have separate roles; verify all on the sample.

The low-profile **JST SM03B-SRSS-TB(LF)(SN)** uses **SHR-03V-S** and **SSH-003T-P0.2** contacts. Its wire range requires **AWG28 pigtails spliced to the pack's AWG26 leads**; do not crimp the stock leads directly into these contacts. Use at most 40 mm per power pigtail, individually insulated joints and strain relief. The 80 mm round-trip AWG28 copper contributes approximately 18 mΩ before connectors and joints. Pin 1 is protected BAT+, pin 2 NTC, pin 3 protected ground; the NTC returns to that ground. Keep the pack's factory protection. [JST SH drawing](https://www.jst-mfg.com/product/pdf/eng/eSH.pdf).

The 54 × 36 × 4.8 mm battery reservation includes the NTC, attachment, insulation and clearance. It is not a supplier-approved lifetime expansion allowance. Capture battery current and rail minima during Wi-Fi, OTA and display refresh. A 350 mA, 3.3 V load at 3.0 V battery and assumed 90% converter efficiency already needs about 428 mA from the cell. Schedule radio and refresh separately where possible and validate shutdown margin above the 3.0 V discharge endpoint. Runtime is not yet measured.

## Controls and magnetic carrier

**Alps SKRABCE010** gives a 6.2 mm body, 3.5 ±0.15 mm height, 0.4 ±0.2 mm travel and 1.2 ±0.4 N force. The [part drawing](https://tech.alpsalpine.com/e/products/detail/SKRABCE010/) supplies the custom lands and internally connected terminal rows. The 22 kΩ pull-ups provide about 150 µA, above the listed 10 µA minimum load. Printed guides take lateral force; the center tip acts vertically through a captured 0.5 mm solid silicone strip. Choose the final tip relief and stop from measured printed coupons.

The removable carrier specifies a made-to-drawing **CIC-MAG-01** accessory array; this is not a stocked MPN. The [Apple accessory guidelines](https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf), 8 June 2026, §42, describe 54.10/46.00 mm ring geometry, N48H magnets, radial polarity and a steel DC shield. The CAD reserves 1.10 mm magnets, 0.05 mm adhesive, 0.70 mm shield, 0.85 mm cover and 0.50 mm support, totaling **3.20 mm**. A generic axial ring is not an equivalent specification. Actual magnetic field, force and certification are unverified.

The nominal fit target is [iPhone 16 Plus](https://developer.apple.com/download/files/accessories/dimensional-drawings/iphone-16-plus.pdf): 77.76 × 160.89 mm; ring center 80.44 mm from the phone top; camera keepout to 43.72 mm. The carrier is **68.8 × 114.8 mm** in portrait, with its ring center 35.535 mm below its top. It occupies y=44.905–159.705 mm: nominal camera gap and bottom margin are both 1.185 mm. The cover has local fastener relief; screws, cover, magnets and shield pass the nominal intersection check. These are 2D screening values. Cases, optical cones, flash, alignment, retention and phone-loaded RF need physical checks. Carry with the radio end down; detach for wireless charging. The core remains usable without the carrier.

## Qualification after fabrication

No further user component decisions are required. Build a small engineering batch, then check power and recovery, the exact panel and fold, charging/NTC and pulse loads, printed button operation, optical support and attached radio behavior. Supplier CAM/assembly review belongs to the prototype order. It does not require delaying digital routing until the user obtains hardware.

If the panel fold fails on a sample, [GDEY037T03](https://www.good-display.com/product/437.html) is the preselected larger-panel redesign candidate. Its 92.99 × 53 mm glass requires a new envelope and circuit review; it is not a drop-in substitute. The smaller [GDEY031T10](https://www.good-display.com/product/426.html) sacrifices image area. Do not force an incompatible flex into the current design.
