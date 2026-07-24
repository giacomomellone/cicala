# BOM — rev A (placeholder part numbers)

Placeholder list for the rev A blocks; quantities and passives firm up with schematic capture. Prices are rough single-unit figures for planning only.

For the bench prototype (dev boards, modules, and order links for stages 1–3), see [docs/prototype_bom.md](../../docs/prototype_bom.md) instead.

| Ref | Part | Package | Placeholder MPN | ~€ |
|---|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N16 | module | ESP32-S3-WROOM-1-N16 | 3.50 |
| DISP1 | 2.13″ e-paper panel | 24-pin FPC | GDEY0213B74 | 5.00 |
| J2 | FPC connector, 24-pin 0.5 mm | SMD | AFC01-S24FCA-00 | 0.40 |
| DISP2 | 0.91″ OLED 128×32, SSD1306 | 4-pin header / SMD | placeholder | 1.50 |
| SW1 | Rotary encoder w/ switch | TH | EC11E15244B2 | 1.00 |
| U2 | LiPo charger / power path | TBD | MCP73831T-2ACI/OT baseline; load sharing unresolved | 0.60+ |
| U3 | 3.3 V LDO, low-IQ | SOT-25 | XC6220B331MR-G | 0.50 |
| J1 | USB-C receptacle, 16-pin | SMD | TYPE-C-31-M-12 | 0.30 |
| R1, R2 | 5.1 kΩ CC pulldowns | 0402 | — | 0.01 |
| BT1 | LiPo 503035, 500 mAh, JST-PH | — | placeholder | 3.00 |
| Q1, L1, D1, D2 | SSD1680 boost circuit | per Good Display ref | Si1308EDL / 10 µH / MBR0530 ×2 | 0.50 |
| — | USB ESD/input protection, VBUS sense, load switches, switched battery sense, passives, test pads, misc | mixed | — | TBD |

Open questions for schematic capture: protected cell supplier and charge current;
integrated power-path charger vs. explicit load sharing; OLED module vs. bare
panel; antenna keep-out and enclosure performance; 3.3 V LDO brownout margin;
and exact power-gating parts.

The encoder is relative: choose a readily available 15–20-detent part with a
good push feel. The five categories are software states, not five physical
detents per revolution.
