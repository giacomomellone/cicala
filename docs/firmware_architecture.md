# Firmware architecture

The ESP32-S3 firmware runs the tabletop interaction, preserves state through
deep sleep, manages the setup portal, and installs signed question and firmware
updates. See [design.md](design.md) for interaction rules,
[sync_protocol.md](sync_protocol.md) for bundle formats, and
[hardware_wiring.md](hardware_wiring.md) for the breadboard pins.

Every module builds and runs on the breadboard. Whole-device sleep current is
still unmeasured because the DevKitC indicators exceed the 30 µA target. The
[firmware primer](firmware_primer.md) covers setup and common development
tasks.

## Runtime model

ESP32-S3 deep sleep restarts the application. SRAM and thread stacks are lost;
`main()` runs again after a button wake. State needed across wakes lives in RTC
slow memory or flash.

The device enters deep sleep after the idle timeout when all three conditions
hold:

1. the tabletop state machine has settled;
2. the portal, sync, and update paths are idle;
3. external power is absent.

External power keeps the application awake. It also opens a bounded window for
sync and update checks.

```mermaid
flowchart LR
    GPIO[Category and Next GPIO] --> WQ[system workqueue<br/>debounce]
    WQ -->|chan_category / chan_next| APP[app thread<br/>tabletop policy]
    APP --> QDB[qdb + shuffle bag]
    APP -->|chan_question| DISPLAY[display thread]
    DISPLAY --> PANEL[e-paper]
    DISPLAY -->|chan_render| APP

    POWER[power work item<br/>cell ADC + VBUS] -->|chan_power| STATUS[status LEDs]
    NET[net thread<br/>portal · sync · OTA] -->|chan_service / chan_corpus| APP
    NET --> NVS[(NVS)]
    NET --> LFS[(LittleFS)]
```

The display has its own thread because an e-paper refresh can block for up to
two seconds. The app thread remains responsive and can apply the rule that
presses during a refresh are discarded. Network coordination also has a thread;
short ADC and VBUS reads run as delayed work.

## Code boundaries

Hardware-free policy lives in `firmware/lib/`. Zephyr threads, drivers,
channels, and storage adapters live in `firmware/app/src/`.

| Area                                         | Owns                                                                |
| -------------------------------------------- | ------------------------------------------------------------------- |
| `lib/fsm`                                    | table-driven state machine engine with an injectable clock          |
| `lib/app_fsm`                                | Category, Next, render, and service-card policy                     |
| `lib/qdb`                                    | QDB3 validation and draw-without-repeats state                      |
| `lib/layout`                                 | UTF-8 decoding, accent composition, and line wrapping               |
| `lib/portal`                                 | portal state machine, form parsing, DNS replies, and HTML rendering |
| `lib/power`                                  | battery and external-power states                                   |
| `lib/status`                                 | LED priority and blink patterns                                     |
| `lib/sync`                                   | manifest parsing and version comparison                             |
| `lib/retained`                               | validation of the RTC-retained block                                |
| `lib/ed25519`                                | detached-signature verification                                     |
| `app/src/input.c`                            | Category and Next events from `gpio-keys`                           |
| `app/src/app.c`, `app_logic.cpp`             | app thread, state machine effects, and corpus binding               |
| `app/src/display.c`, `panel.cpp`             | display thread, layout, and refresh policy                          |
| `app/src/net.c`, `net_logic.cpp`, `portal.c` | Wi-Fi, setup, and network events                                    |
| `app/src/fetch.c`, `sync.cpp`, `ota.cpp`     | HTTP transport and signed updates                                   |
| `app/src/power.c`, `power_logic.cpp`         | ADC/VBUS sampling and power state publication                       |
| `app/src/status.c`, `status_logic.cpp`       | serialized LED output                                               |
| `app/src/sleep.c`                            | idle timer, wake sources, and pin state for deep sleep              |

Files that define zbus objects stay in C because Zephyr's macros use
initializers that C++17 rejects. C++ policy is exposed to those files through
small C APIs.

## Channels

`chan_question` and `chan_power` hold current state. The other channels carry
events.

| Channel         | Carries                            | Publisher       | Consumer |
| --------------- | ---------------------------------- | --------------- | -------- |
| `chan_category` | timestamp and press duration       | input work item | app      |
| `chan_next`     | timestamp and press duration       | input work item | app      |
| `chan_question` | sequence, category, and text       | app             | display  |
| `chan_render`   | sequence, result, and refresh type | display         | app      |
| `chan_service`  | setup or sync result card          | net             | app      |
| `chan_corpus`   | active language change             | net             | app      |
| `chan_power`    | power state, millivolts, and VBUS  | power work item | status   |

