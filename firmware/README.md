# Firmware

**Status: structure only.** The buildable ESP-IDF skeleton is phase C of the [handoff specification](../Tischkarte%20—%20Project%20handoff%20specification.md) §8; this directory currently documents the architecture so the site, tools and hardware can proceed against a stable contract. CI (`firmware.yml`) activates `idf.py build` automatically once `firmware/CMakeLists.txt` lands.

## Target

- **ESP-IDF v5.3+**, target `esp32s3`, 16 MB flash.
- Displays: GDEY0213B74 2.13″ e-paper (SSD1680, SPI) + 0.91″ OLED (SSD1306, I²C).
- Input: one EC11 rotary encoder (quadrature on PCNT) with push button.

## Hard rules (from docs/design.md — not negotiable in code review)

- The e-paper shows **only questions**. Menus, logos, status, progress — all of it belongs on the OLED.
- The OLED is dark whenever hands are off the device.
- Fully functional out of the box with the factory-preloaded database; Wi-Fi optional forever.
- Sync runs opportunistically while charging and never interrupts use.

## State machine (table-driven switch in `main/state_machine.c`)

```
SLEEP → WAKE(last question)
WAKE → BROWSE_CATEGORY   (knob turn: OLED scrolls category names)
     → SHOW_QUESTION     (press: qdb_next(category), e-paper partial refresh)
     → MENU              (long-press 1.5 s: OLED menu, 10 s timeout)
MENU → {SYNC, PORTAL, LANGUAGE, INFO}
any  → SLEEP             (30 s idle; OLED off, question persists on e-paper)
```

E-paper writes happen **only** in `SHOW_QUESTION`: partial refresh per question, full refresh every 10th partial or on entering sleep (ghost-clear counter lives in the `epaper` component).

## Planned layout

```
firmware/
├── CMakeLists.txt          # phase C
├── main/                   # app entry + state_machine.c
├── components/
│   ├── epaper/             # SSD1680 SPI, full+partial refresh, ghost-clear
│   │                       # counter, UTF-8 layout, bundled latin+latin-ext font
│   ├── oled/               # SSD1306 I²C status/menu display
│   ├── input/              # EC11 quadrature on PCNT + debounced button,
│   │                       # short/long press events
│   ├── qdb/                # question store: LittleFS mount, bundle parser
│   │                       # (docs/sync_protocol.md), qdb_next(category) with
│   │                       # on-flash shuffle bag; up to two installed language
│   │                       # bundles, active language switchable in the menu
│   ├── sync/               # HTTPS manifest check, per-language bundle download,
│   │                       # ed25519 verify (trusted_key.h from tools/keygen.py),
│   │                       # atomic swap in LittleFS
│   ├── portal/             # SoftAP + DNS-hijack captive portal for Wi-Fi
│   │                       # credentials AND language selection
│   └── power/              # deep sleep, wake on knob GPIO, battery ADC
└── partitions.csv          # nvs / phy / factory + ota_0 + ota_1 (A/B) /
                            # littlefs ≥ 4 MB (room for two language bundles)
```

Config via `Kconfig`: pins, category list mirror, manifest URL, embedded ed25519 public key, factory-preloaded languages.

## Acceptance targets (documented now, tested in phase C+)

- Deep sleep < 30 µA; wake-to-question < 1 s; ≥ 4 weeks standby on 500 mAh.
- `idf.py build` green in CI; `qdb` round-trips real bundles from `tools/build_bundle.py` (host-side unit test + on-target) for en and de.
