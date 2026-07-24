# Device prototype direction

This page turns the product principles into a device that can be built and
tested. It is a proposal for the first physical prototype, not a frozen
industrial design.

!!! warning "Concept renders, not manufacturing data"

    The images below were generated to make the proposal tangible. They do not
    replace CAD, component drawings, antenna layout guidance, tolerance
    analysis, or a verified PCB placement. In particular, the exploded render
    communicates the intended layers, not their final positions or scale.

![Warm-ivory tabletop device with an e-paper question, dark utility rail, and one rotary crown](assets/device-prototype/hero.webp)

*Proposed “table pebble” form. The question remains the dominant surface; the
utility display visually disappears when it is off.*

## Honest assessment (2026-07-24)

The project has an unusually clear product principle. The question database is
the real asset, the three licenses fit the three kinds of work, and the website
and data tooling already form a useful product without waiting for hardware.
The complete test suite passes, the site and docs build cleanly, and the
firmware/hardware placeholders are candid about their status.

The device is still a hypothesis. The repository specifies its electronics and
interaction more confidently than those choices have been validated at a
table. Before custom hardware, a non-functional full-size model should answer
whether people notice it, understand the crown, can read it from a natural
seat, and pass or rotate it without instruction.

The main risks to resolve are:

1. **Text fit.** The panel has only 250 × 122 pixels. The current longest
   questions are 81 English characters and 86 German characters, but the schema
   allows 140. Every shipped question needs an automated framebuffer fit test
   at a defined minimum type size; “shrink until it fits” is not acceptable.
2. **Future scripts.** The language policy permits independent new languages,
   while the planned firmware font covers only Latin and Latin Extended.
   Either declare the device's supported scripts separately or version the
   bundle format so a language can ship a glyph-subset font.
3. **Power while charging.** The MCP73831 is a charger, not a complete
   load-sharing power path. Because sync intentionally happens while charging,
   system load must not confuse charge termination. Use an integrated
   power-path charger or validate an explicit load-sharing circuit.
4. **Radio and mechanics.** The PCB antenna needs a real keep-out at the case
   edge, clear of the battery, display stack, copper, and metal encoder. The
   crown also needs a load path into the lower chassis so presses do not flex
   the PCB.
5. **Refresh behavior.** A full e-paper refresh takes about three seconds. An
   unconditional full refresh on entering sleep would call attention back to
   the product just as the conversation starts. Perform a full refresh only
   when the ghosting threshold requires it, preferably as part of a requested
   question change or while charging.
6. **Cost and completeness.** The placeholder BOM is useful for architecture,
   not yet for pricing. It omits protection, load sharing, USB ESD, switched
   sensing, connectors, PCB/assembly, mechanical parts, and yield.

None of these invalidates the concept. They are the work between a good thesis
and a reliable object.

## Recommended form: the table pebble

| Property | Prototype target | Reason |
|---|---:|---|
| Envelope | 78 × 52 × 14 mm | Fits the 59.2 × 29.2 mm panel with usable bezels and remains pocketable |
| Mass | 75–90 g | Enough stability for a crown press without feeling like a phone |
| Orientation | landscape, 3° face incline | Keeps the question card-like and reduces the visual bulk of a wedge |
| Main display | 2.13″ e-paper, recessed behind a matte protective lens | Persistent, spill-tolerant question surface |
| Utility rail | smoked charcoal window above the e-paper | OLED disappears when off and does not compete with the question |
| Input | 13 mm low-profile rotary push crown | One obvious, tactile control reachable while the device stays put |
| Port | centered USB-C on the lower edge | Accessible but absent from the tabletop view |
| Construction | two shells, screws hidden under replaceable feet | Serviceable without a decorative field of fasteners |
| Finish | warm-ivory fine matte shell; charcoal crown and rail | Matches the site without turning the object into a branded gadget |

The envelope is deliberately provisional. A component courtyard and tolerance
stack may add a few millimetres. Do not reduce wall thickness, antenna clearance,
or display support to preserve the number.

