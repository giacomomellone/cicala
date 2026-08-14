# Prototype BOM

Parts for the working breadboard rig and the planned rev A integration. Prices
and stock change; order by manufacturer part number and confirm the exact board
revision at checkout.

## Breadboard rig

| Part                                                                 | Quantity | Use                                           | Reference                                                                                                                                                                                                                                         |
| -------------------------------------------------------------------- | -------: | --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Espressif **ESP32-S3-DevKitC-1-N8R8**                                |        1 | firmware target with UART and native USB/JTAG | [Reichelt ESP32S3DK-C1N8R8](https://www.reichelt.de/index.html?ACTION=3&ARTICLE=411139)                                                                                                                                                           |
| Waveshare **12915 2.13-inch e-Paper HAT V4**, black/white, 250 × 122 |        1 | SPI display and partial-refresh testing       | [Welectron](https://www.welectron.com/Waveshare-12915-213inch-e-Paper-HAT_1), [Amazon B07J3FHJVP](https://www.amazon.de/dp/B07J3FHJVP), [Eckstein](https://eckstein-shop.de/Waveshare-213-inch-250x122-E-Ink-Display-HAT-for-Raspberry-Pi-SPI-EN) |
| 6 mm through-hole tactile buttons                                    |        2 | Category and Next                             | any electronics distributor                                                                                                                                                                                                                       |
| Adafruit **6092**, bq25185 charger with 3.3 V buck                   |        1 | USB charging, power path, and regulated 3.3 V | [Adafruit 6092](https://www.adafruit.com/product/6092)                                                                                                                                                                                            |
| **EEMB 524261**, 3.7 V, 1500 mAh, JST-PH                             |        1 | bench cell and discharge measurements         | supplier listing matching the exact MPN                                                                                                                                                                                                           |
| Red LED and green LED                                                |   1 each | status output                                 | any electronics distributor                                                                                                                                                                                                                       |
| LED series resistors                                                 |   1 each | set bench LED current                         | choose from the measurement in [hardware wiring](hardware_wiring.md)                                                                                                                                                                              |
| 1 MΩ, 470 kΩ, 100 kΩ, and 150 kΩ resistors, 1%                       |   1 each | battery and VBUS dividers                     | any electronics distributor                                                                                                                                                                                                                       |
| 100 nF ceramic capacitor                                             |        1 | battery-divider filter                        | any electronics distributor                                                                                                                                                                                                                       |
| Breadboard and jumper wires                                          |    1 set | temporary wiring                              | existing stock                                                                                                                                                                                                                                    |
| USB-C data cables                                                    |        3 | UART, native USB/JTAG, and charger            | existing stock                                                                                                                                                                                                                                    |

The display listing must say black/white, 250 × 122, V4, and SKU 12915.
Three-colour, four-colour, touch, and HAT+ variants use different hardware.
The module includes the eight-wire PH2.0 cable and accepts 3.3 V logic.

The DevKitC N8R8 has enough flash and RAM for firmware development. The rev A
partition layout targets a 16 MB module and must be checked on that board.

### Buttons

Ordinary through-hole buttons cover the breadboard tests. The target switch is
the sealed 2 N C&K **KSC321GLFS**. It is available as
[RS 1769505](https://de.rs-online.com/web/p/tastschalter/1769505) and from
[Sinuss](https://sinuss.nl/en/products/ksc321glfs-switch-spst-005a-32vdc-16n-smd-ck-components).

The SMD switch needs soldered leads for breadboard use. Connect one side to
ground and the opposite side to a GPIO input with its pull-up enabled. Check the
contact pair with a continuity meter before connecting the ESP32.

### Battery safety

Cut the Adafruit 6092 charge-rate jumper before connecting the cell. This sets
the charge current to 500 mA, about 0.33C for the 1500 mAh bench cell.

Check JST-PH polarity against the charger silkscreen with a meter. Suppliers use
both connector polarities; a reversed cell can damage the charger and cell.

The 1500 mAh cell is for measurement. Its roughly 61 × 42 × 5.2 mm envelope
does not fit the planned enclosure, and its runtime must not be quoted as the
503035 product-cell runtime.

## Measurement tools

A multimeter is enough for continuity, divider taps, and steady voltage. Short
Wi-Fi and e-paper current peaks need a current profiler such as the Nordic
Power Profiler Kit II.

The DevKitC power LED and WS2812 prevent a useful whole-device sleep-current
measurement. Measure the 30 µA target on rev A.

## Rev A integration

| Part                                                        |               Quantity | Use                                                                            |
| ----------------------------------------------------------- | ---------------------: | ------------------------------------------------------------------------------ |
| ESP32-S3-WROOM-1-N16                                        |                      1 | production module and 16 MB partition layout                                   |
| **GDEY0213B74** bare panel                                  | 1, plus optional spare | final display stack                                                            |
| 24-pin 0.5 mm FPC connector                                 |                      1 | panel connection                                                               |
| C&K **KSC321GLFS**                                          |                      2 | Category and Next                                                              |
| Protected 503035 LiPo                                       |                      1 | enclosure-sized product cell                                                   |
| Charger, regulator, USB-C, ESD, load switches, and passives |                  1 set | rev A power and service path; exact parts remain open in `hardware/pcb/BOM.md` |
| Printed shell, button caps, lens, ballast, and feet         |                  1 set | tabletop interaction and mechanical tests                                      |

Use [hardware wiring](hardware_wiring.md) to assemble and check the breadboard.
The current validation state and remaining enclosure tests are in
[device prototype](device_prototype.md).
