# Device prototype

The breadboard firmware works. Rev A now has a parametric enclosure and a KiCad constraint board. They establish the design datums described here but remain prototype files rather than manufacturing data. See [Rev A hardware](hardware_rev_a.md) for the files, coordinates and release gates.

!!! warning "Design intent, not built hardware"

    Rev A has not been built. Every dimension, mass, and finish below is a
    study target for CAD, not a measurement of a physical device. Only the
    bench results in the validation sequence come from working hardware, and
    they were taken on the breadboard rig.

Technical CAD images are labelled as models. Photoreal concept renders remain outside the public engineering record until publication approval or built hardware exists.

## Interaction

The device has two adjacent buttons: **Filters** opens or advances the four-row
permissions menu, and **Next** draws during play or changes the selected menu
row. Done applies the draft and resumes the question when still permitted.
Dark, Sexual, and Heavy start excluded. The phone and device use the same
interaction; see [Design](design.md) for the complete contract.

## Proposed form

The face has three elements: e-paper, Filters, and Next. The two buttons sit close together above the display, but they do not have equal visual weight. Filters is a small, 0.2 mm sub-flush round button with its label printed on the shell. Next is a larger flush rounded pill with a shallow concave top and its label on the cap. These heights are coupon starting points. Permissions are printed by the display.

| Property      |                                        Study target | Reason                                                                        |
| ------------- | --------------------------------------------------: | ----------------------------------------------------------------------------- |
| Envelope      |                               about 84 × 56 × 16 mm | Allows the 59.2 × 29.2 mm panel, readable button labels, and finger clearance |
| Mass          |                                            80–100 g | Resists one-handed button presses                                             |
| Orientation   |                          landscape, 3° face incline | Keeps the device readable without becoming a wedge                            |
| Display       |        2.13″ e-paper behind a matte protective lens | Keeps the permission and question visible without power                       |
| Filters       |              small, nearly flush round button, left | Secondary action used when changing permissions                               |
| Next          |            larger, flush rounded-pill button, right | Frequent primary action; easy to find without reading both labels             |
| Display label | permission summary in small type above the question | Allows translated labels without shell changes                                |
| Port          |                    centered USB-C on the lower edge | Absent from the main tabletop view                                            |
| Finish        |        warm-ivory matte shell; two charcoal buttons | Matches the paper-like website                                                |

The size is a planning envelope. Component drawings, button-label legibility, and the e-paper layout test may change it.

The normal tabletop face remains free of branding: no wordmark appears on the e-paper, top shell, or beside the Filters and Next controls. A production enclosure may use a small blind emboss on the underside or concealed lower edge. It is optional and does not change the current prototype renders.

## How Filters works

Rows are Dark, Sexual, Heavy, Done. Filters moves the highlight; Next toggles the
selected permission or finishes. All relevant permissions must be enabled for
a question to enter automatic play. Heavy uses existing depth 3. The display
explains the temporary button functions while their cap labels stay fixed.

A successful render commits the draft/cursor or completed permission change.
A discarded press, denied refresh, or failed render leaves saved state intact.
RTC state retains the question snapshot and menu through sleep. Total power
loss resets all permissions to excluded. The glass itself keeps its last image.

## Candidate controls

Use the same sealed tactile switch under both actions so force, travel, and electrical behavior match. Different external caps create the hierarchy: the Filters cap is small and low; the Next cap is wider and flush, with a shallow concave top.

**C&K KSC321GLFS**: IP67 SPST-NO tact switch, 6.2 × 6.2 mm footprint, 3.5 mm actuator height, 2 ±0.4 N force, at least 15% tactile ratio, 0.2 mm electrical travel with +0.3/−0 mm tolerance, and 300,000-cycle rating. Each switch needs an external button cap with its own mechanical stop so enclosure loads do not crush the switch.

IP67 at the component does not make the assembled enclosure IP67. The lens, USB opening, shell seam, and both button-cap interfaces need their own paths and tests.

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

Put both switches in the same control area above the panel and carry button loads into the top shell with cap stops sized for their different caps. Support the e-paper glass continuously around its perimeter. Use four elastomer feet positioned to resist either button press.

The permission line needs tested fonts, right-to-left layout, and enough width for the longest released label.

## Electrical contract

Rev A must provide:

