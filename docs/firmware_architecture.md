# Firmware architecture

Structure of the ESP32-S3 application: threads, the messages between them, and
which modules depend on Zephyr. Interaction rules are in [design.md](design.md),
the data contract in [sync_protocol.md](sync_protocol.md), the pins and the
board in [hardware wiring](hardware_wiring.md), rationale in
[decisions.md](decisions.md).

**Status: the tabletop loop is built, and its state now survives a reboot.**
`input`, `app_fsm`, `qdb` with its bag, `layout`, `retained` and the panel all
exist and are tested, wired together by four of the six channels. Still design:
`sync`, `portal`, `power`, and deep sleep itself — `CONFIG_PM` is off, so
nothing sleeps, but the state that has to outlive a wake is already in RTC
memory rather than waiting on it. Items marked *verify* have not been run on
hardware. The [firmware primer](firmware_primer.md) is the hands-on tour.

This document follows the code that exists: six one-hot category inputs from
the current DIP switch and one Next button. The target enclosure has adjacent
Category and Next buttons. Its input mapping and retained category state are a
later firmware change, after a second button is available; the sections below
must not be read as rev A schematic requirements.

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
everything else reports or renders. Solid boxes and arrows are built; dashed
ones are design.

```mermaid
flowchart LR
    ISR([GPIO interrupts]):::hw --> WQ

    WQ["`**system workqueue**
    coop -1 · 1.5 KB
    input.c — debounce · settle`"]

    APP["`**app**
    prio 5 · 2 KB
    app.c + app_logic.cpp`"]

    DSP["`**display**
    prio 6 · 2.5 KB
    display.c + panel.cpp`"]

    NET["`**net**
    prio 8 · 12 KB
    portal OR sync`"]:::todo

    WQ -- chan_selector --> APP
    WQ -- chan_next --> APP
    WQ -. chan_power .-> APP
    WQ -. chan_power .-> NET
    APP -- chan_question --> DSP
    DSP -- chan_render --> APP
    NET -. chan_corpus .-> APP
    APP -. starts .-> NET

    DSP --> PANEL[/e-paper/]:::hw
    APP --> QDB[(qdb · corpus in flash)]:::store
    NET -. replaces .-> QDB

    classDef hw fill:#f4efe6,stroke:#b0a086
    classDef store fill:#eef1f4,stroke:#8a97a6
    classDef todo fill:#f7f5f1,stroke:#b3aca0,stroke-dasharray:4 3,color:#7a736a
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

`CONFIG_PM` is off today, so nothing sleeps yet — but `app` already blocks with
`K_FOREVER` except in `REFRESHING`, which is the property enabling it later.

## What each module owns

Every module has one job, one input and one output. The rule that decides which
language a file is in is mechanical: `ZBUS_CHAN_DEFINE`, `ZBUS_MSG_SUBSCRIBER_DEFINE`
and `ZBUS_CHAN_ADD_OBS` expand to out-of-order designated initializers, which C
allows and C++17 rejects, so files using them are `.c` and everything else is
free to be C++.

| Module | File | In | Out |
|---|---|---|---|
| `input` | `app/src/input.c` | GPIO edges via the `gpio-keys` driver | `chan_selector`, `chan_next` |
| `app` | `app/src/app.c` | those two channels, plus `chan_render` | calls into `app_logic` |
| `app_logic` | `app/src/app_logic.cpp` | posts from `app.c` | `chan_question` |
| `app_fsm` | `lib/app_fsm/` | selector, press, render result | which deck to draw, and when |
| `qdb` | `lib/qdb/` | a TKB2 byte range, a deck, a depth cap | one question, no repeats |
| `layout` | `lib/layout/` | UTF-8 text, a column count | lines of base glyph + mark |
| `retained` | `lib/retained/` | the block RTC memory handed back | whether it survived, or a zeroed one |
| `panel` | `app/src/panel.cpp` | a question | pixels, and a full/partial choice |
| `display` | `app/src/display.c` | `chan_question` | `chan_render` |
| `channels` | `app/src/channels.c` | — | the channel definitions themselves |

`lib/` is everything that compiles without a Zephyr header, which is what lets
the suites run it on a host. `app/src/` is the glue that gives it threads,
channels and a display.

## Where the questions come from

The corpus is not fetched at runtime yet. It is compiled into the image, which
is the factory-preloaded set [design.md](design.md) promises: a device whose
Wi-Fi is never configured still works.

```mermaid
flowchart LR
    YAML[/"questions/*.yaml<br/><i>the source of truth</i>"/]
    BUILD["tools/build_bundle.py"]
    TKB[/"dist/bundles/*.tkb<br/><i>gzip, signed</i>"/]
    RAW[/"tests/fixtures/*.tkb2<br/><i>decompressed</i>"/]
    INC["generate_inc_file_for_target"]
    IMG[("corpus in flash")]
    QDB["qdb::open()"]

    YAML --> BUILD
    BUILD --> TKB
    TKB -- "just fw-fixtures" --> RAW
    RAW --> INC
    INC --> IMG
    IMG --> QDB

    classDef f fill:#f7f5f1,stroke:#9a8f7d
