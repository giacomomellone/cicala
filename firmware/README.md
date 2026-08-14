# Firmware

Zephyr firmware for the ESP32-S3 Tischkarte device. The breadboard rig runs the
tabletop loop, deep sleep, setup portal, signed bundle sync, signed firmware
updates, battery monitoring, and the two status LEDs. Whole-device sleep current
still needs measurement on rev A because the DevKitC indicators exceed the
30 µA target.

Start with the [firmware primer](../docs/firmware_primer.md). See the
[firmware architecture](../docs/firmware_architecture.md) for module boundaries
and the [hardware wiring guide](../docs/hardware_wiring.md) for the bench rig.

## Target

- ESP32-S3-DevKitC-1-N8R8 on the breadboard
- ESP32-S3-WROOM-1-N16 on rev A
- 250 × 122 GDEY0213B74 e-paper panel with SSD1680 controller
- Category and Next buttons
- red and green status LEDs; lighting both produces amber
- single-cell LiPo with a bq25185 power-path bench

Application logic is C++17 under `firmware/lib/`. Zephyr threads, drivers, and
channels live in `firmware/app/` and use C where Zephyr macros require it.

## Commands

```sh
just fw-init          # install the west workspace once
just fw-build         # release image with MCUboot
just fw-flash         # build and flash the release image
just fw-monitor       # UART console; ctrl-] exits
just fw-sim           # run the application under qemu
just fw-test          # run all firmware suites under qemu
just fw-test input    # run one suite
```

`fw-build`, `fw-flash`, and `fw-monitor` accept a profile:

| Profile | Purpose |
|---|---|
| `release` | normal MCUboot application; the default |
| `debug` | `-Og`, USB debug console, and OpenOCD support |
| `charset` | step through the renderer's glyph pages |
| `soak` | generate Next presses for ghosting measurements |
| `retain` | warm-reboot during the soak run |
| `power` | log raw and converted battery readings |
| `portal` | open the setup portal at boot |
| `corpus` | write the compiled corpus to LittleFS at boot |
| `bench` | fetch bundles from a local server; requires its host |
| `ota` | fetch firmware from a local server; requires its host on first build |

Examples:

```sh
just fw-flash charset
just fw-monitor charset
just fw-flash bench 192.168.1.20
just fw-build ota 192.168.1.20
just fw-monitor debug
```

Profiles use separate build directories, so switching images does not retain
another profile's CMake configuration. The release, power, and OTA profiles use
sysbuild and produce MCUboot plus the signed application.

For debugging, `just fw-debug` builds and flashes the debug profile, then starts
OpenOCD on port 3333. `just fw-debugserver` starts the server without flashing.

## Interaction

- Category advances through the six decks and shows the new deck name.
- Next draws one question from the active deck.
- A cold boot starts at New People. RTC memory preserves the active deck,
  shuffle bag, panel state, and partial-refresh count across deep sleep.
- Presses during a panel refresh are dropped.
- Normal playback includes depths 1 and 2. Depth 3 remains browse-only.
- Holding both buttons through boot opens the setup portal.
- External power keeps the device awake and opens the sync and update window.
- LOW and CRITICAL battery states refuse panel refreshes. LOW blinks amber
  once; CRITICAL blinks red three times.

Wi-Fi is optional. Every release image contains the shipped corpora, and a
stored corpus replaces its compiled counterpart only after validation.

## Source layout

```text
firmware/
├── west.yml          # pinned Zephyr workspace
├── Kconfig.policy    # shared product policy
├── app/
│   ├── boards/       # board Kconfig and devicetree
│   ├── include/      # C interfaces and channel messages
│   └── src/          # Zephyr integration
├── lib/              # hardware-free logic
├── tests/            # ztest suites
└── patches.yml       # local Zephyr patches
```

The build creates question fixtures from the real YAML database. Firmware tests
therefore exercise the same QDB2 bytes compiled into the application.

## Hardware checks

The breadboard rig has verified:

- Category and Next from GPIO input through the panel refresh
- English and German QDB2 bundles and no-repeat draws
- every shipped question at the minimum font size
- partial and full refresh policy on the GDEY0213B74
- deep-sleep wake and RTC-retained state
- setup with a phone, including saved credentials and language
- bundle validation and atomic LittleFS replacement
- MCUboot installation and rejection of an image signed with the wrong key
- battery and VBUS sensing, refresh gating, and LED states

Network download checks require a reachable bench server. Sleep-current
measurement requires rev A or a power mule without the DevKitC's indicator
loads. The procedures and recorded measurements are in the
[hardware wiring guide](../docs/hardware_wiring.md#bench-measurements).