Messages use subscribers, so publishers do not wait for consumers.
`chan_question` copies the text into the message. This keeps it valid if sync
replaces the underlying corpus before the display thread reads it.

The app is the only publisher of `chan_question` and owns its sequence number.
Render results with an older sequence are ignored. Corpus changes take effect
on the next requested draw and do not refresh the panel by themselves.

## Question data

Every shipped language is compiled into the firmware. A valid synced corpus in
LittleFS takes precedence; the compiled copy remains the fallback.

```mermaid
flowchart LR
    YAML[questions/*.yaml] --> BUILD[tools/build_bundle.py]
    BUILD --> GZIP[dist/bundles/*.qdb.gz]
    GZIP --> RAW[firmware test fixtures<br/>raw QDB3]
    RAW --> EMBED[compiled corpora]
    HTTP[verified sync download] --> LFS[(LittleFS /corpus)]
    EMBED --> OPEN[qdb::open]
    LFS --> OPEN
```

The selected language is stored in NVS. Changing it reopens the corpus and
publishes `chan_corpus`. Binding the shuffle bag to a different bundle
fingerprint clears indices that belong to the previous corpus.

### QDB reader

`Qdb::open()` validates the whole QDB3 buffer: magic, record lengths, question
count, and trailing bytes. Questions point into that buffer, so the buffer must
outlive every returned view.

`Qdb::at()` walks records from the start. The shipped corpora are small enough
that an offset table would cost more RAM than the scan saves.

A question is eligible when its category bit is set and its depth does not
exceed `CONFIG_KVELD_PLAYBACK_DEPTH_MAX`. Corpus validation restricts dark and
spicy questions to Wild.

### Shuffle bag

The bag tracks drawn questions per category and keeps a recent-question ring
shared by all categories. A draw applies these rules:

```mermaid
flowchart TB
    START([draw]) --> A{eligible,<br/>undrawn,<br/>not recent?}
    A -- yes --> PICK[pick uniformly]
    A -- no --> B{eligible and undrawn?}
    B -- yes --> PICK
    B -- no --> RESET[clear this category's drawn bitmap]
    RESET --> C{eligible and not recent?}
    C -- yes --> PICK
    C -- no --> D{any eligible question?}
    D -- yes --> PICK
    D -- no --> FAIL([no question])
    PICK --> MARK[mark drawn and add to recent ring]
```

Every eligible question in a category is shown once before that category's
bitmap resets. The recent ring is relaxed when it would prevent a draw. The bag
state lives in RTC slow memory, so ordinary Next presses do not write flash.

With `CONFIG_KVELD_TEXTURE` on, each uniform pick above becomes a preference.
A candidate scores one point for repeating the depth band of the question just
served and one for sharing a form tag with it, and the draw is uniform within
the cheapest class, relaxing toward uniform when the pool offers nothing
cheaper. The state this needs — last band and last form mask, two bytes,
shared across categories like the ring — describes the panel rather than the
table: texture decorrelates consecutive draws and never escalates.

## Tabletop state machine

Category advances the active category and displays its name. Next draws a
question from the active category. A cold boot displays the active category;
a button wake replays the latched Category or Next action.

```mermaid
stateDiagram-v2
    [*] --> BOOT
    BOOT --> SHOWING: retained question matches
    BOOT --> DRAWING: Next wake
    BOOT --> CATEGORY: cold boot or Category wake
    CATEGORY --> REFRESHING: category card queued
    CATEGORY --> FAIL: queue failed
    SHOWING --> DRAWING: Next
    SHOWING --> CATEGORY: Category
    SHOWING --> SERVICE: service card pending
    DRAWING --> REFRESHING: question queued
    DRAWING --> FAIL: no question
    SERVICE --> REFRESHING: service card queued
    SERVICE --> FAIL: queue failed
    REFRESHING --> SHOWING: matching render result
    REFRESHING --> FAIL: error or timeout
    FAIL --> SHOWING
```

The following rules sit at the state-machine boundary:

- Presses received during `REFRESHING` are discarded.
- Service cards take priority when the machine returns to `SHOWING`.
- Each draw, category card, or service card clears stale render state before it
  queues a refresh.