```

The same decompressed bundle feeds the `qdb`, `layout` and `panel` suites, so
the corpus the tests check is the corpus the device ships. When `sync` lands,
`open()` points at a LittleFS file instead and the embedded copy becomes the
fallback.

## qdb — the question store

`qdb` is the only thing that knows what a question is. It answers exactly one
question itself: *given this deck, what should the device show next?* It is
plain C++17 with no Zephyr headers, about 370 lines, and it is two pieces that
share a header.

### The reader

`Qdb` is a decoder for the TKB2 format in [sync_protocol.md](sync_protocol.md).
It does not own or copy the bundle — `open()` takes a pointer and a length, and
every `Question` it hands back points into that buffer. The bundle outlives the
questions drawn from it.

Two decisions worth knowing:

- **Everything is validated at `open()`.** It walks all records once, checking
  every declared length against the buffer, and refuses the bundle on bad
  magic, a truncated record, trailing bytes, or a corpus larger than the bag can
  track. A file that would run off the end is rejected once, at the start,
  rather than on whichever draw first reaches the bad offset.
- **There is no offset table.** `at(index)` walks the records from the start.
  With 240 questions that is microseconds, and it costs no RAM on a device whose
  memory budget is the reason the corpus lives in flash at all.

A question is **eligible** for a deck when its deck-mask bit is set and its
depth is within `CONFIG_TK_PLAYBACK_DEPTH_MAX`. That is the whole filter, and it
is where the two editorial rules land: depth 3 is never drawn automatically, and
the tone-flagged questions carry only the Wild bit, so no other deck can reach
them.

### The bag

`Bag` is the draw-without-repeats policy. Its state is a plain struct, sized to
live in RTC memory so it can survive the reboot that deep sleep really is:

```
fingerprint   4 B     which bundle this state describes
recent[20]   40 B     question indices last seen, shared across decks
drawn[6][16] 384 B    one bit per question, per deck
```

Under 450 bytes of the 8 KB available. Keeping it out of NVS means a Next press
costs no flash write, so button life rather than flash endurance bounds the
device.

A draw applies exclusions in order of how much each one matters, and gives up
on them in the same order:

```mermaid
flowchart TB
    START([draw deck, depth]) --> A{"eligible,<br/>not drawn,<br/>not recent?"}
    A -- yes --> PICK
    A -- no --> B{"eligible,<br/>not drawn?"}
    B -- yes --> PICK
    B -- no --> RESET["clear this deck's bitmap<br/><i>the cycle is complete</i>"]
    RESET --> C{"eligible,<br/>not recent?"}
    C -- yes --> PICK
    C -- no --> D{"eligible at all?"}
    D -- yes --> PICK
    D -- no --> FAIL([no question — deck yields nothing])
    PICK["pick uniformly among candidates<br/>mark drawn · push onto the ring"] --> OUT([question])

    classDef q fill:#f7f5f1,stroke:#9a8f7d
    class A,B,C,D q
