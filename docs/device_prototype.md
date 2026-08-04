# Device prototype direction

This is the default direction for the staged device prototype. It is not a
finished industrial design. Firmware and electrical work still begin on the
existing USB-powered breadboard before PCB layout, enclosure integration, or
3D printing.

!!! warning "Concept renders, not manufacturing data"

    The renders communicate layout and use. They do not replace CAD, component
    drawings, tolerance analysis, antenna work, or a verified PCB.

!!! note "The breadboard still uses the DIP switch"

    The current firmware reads six one-hot category inputs from the six-way DIP
    switch and one Next button. That rig and its code remain unchanged until a
    second physical button is available. The two-button interface below is the
    target product design, not a description of the current wiring.

![Warm-ivory device with one e-paper display and adjacent Category and Next buttons](assets/device-prototype/hero.webp)

## Evaluation of the proposal

The rotary selector should go. Its fixed labels take most of the space above
the display, freeze the category count and language into the enclosure, and
add a shaft, seal, mechanical support, and uncommon switch. The firmware
already proves that a category name can be rendered on e-paper. Printing the
active category there removes the need for a labelled detent.

The device instead has two adjacent buttons:

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
close together above the display and use the same shape and travel. Their
printed labels identify the action; category names are printed by the display.

| Property | Study target | Reason |
|---|---:|---|
| Envelope | about 84 × 56 × 16 mm | Allows the 59.2 × 29.2 mm panel, readable button labels, and finger clearance |
| Mass | 80–100 g | Resists one-handed button presses |
| Orientation | landscape, 3° face incline | Keeps the device readable without becoming a wedge |
| Display | 2.13″ e-paper behind a matte protective lens | Keeps the category and question visible without power |
| Category | low-profile round button, left | Advances one category per accepted press |
| Next | matching low-profile round button, right | Draws one question; no press-duration vocabulary |
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

The eventual two-button firmware will retain the category alongside the other
RTC state and default to New People after total state loss. That behavior is
not implemented in this documentation change. The current firmware continues
to obtain the category from the DIP switch after every boot.

## Candidate controls

Use the same sealed tactile switch for both actions so force, travel, height,
and enclosure detailing match.

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
loads into the top shell with cap stops. Support the e-paper glass continuously
around its perimeter. Use four elastomer feet positioned to resist either
button press.

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

The current DevKitC overlay is deliberately different: six active-low,
one-hot DIP-switch inputs provide the category and one tactile input provides
Next. It remains the firmware contract until the second button is on the bench.
Changing that input model, retained state, channels, and tests is a later code
change.

The question bundle still stores each question once with a six-bit deck mask
and depth metadata. Normal playback excludes depth 3. See
[sync_protocol.md](sync_protocol.md).

## Validation sequence

### 1. Website

The website exercises all six deck choices, overlapping membership, Wild tone,
the New People default, and the depth 1–2 playback cap.

### 2. Current USB-powered breadboard and firmware

Keep using the ESP32-S3 DevKitC, assembled 2.13-inch e-paper module, six-way
DIP switch, and existing through-hole Next button. Do not change the firmware
input contract merely to match a render. Develop and test flashing, storage,
current deck selection, button debounce, rendering, refresh policy, and
optional Wi-Fi sync while powered from USB. The
[prototype BOM](prototype_bom.md) describes this rig.

Gate:

- every DIP-switch input maps to the intended deck;
- zero or several active inputs fail safely;
- one Next press advances exactly once;
- every released English and German question fits;
- partial updates remain readable through a representative run;
- firmware can be flashed and debugged without extra programming hardware.

### 3. Two-button interaction bench

After a second button is available, replace the DIP-switch input contract in a
separate firmware change and test the Category path before schematic capture.

Gate:

- Category advances exactly one deck in the fixed order and wraps once;
- the displayed category always matches the deck used by Next;
- a discarded press during refresh does not change hidden state;
- cold-state loss defaults visibly to New People;
- repeated category changes do not make the e-paper flash or ghost
  unacceptably;
- Category and Next are distinguishable without explanation.

### 4. Battery and power-path bench

Add a protected cell and a charger with a real system power path only after the
charge current is supported by the selected cell data sheet. Prove charging
under system load, USB/battery handover, Wi-Fi peaks, cutoff behavior, brownout
margin, and sleep current before laying out the complete board.

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
- the two adjacent buttons do not cause frequent wrong presses.

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