The encoder should be a normal **relative** part with roughly 15–20 tactile
detents per revolution. Five categories are five software states, not a reason
to source an unusual five-detent-per-revolution encoder. One detent moves one
category; rotation wraps at either end.

![Three people around a table with one hand turning the device crown](assets/device-prototype/tabletop.webp)

*The use case to test: shared, readable, and operable without picking up a
personal screen.*

## Mechanical stack

A 14 mm prototype is plausible, but only with a designed stack rather than
modules placed inside a box:

| Layer | Planning allowance |
|---|---:|
| Top shell, lens, and display support | 1.8–2.2 mm |
| E-paper panel plus compliant perimeter support | 1.0–1.5 mm |
| PCB | 1.0–1.2 mm |
| Local component height | 2.5–3.2 mm |
| Protected 503035 cell | 5.0–5.5 mm |
| Bottom shell and assembly clearance | 1.8–2.2 mm |

These maxima cannot all overlap. Place tall PCB components beside the battery,
not above it. Give the e-paper glass continuous perimeter support with a thin
compliant gasket and no point loads. The encoder should key into a structural
boss or bracket connected to the lower shell. Four shallow elastomer feet
should resist both rotation and the encoder's push force.

For the first case, use an MJF/SLS PA12 or a carefully printed two-part shell
with heat-set inserts. Validate the form before spending time on cosmetic
FreeCAD surfacing. A later production design can move to PC/ABS and snap
features after drop, spill, and service tests.

![Conceptual exploded stack showing the enclosure, displays, PCB, battery, and bottom shell](assets/device-prototype/exploded.webp)

*Layering intent only. The antenna, battery, connectors, and component
courtyards must be re-laid from source drawings in CAD and KiCad.*

## Electrical direction for rev A

Keep the ESP32-S3 and display choices for the evaluation phase; they match the
software plan and are available as development hardware. Change or resolve the
following before schematic capture:

- Use a protected single-cell LiPo from a documented supplier. Set charge
  current from that cell's data sheet; start conservatively rather than assuming
  the charger's 500 mA maximum is suitable.
- Replace the bare charger block with an integrated power-path part, or add and
  validate a load-sharing circuit. Include VBUS detection so “sync while
  charging” is a real firmware condition.
- Add USB-C ESD protection and input protection. Keep the device-only 5.1 kΩ
  pull-downs on both CC pins.
- Power-gate the OLED and e-paper rails. Switch the battery-divider high side so
  it does not become a permanent sleep load.
- Budget every sleep path. The ESP32-S3 module is about 7–8 µA in the intended
  deep-sleep configuration and the selected XC6220 is about 8 µA in power-save
  mode. A measured whole-device target below 30 µA is credible, but leaves
  little room for divider leakage, pull-ups, load switches, charger leakage,
  and contaminated boards.
- Put the ESP32 module antenna at a board edge using Espressif's exact land
  pattern and placement guidance. If the display, battery, or encoder makes
  that impossible, use the external-antenna module variant and validate the
  antenna location in the final enclosure.
- Add hidden boot/reset access, UART/JTAG pads, battery-rail pads, current
  measurement links, and test pads for both display buses. These are development
  features, not extra user controls.
- Test the selected partial-update waveform across temperature and after
  repeated updates. Treat “full refresh every tenth question” as a starting
  experiment, not a product constant.

The existing 3.3 V LDO is acceptable for an evaluation board. Measure brownout
margin during Wi-Fi transmit near an empty battery before freezing it. If usable
capacity is poor, compare a 3.0 V rail or a low-quiescent buck-boost solution
against the added cost and sleep current.

## Firmware implications

The physical prototype should drive a few changes in the firmware acceptance
tests:

- Render every released question into a 250 × 122 framebuffer in CI. Fail on
  overflow, orphaned punctuation, or type below the agreed minimum.
- Store the last question and category in RTC memory where possible; avoid a
  flash write on every interaction.