```

The order encodes what is negotiable. **The bag is the guarantee**: every
eligible question in a deck is shown once before any is shown twice, and the
bitmap is only cleared when the cycle is genuinely complete. **The ring is a
courtesy** and is dropped first, because the smallest shipped deck is no bigger
than the ring — Here has 20 eligible questions in English and 12 in German
against a 20-entry ring — and honouring it there would leave nothing to draw.
Refusing to answer a press is worse than repeating a question sooner than
ideal.

The ring is shared across decks rather than kept per deck. Close and New People
overlap heavily, so a per-deck ring would let turning the selector hand back a
question just read under the other name.

`fingerprint` is a cheap hash of the bundle's version, language and count.
`bind()` compares it and wipes the state when it differs — without that, a
synced bundle would leave bitmaps indexing questions that no longer exist. It
is already in place even though sync is not, because it is the part that would
be painful to add afterwards.

Randomness is injected as a function pointer rather than called directly, which
is what lets the suite run a deterministic sequence and the device use its
hardware RNG.

## Channels

Two channels are **state** (they hold their last value, so anyone can read
"what is true now") and three are **events**. That distinction is the domain
model: the selector *is* the deck, a press *happened*.

| Channel | Kind | Carries | Published by | Read by | Status |
|---|---|---|---|---|---|
| `chan_selector` | state | deck 0–5, valid flag | workqueue | `app` | built |
| `chan_next` | event | timestamp, press duration | workqueue | `app` | built |
| `chan_question` | state | seq, deck, text | `app` | `display` | built |
| `chan_render` | event | seq, result, was-full | `display` | `app` | built |
| `chan_power` | state | USB, charging, mV | workqueue | `app`, `net` | design |
| `chan_corpus` | event | version, language, count | `net` | `app` | design |

All observers are message subscribers, so a publish never blocks a publisher.

`chan_question` carries the text by value — `CONFIG_TK_MAX_QUESTION_BYTES` of
it — rather than a pointer into the corpus. A sync can replace the bundle
between the publish and the render, and a 128-byte copy is cheaper than the
rule that would otherwise be needed about who may free what.

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
    subgraph pure["firmware/lib — hardware-free · C++17 · runs under qemu"]
        direction LR
        FSM[fsm<br/><i>transition table engine</i>]
        APPFSM[app_fsm<br/><i>the only decision maker</i>]
        QDB[qdb<br/><i>TKB2 reader · bag</i>]
        LAY[layout<br/><i>UTF-8 · accents · wrap</i>]
    end

    subgraph glue["firmware/app/src — Zephyr-bound"]
        direction LR
        CHAN[channels.c<br/><i>zbus definitions</i>]
        INPUT[input.c<br/><i>ISR, debounce, settle</i>]
        APPC[app.c<br/><i>thread + subscriber</i>]
        LOGIC[app_logic.cpp<br/><i>fsm + qdb, behind a C API</i>]
        DISP[display.c<br/><i>thread</i>]
        PANEL[panel.cpp<br/><i>CFB, marks, refresh</i>]
    end

    subgraph todo["still design"]
        direction LR
        POWER[power<br/><i>ADC, VBUS, PM</i>]:::t
        SYNC[sync<br/><i>HTTPS, verify, swap</i>]:::t
        PORTAL[portal<br/><i>SoftAP setup</i>]:::t
    end

    FSM --> APPFSM
    APPFSM --> LOGIC
    QDB --> LOGIC
    LAY --> PANEL
    APPC --> LOGIC
    LOGIC --> CHAN
    INPUT --> CHAN
    DISP --> PANEL
    DISP --> CHAN
    APPC --> CHAN
    SYNC -.-> QDB
    SYNC -.-> CHAN
    POWER -.-> CHAN
    PORTAL -.-> SYNC

    classDef t fill:#f7f5f1,stroke:#b3aca0,stroke-dasharray:4 3,color:#7a736a
```

`panel.cpp` is C++ although it sits in the Zephyr-bound box: it calls `layout`
directly, and the display and CFB APIs are ordinary functions with none of the
macro trouble that keeps the others in C.

C++ stops at the Zephyr boundary for one mechanical reason: `ZBUS_CHAN_DEFINE`
and several HAL macros expand to out-of-order designated initializers, legal in
C and rejected by C++17. Keeping those files `.c` means upstream samples paste
in unmodified.

## The tabletop state machine

`app_fsm` uses the `Fsm` base class: the transition table is the
specification, `handle_current_state()` only picks which transition to return,
and each state gets its own `on_*()` handler so the table stays readable.

```mermaid
stateDiagram-v2
    [*] --> BOOT
    BOOT --> BOOT: REPEAT — selector invalid, no timeout
    BOOT --> SHOWING: RETAINED — panel already holds this deck
    BOOT --> CATEGORY: CONTINUE — announce the deck
    CATEGORY --> REFRESHING: CONTINUE
    CATEGORY --> FAIL: FAILED
    SHOWING --> SHOWING: REPEAT — idle → deep sleep
    SHOWING --> CATEGORY: RELABEL — the selector moved
    SHOWING --> DRAWING: REDRAW — Next
    DRAWING --> REFRESHING: CONTINUE
    DRAWING --> FAIL: FAILED — deck yields nothing
    REFRESHING --> REFRESHING: REPEAT — refresh timeout
    REFRESHING --> SHOWING: CONTINUE — render done
    REFRESHING --> FAIL: FAILED — render reported an error
    FAIL --> SHOWING: CONTINUE
```

