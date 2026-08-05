# Firmware

**Status: the tabletop loop works.** Pressing Category or Next
draws a question from the compiled-in corpus and renders it on the e-paper, and
the shuffle bag and refresh counter now live in RTC memory so a reboot does not
restart them. Still missing: deep sleep itself and the power path, Wi-Fi sync,
and the setup portal.
[docs/firmware_primer.md](../docs/firmware_primer.md) is the hands-on tour; the
design is [docs/firmware_architecture.md](../docs/firmware_architecture.md). Initial
development targets the USB-powered breadboard rig in
[prototype_bom.md](../docs/prototype_bom.md).

## Target

- ESP32-S3 DevKitC-1-N8R8 for breadboard development.
- ESP32-S3-WROOM-1-N16 on the later target PCB.
- Waveshare 2.13-inch e-Paper HAT, 250 × 122, SSD1680 (V3/V4 revisions).
- Two buttons: Category advances the deck, Next asks a question.
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
just fw-monitor   # serial console: deck changes and presses
just fw-sim       # run under qemu on the host
just fw-test      # ztest suites via twister
```

The west workspace uses T2 topology: `west.yml` here is the manifest, the repo
root is the topdir, and Zephyr plus its modules land in a gitignored `deps/`.
Nothing from upstream is committed.

## Checking glyphs on hardware

`CONFIG_TK_DEBUG_CHARSET=y` replaces the drawn question with a series of test
pages — ASCII, then one per language, then every diacritic the renderer
composes. Next steps through them and wraps around, and each page is logged as
it is drawn, so `just fw-monitor` says what the panel should be showing. Useful
for photographing accents and for judging a font change; never enabled in a
shipped build.

```sh
just fw-charset     # build + flash the test image
just fw-monitor     # each page is logged as it is drawn
just fw-flash       # back to the real thing
```

It builds into `build/esp32s3-charset/`, so switching back is a flash rather
than a rebuild.

## Measuring refresh and ghosting

`CONFIG_TK_DEBUG_SOAK=y` makes the device press its own Next button every few
seconds, and `app/soak.conf` suppresses full refreshes while it does. That
produces the long chain of partial refreshes that ghosting has to be watched
accumulating in — the measurement `CONFIG_TK_FULL_REFRESH_INTERVAL` is waiting
on, and one nobody wants to make by pressing a button forty times.

```sh
just fw-soak        # build + flash, then leave it running
just fw-monitor     # every refresh logs its position in the chain
just fw-flash       # back to the real thing
```

The chain starts itself at boot and keeps
going until the board is reflashed. The same console output carries refresh
durations and, once a minute, thread stack high-water marks. The full
procedure, including what to write down, is in
[docs/hardware_wiring.md](../docs/hardware_wiring.md).

## Checking what survives a reboot

Deep sleep is a reboot, so everything the device remembers between presses
lives in RTC memory. `just fw-retain` is the same soak image warm-rebooting
itself every three presses, which is the closest thing to a wake that exists
before `CONFIG_PM` does.

```sh
just fw-retain
just fw-monitor     # every reboot says what came back
```

The boot line after each one should read `retained state: kept across the
reboot`, and the run should continue — same sequence, same refresh count, no
full refresh, nothing redrawn.

Use the image rather than the reset button: an EN-pin reset reports as
`POWERON` and clears the RTC domain.

## Interaction contract

- E-paper shows one question, or the name of the deck just selected, and no
  status or menu UI.
- Category puts the deck's name on the panel and leaves it there. Next is what
  asks for a question. This is why the deck names need not be printed on the
  case: a deck you can read on the glass needs no labelled detent, which is
  what lets one button reach all six.
- Category advances one step and wraps from Wild back to New People.
- The active deck is remembered in RTC memory, because a button has no position
  to read. A cold boot starts on New People.
- Every Next press duration has the same meaning and draws exactly once.
- Normal playback includes depths 1 and 2. Depth 3 is never drawn
  automatically.
- The current question persists on e-paper while power is removed.
- Wi-Fi remains optional. Sync runs only during an explicit service or charging
  flow and never interrupts tabletop use.

## State outline

```text
BOOT
  → read the active deck from RTC memory (New People if there is none)
  → keep the question already on the e-paper, or name the deck

CATEGORY
  → advance one deck, wrapping
  → put its name on the e-paper

NEXT
  → draw from the active deck
  → refresh e-paper

IDLE
  → enter deep sleep

WAKE
  → a fresh boot, from the top
```

## Layout

```text
firmware/
├── west.yml                # manifest: zephyr revision + module allowlist
├── Kconfig.policy          # debounce, refresh interval, depth cap
├── app/
│   ├── CMakeLists.txt
│   ├── prj.conf            # shared config; LATER blocks track the architecture
│   ├── Kconfig             # pulls in ../Kconfig.policy
│   ├── boards/             # per-board conf + devicetree overlay
│   ├── include/            # channels.h, input.h, panel.h, app_logic.h
│   └── src/                # zbus glue: app, display, input, panel, channels
├── lib/                    # hardware-free C++17, shared with the suites
│   ├── fsm/                # table-driven state machine
│   ├── app_fsm/            # the tabletop machine built on it
│   ├── qdb/                # TKB2 reader and shuffle bag
│   ├── layout/             # UTF-8, accent decomposition, word wrap
│   └── retained/           # what survives a wake, and how that is known
├── tests/                  # ztest suites, run by twister
└── components/             # per-area contracts (README only, pre-Zephyr)
```

Everything under `lib/` builds without a Zephyr header, which is what lets the
suites run it on the host. `app/src/` is the glue that gives it channels,
threads and a display.

`Kconfig.policy` sits above `app/` because the test suites source the same file
the application does; a suite that copied the debounce would keep passing after
someone changed it.

`components/` holds the written contracts for epaper, input, qdb, sync, portal
and power. They predate the Zephyr decision and describe responsibilities
rather than a directory layout; the code lands under `app/src/`.

Pins live in the board overlay so the same application logic moves to the
target PCB by changing one file. The full map, the reasoning behind each pin,
and how the two buttons and the e-paper HAT connect are in
[docs/hardware_wiring.md](../docs/hardware_wiring.md).

The short version: Category is GPIO 4 and Next is GPIO 17 — both inside
GPIO0..21, the ESP32-S3's RTC-capable range, without which EXT1 deep-sleep
wake is impossible. VBUS detect and battery sense are reserved
on GPIO21 and GPIO1 under the same constraint, and neither is wired yet. The
panel avoids GPIO19 and GPIO20: they are the native USB pair the debug console
and JTAG share.

## Breadboard acceptance

The rig runs with a tactile button on each of Category and Next, and the
e-paper HAT wired to the panel pins.

| Item | Confirmed by |
|---|---|
| Firmware flashes, logs, and debugs over the DevKitC USB connection | the rig |
| Category advances and wraps through all six decks | the rig |
| One physical press produces one Next event | the rig |
| A press draws a question and the panel shows it | the rig |
| Real English and German TKB2 bundles round-trip through the question store | the suites, against bundles built from the database |
| Every released question fits at the fixed minimum type size | the suites, at the real panel geometry |
| Partial and full refresh behavior over a representative run | the rig — 193 partials clean at 622 ms, full refresh 2315 ms |
| Wi-Fi and bundle sync from USB power | nothing yet — `sync` is still design |

Two of these are honest only with their qualifier. The bundle and layout rows
are checked off the host suites against the corpus the device ships, which
proves the text fits and not that it reads well on the glass; and the refresh
row needs a number, not a yes — see "Bench measurements" in
[docs/hardware_wiring.md](../docs/hardware_wiring.md).
