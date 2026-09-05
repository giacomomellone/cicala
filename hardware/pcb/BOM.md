# BOM — Rev A

The board is placed and routed and passes ERC, DRC and schematic-to-PCB
parity. This list records the selected electrical parts and off-board
assemblies. It is not yet an orderable PCBA BOM: the project-drawn land
patterns need a manufacturer-drawing review, and manufacturer part numbers,
alternates and live assembler stock still need to be pinned. The generated
BOM that goes to an assembler is `cicala_rev_a/exports/assembly/`, written by
`hardware/pcb/export_fab.sh`. For the development rig and physical-model
purchases, use [docs/prototype_bom.md](../../docs/prototype_bom.md).

Project-drawn land patterns live in `cicala_rev_a/cicala.pretty` (registered
through `fp-lib-table`): `Texas_DLA0010A_VSON-HR-10_2x3mm_P0.5mm` (U3, no
exposed pad), `L_Coilcraft_XFL4015` (L2), `L_TDK_VLS4012` (L1), the
preliminary `LED_Kingbright_APBA2006SURKCGKC` (D4), and
`USB_C_Receptacle_HRO_TYPE-C-31-M-12_EdgeOverhang` (J1), which is the KiCad
library land with its silkscreen trimmed at the board edge because the part
deliberately overhangs it. All other parts use KiCad library footprints.

## Selected parts