Transitions are named for what happened, not for where they lead — the table
owns the destinations. `REDRAW`, `RELABEL` and `RETAINED` exist because
`SHOWING` and `BOOT` have more than one way out and `CONTINUE` cannot mean all
of them.

**Turning the selector announces the deck; Next asks a question.** The deck's
name goes on the panel and stays there until someone presses. That is what
makes the deck legible without printing the six names on the case — and it is
what lets the target Category button replace the rotary selector, since a deck
you can read on the glass does not need a labelled detent. The current input
path remains the DIP switch until the hardware and tests change together.

`SHOWING` checks Next before the selector, so a press made while the name is up
gets a question rather than the name again. In practice the two cannot both be
pending — the selector needs 600 ms to settle — but the order is the rule.

The table above is `firmware/lib/app_fsm/app_fsm.cpp` line for line, and
`firmware/tests/app_fsm` drives every edge in it with a fake display and an
injected clock.

| State | Handler | Leaves when |
|---|---|---|
| `BOOT` | `on_boot()` | the selector reads one valid position |
| `CATEGORY` | `on_category()` | immediately — the deck's name is sent in `on_enter_state()` |
| `SHOWING` | `on_showing()` | Next arrives, or the deck differs from the one on screen |
| `DRAWING` | `on_drawing()` | immediately — the draw itself happens in `on_enter_state()` |
| `REFRESHING` | `on_refreshing()` | the display reports back, or `CONFIG_TK_REFRESH_TIMEOUT_MS` passes |
| `FAIL` | `on_fail()` | immediately |

`BOOT` has no timeout, so a device with a broken or mid-travel selector waits
indefinitely while keeping the panel readable. `REFRESHING` has one, so a dead
panel cannot wedge the device.

Two rules are in the handlers rather than the table, because they are about
what the machine remembers rather than where it goes:

- **A press that lands during `REFRESHING` is dropped, not queued.** A panel
  takes up to two seconds; honouring presses made during it would spend that
  time drawing questions nobody has read.
- **Entering `FAIL` adopts the selector's deck as the one on screen.** Without
  that, a deck yielding nothing would leave `SHOWING` looking at a deck it has
  not drawn, ask again, fail again, and spin. Adopting it means the device sits
  on the previous question until the user presses or turns something.
- **Entering `DRAWING` discards any render result still pending.** It can only
  belong to an earlier question, one whose refresh timed out and then finished
  anyway. Left in place it satisfies the next refresh the instant that refresh
  begins, so the panel is told to draw and the machine calls it done in the same
  breath. This was a real bug on hardware: one press appeared to do nothing, and
  the next showed two questions in quick succession. `app_logic` adds a second
  guard by matching the `seq` on `chan_render` against the last question
  published, so a late result is dropped before it reaches the machine at all.

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

## A press, as the firmware runs today

Every participant below is a real thread or driver, and the whole path is
covered by `firmware/tests/integration`.

```mermaid
sequenceDiagram
    autonumber
    participant BTN as Next button
    participant GK as gpio-keys driver
    participant WQ as system workqueue
    participant APP as app thread
    participant QDB as qdb + bag
    participant DSP as display thread
    participant PNL as panel + CFB

    BTN->>GK: falling edge
    GK->>GK: 30 ms debounce
    BTN->>GK: rising edge (release)
    GK->>WQ: input_report_key(KEY_ENTER)
    WQ->>WQ: measure press duration
    WQ->>APP: chan_next
    APP->>APP: SHOWING → DRAWING
    APP->>QDB: draw(deck, depth ≤ 2)
    QDB-->>APP: question, marked drawn + ringed
    APP->>DSP: chan_question (seq, deck, text)
    APP->>APP: DRAWING → REFRESHING
    DSP->>PNL: wrap, decompose accents, draw glyphs
    PNL->>PNL: write with blanking off → partial refresh
    PNL-->>DSP: done
    DSP->>APP: chan_render (seq, result)
    APP->>APP: REFRESHING → SHOWING, block on K_FOREVER
    Note over APP,DSP: a press arriving between 11 and 17 is dropped
```

Turning the selector is the same path with the 600 ms settle inserted before
`chan_selector`, so crossing three detents produces one question rather than
three.

## The wake path

