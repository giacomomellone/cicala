# Firmware architecture

Structure of the ESP32-S3 application: threads, the messages between them, and
which modules depend on Zephyr. Interaction rules are in [design.md](design.md),
the data contract in [sync_protocol.md](sync_protocol.md), rationale in
[decisions.md](decisions.md).

**Status: partly built.** `fsm` and a bring-up blinky exist and are tested;
everything else is design. Items marked *verify* have not been run on hardware.
The [firmware primer](firmware_primer.md) is the hands-on tour of what is
there.

## The one constraint

Below 30 µA means deep sleep, and on the ESP32-S3 deep sleep is a reboot: SRAM
and every thread stack are gone, and waking runs `main()` from the top.

- The steady state is **off**, not idle. Threads live for the milliseconds
  between wake and sleep.
- "Next question under 1 s" is a **boot-time** budget, not a scheduling one.
- Persisted state lives in **RTC slow memory**, never in `.bss`.
- Sleep is never called. It is what happens when no thread is runnable.

## Threads and data flow

Every arrow is a zbus message. `app` is the only thread that decides anything;
everything else reports or renders.

```mermaid
flowchart LR
    ISR([GPIO interrupts]):::hw --> WQ

    WQ["`**system workqueue**
    coop -1 · 1.5 KB
    debounce · settle · battery`"]

    APP["`**app**
    prio 5 · 2 KB
    state machine`"]

    DSP["`**display**
    prio 6 · 2.5 KB
    layout · refresh`"]

    NET["`**net**
    prio 8 · 12 KB
    portal OR sync`"]

    WQ -- chan_selector --> APP
    WQ -- chan_next --> APP
    WQ -- chan_power --> APP
    WQ -- chan_power --> NET
    APP -- chan_question --> DSP
    DSP -- chan_render --> APP
    NET -- chan_corpus --> APP
    APP -. starts .-> NET

    DSP --> PANEL[/e-paper/]:::hw
    APP --> QDB[(qdb · LittleFS)]:::store
    NET --> QDB

    classDef hw fill:#f4efe6,stroke:#b0a086
    classDef store fill:#eef1f4,stroke:#8a97a6
```

A thread exists only where something must block independently.

| Context | Blocks on | Why not merged |
|---|---|---|
| system workqueue | nothing (delayed work) | An input thread would buy nothing; `k_work_reschedule` *is* the 600 ms settle |
| `app` | its zbus queue | — |
| `display` | the panel, 0.3–2 s | So `app` stays awake during a refresh and can drop the Next press |
| `net` | sockets, TLS | Portal and sync never overlap; one thread saves ~12 KB |

There is no power thread. With `CONFIG_PM` the idle thread picks the sleep
state once nothing is runnable, so the design's job is to make every thread
block with **no timeout** when the table is quiet. `net` holds a PM lock while
active; nothing else inhibits sleep.

## Channels

Two channels are **state** (they hold their last value, so anyone can read
"what is true now") and three are **events**. That distinction is the domain
model: the selector *is* the deck, a press *happened*.

| Channel | Kind | Carries | Published by | Read by |
|---|---|---|---|---|
| `chan_selector` | state | deck 0–5, valid flag | workqueue | `app` |
| `chan_next` | event | timestamp, press duration | workqueue | `app` |
| `chan_question` | state | seq, deck, text | `app` | `display` |
| `chan_render` | event | seq, result, was-full | `display` | `app` |
| `chan_power` | state | USB, charging, mV | workqueue | `app`, `net` |
| `chan_corpus` | event | version, language, count | `net` | `app` |

All observers are message subscribers, so a publish never blocks a publisher.

Two contract rules that are easy to violate:

- **Press duration travels but is never read by policy.** It exists so a table
  study can answer "did anyone try to long-press?". Long press is Next.
- **`chan_corpus` must not trigger a redraw.** A new bundle applies on the next
  *requested* draw, silently.

## Modules

The split that matters is not epaper-versus-input, it is **what needs
hardware**. Everything in the upper box runs under qemu and is tested without a
board. `fsm` touches Zephyr only for its default clock, which tests override,
so every timeout is exercised without sleeping.

```mermaid
flowchart TB
    subgraph pure["hardware-free · C++17 · runs under qemu"]
        direction LR
        FSM[fsm<br/><i>transition table engine</i>]
        QDB[qdb<br/><i>TKB2 reader, eligibility</i>]
        BAG[bag<br/><i>draw without repeats</i>]
        LAY[layout<br/><i>UTF-8 word wrap</i>]
    end

    subgraph glue["Zephyr-bound · C"]
        direction LR
        CHAN[channels<br/><i>zbus definitions</i>]
        INPUT[input<br/><i>ISR, debounce, settle</i>]
        PANEL[panel<br/><i>ssd16xx, refresh</i>]
        POWER[power<br/><i>ADC, VBUS, PM</i>]
        SYNC[sync<br/><i>HTTPS, verify, swap</i>]
        PORTAL[portal<br/><i>SoftAP setup</i>]
    end

    APPFSM["app_fsm · C++17<br/><i>the only decision maker</i>"]

    FSM --> APPFSM
    QDB --> APPFSM
    BAG --> QDB
    LAY --> PANEL
    APPFSM --> CHAN
    INPUT --> CHAN
    PANEL --> CHAN
    POWER --> CHAN
    SYNC --> QDB
    SYNC --> CHAN
    PORTAL --> SYNC

    classDef box fill:#f7f5f1,stroke:#9a8f7d
```

