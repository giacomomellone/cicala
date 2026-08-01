# Firmware

**Status: bring-up.** Build system, board configuration, test harness, the
`fsm` library and a blinky exist. The application state machine does not yet.
[FIRMWARE_GUIDE.md](../FIRMWARE_GUIDE.md) is the hands-on tour; the design is
[docs/firmware_architecture.md](../docs/firmware_architecture.md). Initial
development targets the USB-powered breadboard rig in
[prototype_bom.md](../docs/prototype_bom.md).

## Target

- ESP32-S3 DevKitC-1-N8R8 for breadboard development.
- ESP32-S3-WROOM-1-N16 on the later target PCB.
- One 2.13-inch, 250 × 122 e-paper display over SPI.
- Six one-hot selector inputs and one independent Next button.
- No OLED, encoder, depth control, or user-facing status LED.

Zephyr, not ESP-IDF. It has in-tree SSD1680/SSD16xx support and an exact
`waveshare_epaper_gdey0213b74` configuration for the planned
GDEY0213B74/FPC-A002 panel, so the display needs a project devicetree overlay
for the chosen SPI and GPIO pins rather than a new driver.

Application logic is C++17; files using Zephyr's macro-heavy APIs stay C. See
the decision log for why.

## Getting started

```sh
just fw-init      # once: west init + update into deps/, fetch espressif blobs
just fw-build     # build for the ESP32-S3 devkit
just fw-flash     # flash over the devkit's USB connection
just fw-monitor   # serial console
just fw-sim       # run under qemu on the host
just fw-test      # ztest suites via twister
```

The west workspace uses T2 topology: `west.yml` here is the manifest, the repo
root is the topdir, and Zephyr plus its modules land in a gitignored `deps/`.
Nothing from upstream is committed.

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

## Layout

```text
firmware/
├── west.yml                # manifest: zephyr revision + module allowlist
├── app/
│   ├── CMakeLists.txt
│   ├── prj.conf            # shared config; LATER blocks track the architecture
│   ├── Kconfig             # settle window, refresh interval, depth cap
│   ├── boards/             # per-board conf + devicetree overlay
│   └── src/main.c          # bring-up blinky, not the product
├── lib/fsm/                # table-driven state machine, C++17
├── tests/                  # ztest suites, run by twister
└── components/             # per-area contracts (README only, pre-Zephyr)
```

`components/` holds the written contracts for epaper, input, qdb, sync, portal
and power. They predate the Zephyr decision and describe responsibilities
rather than a directory layout; the code lands under `app/src/`.

Pin assignments remain open until the breadboard wiring is documented. They
live in the board overlay so the same application logic moves to the target PCB
by changing one file. Two constraints apply when they are chosen: all six
selector contacts, Next, and VBUS detect must be on RTC-capable GPIOs, or
EXT1 deep-sleep wake is impossible.

## Breadboard acceptance

- Firmware flashes, logs, and debugs over the DevKitC USB connection.
- All six selector positions and invalid contact combinations are tested.
- One physical press produces one Next event.
- Real English and German TKB2 bundles round-trip through the question store.
- Every released question fits at the fixed minimum type size.
- Partial and full refresh behavior is tested over a representative run.
- Wi-Fi and bundle sync work from USB power before battery measurements begin.
