# Device prototype direction

This is the default direction for the staged device prototype. It is not a
finished industrial design. Firmware and electrical work still begin on the
existing USB-powered breadboard before PCB layout, enclosure integration, or
3D printing.

!!! warning "Concept renders, not manufacturing data"

    The renders communicate layout and use. They do not replace CAD, component
    drawings, tolerance analysis, antenna work, or a verified PCB.

![Warm-ivory device with one e-paper display and adjacent Category and Next buttons](assets/device-prototype/hero.webp)

## Why two buttons

The rotary selector was dropped. Its fixed labels took most of the space above
the display, froze the category count and language into the enclosure, and
added a shaft, seal, mechanical support, and an uncommon switch. A category name
renders on e-paper, which removes the need for a labelled detent.

The device has two adjacent buttons:

- **Category** advances through the six categories in a fixed order and shows
  the selected name on the e-paper;
- **Next** draws a question from the category named on the display.

The change trades a mechanically visible selection for software state. It also
makes reaching a particular category sequential: Wild can be five presses away
from New People. The physical study must measure whether that delay and the
e-paper refresh are acceptable. A knob should not return unless the two-button
study shows a concrete problem that outweighs its size and mechanical cost.

The earlier OLED and public depth-slider proposals remain rejected. Category
feedback belongs on the e-paper. Wi-Fi, language, and maintenance remain phone
or service-flow concerns. Normal playback mixes editorial depths 1 and 2;
depth 3 stays out of normal playback.

## Proposed form

The face has three elements: e-paper, Category, and Next. The two buttons sit
close together above the display, but they do not have equal visual weight.
Category is a small, nearly flush round button with its label printed on the
shell. Next is a larger, slightly raised rounded pill with a shallow concave
top and its label on the cap. Category names are printed by the display.

| Property | Study target | Reason |
|---|---:|---|
| Envelope | about 84 × 56 × 16 mm | Allows the 59.2 × 29.2 mm panel, readable button labels, and finger clearance |
| Mass | 80–100 g | Resists one-handed button presses |
| Orientation | landscape, 3° face incline | Keeps the device readable without becoming a wedge |
| Display | 2.13″ e-paper behind a matte protective lens | Keeps the category and question visible without power |
| Category | small, nearly flush round button, left | Secondary action used only when changing context |
| Next | larger, raised rounded-pill button, right | Frequent primary action; easy to find without reading both labels |
| Display label | active category in small type above the question | Avoids a printed category ring and supports future languages |
| Port | centered USB-C on the lower edge | Absent from the main tabletop view |
| Finish | warm-ivory matte shell; two charcoal buttons | Matches the paper-like website |

The size is a planning envelope. Component drawings, button-label legibility,
and the e-paper layout test may change it.

![People using the two-button prototype at a table](assets/device-prototype/tabletop.webp)

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

Start at 16 mm and earn any reduction through CAD.

| Layer | Planning allowance |
|---|---:|
| Top shell, lens, display support | 1.8–2.2 mm |
| E-paper plus compliant perimeter support | 1.0–1.5 mm |
| PCB | 1.0–1.2 mm |
| Local electronics and tactile switches | 2.5–3.5 mm |
| Protected 503035 cell | 5.0–5.5 mm |
| Bottom shell and clearance | 1.8–2.2 mm |

Put both switches in the same control area above the panel and carry button
loads into the top shell with cap stops sized for their different caps. Support
the e-paper glass continuously around its perimeter. Use four elastomer feet
positioned to resist either button press.

Moving category names onto the display removes the replaceable bezel and its
English-only geometry. It does not solve every language problem: the category
line still needs tested fonts, right-to-left layout, and enough width for the
longest released label.

![Exploded concept with two tactile buttons and no rotary control](assets/device-prototype/exploded.webp)

## Electrical contract

The later two-button rev A board must account for:

- ESP32-S3-WROOM-1-N16 and the GDEY0213B74 e-paper circuit;
- one Category wake input and one independent Next wake input;
- retained category state with a defined New People cold default;
- an integrated charging power path or a validated load-sharing circuit;
- USB-C input and ESD protection, VBUS detection, and switched battery sensing;
- a protected 503035 cell with charge current set from its data sheet;
- e-paper power gating and a measured whole-device sleep budget below 30 µA;
- antenna keep-out clear of the display, battery, button hardware, and copper;
- hidden development pads, with no extra user-facing controls.

Four of these the bench has now put numbers or constraints on:

- **No always-on indicator, anywhere.** The bench cannot measure its own sleep
  current because the DevKitC's power LED and WS2812 sit on the 3V3 rail. Rev A
  is the first board that can produce that number, and only if nothing on it
  draws continuously. That includes a power LED.
