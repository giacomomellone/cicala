# Device prototype

The breadboard firmware works. Rev A is a PCB and enclosure design; its
mechanical files are concept inputs rather than manufacturing data.

!!! warning "Design intent, not built hardware"

    Rev A has not been built. Every dimension, mass, and finish below is a
    study target for CAD, not a measurement of a physical device. Only the
    bench results in the validation sequence come from working hardware, and
    they were taken on the breadboard rig.

This page stays text-first. It carries no illustrations until photographs of
real hardware exist, so nothing here can be mistaken for evidence that the
enclosure has been made.

## Interaction

The device has two adjacent buttons:

- **Category** advances through the six categories in a fixed order and shows
  the selected name on the e-paper;
- **Next** draws a question from the category named on the display.

The active category is visible on the e-paper. Reaching a category can take up
to five presses, so the physical study must check whether the cycle and refresh
delay remain easy to follow. Wi-Fi, language, and maintenance stay in the phone
setup flow. Normal playback uses editorial depths 1 and 2.

## Proposed form

The face has three elements: e-paper, Category, and Next. The two buttons sit
close together above the display, but they do not have equal visual weight.
Category is a small, nearly flush round button with its label printed on the
shell. Next is a larger, slightly raised rounded pill with a shallow concave
top and its label on the cap. Category names are printed by the display.

| Property      |                                     Study target | Reason                                                                        |
| ------------- | -----------------------------------------------: | ----------------------------------------------------------------------------- |
| Envelope      |                            about 84 × 56 × 16 mm | Allows the 59.2 × 29.2 mm panel, readable button labels, and finger clearance |
| Mass          |                                         80–100 g | Resists one-handed button presses                                             |
| Orientation   |                       landscape, 3° face incline | Keeps the device readable without becoming a wedge                            |
| Display       |     2.13″ e-paper behind a matte protective lens | Keeps the category and question visible without power                         |
| Category      |           small, nearly flush round button, left | Secondary action used only when changing context                              |
| Next          |        larger, raised rounded-pill button, right | Frequent primary action; easy to find without reading both labels             |
| Display label | active category in small type above the question | Allows translated labels without shell changes                                |
| Port          |                 centered USB-C on the lower edge | Absent from the main tabletop view                                            |
| Finish        |     warm-ivory matte shell; two charcoal buttons | Matches the paper-like website                                                |

The size is a planning envelope. Component drawings, button-label legibility,
and the e-paper layout test may change it.

## How category selection works

The order stays `new people`, `close`, `family`, `work`, `here`, `wild`.
Category advances one step and wraps from Wild to New People. A category press
replaces the old question with the new category name; another Category press
continues cycling, while Next draws from the category currently shown. A
question screen keeps the active category in a small line above the question.

This gives every state a visible result:

- an accepted Category press changes the name on the e-paper;
- an accepted Next press changes the question under that name;
- a press discarded during a panel refresh changes neither the display nor the
  stored category;
- removing power leaves the last category and question readable on the panel.

The firmware retains the category alongside the other RTC state and defaults to
New People after total state loss.

## Candidate controls

Use the same sealed tactile switch under both actions so force, travel, and
electrical behavior match. Different external caps create the hierarchy: the
Category cap is small and low; the Next cap is wider, slightly raised, and
shallowly concave.

**C&K KSC321GLFS**: IP67 SPST-NO tact switch, 6.2 × 6.2 mm footprint,
3.5 mm actuator height, 2 N force, and 300,000-cycle rating. Each switch needs
an external button cap with its own mechanical stop so enclosure loads do not
crush the switch.

IP67 at the component does not make the assembled enclosure IP67. The lens,
USB opening, shell seam, and both button-cap interfaces need their own paths
and tests.

## Mechanical stack

Use 16 mm as the initial CAD envelope.

| Layer                                    | Planning allowance |
| ---------------------------------------- | -----------------: |
| Top shell, lens, display support         |         1.8–2.2 mm |
| E-paper plus compliant perimeter support |         1.0–1.5 mm |
| PCB                                      |         1.0–1.2 mm |
| Local electronics and tactile switches   |         2.5–3.5 mm |
| Protected 503035 cell                    |         5.0–5.5 mm |
| Bottom shell and clearance               |         1.8–2.2 mm |

Put both switches in the same control area above the panel and carry button
loads into the top shell with cap stops sized for their different caps. Support
the e-paper glass continuously around its perimeter. Use four elastomer feet
positioned to resist either button press.

The category line needs tested fonts, right-to-left layout, and enough width
for the longest released label.

## Electrical contract

Rev A must provide:

