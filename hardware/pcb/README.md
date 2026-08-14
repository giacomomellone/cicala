# PCB — rev A plan

Schematic capture has not started. The electrical contract is in the
[device prototype review](../../docs/device_prototype.md#electrical-contract).
Hardware files use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

Rev A must measure whole-device sleep current. The DevKitC's power LED and
WS2812 exceed the 30 µA target, so breadboard measurements cannot bound it.

## Blocks

| Block | Part | Notes |
|---|---|---|
| MCU / radio | **ESP32-S3-WROOM-1-N16** | 16 MB flash; USB-JTAG for programming; Wi-Fi only, no Bluetooth by policy |
| E-paper | **GDEY0213B74** 2.13″, 24-pin FPC | SSD1680 controller; boost circuit per the Good Display reference design (MOSFET + inductor + diodes) |
| Category | **KSC321GLFS** sealed tactile switch | wake GPIO; advances one retained category and updates the e-paper label |
| Next | **KSC321GLFS** sealed tactile switch | independent wake GPIO; every press duration means Next |
| Charger / power path | **open** | rev A needs an integrated power path or validated load sharing because sync runs while charging. Prefer a part that reports charge termination: the bench's bq25185 has no such pin, so "charged" is inferred from voltage and only the LED is allowed to believe it |
| Regulator | **XC6220** 3.3 V LDO | low-IQ for deep-sleep budget (< 30 µA target) |
| USB | USB-C 16-pin receptacle | 5.1 kΩ pulldowns on both CC lines (device role). One connector does charge, flashing, console and JTAG — D+/D− to GPIO19/20, no bridge chip |
| Battery | **503035 LiPo, 500 mAh** | JST-PH; fits the case envelope with the e-paper module. The 1500 mAh cell on the bench is a bench part and does not fit |
| Status LED | bi-colour red/green, two GPIOs | Both lit is amber. Use no addressable controller; a WS2812 idles at about 0.6 mA with its emitters dark |
| Boot strap | test point or internal button on **IO0** | The only way back from an image that reconfigures GPIO19/20 or crashes before USB enumerates. The two product buttons are on other pins |

## Schematic requirements

- Parts active during sleep must stay within the power budget. Category and Next are wake inputs.
- The e-paper stays powered off between refreshes; the SSD1680 boost is enabled only during a refresh window.
- USB-C needs ESD/input protection and a VBUS-sense path in addition to the two 5.1 kΩ CC pull-downs.
- Switch the battery divider off during sleep. A permanent 1M/1M divider draws about 2.1 µA. A 1M/470k divider adds about 25 mV of ADC-leakage error at the tap; policy thresholds are 200 mV apart.
- The status LED must be genuinely off when off. Two GPIOs held low are; anything with a controller in the package is not.
- Keep the module antenna at a board edge with the manufacturer keep-out clear of the display, cell, button hardware, and copper.
- Test pads for UART0/JTAG, display buses, VBUS, 3V3, and battery rail on the back side; include current-measurement links.

See [`BOM.md`](BOM.md) for the placeholder part list.