- A failed category or draw is considered settled until another input arrives.
- `CONFIG_KVELD_REFRESH_TIMEOUT_MS` prevents a stalled panel from holding the
  machine in `REFRESHING` forever.

## Display

`panel.cpp` lays out UTF-8 text through `lib/layout`, draws through Zephyr CFB,
and selects full or partial refresh. A full refresh is forced at boot and after
`CONFIG_KVELD_FULL_REFRESH_INTERVAL` partial updates.

The SSD16xx driver performs a full update when blanking changes from on to off.
`panel.cpp` therefore uses `display_blanking_on()`, `display_write()`, then
`display_blanking_off()` for a full refresh. A partial refresh writes while
blanking is already off.

The local Zephyr patch controlled by `firmware/patches.yml` preserves the image
during driver initialization. Without it, every deep-sleep wake would clear the
bistable panel before the app could decide that no refresh was needed.

Before system power-off, `sleep.c` sends the panel controller to deep sleep and
holds its reset, D/C, and chip-select pins inactive.

## Retained state and wake

`app/src/retained_block.cpp` places one validated block in `.rtc_noinit` inside
RTC slow memory. It contains:

- shuffle-bag state and the shared recent ring;
- the partial-refresh counter;
- active and displayed categories;
- whether the panel holds a question;
- the last question sequence number.

The block carries a magic value, layout version, size, and payload hash. A
failed check clears the whole block and selects New People. The block is sealed
after each completed render.

```mermaid
flowchart TB
    BOOT([boot]) --> CHECK{retained block valid?}
    CHECK -- no --> COLD[clear state<br/>select New People]
    CHECK -- yes --> MATCH{panel holds a question<br/>from the active category?}
    MATCH -- yes --> KEEP[keep panel unchanged]
    MATCH -- no --> ACTION[replay wake action<br/>or show category]
    COLD --> ACTION
```

Category and Next are active-low RTC-capable inputs. At sleep time, firmware
arms every input that currently reads high with EXT1 `ANY_LOW`. A held or stuck
button is omitted from the mask, avoiding an immediate wake loop while leaving
the other button usable. The EXT1 wake status is latched at `PRE_KERNEL_1` so
later driver initialization cannot erase it. Next wins if both bits are set.

VBUS uses a separate active-high EXT0 trigger when
`CONFIG_KVELD_POWER_WAKE_ON_USB` is enabled. It is disabled by default because it
keeps the RTC peripheral domain powered. With the default setting, plugging in
does not wake the device; the next button press wakes it, VBUS is sampled during
boot, and the network window opens.

## Power and status

`power.c` samples the switched battery divider and VBUS. ADC failure leaves the
state `UNKNOWN`; VBUS remains usable because it is read independently.

```mermaid
stateDiagram-v2
    [*] --> UNKNOWN
    UNKNOWN --> NORMAL: plausible cell reading
    NORMAL --> LOW: below refresh floor
    LOW --> CRITICAL: below critical threshold
    CRITICAL --> LOW: recovered with hysteresis
    LOW --> NORMAL: recovered with hysteresis
    NORMAL --> UNKNOWN: implausible reading
    LOW --> UNKNOWN: implausible reading
    CRITICAL --> UNKNOWN: implausible reading
    UNKNOWN --> CHARGING: VBUS high
    NORMAL --> CHARGING: VBUS high
    LOW --> CHARGING: VBUS high
    CRITICAL --> CHARGING: VBUS high
    CHARGING --> CHARGED: above full estimate
    CHARGED --> CHARGING: below full estimate with hysteresis
    CHARGING --> UNKNOWN: VBUS low
    CHARGED --> UNKNOWN: VBUS low
```

LOW and CRITICAL refuse panel refreshes. UNKNOWN permits them, so a missing or
failed divider does not disable the tabletop interaction. Downward transitions
are immediate. Recovery uses hysteresis. A reading below
`CONFIG_KVELD_POWER_PLAUSIBLE_MV` is treated as an invalid divider reading.

The CHARGED state is a voltage estimate; the breadboard charger has no
termination-status output. External power keeps the device awake in both
CHARGING and CHARGED.

The red and green LEDs share one priority arbiter:

| Condition                          | Output           |
| ---------------------------------- | ---------------- |
| critical refresh refused           | three red blinks |
| low refresh refused                | one amber blink  |
| sync or update active              | slow green pulse |
| setup portal active                | steady amber     |
| charged estimate                   | steady green     |
| external power present             | steady red       |
| healthy battery or unknown reading | off              |

