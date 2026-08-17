# BOM — Rev A schematic baseline

The seven KiCad subsystem sheets are captured and pass ERC. This list records the selected electrical parts and off-board assemblies. It is suitable for component sourcing and footprint review, but it is not yet an orderable PCBA BOM: six physical footprints, board placement, routing, alternates and live assembler stock still need review. For the development rig and physical-model purchases, use [docs/prototype_bom.md](../../docs/prototype_bom.md).

## Selected parts

| Ref      | Function                                      | Package or interface             | Selected MPN                                                | Qty | Status |
| -------- | --------------------------------------------- | -------------------------------- | ----------------------------------------------------------- | --: | ------ |
| U1       | ESP32-S3 module, 16 MB flash                 | Espressif module                 | ESP32-S3-WROOM-1-N16; alternate ESP32-S3-WROOM-1U-N16       |   1 | WROOM-1 preferred; placement/RF gate |
| DISP1    | 2.13″, 250 × 122 e-paper panel               | 24-pin, 0.5 mm FPC               | [GDEY0213B74](https://www.good-display.com/product/391.html) |   1 | Selected off-board assembly |
| J2       | Bottom-contact ZIF display connector          | 24-pin, 0.5 mm                   | FH12-24S-0.5SH(55)                                          |   1 | Footprint candidate; folded-cable orientation gate |
| J1       | USB-C USB 2.0 receptacle                     | 16-pin mid-mount SMD             | TYPE-C-31-M-12                                              |   1 | Selected; enclosure/courtyard gate |
| U4       | USB 2.0 ESD protection                      | SOT-23-6                         | USBLC6-2SC6                                                 |   1 | Selected |
| U2       | Single-cell charger and power path           | DLH WSON-10, 2.2 × 2.0 mm        | BQ25185DLHR                                                 |   1 | Selected; 250 mA charge, 500 mA input, 4.2 V cell |
| U3       | 3.3 V low-IQ buck-boost regulator            | DLA VSON-10, 2.0 × 3.0 mm        | TPS63802DLAR                                                |   1 | Selected; footprint open |
| U5, U6   | Battery-divider and display load switches    | SOT-23-6                         | TPS22917DBVR                                                |   2 | Selected |
| L2       | Buck-boost inductor, 0.47 µH                 | 4.0 × 4.0 mm                     | XFL4015-471ME                                               |   1 | Selected; footprint open |
| L1       | E-paper boost inductor, 47 µH                | 4.0 × 4.0 × 1.2 mm               | VLS4012CX-470M-1                                            |   1 | Selected; footprint open |
| Q1       | E-paper boost MOSFET                         | SC-70                            | Si1308EDL-T1-GE3                                           |   1 | Selected |
| D1–D3    | E-paper boost Schottky diodes                | SOD-123                          | MBR0530                                                    |   3 | Selected |
| SW1, SW2 | Sealed 2 N SPST-NO tact switches             | 6.2 × 6.2 mm SMD                 | KSC321GLFS                                                 |   2 | Selected; footprint/cap stack open |
| D4       | Right-angle red/green common-cathode LED     | 2.0 × 1.0 mm side-view SMD       | APBA2006SURKCGKC                                           |   1 | Selected; footprint and optical coupling open |
| J3       | Protected-cell connector with thermistor     | 3-pin JST-PH, horizontal SMD      | S3B-PH-SM4-TB(LF)(SN), mated custom harness                |   1 | Connector selected; wire order must be keyed in drawing |
| BT1      | Protected 1S LiPo, nominal 500 mAh           | 503035-class pack, 3-wire harness | Custom pack with PCM and 10 kΩ, B=3435 K NTC               |   1 | Supplier drawing and enclosure fit open |
| J4       | Concealed recovery connector                 | Tag-Connect TC2030-IDC-NL pads    | PCB footprint only                                         |   1 | DNP connector; underside access gate |

The WROOM variants are alternatives on one assembly position. WROOM-1 is preferred because setup-portal use is expected at close range and avoids an external antenna, cable and connector. It remains conditional on a placement that satisfies Espressif's antenna keep-out and an assembled radio test. WROOM-1U stays available if that placement cannot clear the display, cell, buttons and enclosure metal.

The protected cell is a specification rather than a frozen supplier MPN. A [published protected LP503035 example](https://www.lipolbattery.com/LiPo-Battery-Datahseet/LiPo_Battery_LP503035_3.7V_500mAh.pdf) is approximately 36 × 30 × 5 mm before allowing for the wire exit and swelling. The current CAD keep-out is 35 × 30 × 5.4 mm, so an exact pack cannot be ordered until the enclosure volume is increased or a verified drawing fits it.

## Captured settings and passives

- USB-C is a 5 V sink with separate 5.1 kΩ CC1/CC2 pull-downs. There is no USB-PD controller. Native USB D−/D+ passes through USBLC6-2SC6 and 22 Ω series resistors to ESP32-S3 GPIO19/20. Optional 3 pF shunts are DNP.
- BQ25185 uses 18 kΩ on ILIM/VSET and 1.20 kΩ on ISET for a 4.2 V cell, 500 mA input limit and 250 mA charge current. STAT1/STAT2 have 10 kΩ pull-ups. `/CE` has a 100 kΩ pull-down so charging is enabled when firmware is absent.
- The battery ADC uses a 1 MΩ/470 kΩ divider. TPS22917 switches the complete divider off during sleep; a 100 nF capacitor filters the ADC node.
- TPS63802 uses a 0.47 µH inductor, 511 kΩ/91 kΩ feedback divider, 10 µF input capacitance and 22 µF plus 47 µF output bulk. MODE is low for power-save operation.
- The e-paper boost and reservoir network follows the GDEY0213B74 reference circuit. TPS22917 disconnects panel power between refreshes and discharges the switched rail through 150 Ω.
- Both switches use 47 kΩ pull-ups and 10 nF hardware debounce. The LED dies each use 1 kΩ in series so both GPIOs low leave no standing LED current.
- Ordinary resistors and small capacitors are 0603. High-capacitance and 25 V pump capacitors use 0805 where marked in the schematic. Voltage bias, tolerance and temperature rating must be checked when manufacturer part numbers are assigned.

These packages are compatible with professional PCBA. The 0.4 mm-pitch BQ25185 WSON, exposed-pad regulator, fine-pitch FPC and USB receptacle make assembler placement preferable to hand soldering. Before requesting JLCPCB or PCBWay assembly, map every line to a stocked manufacturer part, add approved alternates, confirm any extended-part fees, and request inspection appropriate to the fine-pitch and bottom-terminated joints.

## Gates before a PCBA quote

1. Create and review land patterns for U3, L1, L2, SW1/SW2 and D4 against current manufacturer drawings. Recheck the J1 and J2 library footprints and J2 contact orientation.
2. Resolve WROOM-1 antenna placement or select WROOM-1U with an antenna and cable route. Test radio performance in the assembled enclosure.
3. Obtain an exact protected-cell drawing with PCM, 10 kΩ NTC, connector pin order, wire exit and swelling allowance; then update the enclosure keep-out.
4. Place and route the schematic, obtain the assembler's four-layer controlled-impedance stack, and re-enable schematic-to-PCB parity in `check_rev_a.sh`.
5. Verify e-paper refresh brownout margin, charger temperature, USB flashing/serial/JTAG, sleep current and display power-off leakage on assembled boards.
