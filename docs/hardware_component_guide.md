# Rev A component guide

Why each part on the Rev A schematic is there. It walks the three functional
sheets — Power and USB, Controller and user I/O, E-paper display — and gives the
role of every component, not just the ICs. The selected part numbers and
sourcing notes are in [hardware/rev_a/BOM.md](https://github.com/giacomomellone/cicala/blob/main/hardware/rev_a/BOM.md);
the coordinate, pin and validation contracts are in [rev A hardware](hardware_rev_a.md).

This is an electrical-review baseline, not a fabrication release. The wiring
errors found so far are fixed and recorded under
[Wiring errors found and fixed](#wiring-errors-found-and-fixed); none of them
were visible to ERC.

Values quoted are the schematic values. Voltage rating, tolerance and dielectric
still have to be pinned when each line gets an ordered manufacturer part.

## Power and USB

### USB-C receptacle and CC detection

One USB-C port brings in 5 V and carries USB 2.0 data. There is no USB-PD
controller: the device is a plain 5 V sink.

| Ref | Value            | Role                                                                                                  |
| --- | ---------------- | ----------------------------------------------------------------------------------------------------- |
| J1  | USB-C 2.0 16-pin | Power and data inlet. VBUS and GND are doubled on the receptacle; the SBU and shield pins are unused. |
| R1  | 5.1 kΩ           | CC1 pull-down (Rd). Advertises the board as a sink so a source turns on 5 V.                          |
| R2  | 5.1 kΩ           | CC2 pull-down (Rd). The second CC line makes detection work in both plug orientations.                |

Two 5.1 kΩ resistors (one per CC line) are required rather than one, because a
reversible cable can land the source's CC on either pin.

### Data-line ESD and series termination

| Ref | Value       | Role                                                                                                                          |
| --- | ----------- | ----------------------------------------------------------------------------------------------------------------------------- |
| U4  | USBLC6-2SC6 | Low-capacitance ESD/TVS array on the D+/D− pair, placed at the connector so a discharge is clamped before it reaches the MCU. |
| R3  | 22 Ω        | Series resistor on D− between the TVS and the ESP32-S3, for impedance matching and edge control.                              |
| R4  | 22 Ω        | Series resistor on D+, same purpose.                                                                                          |
| C1  | 3 pF, DNP   | Optional D− shunt for extra edge slowing. Not populated; a footprint kept for tuning.                                         |
| C2  | 3 pF, DNP   | Optional D+ shunt, same.                                                                                                      |

The 22 Ω series parts sit between the TVS and the MCU so the clamp is closest to
the connector. The native USB pins (GPIO19/20) are the ESP32-S3 USB-Serial/JTAG
block, so this one port does flashing, serial console and JTAG without a USB-UART
bridge chip.

### Shell grounding

| Ref | Value      | Role                                                                             |
| --- | ---------- | -------------------------------------------------------------------------------- |
| R33 | 0 Ω        | Ties the USB-C shell to board ground. Fitted by default.                         |
| C31 | 1 nF, 2 kV | The alternative if a DC-isolated shell is ever wanted; not fitted alongside R33. |

The enclosure is plastic and carries no chassis ground, so the shell's only
discharge path is into the board. A direct connection at the receptacle gives
that discharge the shortest route to the ground plane.

### VBUS presence sensing

| Ref | Value     | Role                                                                                                                                                                                  |
| --- | --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| R5  | 100 kΩ    | Top of the VBUS divider, from VBUS to the sense node.                                                                                                                                 |
| R6  | 150 kΩ    | Bottom of the VBUS divider. 100 k over 150 k scales 5 V to 3.0 V, which clears the ESP32-S3 guaranteed high-input threshold at 3.3 V. The earlier 150 k/100 k gave 2.0 V and did not. |
| C4  | 1 µF 25 V | Bulk/decoupling on VBUS at the charger input.                                                                                                                                         |
| C3  | 100 nF    | Filters the divided VBUS-sense node against noise and switching transients.                                                                                                           |

The high-value divider keeps the standing current on VBUS at 20 µA, which
matters for the sleep budget when USB is attached but idle.

### BQ25185 charger and power path

The charger runs the cell and supplies the system rail (VSYS) with power-path
switching between USB and battery.

| Ref | Value      | Role                                                                                              |
| --- | ---------- | ------------------------------------------------------------------------------------------------- |
| U2  | BQ25185    | Single-cell linear charger with power path. Sets charge from USB and hands VSYS to the regulator. |
| C4  | 1 µF 25 V  | Input (VBUS/IN) capacitor, shared with the VBUS sense group above.                                |
| C5  | 10 µF 25 V | SYS/VSYS output capacitor for the power path.                                                     |
| C6  | 1 µF 16 V  | BAT-pin decoupling next to the cell connection. 16 V keeps useful capacitance at 4.2 V.           |
| R7  | 18 kΩ      | ILIM/VSET — sets the 4.2 V cell target and the 500 mA input current limit.                        |
| R8  | 1.20 kΩ    | ISET — sets the 250 mA fast-charge current for the ~500 mAh pack.                                 |
| R9  | 10 kΩ      | Pull-up on the open-drain STAT1 status output so the MCU can read it.                             |
| R10 | 10 kΩ      | Pull-up on the open-drain STAT2 status output.                                                    |
| R13 | 100 kΩ     | Pull-down on /CE so charging is enabled by default even before firmware runs.                     |

STAT1/STAT2 are open-drain, so they need the two 10 kΩ pull-ups to present a
logic level. The /CE pull-down is a deliberate fail-safe: a blank board still
charges.

### Protected cell and temperature sense

| Ref | Value                | Role                                                                                                                                                                                                               |
| --- | -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| J3  | 3-pin JST-PH         | Protected LiPo connector: VBAT, the NTC sense wire, and GND.                                                                                                                                                       |
| R34 | 0 Ω                  | Removable link in the cell lead, with TP22 on the pack side. Lifting it puts a meter between the pack and everything else, which is how whole-device sleep current is measured. Specify a part rated at least 1 A. |
| BT1 | 503035-class 1S LiPo | Off-board protected pack with PCM and a 10 kΩ B=3435 NTC on a keyed three-wire harness.                                                                                                                            |

The charger sources about 38 µA into its TS pin, so the pack's 10 kΩ NTC to GND
sets the temperature window directly (roughly 1.5 °C cold to 59 °C hot) with no
external bias resistors. TS/MR is its own `BATT_NTC` net carrying only the NTC
wire; tying it to VBAT (an earlier error, now fixed) would have read as
out-of-range and suspended charging.

### TPS63802 buck-boost 3.3 V rail

A buck-boost holds 3.3 V across the whole useful LiPo range, where a linear
regulator would drop out as the cell approaches 3.3 V.

| Ref | Value             | Role                                                                                                                                                                                                                    |
| --- | ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| U3  | TPS63802          | Low-IQ buck-boost regulator, VSYS in, 3V3 out.                                                                                                                                                                          |
| L2  | 0.47 µH (XFL4015) | Buck-boost inductor; the single energy-storage element the topology switches.                                                                                                                                           |
| C8  | 10 µF 25 V        | Input capacitor on VSYS at the regulator. 25 V because SLUSF65B section 7.2.2.3 asks for it on the SYS node, and because the regulator needs 4 µF effective on VIN, which a 6.3 V part at 4.5 V does not reliably give. |
| C9  | 22 µF 6.3 V       | Output bulk on 3V3.                                                                                                                                                                                                     |
| C10 | 47 µF 6.3 V       | Additional 3V3 output bulk for transient response and display-refresh load steps.                                                                                                                                       |
| C11 | 100 nF            | High-frequency 3V3 decoupling.                                                                                                                                                                                          |
| R14 | 511 kΩ 1%         | Top of the feedback divider.                                                                                                                                                                                            |
| R15 | 91 kΩ 1%          | Bottom of the feedback divider. 511 k/91 k sets V_out = 0.5 V·(1 + 511/91) ≈ 3.31 V.                                                                                                                                    |
| R16 | 100 kΩ            | Pull-up on the open-drain PG (power-good) output, exposed as REG_PG.                                                                                                                                                    |

The 1 % feedback resistors set the output voltage, so their tolerance matters;
ordinary parts elsewhere can be 5 %. MODE is tied low for power-save operation at
light load, which is where the device spends most of its life.

### Switched battery measurement

Measuring the cell through a fixed divider would leak current continuously, so
the divider is switched off except during a reading.

| Ref | Value    | Role                                                                                   |
| --- | -------- | -------------------------------------------------------------------------------------- |
| U5  | TPS22917 | Load switch that connects the battery divider only when the MCU asserts VBAT_SENSE_EN. |
| R11 | 1 MΩ     | Top of the battery-sense divider.                                                      |
| R12 | 470 kΩ   | Bottom of the battery-sense divider; 1 M/470 k scales VBAT into the ADC range.         |
| C7  | 100 nF   | Filters the ADC sense node for a stable reading.                                       |

The megohm-scale divider keeps the measurement current tiny, and the load switch
removes even that during sleep.

## Controller and user I/O

### ESP32-S3 module and boot straps

| Ref | Value                | Role                                                                                                               |
| --- | -------------------- | ------------------------------------------------------------------------------------------------------------------ |
| U1  | ESP32-S3-WROOM-1(-U) | The controller: Wi-Fi/BLE SoC module running the firmware. WROOM-1 is preferred; WROOM-1U is the antenna fallback. |
| C28 | 100 nF               | High-frequency decoupling at the module 3V3 pin.                                                                   |
| C29 | 10 µF                | Mid-band bulk for the module supply.                                                                               |
| C30 | 47 µF                | Bulk to hold the rail through radio and flash current bursts.                                                      |
| R25 | 10 kΩ                | EN pull-up so the module runs by default.                                                                          |
| C27 | 1 µF                 | EN capacitor: an RC delay on reset for a clean power-up, and debounce on EN.                                       |
| R26 | 10 kΩ                | IO0 pull-up so the module boots from flash normally; pulling IO0 low selects download mode.                        |

The EN pull-up plus 1 µF gives the reset an RC rise; the IO0 pull-up sets normal
boot. Both nets reach the recovery header so a bad image can be forced into
download mode.

### Buttons

| Ref | Value   | Role                                                                   |
| --- | ------- | ---------------------------------------------------------------------- |
| SW1 | KSC321G | "Category" tactile switch (also an RTC wake source).                   |
| SW2 | KSC321G | "Next" tactile switch (also an RTC wake source).                       |
| R21 | 47 kΩ   | Pull-up on the Category line; the switch pulls it to GND when pressed. |
| R22 | 47 kΩ   | Pull-up on the Next line.                                              |
| C25 | 10 nF   | Hardware debounce on Category, with R21.                               |
| C26 | 10 nF   | Hardware debounce on Next, with R22.                                   |

47 kΩ pull-ups keep the standing current on each idle button under 100 µA; the
10 nF caps give an RC debounce so a single press is not read as several.

### Status LED

| Ref | Value              | Role                                                                  |
| --- | ------------------ | --------------------------------------------------------------------- |
| D4  | APBA2006 red/green | Side-view bi-colour status indicator, driven anode-high by two GPIOs. |
| R23 | 1 kΩ               | Red-channel series/current-limit resistor.                            |
| R24 | 1 kΩ               | Green-channel series resistor.                                        |

Both dies are AlGaInP: hyper red at 1.95 V typical and 2.5 V maximum, green at
574 nm, 2.1 V typical and 2.5 V maximum. From 3.3 V through 1 kΩ that is
0.8 mA in the worst case and about 1.3 mA typically. That is a deliberately
small current, and how much of it reaches the eye through the light pipe is a
bench measurement, not a calculation.

Each die has its own series resistor, so both GPIOs low leaves no standing LED
current in sleep. The part is two independent diodes; the board ties both
cathodes to GND to use it as a common-cathode LED.

### Recovery header and production test points

| Ref      | Value                    | Role                                                                                                                                                       |
| -------- | ------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| J4       | Tag-Connect TC2030 (DNP) | Concealed recovery interface: 3V3, GND, EN, IO0, UART0_TX, UART0_RX — a full UART download path when native USB is unavailable. Pads only, no fitted part. |
| TP1–TP11 | test points              | Bring-up/probe access: BOOT_IO0, CHIP_EN, GND, UART0_TX, UART0_RX, FACTORY_RESET_OD, 3V3, VBAT, VSYS, USB_VBUS, REG_PG.                                    |

USB-C is the normal flashing/console/JTAG port. J4 and the test points are the
fallback for when firmware has reconfigured or disabled the native USB; they live
on the underside and are reached only with the case open.

## E-paper display

The panel is a GDEY0213B74 with an on-glass SSD1680 controller. Its analog
supplies are generated on-board by a boost and a pair of charge pumps, following
Good Display's reference circuit, and are switched off between refreshes.

### Switched panel supply

| Ref | Value    | Role                                                                                                                   |
| --- | -------- | ---------------------------------------------------------------------------------------------------------------------- |
| U6  | TPS22917 | Load switch that removes 3V3 from the panel and boost network between refreshes (gated by EPD_PWR_EN).                 |
| C12 | 1 µF     | Input decoupling on the switch's 3V3 input.                                                                            |
| C13 | 10 µF    | Output bulk on the switched panel rail (EPD_3V3).                                                                      |
| C14 | 1 nF     | CT capacitor setting the switch's controlled turn-on slew, to limit inrush into the panel caps.                        |
| R18 | 150 Ω    | Quick-output-discharge resistor: bleeds the panel rail down fast when the switch opens, so the panel restarts cleanly. |

Switching the whole panel supply is what makes the display-off leakage near zero;
the CT slew limits inrush and the QOD resistor guarantees a clean rail collapse.

### SSD1680 boost and charge pumps

| Ref | Value           | Role                                                                                       |
| --- | --------------- | ------------------------------------------------------------------------------------------ |
| L1  | 47 µH (VLS4012) | Boost inductor for the panel's high-voltage generation.                                    |
| Q1  | Si1308EDL       | Switching MOSFET driven by the SSD1680 gate-driver output (EPD_GDR).                       |
| R20 | 2.2 Ω           | Boost current-sense resistor at the RESE pin, setting the switch current.                  |
| R19 | 1 MΩ            | Gate pull-down on EPD_GDR, keeping the MOSFET defined and off when the driver is inactive. |
| D1  | MBR0530         | Output diode of the negative (VGL) pump: cathode on EPD_PUMP, anode on EPD_PREVGL.         |
| D2  | MBR0530         | Clamp diode of the VGL pump: anode on EPD_PUMP, cathode to GND.                            |
| D3  | MBR0530         | Schottky in the positive (VGH) charge pump from the switch node.                           |
| C15 | 4.7 µF 25 V     | Boost/panel reservoir on EPD_3V3.                                                          |
| C16 | 4.7 µF 25 V     | Flying capacitor between the switch node and the pump node EPD_PUMP.                       |

Schottky diodes are used for their low forward drop and fast recovery, which the
charge pumps need to reach the panel's ±15 V-class rails efficiently.

### Bus isolation and reset parking

The panel's logic pins sit behind series resistors so that driving one while
`EPD_3V3` is off cannot push current into the panel through its own ESD diodes.

| Ref     | Value | Role                                                                                                                                                                                                |
| ------- | ----- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| R27–R31 | 470 Ω | In series with CLK, MOSI, CS, D/C and RES#. Caps the injected current at about 5.7 mA per pin against a 3.3 V output and a diode drop, and damps the edges; fit 0 Ω to remove them during bring-up. |
| R32     | 1 MΩ  | Pull-down on the panel side of RES#, so the panel is held in reset while its rail is off and while the MCU pin is high-impedance.                                                                   |

At 4 MHz into roughly 20 pF of connector and trace, 470 Ω adds about 21 ns of
rise time, which is a small fraction of the 125 ns half-period. The resistors
are the fallback, not the plan: firmware still parks the bus before removing
panel power.

### Panel rail reservoirs and FPC

Each SSD1680 analog rail gets its own reservoir capacitor so it holds voltage
through a refresh.

| Ref | Value          | Rail / role                                                   |
| --- | -------------- | ------------------------------------------------------------- |
| C17 | 1 µF 25 V      | EPD_PREVGH pre-charge-pump reservoir                          |
| C18 | 1 µF 25 V      | EPD_VSH2 (positive source)                                    |
| C19 | 1 µF 25 V      | EPD_3V3 (panel VCI/VDDIO)                                     |
| C20 | 1 µF 25 V      | EPD_VDD                                                       |
| C21 | 1 µF 25 V      | EPD_VSH1                                                      |
| C22 | 1 µF 25 V      | EPD_VSL (negative source)                                     |
| C23 | 1 µF 25 V      | EPD_PREVGL pre-charge-pump reservoir                          |
| C24 | 1 µF 25 V      | EPD_VCOM (common electrode)                                   |
| J2  | FH12-24S-0.5SH | 24-pin 0.5 mm bottom-contact ZIF connector for the panel FPC. |

The 25 V rating on the pump and rail capacitors is deliberate: the charge-pump
nodes swing well above 3.3 V.

## Wiring errors found and fixed

Four errors reached the captured schematic and all four passed ERC, because each
was an electrically valid connection that was simply wrong. They are fixed; the
list is kept so the same checks get repeated on the next capture.

- **USB-C D+/D− shorted, then crossed (critical).** J1's D+ (A6/B6), D− (A7/B7)
  and CC2 (B5) first sat on one net tied to both USBLC6 line pins. Separating
  them left the pair crossed: the connector's D− reached GPIO20 (USB_D+) and D+
  reached GPIO19 (USB_D−). D− now runs J1 → U4 I/O1 → R3 → USB_DN → GPIO19 and
  D+ runs J1 → U4 I/O2 → R4 → USB_DP → GPIO20.
- **Q1 gate and source swapped.** The capture used `Q_NMOS_SGD`, but the
  Si1308EDL in SOT-323 is pin 1 = G, 2 = S, 3 = D. The symbol is now
  `Q_NMOS_GSD`, so EPD_GDR lands on the gate and EPD_RESE on the source.
- **D1 shorted, then D2 on the wrong node.** Both D1 terminals sat on
  EPD_PREVGL. Freeing D1 left D2's anode on EPD_PREVGL instead of the pump node,
  which gave the flying capacitor no charging path. D1's cathode, D2's anode and
  C16 now meet at EPD_PUMP, matching the GDEY0213B74 reference circuit.
- **C21 was 1 nF.** VSH1 is a driving rail and the reference gives it 1 µF/25 V,
  as every other panel rail here already had.

Three net names were also lost in a sheet re-layout and restored: `BATT_NTC`,
`BTN_CATEGORY` and `BTN_NEXT`, all of which `pin_contract.csv` refers to.

Two earlier errors from the same review were fixed before these: the TPS63802
feedback divider (FB had been shorted to VOUT, bypassing R14) and the BQ25185
TS/MR path (now the `BATT_NTC` net). See [decisions](decisions.md) for the
record.
