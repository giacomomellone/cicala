# PCB — rev A (block description, phase D placeholder)

**Status: no schematic capture yet.** This file describes the current rev A
blocks so later pin planning and pricing start from the same assumptions. KiCad
work begins after the USB breadboard firmware rig and power-path bench pass.
Licensed
[CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

Before schematic capture, resolve the power-path, protection, antenna, and test
requirements in the [device prototype review](../../docs/device_prototype.md#electrical-contract).

The current DevKitC firmware still reads six one-hot category inputs from a
DIP switch. The two-button rev A input contract is the target, but schematic
capture waits until a second button is available and that firmware transition
has been tested.

## Blocks

| Block | Part | Notes |
|---|---|---|
| MCU / radio | **ESP32-S3-WROOM-1-N16** | 16 MB flash; USB-JTAG for programming; Wi-Fi only, no Bluetooth by policy |
| E-paper | **GDEY0213B74** 2.13″, 24-pin FPC | SSD1680 controller; boost circuit per the Good Display reference design (MOSFET + inductor + diodes) |
| Category | **KSC321GLFS** sealed tactile switch | wake GPIO; advances one retained category and updates the e-paper label |
| Next | **KSC321GLFS** sealed tactile switch | independent wake GPIO; every press duration means Next |
| Charger / power path | **open** | rev A needs an integrated power path or validated load sharing because sync runs while charging |
| Regulator | **XC6220** 3.3 V LDO | low-IQ for deep-sleep budget (< 30 µA target) |
| USB | USB-C 16-pin receptacle | 5.1 kΩ pulldowns on both CC lines (device role). One connector does charge, flashing, console and JTAG — D+/D− to GPIO19/20, no bridge chip |
| Battery | **503035 LiPo, 500 mAh** | JST-PH; fits the case envelope with the e-paper module. The 1500 mAh cell on the bench is a bench part and does not fit |
| Status LED | bi-colour red/green, two GPIOs | Lighting both is amber, which is the whole palette. **Not addressable** — a WS2812's controller idles at about 0.6 mA with the emitters dark, twenty times the sleep budget |
| Boot strap | test point or internal button on **IO0** | The only way back from an image that reconfigures GPIO19/20 or crashes before USB enumerates. The two product buttons are on other pins |

## Design rules of thumb (carry into schematic capture)

- Everything not essential in sleep must be power-gated or chosen for < 1 µA quiescent. Category and Next are wake inputs.
- The e-paper stays powered off between refreshes; the SSD1680 boost is enabled only during a refresh window.
- USB-C needs ESD/input protection and a VBUS-sense path in addition to the two 5.1 kΩ CC pull-downs.
- The battery divider must be switched so it does not become a permanent sleep load. The bench runs 470k/470k unswitched, which is 4.5 µA and fits under the budget on its own; a product should not spend any of it here.
- The status LED must be genuinely off when off. Two GPIOs held low are; anything with a controller in the package is not.
- Keep the module antenna at a board edge with the manufacturer keep-out clear of the display, cell, button hardware, and copper.
- Test pads for UART0/JTAG, display buses, VBUS, 3V3, and battery rail on the back side; include current-measurement links.

See [`BOM.md`](BOM.md) for the placeholder part list.