Once deep sleep exists, the same work happens from a cold boot instead. This is
the design, not the current build — `CONFIG_PM` is off and nothing sleeps yet:

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

**The retained block is built.** `lib/retained` owns one struct — the bag's
state, the partial-refresh counter, and what the panel is currently showing —
and `app/src/retained_block.cpp` places it in `.rtc_noinit`, a NOLOAD section
inside `rtc_slow_seg` that nothing zeroes at startup. `.rtc.bss` would survive
the sleep and then be cleared on the way back up, which retains the memory and
discards the point of it.

RTC memory holds whatever it held, and a cold power-on is not distinguishable
from a wake by content alone, so the block carries a magic number, a layout
version, its own size and a hash of the payload. Anything failing that check is
zeroed. The check is not decoration: `Bag::State` carries `recent_len` and
`recent_next`, which index fixed arrays without bounds checks of their own, so
a block of garbage accepted as state is an out-of-bounds write.

```mermaid
flowchart TB
    BOOT([boot]) --> LOAD["retained_load()"]
    LOAD --> Q{"magic · version<br/>size · payload hash<br/>all agree?"}

    Q -- no --> ZERO["zero the block"]
    ZERO --> COLD["**cold boot**
    fresh shuffle cycle
    next refresh is full
    draw a question"]

    Q -- yes --> WAKE{"was a question
    on the glass,
    from this deck?"}

    WAKE -- yes --> FREE["**free wake**
    bag continues · counter continues
    nothing drawn at all"]
    WAKE -- no --> DRAW["**wake**
    bag continues · counter continues
    draw, partial refresh"]

    COLD --> SEAL
    FREE --> SEAL
    DRAW --> SEAL["seal after each render"]

    classDef q fill:#f7f5f1,stroke:#9a8f7d
    classDef bad fill:#f4efe6,stroke:#b0a086
    class Q,WAKE q
    class ZERO,COLD bad
```

The right-hand path is the one that pays for the whole mechanism. A cold boot
costs a 2315 ms full refresh; a free wake costs nothing at all.

`retained_matches()` therefore answers truthfully, and a wake to a question the
panel already holds costs no refresh. A deck name does not count — waking to
one means the selector was turned and Next never pressed, so the name stays up
rather than being read as an answer.

*Verified at link time:* the block lands at `0x50000000`, which is
`rtc_slow_ram`, and takes `rtc_slow_seg` from 36 to 488 bytes of the 8 KB
available. The `esp32s3_devkitc/esp32s3/procpu` board also lists `retained_mem`
as supported, so Zephyr's driver API is available; the section attribute is
used instead because it hands out a struct rather than a read/write interface,
and the bag mutates in place.

*Verified on hardware*, with `just fw-retain` — the soak image rebooting itself
every three presses, since a warm reboot is what a deep-sleep wake will be.
Across three consecutive reboots:

```
rst:0xc (RTC_SW_CPU_RST)
<inf> tk_main: reset: software
<inf> tk_app: retained state: kept across the reboot
<inf> tk_soak: partial #6 of seq 7      <- before
rst:0xc (RTC_SW_CPU_RST)
<inf> tk_soak: partial #7 of seq 8      <- after
```

Four things in that, and all four are the point:

- the bag kept drawing new questions rather than restarting its cycle;
- `seq` continued rather than resetting to 1;
- the refresh counter continued, so **no full refresh followed any reboot** —
  the 2315 ms this was built to avoid;
- nothing was drawn at boot at all. The first question after each reboot
  arrives one soak interval later, not immediately, which is
  `retained_matches()` sending `BOOT` straight to `SHOWING`.

*The reset button proves nothing here.* A reset through the DevKitC's EN pin
reports as `rst:0x1 (POWERON)`, takes the RTC domain with it, and the firmware
correctly says `cold boot, starting a fresh cycle`. Only a warm reset from
software keeps RTC memory, which is why the check needs an image that reboots
itself rather than a finger on the button.

*Still unconfirmed:* `PM_STATE_SOFT_OFF` mapping to real deep sleep rather than
light sleep, whether a genuine wake behaves like the warm reboot tested here,
and EXT1 wake — which cannot be armed the way this document used to describe.
See below.

### The selector cannot be a plain EXT1 wake source

A deck is selected by *holding* one contact closed. That pin is low for as long
as the device sits on the table, so arming all six for EXT1 ANY_LOW gives a
device that wakes the instant it sleeps, forever.