- Let the first encoder movement wake the unit, then begin category changes on a
  complete stable detent. Losing the wake-up edge is better than choosing the
  wrong category.
- Keep Wi-Fi physically and logically off outside setup, manual sync, and the
  charging sync window.
- Measure wake-to-readable-question, not merely wake-to-first-log-line.
- Record partial-update count and observed ghosting during the first hardware
  study before fixing a full-refresh cadence.

## Development sequence

Bench parts for stages 1–3, with checked order links for DE/EU, are listed in
the [prototype BOM](prototype_bom.md).

### 1. Interaction model — two days

Print the three views at full size and make two weighted shells: one flat and
one with the proposed 3° incline. Use a real encoder and paper question inserts.
Run at least five small table sessions. Observe without explaining first.

**Gate:** most people can discover “turn” and “press”; the text is readable from
a normal seated position; pressing does not slide or tip the object.

### 2. Display and type rig — one week

Drive the exact e-paper panel and OLED from an ESP32-S3 development board.
Implement only font layout, partial/full refresh, encoder events, and sleep.
Export framebuffer snapshots for the entire English and German corpus.

**Gate:** every current question fits at the minimum type size; partial updates
remain acceptable through a representative interaction run; no full refresh is
needed merely because the device went idle.

### 3. Power-path proof — one week

Build the charger, power path, regulator, switched peripheral rails, VBUS sense,
and protected cell on an evaluation board or small power test PCB. Exercise
Wi-Fi sync while charging and near battery cutoff.

**Gate:** correct charge termination with system load, stable Wi-Fi peaks,
verified cutoff behavior, and a measured sleep budget with margin below 30 µA.

### 4. Rev A PCB and fit-check case — two to three weeks

Lay out the complete board from manufacturer land patterns, then import the PCB,
panel, cell, OLED, encoder, and USB connector STEP models into the case assembly.
Print the case before ordering assembled boards if connector or crown location is
still changing.

**Gate:** no courtyard collisions; verified antenna keep-out; FPC is installable
without a sharp fold; the case can be assembled and reopened without stressing
the display.

### 5. EVT — five to ten units

Test sleep current, charge temperature, radio performance on-table and in-hand,
display ghosting, 1 m drop behavior, crown life, pocket lint, and a small spill.
Keep the enclosure screwed, not glued.

**Gate:** the original interaction principle survives real hardware. Cosmetic
refinement starts only after this gate.

## Acceptance targets for the first neat prototype

| Area | Target |
|---|---|
| Time to first question | already visible when picked up |
| Next question | readable in under 1 s for partial refresh |
| Idle behavior | OLED and Wi-Fi off; no attention-catching refresh |
| Sleep current | < 30 µA measured at the cell |
| Stability | no slide or tip during one-handed turn and press |
| Text | all released questions fit at the agreed minimum size |
| Radio | reliable sync on charger in the final closed case |
| Service | cell, display, and PCB replaceable without destructive adhesive |
| Safety | protected cell, validated charge current, USB input/ESD protection |

## Primary references

- [Good Display GDEY0213B74 product data](https://www.good-display.com/product/391.html)
  — 59.2 × 29.2 mm outline, 250 × 122 pixels, approximately 0.3 s partial
  and 3 s full refresh.
- [Espressif ESP32-S3-WROOM-1/1U data sheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
  — module dimensions, low-power modes, land pattern, and antenna placement.
- [Espressif module current-measurement guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/current-consumption-measurement-modules.html)
  — a measured ESP32-S3-WROOM-1 deep-sleep example around 8 µA.
- [Microchip MCP73831/2 data sheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP73831-Family-Data-Sheet-DS20001984H.pdf)
  — charger behavior and programmed-current limits.
- [Torex XC6220 product data](https://product.torexsemi.com/en/series/xc6220)
  — regulator current, output capability, and package options.

Render provenance and the prompt set are recorded in
[`assets/device-prototype/prompts.txt`](assets/device-prototype/prompts.txt).