C++ stops at the Zephyr boundary for one mechanical reason: `ZBUS_CHAN_DEFINE`
and several HAL macros expand to out-of-order designated initializers, legal in
C and rejected by C++17. Keeping those files `.c` means upstream samples paste
in unmodified.

## The tabletop state machine

`app_fsm` uses the `Fsm` base class: the transition table is the specification,
and `handle_current_state()` only picks which transition to return.

```mermaid
stateDiagram-v2
    [*] --> BOOT
    BOOT --> BOOT: selector invalid — keep panel, no timeout
    BOOT --> SHOWING: retained question still valid
    BOOT --> DRAWING: nothing retained
    SHOWING --> SHOWING: idle → deep sleep
    SHOWING --> DRAWING: Next, or deck changed
    DRAWING --> REFRESHING
    DRAWING --> FAIL: deck yields nothing
    REFRESHING --> SHOWING: render done
    REFRESHING --> FAIL: 5 s timeout
    FAIL --> SHOWING
```

`BOOT` has no timeout, so a device with a broken or mid-travel selector waits
indefinitely while keeping the panel readable. `REFRESHING` has one, so a dead
panel cannot wedge the device.

Service entry is not a state. USB-plus-Next starts `net` in portal mode and the
FSM continues in `SHOWING`, because the tabletop face stays a question display.

## Sync

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> CHECK: USB + known Wi-Fi + battery ok
    CHECK --> IDLE: already current
    CHECK --> DOWNLOAD: newer version
    DOWNLOAD --> VERIFY
    VERIFY --> SWAP: size → sha256 → ed25519
    SWAP --> IDLE: decompress, rename, chan_corpus
    CHECK --> FAILED
    DOWNLOAD --> FAILED
    VERIFY --> FAILED
    SWAP --> FAILED
    FAILED --> IDLE: old bundle intact
```

Checks run cheapest-first so a truncated download costs no signature work. Any
failure leaves the previous bundle in place. The gzip in a `.tkb` is transport
only — `SWAP` decompresses, because a device that reboots on every press must
not re-inflate the bundle each time.

## The wake path

The whole latency budget, from press to readable:

```mermaid
sequenceDiagram
    autonumber
    participant HW as Next button
    participant Z as ROM + Zephyr init
    participant A as app
    participant Q as qdb
    participant D as display

    HW->>Z: EXT1 wake (a reboot, not a resume)
    Z->>A: main()
    A->>A: read six selector GPIOs
    A->>Q: draw(deck, depth ≤ 2)
    Q-->>A: question
    A->>D: chan_question
    D->>D: partial refresh
    D-->>A: chan_render
    A->>A: block with no timeout → deep sleep
    Note over HW,D: under 1 s, end to end
```

Turning the selector follows the same path with the 600 ms settle inserted
before the draw, so crossing three detents produces one question rather than
three.

## Where state lives

```mermaid
flowchart LR
    RTC["`**RTC slow memory** ~512 B of 8 KB
    bag bitmaps · recent ring
    current question · refresh counter
    bundle fingerprint`"]
    NVS["`**NVS**
    language · Wi-Fi credentials
    bundle version`"]
    LFS["`**LittleFS**
    decompressed TKB2 corpus`"]
    NONE["`**nowhere**
    active deck`"]

    RTC -- lost when --> B1[cell removed or flat]
    NVS -- lost when --> B2[factory reset]
    LFS -- replaced by --> B3[atomic sync swap]
    NONE -- re-read from GPIO --> B4[every boot, by design]

    classDef k fill:#f7f5f1,stroke:#9a8f7d
    class RTC,NVS,LFS,NONE k
```

Keeping the bag out of NVS means a Next press costs no flash write, so button
life rather than flash endurance bounds the device. The active deck has no
storage at all, which is how "the selector is the deck state" is enforced
structurally rather than by discipline.

*Verify:* the `esp32s3_devkitc/esp32s3/procpu` board lists `retained_mem` as
supported, so Zephyr's retained-memory API is the likely mechanism rather than
raw section attributes. Still unconfirmed: `PM_STATE_SOFT_OFF` mapping to deep
sleep, and EXT1 wake on seven pins. Only the structures above depend on this,
so the fallback is a write-coalesced NVS record, not a redesign.

## Testing boundary

```mermaid
flowchart LR
    A["fsm · qdb · bag · layout<br/><b>no Zephyr headers</b>"] --> H["qemu_xtensa/dc233c<br/><i>macOS, full kernel</i>"]
    B["state machine · settle window<br/>drop-during-refresh"] --> N["native_sim<br/><i>Linux only, emulates GPIO</i>"]
    C["timing · power · panel · ghosting"] --> HW["breadboard<br/><i>hardware required</i>"]

    classDef t fill:#eef1f4,stroke:#8a97a6
    class H,N,HW t
```

`qdb` suites run against real bundles built from the question database by
`just fw-fixtures`, not hand-written bytes.

## Open items

- Pin assignments. Constraint: all six selector contacts, Next and VBUS must be
  RTC-capable GPIOs, or EXT1 wake is impossible.
- Full-refresh interval, from observed ghosting rather than a chosen number.
- Bundled font and its German coverage.
- Whether `build_bundle.py` should assert the device's text buffer size so an
  over-long question fails in CI rather than on the device.
- The recent ring is specified here as shared across decks; the website keeps a
  per-deck window. They should agree before either is called done.
