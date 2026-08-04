# BOM — rev A planning list

No schematic exists. This list identifies the blocks and candidate mechanical
parts that a later rev A must price and fit. It is not an orderable production
BOM. For development-board and physical-model purchases, use
[docs/prototype_bom.md](../../docs/prototype_bom.md).

| Ref | Part | Package | Candidate MPN | Qty | Single-unit indication |
|---|---|---|---|---:|---:|
| U1 | ESP32-S3 module, 16 MB flash | module | ESP32-S3-WROOM-1-N16 | 1 | 3.50 EUR |
| DISP1 | 2.13″, 250 × 122 e-paper panel | 24-pin FPC | [GDEY0213B74](https://www.buy-lcd.com/products/213-inch-250x122-raspberry-pi-epd-electronic-epaper-price-tag) | 1 | about 5–9 EUR |
| J2 | 24-pin, 0.5 mm FPC connector | SMD | AFC01-S24FCA-00 | 1 | 0.40 EUR |
| SW1 | Category: sealed SPST-NO tact switch, 2 N | 6.2 × 6.2 mm SMD | C&K KSC321GLFS | 1 | 0.413 EUR |
| SW2 | Next: sealed SPST-NO tact switch, 2 N | 6.2 × 6.2 mm SMD | C&K KSC321GLFS | 1 | 0.413 EUR |
| U2 | Charger with system power path | TBD | BQ2407x class; exact part open | 1 | TBD |
| U3 | 3.3 V low-IQ regulator | SOT-25 | XC6220B331MR-G | 1 | 0.50 EUR |
| J1 | USB-C receptacle, 16 pin | SMD | TYPE-C-31-M-12 | 1 | 0.30 EUR |
| R1, R2 | 5.1 kΩ USB-C CC pull-downs | 0402 | — | 2 | 0.02 EUR |
| BT1 | Protected 503035 LiPo, 500 mAh | wire/JST-PH | supplier qualification open | 1 | 4–6 EUR |
| Q1, L1, D1, D2 | SSD1680 boost circuit | reference packages | per Good Display design | 1 set | 0.50 EUR |
| — | USB ESD/input protection, VBUS sense, e-paper load switch, switched battery sense, passives, test pads | mixed | open | — | TBD |
| MECH1 | Small round Category cap with overload stop | custom | open | 1 | TBD |
| MECH2 | Larger rounded-pill Next cap with shallow concave top and overload stop | custom | open | 1 | TBD |

The OLED, OLED rail, OLED load switch, EC11 encoder, encoder push switch, rotary
selector, pointer knob, and printed selector bezel are deleted.

The roughly 20 EUR Waveshare HAT is a reusable development module, not the
production display cost. The target board uses the bare GDEY0213B74 panel,
listed at 5.36 USD by Good Display in July 2026, plus the FPC connector and
roughly 0.50 EUR of SSD1680 support components. Allow about 6–10 EUR for the
display-specific parts at prototype quantities, before PCB assembly, shipping,
tax, yield loss, and a protective lens. Request a manufacturer quote once the
expected production quantity is known.

Open before schematic capture:

- power-path IC and protected cell supplier;
- charge current;
- two GPIO allocations and wake behavior for Category and Next;
- retained category behavior and the New People cold default;
- button/antenna separation;
- both button-cap, lens, USB, and case-seam ingress paths;
- e-paper brownout margin during Wi-Fi transmit;
- exact ESD, load-switch, and test components.
