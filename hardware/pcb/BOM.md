# BOM — rev A (placeholder part numbers)

Placeholder list for the rev A blocks; quantities and passives firm up with schematic capture. Prices are rough single-unit figures for planning only.

| Ref | Part | Package | Placeholder MPN | ~€ |
|---|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N16 | module | ESP32-S3-WROOM-1-N16 | 3.50 |
| DISP1 | 2.13″ e-paper panel | 24-pin FPC | GDEY0213B74 | 5.00 |
| J2 | FPC connector, 24-pin 0.5 mm | SMD | AFC01-S24FCA-00 | 0.40 |
| DISP2 | 0.91″ OLED 128×32, SSD1306 | 4-pin header / SMD | placeholder | 1.50 |
| SW1 | Rotary encoder w/ switch | TH | EC11E15244B2 | 1.00 |
| U2 | LiPo charger | SOT-23-5 | MCP73831T-2ACI/OT | 0.60 |
| U3 | 3.3 V LDO, low-IQ | SOT-25 | XC6220B331MR-G | 0.50 |
| J1 | USB-C receptacle, 16-pin | SMD | TYPE-C-31-M-12 | 0.30 |
| R1, R2 | 5.1 kΩ CC pulldowns | 0402 | — | 0.01 |
| BT1 | LiPo 503035, 500 mAh, JST-PH | — | placeholder | 3.00 |
| Q1, L1, D1, D2 | SSD1680 boost circuit | per Good Display ref | Si1308EDL / 10 µH / MBR0530 ×2 | 0.50 |
| — | passives, test pads, misc | 0402 | — | 1.00 |

Open questions for schematic capture: battery protection (cell-integrated vs. discrete), OLED module vs. bare panel, encoder detent count vs. category feel (5 detents/rev would be ideal but nonstandard).
