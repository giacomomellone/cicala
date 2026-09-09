# Rev A bill of materials

The schematic contains exact manufacturer part numbers for every fitted PCB
component. The generated grouped BOM is
`cicala_rev_a/exports/assembly/cicala_rev_a_bom.csv`; the JLCPCB version also
includes purchasing identifiers where available. Do not substitute a package,
voltage rating or capacitor dielectric on value alone.

## Main PCB parts

| Reference | Selected part | Purpose |
| --- | --- | --- |
| U1 | ESP32-S3-WROOM-1-N16 | Controller, 16 MB flash, PCB antenna |
| U2 | BQ25185DLHR | Protected 1S cell charger and power path |
| U3 | TPS63802DLAR | 3.3 V buck-boost supply |
| U4 | USBLC6-2SC6 | USB ESD protection |
| U5, U6 | TPS22917DBVR | Switched battery measurement and display supply |
| U7 | TLV9022DGKR | Independent charge-temperature window comparator |
| Q2 | 2N7002P,215 | Charge-enable veto interface |
| J1 | HRO TYPE-C-31-M-12 | USB-C power, ROM flashing, serial and JTAG |
| J2 | Hirose FH12-24S-0.5SH(55) | Top-side, bottom-contact display ZIF |
| J3 | JST S3B-PH-SM4-TB(LF)(SN) | Protected cell, NTC and ground |
| L1 | TDK VLS4012CX-470M-1 | 47 µH display boost inductor |
| L2 | Coilcraft XFL4015-471ME | 0.47 µH buck-boost inductor |
| Q1 | Si1308EDL-T1-GE3 | Display boost MOSFET |
| D1–D3 | MBR0530-7-F | Display rail Schottky diodes |
| D4 | APBA2006SURKCGKC | Red/green side-view status LED |
| SW1, SW2 | KSC323GLFG | Gold-contact Filters and Next switches; R21/R22 = 22 kΩ |

Resistors are Yageo RC0603, 1% except the 0 Ω links. Capacitors are Samsung
MLCCs with exact purchasing suffixes in the schematic. The DLA0010A,
Kingbright LED and two inductor lands follow the manufacturer drawings.
U3 has an extended PGND terminal with split paste apertures and no central
exposed pad. U7's adjacent package lands require 0.15 mm pad clearance; routed
copper remains at least 0.20 mm from foreign copper.

C1/C2 (optional USB shunts) and C31 (optional shell capacitor) are DNP.
R33 is the fitted 0 Ω USB-shell ground bond. J4, TP1–TP22 and H1–H4 are bare
PCB features, excluded from the assembly BOM and placement file.

## Off-board parts per device

| Item | Selection | Quantity |
| --- | --- | ---: |
| Display | Good Display GDEY0213B74, W2 mechanical drawing | 1 |
| Battery | Adafruit 258, protected PKCELL LP503562, 1200 mAh | 1 |
| Cell thermistor | Semitec 103AT-2, 10 kΩ at 25°C | 1 |
| Harness | PH2 battery adapter to PHR-3, with bonded NTC; see assembly instructions | 1 |
| Battery adapter source | Adafruit 1131 extension, retaining its battery-mating end | 1 |
| PCB harness housing | JST PHR-3 | 1 |
| Harness terminals | JST SPH-002T-P0.5S, crimped for AWG28 | 3 |
| Case and controls | Printed parts in [case/README.md](../case/README.md) | 1 set |
| Base screws | ISO 7046 / DIN 965 M2.5 × 20, countersunk machine screws | 3 |
| Lens | 55.0 × 30.1 × 0.8 mm clear PMMA, R1 corners | 1 |
| Light pipe | Ø2.0 × 3.5 mm clear PMMA rod, polished ends | 1 |
| Ballast | Optional 1.2 mm steel cut to `steel_cut.svg`, countersunk after cutting | 1 |
| Feet | Ø8 × 1.5 mm self-adhesive elastomer disks | 4 |

The harness rows list its constituent connector parts, not additional complete
harnesses. Also allow AWG28 insulated wire, splice insulation, polyimide NTC
tape, 0.10 mm display-perimeter adhesive and 0.2 mm removable battery tabs.
Print the steel surrogate when omitting the metal so the screw stack is retained.
Bench equipment includes a USB-C data cable; UART recovery additionally uses
a TC2030-IDC-NL breakout and a 3.3 V USB/UART adapter.

The battery's manufacturer drawing specifies a maximum new-pack envelope of
62.3 × 35.3 × 5.3 mm. The case reserves 63 × 36 × 6.3 mm plus 0.2 mm adhesive.
The NTC is attached to the pouch surface with electrically insulating tape;
never solder directly to a pouch cell. The factory pack protection circuit
and connector must remain intact. Wire J3 as 1=protected pack positive,
2=NTC, 3=pack negative; the other NTC lead goes to pin 3.

## Circuit settings

USB-C is a 5 V sink with separate 5.1 kΩ CC pull-downs; it has no PD controller.
BQ25185 is set to 4.2 V, approximately 250 mA charge and 500 mA input limit.
R13 requests charging when the MCU is unpowered; U7/Q2 can independently
force charge disable. The nominal external temperature window is roughly
4–37°C and is deliberately conservative relative to the pack's 0–45°C charge
range. The native charger TS protection remains connected.

The battery divider is 1 MΩ / 470 kΩ with 100 nF filtering. Firmware enables
it only for measurement and waits 200 ms before conversion. The buck-boost
input C8 is 47 µF; its output is 22 µF + 47 µF. C4/C6/C34 use 10 µF parts so
their effective capacitance stays above the required 1 µF after DC bias.
The numerical capacitor and temperature review is in [REVIEW.md](REVIEW.md).

R27–R31 are 470 Ω display signal resistors. Firmware parks the physical signal
levels low before removing panel power. R34 is a removable battery-current
measurement link; TP22 is on its protected-pack side. A WROOM-1U replacement
requires a separate antenna and mechanical review and is not an approved BOM
substitution.