- ESP32-S3-WROOM-1-N16 and the GDEY0213B74 e-paper circuit;
- one Category wake input and one independent Next wake input;
- retained category state with a defined New People cold default;
- an integrated charging power path or a validated load-sharing circuit;
- USB-C input and ESD protection, VBUS detection, and switched battery sensing;
- a protected 503035 cell with charge current set from its data sheet;
- e-paper power gating and a measured whole-device sleep budget below 30 µA;
- antenna keep-out clear of the display, battery, button hardware, and copper;
- hidden development pads, with no extra user-facing controls.

Bench results add these constraints:

- No always-on indicator. DevKitC indicators prevent a useful sleep-current
  measurement; rev A must expose the whole-device current without a permanent
  LED load.
- Use a switched 1 MΩ/470 kΩ battery divider. Its unswitched draw is about
  2.9 µA, and switching removes that sleep load.
- Sense VBUS from the USB rail. Firmware uses it for the sync window, charging
  wake policy, and the status LED.
- Prefer a charger with a termination-status output. The bq25185 rig can only
  infer a full cell from voltage.

The question bundle stores each question once with a six-bit deck mask and
depth metadata. Normal playback excludes depth 3. See
[sync_protocol.md](sync_protocol.md).

## Validation sequence

### 1. Website

The website exercises all six deck choices, overlapping membership, Wild tone,
the New People default, and the depth 1–2 playback cap.

### 2. USB-powered breadboard and firmware

The ESP32-S3 DevKitC, the assembled 2.13-inch e-paper module and two tactile
switches. Flashing, storage, deck selection, debounce, rendering, refresh policy
and Wi-Fi sync all run there. The [prototype BOM](prototype_bom.md) describes the
rig and [hardware wiring](hardware_wiring.md) wires it.

Verified:

- one Next press advances exactly once, and a held button counts once;
- every released English and German question fits;
- partial updates remain readable through a representative run — 193 of them,
  which is where the full-refresh interval comes from;
- firmware can be flashed and debugged without extra programming hardware.

### 3. Two-button interaction bench

The electrical and firmware checks pass:

- Category advances exactly one deck in the fixed order and wraps once;
- the displayed category always matches the deck used by Next;
- a discarded press during refresh does not change hidden state;
- cold-state loss defaults visibly to New People;
- repeated category changes stay readable.

The enclosure study must check whether people distinguish Category and Next
without explanation.

### 4. Battery and power-path bench

The breadboard uses a bq25185 charger board, protected cell, and both sense
dividers. It confirms charging under load, USB/battery handover, battery readings
within 1% of a meter, boot-time VBUS detection, the plug-in sync window, and
battery-only operation.

Sleep current requires rev A because the DevKitC indicators exceed the 30 µA
budget. Brownout margin and the panel's minimum reliable refresh voltage also
need measurement; see the acceptance targets below.

### 5. Controls, PCB, and enclosure integration

Import manufacturer STEP models, place the tested buttons, PCB, and battery,
then build the first printed enclosure.

Run table sessions without explaining the controls first. Check:

- people understand that Category cycles and Next draws;
- they can reach the intended category without losing track of the order;
- they distinguish New People from Close;
- Wild is chosen deliberately rather than mistaken for Random;
- the e-paper category line is readable from ordinary seats;
- the device does not slide, tip, or need to be picked up;
- the small Category control is easy to operate deliberately;
- the larger Next control is identifiable without reading both labels;
- the adjacent controls do not cause frequent wrong presses.

If category choice takes more attention than rejecting a poor question with
Next, revise the cycling interaction before another PCB revision. EVT then
covers drop, button life, cap overload, pocket lint, radio performance, and
small spills.

## Acceptance targets

| Area                | Target                                                                      |
| ------------------- | --------------------------------------------------------------------------- |
| First question      | already visible, with its active category                                   |
| Next question       | readable in under 1 s on a partial refresh                                  |
| Category change     | displayed category changes once per accepted press                          |
| State at zero power | active category and current question are readable                           |
| Sleep current       | below 30 µA measured at the cell                                            |
| Stability           | no slide or tip during a one-handed button press                            |
| Text                | all released questions and category labels fit at their fixed minimum sizes |
| Ingress             | no path from a small tabletop spill to PCB or cell                          |
| Service             | cell, display, and PCB replaceable without destructive adhesive             |

## Primary references

- [C&K KSC321GLFS](https://www.ckswitches.com/products/switches/product-details/Tactile/KSC3/KSC321GLFS/)
- [Good Display GDEY0213B74](https://www.good-display.com/product/391.html)
- [Espressif ESP32-S3-WROOM-1/1U data sheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [Espressif module current measurement](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/current-consumption-measurement-modules.html)
