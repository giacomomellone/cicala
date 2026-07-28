# epaper

The target is the GDEY0213B74/FPC-A002 2.13-inch, 250 × 122 panel with its
SSD1680 controller over SPI. Zephyr already provides the SSD16xx display driver,
the `solomon,ssd1680` devicetree binding, and a
`waveshare_epaper_gdey0213b74` shield configuration for this exact panel. An
ESP32-S3 board overlay still has to assign SPI, chip-select, data/command,
reset, and busy pins for the breadboard HAT and later PCB.

Application work covers full/partial-refresh policy, UTF-8 text layout, and a
bundled font for the released English and German corpora; it does not require a
new low-level display driver.

The e-paper renders only questions, never menus, logos, or status. Determine
the full-refresh interval from observed ghosting on the breadboard display
rather than fixing an untested count in advance.