| Ref      | Function                                      | Package or interface             | Selected MPN                                                | Qty | Status |
| -------- | --------------------------------------------- | -------------------------------- | ----------------------------------------------------------- | --: | ------ |
| U1       | ESP32-S3 module, 16 MB flash                 | Espressif module                 | ESP32-S3-WROOM-1-N16; alternate ESP32-S3-WROOM-1U-N16       |   1 | WROOM-1 preferred; placement/RF gate |
| DISP1    | 2.13″, 250 × 122 e-paper panel               | 24-pin, 0.5 mm FPC               | [GDEY0213B74](https://www.good-display.com/product/391.html) |   1 | Selected off-board assembly |
| J2       | Bottom-contact ZIF display connector          | 24-pin, 0.5 mm                   | FH12-24S-0.5SH(55)                                          |   1 | Footprint candidate; folded-cable orientation gate |
| J1       | USB-C USB 2.0 receptacle                     | 16-pin mid-mount SMD             | TYPE-C-31-M-12                                              |   1 | Selected; enclosure/courtyard gate |
| U4       | USB 2.0 ESD protection                      | SOT-23-6                         | USBLC6-2SC6                                                 |   1 | Selected |
| U2       | Single-cell charger and power path           | DLH WSON-10, 2.2 × 2.0 mm        | BQ25185DLHR                                                 |   1 | Selected; 250 mA charge, 500 mA input, 4.2 V cell |
| U3       | 3.3 V low-IQ buck-boost regulator            | DLA VSON-10, 2.0 × 3.0 mm        | TPS63802DLAR                                                |   1 | Selected; footprint drafted (cicala, no EP); land review pending |
| U5, U6   | Battery-divider and display load switches    | SOT-23-6                         | TPS22917DBVR                                                |   2 | Selected |
| L2       | Buck-boost inductor, 0.47 µH                 | 4.0 × 4.0 mm                     | XFL4015-471ME                                               |   1 | Selected; footprint drafted (cicala); land review pending |
| L1       | E-paper boost inductor, 47 µH                | 4.0 × 4.0 × 1.2 mm               | VLS4012CX-470M-1                                            |   1 | Selected; footprint drafted (cicala); land review pending |
| Q1       | E-paper boost MOSFET                         | SC-70                            | Si1308EDL-T1-GE3                                           |   1 | Selected |
| D1–D3    | E-paper boost Schottky diodes                | SOD-123                          | MBR0530                                                    |   3 | Selected |
| SW1, SW2 | Sealed 2 N SPST-NO tact switches             | 6.2 × 6.2 mm SMD                 | KSC321GLFS                                                 |   2 | Selected; footprint KiCad CK_KSC6xxG confirmed vs KSC3 land; cap stack open |
| D4       | Right-angle red/green bi-colour LED          | 2.0 × 1.0 mm side-view SMD       | APBA2006SURKCGKC                                           |   1 | Selected; footprint drafted (cicala, PRELIMINARY); pad-map + optical coupling open |
| J3       | Protected-cell connector with thermistor     | 3-pin JST-PH, horizontal SMD      | S3B-PH-SM4-TB(LF)(SN), mated custom harness                |   1 | Connector selected; wire order must be keyed in drawing |
| BT1      | Protected 1S LiPo, nominal 500 mAh           | 503035-class pack, 3-wire harness | Custom pack with PCM and 10 kΩ, B=3435 K NTC               |   1 | Supplier drawing and enclosure fit open |
| J4       | Concealed recovery connector                 | Tag-Connect TC2030-IDC-NL pads    | PCB footprint only                                         |   1 | DNP connector; underside access gate |
| —        | Case screws                                  | M2.5 × 8 thread-forming, pan head | Into the shell bosses from below, through steel and base   |   4 | Length set by the coupon result |

The WROOM variants are alternatives on one assembly position. WROOM-1 is preferred because setup-portal use is expected at close range and avoids an external antenna, cable and connector. It remains conditional on a placement that satisfies Espressif's antenna keep-out and an assembled radio test. WROOM-1U stays available if that placement cannot clear the display, cell, buttons and enclosure metal.

The protected cell is a specification rather than a frozen supplier MPN. A [published protected LP503035 example](https://www.lipolbattery.com/LiPo-Battery-Datahseet/LiPo_Battery_LP503035_3.7V_500mAh.pdf) is approximately 36 × 30 × 5 mm before allowing for the wire exit and swelling. The current CAD keep-out is 35 × 30 × 5.4 mm, so an exact pack cannot be ordered until the enclosure volume is increased or a verified drawing fits it.

## Captured settings and passives

- USB-C is a 5 V sink with separate 5.1 kΩ CC1/CC2 pull-downs. There is no USB-PD controller. Native USB D−/D+ passes through USBLC6-2SC6 and 22 Ω series resistors to ESP32-S3 GPIO19/20. Optional 3 pF shunts are DNP.
- BQ25185 uses 18 kΩ on ILIM/VSET and 1.20 kΩ on ISET for a 4.2 V cell, 500 mA input limit and 250 mA charge current. STAT1/STAT2 have 10 kΩ pull-ups. `/CE` has a 100 kΩ pull-down so charging is enabled when firmware is absent. TS/MR is a separate `BATT_NTC` net carrying only the pack NTC wire (J3.2); the charger sources ~38 µA into TS, so the 10 kΩ B=3435 thermistor to GND needs no external bias resistors and sets roughly a 1.5 °C to 59 °C charge window.
- The battery ADC uses a 1 MΩ/470 kΩ divider. TPS22917 switches the complete divider off during sleep; a 100 nF capacitor filters the ADC node.
- TPS63802 uses a 0.47 µH inductor, 511 kΩ/91 kΩ feedback divider, 10 µF input capacitance and 22 µF plus 47 µF output bulk. MODE is low for power-save operation.
- The e-paper boost and reservoir network follows the GDEY0213B74 reference circuit. TPS22917 disconnects panel power between refreshes and discharges the switched rail through 150 Ω.
- Both switches use 47 kΩ pull-ups and 10 nF hardware debounce. The LED dies each use 1 kΩ in series so both GPIOs low leave no standing LED current.
- R27–R31 put 470 Ω in series with every MCU-driven panel line. They cap the current injected into an unpowered panel's ESD diodes at about 5.7 mA per pin and double as the series-tuning positions; fit 0 Ω to remove them during bring-up. R32 is a 1 MΩ pull-down that holds the panel in reset while its rail is off.
- R33 (0 Ω, fitted) ties the USB-C shell to board ground; C31 (1 nF, 2 kV) is the alternative for a DC-isolated shell. The enclosure is plastic and carries no chassis ground, so this is the shell's only discharge path.
- R34 is a removable 0 Ω link in the cell lead, with TP22 on the pack side. Lifting it puts a meter between the pack and everything else, which is how whole-device sleep current gets measured against the 30 µA target.
- Ordinary resistors and small capacitors are 0603. High-capacitance and 25 V pump capacitors use 0805 where marked in the schematic. Voltage bias, tolerance and temperature rating must be checked when manufacturer part numbers are assigned.

These packages are compatible with professional PCBA. The 0.4 mm-pitch BQ25185 WSON, exposed-pad regulator, fine-pitch FPC and USB receptacle make assembler placement preferable to hand soldering. Before requesting JLCPCB or PCBWay assembly, map every line to a stocked manufacturer part, add approved alternates, confirm any extended-part fees, and request inspection appropriate to the fine-pitch and bottom-terminated joints.

## Gates before a PCBA quote

1. Confirm the drafted project land patterns for U3, L1, L2 and D4 against current manufacturer drawings. The KSC6xxG library land is accepted for SW1/SW2 — it matches the KSC321G drawing (3.1 × 1.0 mm pads, 8.9 mm column and 4.0 mm row centres). The U3 land omits an exposed pad because the DLA0010A HotRod package has none. The D4 land is preliminary and its pad numbers follow the `LED_Dual_AAKK` symbol (pad1 = red anode … pad4 = green cathode), not the datasheet pin numbers; verify both, along with which face the part emits from — the light pipe depends on it. Recheck the J1 and J2 library footprints and J2 contact orientation.
2. Map every line to a stocked manufacturer part with approved alternates, and confirm any extended-part fees.
3. Test radio performance with WROOM-1 in the assembled enclosure. Espressif's keep-out is honoured on the board, but a keep-out is not a measurement.
4. Obtain an exact protected-cell drawing with PCM, 10 kΩ NTC, connector pin order, wire exit and swelling allowance. The 36 × 30 × 5.4 mm keep-out clears the published LP503035 drawing; the ordered pack still has to fit it.
5. Obtain the assembler's four-layer controlled-impedance stack and re-check the USB pair against it. The 0.29 mm/0.29 mm geometry assumes the declared 0.18 mm prepreg at εr 4.5.
6. Verify e-paper refresh brownout margin, charger temperature (including the TS/NTC charge window), USB flashing/serial/JTAG, sleep current at the R34 link and display power-off leakage on assembled boards.