Calls from app and network threads set atomic flags. The status work item owns
the arbiter and GPIO writes, so blink state has a single writer.

## Setup, sync, and updates

Holding Category and Next for `CONFIG_KVELD_PORTAL_ENTRY_HOLD_MS` during boot opens
the setup portal. The device scans first, starts a WPA2 access point with a
fresh session password, serves DHCP/DNS/HTTP, stores submitted credentials, then
joins the selected network. The password is generated from the device random
source, shown on the e-paper service card, and cleared when the portal closes.
The setup page can rescan nearby networks and save the question language
separately from Wi-Fi. Its status page shows the access-point name, remaining
portal window, board and firmware versions, corpus information, connection
diagnostics, and the last requested sync result. It can request a question or
firmware update, or forget all saved Wi-Fi credentials after confirmation. The
stored password is never shown. The portal closes at its configured deadline.

```mermaid
stateDiagram-v2
    [*] --> OFF
    OFF --> SCANNING: both buttons held through boot
    SCANNING --> AP_STARTING: scan complete or timed out
    AP_STARTING --> SERVING: access point ready
    AP_STARTING --> SHUTDOWN: start failed
    SERVING --> CONNECTING: credentials submitted
    SERVING --> SHUTDOWN: portal deadline
    CONNECTING --> CONNECTED: station joined
    CONNECTING --> SERVING: join failed or timed out
    CONNECTED --> SHUTDOWN: portal deadline
    SHUTDOWN --> OFF
```

A cold boot with stored credentials checks for new data. A button wake also
checks while the external-power window is open. Battery-only wakes skip Wi-Fi
so the requested question is not delayed by network association.

Question sync performs these checks before storing a bundle:

1. manifest schema and minimum firmware version;
2. version newer than the installed corpus;
3. declared download size;
4. SHA-256 digest;
5. Ed25519 signature against the compiled public key.

The verified bytes are written beside the installed corpus and renamed over it.
A failure leaves the previous file intact. An automatic sync keeps the current
panel content. A sync requested from the portal publishes a result card.

OTA uses the same fetch, manifest, hash, and signature code. The image streams
into `slot1_partition`; it is marked for upgrade only after verification.
MCUboot validates and installs it on the next boot. `update_notice.c` displays
the installed version after a successful update. See
[firmware_update.md](firmware_update.md).

## Persistent storage and security

```mermaid
flowchart LR
    RTC[RTC slow memory<br/>bag · display · active category] -->|lost on total power loss| COLD[cold defaults]
    NVS[NVS<br/>Wi-Fi credentials · language · versions] -->|cleared by factory reset| EMPTY[unset]
    LFS[LittleFS<br/>synced QDB3 files] -->|atomic rename| NEW[new corpus]
    SLOT[MCUboot secondary slot<br/>candidate firmware] -->|verified boot| APP[running image]
```

The current breadboard build does not enable flash encryption. Physical access
to its flash exposes Wi-Fi credentials and stored corpora. Product provisioning
needs ESP32-S3 eFuse-backed flash encryption before the device is used on a
sensitive network.

## Testing boundary

```mermaid
flowchart LR
    PURE[fsm · app_fsm · qdb · layout<br/>portal · power · status] --> HOST[qemu or native_sim]
    SEAMS[GPIO debounce · zbus flow<br/>panel policy · real QDB fixtures] --> HOST
    PHYSICAL[panel image · timing · radio<br/>battery · sleep current] --> BENCH[breadboard or rev A]
```

`just fw-test` runs the firmware suites under qemu. Linux can use
`just fw-test-linux` for `native_sim`. Both platforms use `gpio-emul` for the
button suites. Generated QDB fixtures keep the writer and firmware reader on
the same data contract.

The dummy display checks bounds, refresh selection, and channel behavior. Image
quality, ghosting, radio behavior, power thresholds, and current require
hardware.

## Open measurements

- Measure whole-device deep-sleep current on rev A without DevKitC indicator
  loads.
- Measure the lowest reliable e-paper refresh voltage and set
  `KVELD_REFRESH_MIN_MV` from that result.
- Measure the current cost of enabling VBUS wake through EXT0.
- Verify font coverage and layout for every released language.
- Run multi-phone setup-portal and long-transfer stress tests.
