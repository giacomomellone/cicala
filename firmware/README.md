# Firmware

Zephyr firmware for the ESP32-S3 Cicala device. The breadboard rig runs the
tabletop loop, deep sleep, setup portal, signed bundle sync, signed firmware
updates, battery monitoring, and the two status LEDs. Whole-device sleep current
still needs measurement on rev A because the DevKitC indicators exceed the
30 µA target.

Start with the [firmware primer](../docs/firmware_primer.md). See the
[firmware architecture](../docs/firmware_architecture.md) for module boundaries
and the [hardware wiring guide](../docs/hardware_wiring.md) for the bench rig.


## Integrated Rev A board

Select `hw=rev_a` for the integrated PCB; the default remains the breadboard.
The Rev A overlay/configuration control the switched battery divider and panel
rail, set the ADC divider to 1 MΩ/470 kΩ, wait 200 ms before sampling, and use
native USB Serial/JTAG for console and debugging. A dedicated power-sampling
work queue keeps the settling delay out of the input/system queue. Conversion
failure and deep-sleep entry both switch the divider off.

```sh
just hw=rev_a fw-build debug
just hw=rev_a fw-flash debug
just hw=rev_a fw-monitor debug
just hw=rev_a fw-debugserver
just hw=rev_a fw-flash                 # release with MCUboot
```

Rev A build directories are `build/cicala-rev-a` and
`build/cicala-rev-a-debug` (other profiles use the corresponding suffix).
Use `port=/dev/cu.usbmodem…` or the Linux serial device as a `just` override
when automatic detection selects the wrong port. The debug ELF is
`build/cicala-rev-a-debug/zephyr/zephyr.elf`; the existing OpenOCD configuration
uses the ESP32-S3 built-in USB JTAG adapter. No external JTAG probe is required.

GPIO14 low requests charging; the independent analog temperature guard can
still veto it. GPIO5 enables the panel before driver initialization. Deep
sleep parks physical display-bus levels low before disabling the rail and
holds those pins. The first refresh after power restoration is full because
the display controller RAM has been lost. The PCB recovery pinout and first
power checks are in [the hardware review](../hardware/pcb/REVIEW.md).

The [Rev A build and flash guide](../docs/firmware_rev_a.md) covers toolchain
installation, first USB download, JTAG, UART recovery and hardware acceptance.
Rev A reads both BQ25185 status pins: high/high means idle or disabled, so it
does not claim a full battery from voltage alone. The release application and
MCUboot both use 16 MiB flash without PSRAM. Firmware validation passed 195
QEMU cases across 16 configurations, with two skips.
Physical USB flashing, JTAG, charging and display operation remain prototype
acceptance tests. The accepted question database is currently empty; use the
existing charset diagnostic for initial display testing.

## Target

- ESP32-S3-DevKitC-1-N8R8 on the breadboard
- ESP32-S3-WROOM-1-N16 on rev A
- 250 × 122 GDEY0213B74 e-paper panel with SSD1680 controller
- Filters and Next buttons
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

- Filters opens the menu or advances through Dark, Sexual, Heavy, and Done.
- Next draws during play, toggles a permission in the menu, or applies Done.
- A cold boot excludes all three permissions. RTC memory preserves the question
  snapshot, menu draft/cursor, per-language bags, and refresh count through sleep.
- Presses during a panel refresh are dropped.
- Ordinary playback includes depths 1 and 2. Heavy permits depth 3.
- Display errors and refused refreshes do not commit pending selection state.
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
therefore exercise the same QDB4 bytes compiled into the application.

## Hardware checks

The breadboard rig has verified:

- Filters and Next from GPIO input through the panel refresh
- English and German QDB4 bundles and no-repeat draws
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
