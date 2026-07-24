# PCB — rev A (block description, phase D placeholder)

**Status: no schematic capture yet.** This file freezes the rev A block-level design so firmware pin planning and the BOM can proceed. KiCad 8 project lands here when phase D starts. Licensed [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

Before schematic capture, resolve the power-path, protection, antenna, and test
requirements in the [device prototype review](../../docs/device_prototype.md#electrical-direction-for-rev-a).

## Blocks

| Block | Part | Notes |
|---|---|---|
| MCU / radio | **ESP32-S3-WROOM-1-N16** | 16 MB flash; USB-JTAG for programming; Wi-Fi only, no Bluetooth by policy |
| E-paper | **GDEY0213B74** 2.13″, 24-pin FPC | SSD1680 controller; boost circuit per the Good Display reference design (MOSFET + inductor + diodes) |
| Status display | 0.91″ OLED, **SSD1306**, I²C | dark unless hands are on the device |
| Input | **EC11** rotary encoder with push | quadrature to PCNT pins, button to GPIO with RC debounce |
| Charger / power path | **open** | MCP73831 is the charger baseline, but rev A needs validated load sharing or an integrated power-path part because sync runs while charging |
| Regulator | **XC6220** 3.3 V LDO | low-IQ for deep-sleep budget (< 30 µA target) |
| USB | USB-C 16-pin receptacle | 5.1 kΩ pulldowns on both CC lines (device role) |
| Battery | **503035 LiPo, 500 mAh** | JST-PH; fits the case envelope with the e-paper module |

## Design rules of thumb (carry into schematic capture)

- Everything not essential in deep sleep must be power-gated or chosen for < 1 µA quiescent (the wake source is the knob GPIO).
- The e-paper stays powered off between refreshes; the SSD1680 boost is enabled only during a refresh window.
- USB-C needs ESD/input protection and a VBUS-sense path in addition to the two 5.1 kΩ CC pull-downs.
- The battery divider must be switched so it does not become a permanent sleep load.
- Keep the module antenna at a board edge with the manufacturer keep-out clear of the display, cell, encoder metal, and copper.
- Test pads for UART0/JTAG, display buses, VBUS, 3V3, and battery rail on the back side; include current-measurement links.

See [`BOM.md`](BOM.md) for the placeholder part list.