- **The battery divider wants 1 MΩ over 470 kΩ**, and switching over both. 1M/1M
  halves the standing draw to 2.1 µA and costs about 25 mV of ADC-leakage offset
  at the tap, which is nothing against thresholds 200 mV apart. Switched, it
  costs nothing at all, which is what a product should spend here.
- **VBUS detect earns its place.** It is what opens the sync window on a
  plug-in, what keeps the device awake through a charge, and what the status LED
  reads. Tap it from the USB rail, not from a wider DC input.
- **A charge-status pin would be worth having.** The bq25185 has none, so
  "charged" is inferred from voltage and is early by an amount that depends on
  load. Nothing but the LED may read it. A charger that reports termination
  removes an estimate from the design.

The question bundle still stores each question once with a six-bit deck mask
and depth metadata. Normal playback excludes depth 3. See
[sync_protocol.md](sync_protocol.md).

## Validation sequence

### 1. Website

The website exercises all six deck choices, overlapping membership, Wild tone,
the New People default, and the depth 1–2 playback cap.

### 2. USB-powered breadboard and firmware — passed

The ESP32-S3 DevKitC, the assembled 2.13-inch e-paper module and two tactile
switches. Flashing, storage, deck selection, debounce, rendering, refresh policy
and Wi-Fi sync all run there. The [prototype BOM](prototype_bom.md) describes the
rig and [hardware wiring](hardware_wiring.md) wires it.

Gate, all met:

- one Next press advances exactly once, and a held button counts once;
- every released English and German question fits;
- partial updates remain readable through a representative run — 193 of them,
  which is where the full-refresh interval comes from;
- firmware can be flashed and debugged without extra programming hardware.

### 3. Two-button interaction bench — passed

Gate, all met except the last, which needs people rather than a bench:

- Category advances exactly one deck in the fixed order and wraps once;
- the displayed category always matches the deck used by Next;
- a discarded press during refresh does not change hidden state;
- cold-state loss defaults visibly to New People;
- repeated category changes do not make the e-paper flash or ghost
  unacceptably;
- Category and Next are distinguishable without explanation.

### 4. Battery and power-path bench — passed except current

A bq25185 charger board, a protected cell and both sense dividers on the
breadboard. What it proved: charging under system load, USB and battery
handover, the device reading its own cell within 1 % of a meter, VBUS detection
at boot, the sync window opening on a plug-in, and the device staying awake for
the whole charge and running from the cell alone with no USB attached.

What it cannot prove is **sleep current**, and this is the finding that matters
for rev A: the DevKitC's own power LED and WS2812 draw one to two orders of
magnitude more than the 30 µA budget, so no figure taken on this rig bounds
anything. Brownout margin and the voltage at which a refresh actually corrupts
are also still open — see the acceptance targets below.

### 5. Controls, PCB, and enclosure integration — next

Import manufacturer STEP models, place the tested buttons, PCB, and battery,
then build the first printed enclosure.

Run table sessions without explaining the controls first. Check:

- people understand that Category cycles and Next draws;
- they can reach the intended category without losing track of the order;
- they distinguish New People from Close;
- Wild is chosen deliberately rather than mistaken for Random;
- the e-paper category line is readable from ordinary seats;
- the device does not slide, tip, or need to be picked up;
- the small Category control is still easy to operate deliberately;
- the larger Next control is identifiable without reading both labels;
- the adjacent controls do not cause frequent wrong presses.

If category choice takes more attention than rejecting a poor question with
Next, revise the cycling interaction before another PCB revision. EVT then
covers drop, button life, cap overload, pocket lint, radio performance, and
small spills.

## Acceptance targets

| Area | Target |
|---|---|
| First question | already visible, with its active category |
| Next question | readable in under 1 s on a partial refresh |
| Category change | displayed category changes once per accepted press |
| State at zero power | active category and current question are readable |
| Sleep current | below 30 µA measured at the cell |
| Stability | no slide or tip during a one-handed button press |
| Text | all released questions and category labels fit at their fixed minimum sizes |
| Ingress | no path from a small tabletop spill to PCB or cell |
| Service | cell, display, and PCB replaceable without destructive adhesive |

## Primary references

- [C&K KSC321GLFS](https://www.ckswitches.com/products/switches/product-details/Tactile/KSC3/KSC321GLFS/)
- [Good Display GDEY0213B74](https://www.good-display.com/product/391.html)
- [Espressif ESP32-S3-WROOM-1/1U data sheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [Espressif module current measurement](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/current-consumption-measurement-modules.html)

Render prompts and provenance are in
[`assets/device-prototype/prompts.txt`](assets/device-prototype/prompts.txt).