The mask has to be computed at sleep time from the current selector reading:
arm the five *open* contacts plus Next, and leave out the one that is closed.
A rotary switch breaks before it makes, so turning the knob releases the old
contact — no wake, nothing is listening for it — and then closes a new one,
which is in the mask and is the wake. Next is open when unpressed, so it needs
no special handling.

```mermaid
flowchart LR
    subgraph rest["at rest, deck 2 selected"]
        direction TB
        P0["GPIO4 · deck 0 — open, high"]:::armed
        P2["GPIO6 · deck 2 — CLOSED, low"]:::held
        P5["GPIO16 · deck 5 — open, high"]:::armed
        PN["GPIO17 · Next — open, high"]:::armed
    end

    rest --> MASK["EXT1 mask = every pin that is high<br/><i>ANY_LOW</i>"]
    MASK --> SLEEP([deep sleep])
    SLEEP --> W1["knob leaves 2<br/><i>nothing happens — 2 is not armed</i>"]
    W1 --> W2["knob reaches 3<br/><i>GPIO7 goes low</i>"]
    W2 --> WAKE([wake])
    SLEEP --> NX["Next pressed<br/><i>GPIO17 goes low</i>"] --> WAKE

    classDef armed fill:#eef1f4,stroke:#8a97a6
    classDef held fill:#f4efe6,stroke:#b0a086
```

Arming the closed pin is the mistake: it is already low, so the wake condition
is satisfied before sleep is entered and the device never stays asleep.

VBUS detect joins the mask with the opposite polarity, since plugging in drives
it high.

Two consequences worth stating: the wake mask is state that has to be
recomputed on every sleep rather than configured once, and a device whose
selector is between detents has no closed contact, so it arms all six and wakes
on whichever is reached first — which is the behaviour wanted anyway.

## Testing boundary

```mermaid
flowchart LR
    A["fsm · app_fsm · qdb · layout<br/><b>no Zephyr headers</b>"] --> H
    B["settle window · drop-during-refresh<br/>panel refresh policy · GPIO to panel<br/><i>gpio-emul + dummy display</i>"] --> H["qemu_xtensa/dc233c<br/><i>macOS, full kernel</i>"]
    B --> N["native_sim<br/><i>Linux only, faster</i>"]
    C["the image itself · ghosting<br/>timing · power · sleep current"] --> HW["breadboard<br/><i>hardware required</i>"]

    classDef t fill:#eef1f4,stroke:#8a97a6
    class H,N,HW t
```

The dummy display accepts writes and discards them, so everything about
rendering *except the picture* is testable off-target: that each shipped
question lays out inside 25 × 7 cells, that accented text draws without
falling back, and that full refreshes come round on the interval. Whether it
looks right is a bench question.

Emulated GPIO is not a `native_sim` feature: `CONFIG_GPIO_EMUL` follows a
`zephyr,gpio-emul` devicetree node and works on either host platform, so the
suites that drive the selector and Next run in the default macOS loop.

`qdb` suites run against real bundles built from the question database by
`just fw-fixtures`, not hand-written bytes.

## Open items

- Category-button transition: replace the six one-hot selector inputs with one
  wake input, retain the category in RTC state, define the New People cold
  default, and test dropped Category presses during refresh. This waits for a
  second physical button.
- The refresh counter has to reach RTC memory before the full-refresh interval
  means anything. `tk_panel_init()` seeds it with the interval, so a cold boot
  always refreshes fully — correct today, and wrong the moment deep sleep makes
  every press a cold boot, because every question would then cost a 2.3 s full
  refresh instead of 622 ms. It is already listed under "Where state lives"; the
  measured numbers are what make it load-bearing rather than tidy.
- The bundled font. Zephyr's CFB fonts are 10x16 monospace and cover ASCII
  only, so `lib/layout` decomposes accented letters into a base glyph plus a
  mark that `app/src/panel.cpp` draws itself — enough for German, French,
  Spanish and Italian without licensing a font file. Two gaps remain: `¿` and
  `¡` fall back to `?` and `!`, and a monospace terminal font is not what this
  object should look like. Latin-1 coverage is a requirement to put on the
  eventual product font rather than a separate task.
- Whether `build_bundle.py` should assert the device's text buffer size so an
  over-long question fails in CI rather than on the device. Partly covered
  already: the `layout` and `qdb` suites fold every shipped question at the
  real panel geometry, so an over-long one fails a test — but only once
  someone runs the firmware suites.
- The recent ring is specified here as shared across decks; the website keeps a
  per-deck window. They should agree before either is called done.
