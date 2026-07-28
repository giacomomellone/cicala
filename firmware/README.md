# Firmware

**Status: structure only.** There is no buildable firmware project yet. Initial
development targets the USB-powered breadboard rig in
[prototype_bom.md](../docs/prototype_bom.md).

## Target

- ESP32-S3 DevKitC-1-N8R8 for breadboard development.
- ESP32-S3-WROOM-1-N16 on the later target PCB.
- One 2.13-inch, 250 × 122 e-paper display over SPI.
- Six one-hot selector inputs and one independent Next button.
- No OLED, encoder, depth control, or user-facing status LED.

ESP-IDF and Zephyr both support the ESP32-S3 development board. The repository
has not selected one yet. Existing `just fw-build` and `just fw-flash` recipes
are inactive placeholders written for ESP-IDF; choose the framework before
creating the buildable skeleton.

Zephyr also has in-tree SSD1680/SSD16xx support and an exact
`waveshare_epaper_gdey0213b74` configuration for the planned
GDEY0213B74/FPC-A002 panel. Using it on the ESP32-S3 requires a project
devicetree overlay for the selected SPI and GPIO pins, not a new display driver.

## Interaction contract

- E-paper shows one question and no status or menu UI.
- The physical selector is the active deck. Read it at boot and after every
  selector wake instead of restoring a remembered deck.
- A selector change must remain stable for about 600 ms before drawing.
- Every Next press duration has the same meaning and draws exactly once.
- Normal playback includes depths 1 and 2. Depth 3 is never drawn
  automatically.
- The current question persists on e-paper while power is removed.
- Wi-Fi remains optional. Sync runs only during an explicit service or charging
  flow and never interrupts tabletop use.

## State outline

```text
BOOT
  → read selector
  → keep the existing e-paper question, or draw if the display is uninitialized

SELECTOR_CHANGE
  → wait for one stable valid position
  → draw from that deck
  → refresh e-paper

NEXT
  → debounce
  → draw from the selected deck
  → refresh e-paper

IDLE
  → enter deep sleep

WAKE
  → read the physical selector again
  → handle selector change or Next
```

Zero or several active selector contacts are invalid states. Firmware must not
guess a deck; it should wait for one stable contact and retain the displayed
question.

## Planned layout

```text
firmware/
├── build-system files      # ESP-IDF or Zephyr, decision open
├── main/                   # application entry and state transitions
├── components/
│   ├── epaper/             # SPI driver, text layout, full/partial refresh
│   ├── input/              # six selector GPIOs and debounced Next GPIO
│   ├── qdb/                # TKB2 parser and per-deck shuffle bags
│   ├── sync/               # optional signed bundle download and atomic swap
│   ├── portal/             # service-only Wi-Fi and language setup
│   └── power/              # deep sleep, wake sources, and later battery sensing
└── partition definition    # sized for firmware and language bundles
```

Pin assignments remain open until the breadboard wiring is documented. Keep
them configurable so the same application logic can move to the target PCB.

## Breadboard acceptance

- Firmware flashes, logs, and debugs over the DevKitC USB connection.
- All six selector positions and invalid contact combinations are tested.
- One physical press produces one Next event.
- Real English and German TKB2 bundles round-trip through the question store.
- Every released question fits at the fixed minimum type size.
- Partial and full refresh behavior is tested over a representative run.
- Wi-Fi and bundle sync work from USB power before battery measurements begin.