- ESP32-S3-WROOM-1-N16 preferred for close-range use, with WROOM-1U-N16 as the placement/RF fallback, plus the GDEY0213B74 e-paper circuit;
- one Filters wake input and one independent Next wake input;
- retained permissions with all excluded after total state loss;
- a BQ25185 charging power path set for a 500 mA input limit and 250 mA charge current;
- USB-C input and ESD protection, VBUS detection, and switched battery sensing;
- a protected 500 mAh 503035-class cell with PCM and 10 kΩ NTC, after its exact drawing closes the enclosure fit;
- e-paper power gating and a measured whole-device sleep budget below 30 µA;
- antenna keep-out clear of the display, battery, button hardware, and copper;
- hidden development pads, with no extra user-facing controls.
- concealed IO0 and EN pads for recovery from firmware that prevents USB enumeration;
- a sealed front-edge light pipe beside USB-C, with no LED on the tabletop face.

Bench results add these constraints:

- No always-on indicator. DevKitC indicators prevent a useful sleep-current measurement; rev A must expose the whole-device current without a permanent LED load.
- Use a switched 1 MΩ/470 kΩ battery divider. Its unswitched draw is about 2.9 µA, and switching removes that sleep load.
- Sense VBUS from the USB rail. Firmware uses it for the sync window, charging wake policy, and the status LED.
- Connect BQ25185 STAT1 and STAT2 to firmware on Rev A. The current bench breakout does not expose them, so the rig still infers a full cell from voltage.

QDB4 stores each question once with depth, form and content flags. Heavy permits depth 3. See [sync_protocol.md](sync_protocol.md).

## Validation sequence

### 1. Website

The website exercises the same Filters / Next menu, independent permissions, default exclusions, and draw-time variance.

### 2. USB-powered breadboard and firmware

The ESP32-S3 DevKitC, the assembled 2.13-inch e-paper module and two tactile switches. Flashing, storage, input handling, debounce, rendering, refresh policy and Wi-Fi sync all run there. The [prototype BOM](prototype_bom.md) describes the rig and [hardware wiring](hardware_wiring.md) wires it.

Verified:

- one Next press advances exactly once, and a held button counts once;
- every released English and German question fits;
- partial updates remain readable through a representative run — 193 of them, which is where the full-refresh interval comes from;
- firmware can be flashed and debugged without extra programming hardware.

### 3. Two-button interaction bench

The Filters interaction needs physical validation in addition to emulator tests:

- accepted presses move or toggle exactly once, including wake replay;
- Done retains a permitted question or selects an eligible replacement;
- discarded presses and failed refreshes do not change saved state;
- sleep retains the draft and cursor; full state loss excludes all permissions;
- the four menu rows and button hints remain readable on real glass.

The enclosure study must check whether people distinguish Filters and Next
without explanation. Earlier permission-cycle bench results do not validate this
new menu.

### 4. Battery and power-path bench

The breadboard uses a bq25185 charger board, protected cell, and both sense dividers. It confirms charging under load, USB/battery handover, battery readings within 1% of a meter, boot-time VBUS detection, the plug-in sync window, and battery-only operation.

Sleep current requires rev A because the DevKitC indicators exceed the 30 µA budget. Brownout margin and the panel's minimum reliable refresh voltage also need measurement; see the acceptance targets below.

### 5. Controls, PCB, and enclosure integration

Import manufacturer STEP models, place the tested buttons, PCB, and battery, then build the first printed enclosure.

Run table sessions without explaining the controls first. Check:

- people understand that Filters opens settings and Next draws during play;
- they can navigate all four rows and apply Done;
- allowing a permission is understood as adding it to the mixed pool;
- the e-paper permission line is readable from ordinary seats;
- the device does not slide, tip, or need to be picked up;
- the small Filters control is easy to operate deliberately;
- the larger Next control is identifiable without reading both labels;
- the adjacent controls do not cause frequent wrong presses.

If permission choice takes more attention than rejecting a poor question with Next, revise the cycling interaction before another PCB revision. EVT then covers drop, button life, cap overload, pocket lint, radio performance, and small spills.

## Acceptance targets

| Area                | Target                                                                        |
| ------------------- | ----------------------------------------------------------------------------- |
| First question      | already visible, with its permission summary                                  |
| Next question       | readable in under 1 s on a partial refresh                                    |
| Filters change      | displayed permission changes once per accepted press                          |
| State at zero power | permission summary and current question are readable                          |
| Sleep current       | below 30 µA measured at the cell                                              |
| Stability           | no slide or tip during a one-handed button press                              |
| Text                | all released questions and permission labels fit at their fixed minimum sizes |
| Ingress             | no path from a small tabletop spill to PCB or cell                            |
| Service             | cell, display, and PCB replaceable without destructive adhesive               |

## Primary references

- [C&K KSC321GLFS](https://www.ckswitches.com/products/switches/product-details/Tactile/KSC3/KSC321GLFS/)
- [Good Display GDEY0213B74](https://www.good-display.com/product/391.html)
- [Espressif ESP32-S3-WROOM-1/1U data sheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [Espressif module current measurement](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/current-consumption-measurement-modules.html)
