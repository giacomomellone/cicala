# Reading the Rev B board

The native `F.SilkS` and `B.SilkS` layers contain the Cicala wordmark and cicada,
functional boundaries, connector legends and a service guide. These markings
are part of the manufactured Gerbers. KiCad groups the added graphics as
**Rev B identity and service guide**.

## Front

Open brackets identify the display/boost, charge-temperature guard, power and
debug areas without crossing component bodies or solder openings. Large IC and
connector labels give the function alongside the reference. Dense passive
designators remain on `F.Fab` and in the assembly PDF.

The probe labels name the signals directly: `BOOT`, `EN`, `GND`, `3V3`, `BAT`,
`SYS`, `5V`, `S1`, `CLK`, `MOSI`, `CS`, `DC` and `BUSY`. `S1` means charge
status 1. J3's `+`, `T` and `-` align with its battery-positive, thermistor and
ground pins. J2 identifies the e-paper connector and downward-facing flex
contacts. Both switches and the status LED are named.

## Back

The underside is read with the board flipped over; its artwork is mirrored in
KiCad coordinates. It provides:

- Revision B.04, module/flash identification, four layers and 1.2 mm thickness.
- USB/battery/system/3.3 V power flow, nominal 100 mA charging and the U7/Q2
  temperature veto.
- The top-side test-point number key and direct labels for all seven bottom
  test contacts. Numbers refer to `TP` references; `BAT` is the charger-side
  battery rail and `CELL+` is the protected pack connector's positive rail.
- J4's six-pin recovery mapping, MCU-relative TX/RX direction and boot/reset
  sequence. J4 pin 1 is **3.3 V reference only**, not a power input.
- J3 polarity and the protected single-cell LiPo / bonded 10 kΩ B3435 NTC
  requirement, plus the J2 UC8253 display identification.
- An antenna-clearance reminder and the hardware license identifier.

`FACTORY OD` identifies an open-drain firmware signal; its eventual factory
operation remains unqualified. The printed service guide does not replace the
schematic or the first-power procedure.

## Print rules and checks

Main legends are 1.0 mm high; compact probe and connector labels are 0.8 mm.
Text and outline strokes are at least 0.15 mm. KiCad enforces 0.15 mm silk
clearance to edges and other markings, and checks solder-mask clipping. The
front text was also checked against the fitted component envelopes. The local
L1 outline is shortened on its Q1-facing side to preserve the ink gap; its
lands, courtyard and component position are unchanged.

The generator verifies every test-point net and both service-connector pinouts
before editing. Regression checks preserve copper, pads, placements, outline
and model geometry. The manufacturer must still review the resulting artwork;
physical print quality has not been measured.

## References

- [Adafruit ESP32-S3 Feather](https://learn.adafruit.com/adafruit-esp32-s3-feather/pinouts):
  nearby power, pin, reset/boot and hardware-variant labels. Rev B uses this
  approach for service information placed beside the relevant contact.
- [SparkFun PCB Basics](https://learn.sparkfun.com/tutorials/pcb-basics/all):
  silkscreen as a human-readable explanation of pins, LEDs and assembly.
- [PCBWay legend guidance](https://www.pcbway.com/helpcenter/design_instruction/How_to_make_my_silkscreen_clear_and_beautiful_on_PCB__.html):
  at least 0.15 mm strokes and 0.8 mm character height for ordinary positive text.
- [JLCPCB silkscreen guide](https://jlcpcb.com/blog/pcb-silkscreen-printing-guide):
  readable lettering, pad clearance, identity and service markings.

Reviewed 15 September 2026. No third-party artwork was copied. The logo and
wordmark are the project's existing public brand assets.

## Regeneration

From the repository root:

```sh
.venv/bin/python hardware/tools/rev_b/silkscreen.py
just hw-rev-b-check
just hw-rev-b-pcb-renders
just hw-rev-b-fab-export
```

The [render directory](renders/README.md) includes straight top/bottom PNGs and
zoomable SVG plots of both silkscreen layers with solder openings. The device
page imports these files directly. Refresh the complete enclosure export when
updating its source manifest, even though silkscreen does not change the fit.
