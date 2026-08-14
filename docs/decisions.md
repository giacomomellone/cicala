# Decisions

Append-only decision log (ADR-lite). One `##` per decision, newest last. Decisions marked DECIDED in the handoff specification (`Tischkarte — Project handoff specification.md` at the repo root) are not repeated here; this file records choices the spec left OPEN, plus every dependency added.

## 2026-07-24: Repo bootstrapped from the handoff specification

Phase A (database + tools), Phase B (website), and structure-only stubs for firmware/hardware created in one pass. Git history starts here; the spec document itself is kept at the repo root as the founding reference.

## 2026-07-24: Placeholder org and domain

GitHub org/repo is assumed as `tischkarte/tischkarte` and the site domain as `tischkarte.pages.dev` until the real ones exist. Both live in exactly one place each (`website/src/config.ts`; `$id` in `questions/schema.json`; `--base-url` default in `tools/build_bundle.py`; CODEOWNERS handles in `.github/CODEOWNERS` and maintainer names in `docs/languages.md`), so the rename is a one-commit operation as the spec requires.

## 2026-07-24: Tests use stdlib unittest, not pytest

The tools dependency policy is "stdlib + pyyaml + jsonschema only"; adding pytest for tests would be the first violation of the policy in the same commit that introduces it. `python -m unittest discover tools/tests` needs nothing extra.

## 2026-07-24: Ed25519 signing via system openssl, not a Python package

`build_bundle.py`/`keygen.py` shell out to `openssl` (≥ 1.1.1, present on GitHub runners and dev machines) instead of adding `cryptography` or `pynacl`. Keeps the tools dependency policy intact; the signature is standard ed25519 over the bundle's raw SHA-256 digest, so the firmware side can use any conforming verifier. Unsigned dev builds are allowed (`"sig": null`) and must be rejected by release firmware.

## 2026-07-24: Denylist semantics: whole-word match on normalized text

Terms (including multi-word phrases) match case-insensitively as whole words against the NFC-lowercased, whitespace-collapsed text. Substring matching was rejected for Scunthorpe-problem false positives. A hit is a CI failure that asks for manual maintainer review, and no cleverness beyond that, per spec.

## 2026-07-24: `--fix` rewrites only files that validate cleanly

`validate.py --fix` never rewrites a file that produced errors, so a broken submission can't cause data-destroying reformats. Rewritten files carry a fixed 3-line header comment; hand-written comments in question files are not preserved (the files are tool-managed).

## 2026-07-24: Browse "newest" sort uses reverse file order

`added` is stripped from the main payloads (spec §5), so the browse page sorts "newest" by reverse position within each category file. Files are append-only, so file order _is_ chronological order. `recent.{lang}.json` (which keeps `added`) stays the source for the contribute page's recent list.

## 2026-07-24: Website shows the device's display id

The meta line's `#<id-short>` uses the same formula as the OLED (`decimal of first 2 id-hash bytes mod 10000`, e.g. `#6666`), so a question has one human-visible number across site and device. Collisions are cosmetic and accepted, same as on the device.

## 2026-07-24: Permalinks switch the active language

Visiting `/q/<id>` sets the active language (`tk.lang`) to that question's language. A shared German link therefore lands in a fully German experience, and "next question" continues in German. The alternative (keeping the visitor's old language) would show a German question inside English chrome and then jump languages on the first keypress.

## 2026-07-24: Generated site data is not committed

`website/src/data/*.json` is gitignored; `tools/build_site_data.py` regenerates it locally and in CI before every build. The question YAML stays the single source of truth, and PR diffs never contain generated blobs.

## 2026-07-24: npm dependencies (website)

Per the guardrail, every package with a one-line justification:

- `astro`: the decided framework (spec §7.1).
- `@fontsource/literata`, `@fontsource/ibm-plex-mono`: the decided self-hosted fonts (spec §7.1); fontsource is the standard way to self-host with subset files.
- `typescript` (dev): required for `.ts` islands and editor support; ships no runtime bytes.

## 2026-07-24: firmware.yml gates on the project existing

The firmware CI workflow checks for `firmware/CMakeLists.txt` and exits green with a notice while only the structure stubs exist (this build), so CI isn't red for months. The `idf.py build` path activates automatically once the phase-C skeleton lands.

## 2026-07-24: GitHub issue form uses a multi-select dropdown for tags

GitHub issue forms cannot URL-prefill checkbox groups, and the contribute page must prefill everything. Tags are therefore a `dropdown` with `multiple: true` (prefillable); the CC0 confirmation stays a required checkbox the contributor must tick on GitHub, which is exactly the explicit consent we want anyway.

## 2026-07-24: Doc filenames are snake_case; just is the repo entry point

`docs/` files renamed from SCREAMING_SNAKE_CASE to snake_case (`design.md`, `decisions.md`, `languages.md`, `sync_protocol.md`) per maintainer preference; root-level `README.md`/`CONTRIBUTING.md`/`CODE_OF_CONDUCT.md` keep their conventional names because GitHub's UI treats them specially. A `justfile` is the single entry point: typing `just` lists every command (database, website, docs, tests, firmware); recipes call the `.venv` tools directly so nothing needs to be on PATH except `just`, `python3` and `npm`.

## 2026-07-24: Docs are an MkDocs site (mkdocs-material)

`docs/` doubles as an MkDocs site (`mkdocs.yml`, strict mode, output gitignored at `site/`). Dependency justification: `mkdocs-material`, the de-facto standard MkDocs distribution. It is configured with `font: false` so the docs load no third-party resources, matching the website's ethos. Installed via the same `.venv` as the tools (`requirements.txt`).

## 2026-07-24: Website tests: vitest + happy-dom

Dependency justifications (dev-only, zero runtime bytes): `vitest` is the Vite-native runner and shares Astro's transform pipeline, so TS test files just work; `happy-dom` provides a lightweight DOM (localStorage, events) for testing `store.ts` and row rendering without a browser download. To make the core logic testable, the shuffle bag moved to `lib/bag.ts` and the question-text rules to `lib/rules.ts` as pure modules; `play.ts`/`contribute.ts` are thin DOM wrappers over them. CI runs `npm test` before every site build.

## 2026-07-26: Delete the OLED

The device has one display: e-paper, showing only the question. An absolute
selector makes category preview redundant; follow-up nudges belong in question
text or human listening; service setup belongs on a phone. This deletes the
OLED, its window, driver, rail, load switch, and status vocabulary.

Accepted cost: battery, sync, and language state have no tabletop display.
Setup must be discoverable through packaging and the USB-plus-Next service
gesture. This supersedes the 2026-07-24 decision to show a device display ID on
the website; human-visible question numbers are removed from both products.

## 2026-07-26: Use a six-position absolute selector

The physical selector order is `new_people`, `close`, `family`, `work`, `here`,
`wild`. Position is readable at zero power, a turn cannot silently wrap, and
the selected deck is read from contacts on every wake. A separate button owns
Next; turning to a stable detent draws immediately.

Accepted cost: a compact absolute switch is less common and more expensive
than an EC11 encoder, uses six GPIOs in the simple circuit, and freezes the
position count. Alps Alpine SRBV160803 is only the study candidate because it
is not ingress rated. The legend and taxonomy must pass a physical model before
custom electronics.

## 2026-07-26: Store questions once with overlapping deck eligibility

The corpus is one `questions.yaml` per language. Each question has one or more
eligible decks rather than an owning category. New People assumes no shared
history; Close assumes familiarity; Family and Work apply their relationship
constraints; Here supplies a shared third object.

Wild is a deliberate exception to the rule that labels name a relationship or
place. It communicates an opt-in to dark, spicy, macabre, or absurd tone.
`random` describes sampling and `anything` hides the tone change. Dark and
spicy questions are Wild-only.

Accepted cost: deck membership becomes editorial judgment and can change
without changing a question ID. Coverage totals count eligibility and therefore
sum to more than the number of stored questions.

## 2026-07-26: Keep depth editorial; defer the physical control

Depth 1–3 measures exposure cost and remains independent of tone. Normal
website and device playback serves depths 1 and 2; depth 3 remains in browse
and the corpus. There is no depth slider in the first physical prototype.

The proposed slider could make a boundary cheap to express, but a visible
“light” position can also signal rejection on a date or at work. It returns
only if an unexplained table study shows people moving it publicly and
unprompted.

Accepted cost: the table cannot request depth 3 from the normal player and
cannot set a precise exposure ceiling. Next is the only rejection mechanism.

## 2026-07-26: Reject ramping and hidden session state

Selection is a pure filter of deck eligibility and the playback depth cap.
Questions do not escalate with presses, time, RTC gaps, or a guessed session
boundary. The device cannot observe conversational readiness.

Accepted cost: a sequence has no designed dramatic arc. Corpus quality and the
people at the table must create progression.

## 2026-07-26: New People is the player default

The website opens on New People. There is no depth-control default: including
depths 1 and 2 in every normal deck replaces the earlier proposal to default a
three-position control to its middle setting.

Accepted cost: returning users must turn or click back to a preferred deck
once on a new browser. The website persists their later selection locally; the
physical device always uses its visible selector.

## 2026-07-26: Long press is Next, not Favorite

Every Next press duration draws another question. Physical favorites would
need confirmation, recovery, and a way to enter a saved collection, creating
hidden state or another mode. Website favorites remain because the browser can
show ownership and feedback.

Accepted cost: a device user cannot save a question on the object. They can
continue talking, take a photo, or find questions later on the website.

## 2026-07-26: The first physical bezel is English

The model uses complete English words and center ticks. German questions remain
in the corpus as the first multilingual content test, but the project will not
invent German deck labels or supposedly universal icons before the English
interaction passes.

Accepted cost: the first model is not a multilingual industrial design.
Replaceable bezel artwork adds a part and future scripts may need a larger
legend or a different layout.

## 2026-07-26: QDB2 stores one question with a deck mask

Device bundles store each question once with a six-bit eligibility mask,
editorial depth, and dark/spicy flags. The physical device no longer needs
repository IDs or OLED display numbers. Manifest schema and bundle magic both
advance to version 2.

Accepted cost: no backward compatibility with the unshipped QDB1 format. This
is intentional while firmware is still a stub.

## 2026-08-02: Firmware framework is Zephyr

Zephyr rather than ESP-IDF. It has an in-tree SSD16xx driver with a
`waveshare_epaper_gdey0213b74` configuration for the exact planned panel, zbus
for decoupled message passing, ztest and twister for host-side tests, and one
devicetree description that moves from the DevKitC to the rev A board by
changing an overlay rather than editing code.

The workspace uses west T2 topology: `firmware/west.yml` is the manifest,
`west init -l firmware` makes the repo root the topdir, and zephyr plus its
modules land in a gitignored `deps/`. The monorepo therefore stays one
checkout, and nothing from upstream is committed.

Pinned at Zephyr v4.4.1 with Zephyr SDK 1.0.1, which `deps/zephyr/SDK_VERSION`
requires exactly. The SDK moved from 0.x to 1.0 in March 2026 and its release
assets were renamed, so an older install or a guessed download URL will not
work. The board target `esp32s3_devkitc/esp32s3/procpu` is verified against the
v4.4.1 board definition.

Accepted cost: `just fw-build`/`fw-flash` and `.github/workflows/firmware.yml`
were written for ESP-IDF. The justfile is rewritten here; the workflow is a
separate pass. Bumping Zephyr means checking `SDK_VERSION` and possibly
installing a matching SDK, so it stays a deliberate act.

## 2026-08-02: C++17 for application logic, C for Zephyr glue

`Fsm`, `qdb`, the bag and text layout are C++17. Files that use
`ZBUS_CHAN_DEFINE`, `ZBUS_MSG_INIT`, ISR registration and devicetree glue stay
C. The reason is mechanical rather than stylistic: several of those macros
expand to out-of-order designated initializers, which C accepts and C++17
rejects. Keeping them in `.c` means upstream samples paste in unmodified.

Exceptions and RTTI stay disabled and nothing allocates, so the C++ surface
costs no binary size beyond vtables. Objects are constructed inside `main()`
rather than as globals, because a static constructor runs before device
drivers are ready.

Accepted cost: two languages in one component directory, and a rule that has
to be explained to every contributor.

## 2026-08-02: Deep sleep is a reboot, so retained state lives in RTC memory

Below 30 µA on the ESP32-S3 means deep sleep, which does not preserve SRAM or
thread stacks. Every wake runs the bootloader and `main()` from the top. The
shuffle bag, the recent ring, the current question, the partial-refresh counter
and the bundle fingerprint therefore live in RTC slow memory — about 512 bytes
against an 8 KB budget. NVS holds only slow-moving configuration.

This settles the flash-wear question in the bag's favour: Next costs no flash
write, so button life rather than flash endurance bounds the device.

Accepted cost: the bag resets when the cell is removed or goes flat, and "next
question in under 1 s" becomes a boot-time budget covering ROM boot, Zephyr
init, selector read, draw and panel refresh. It has to be measured on the
breadboard rather than assumed.

## 2026-08-02: A Next press during a refresh is dropped, not queued

A panel refresh blocks for 0.3–2 s. Queueing presses that arrive during one
would replay them afterwards as a burst of draws nobody saw, consuming
questions from the bag invisibly. `app` stays awake during the refresh and
discards the press instead. One press produces one question and one refresh.

Accepted cost: a press during a refresh does nothing at all, with no
acknowledgement, because the device has no status surface to acknowledge it on.

## 2026-08-02: A cold boot draws immediately

A device with a valid selector and no retained question draws and does a full
refresh, because panel content is unknown after a cold boot. A device out of
the box shows a question without being touched.

With an invalid selector — zero or several contacts — nothing is drawn and
nothing is guessed. On a cold boot that means the panel stays blank until one
contact settles.

Accepted cost: packaging has to account for the first question being visible
before anyone interacts with the device.

## 2026-08-02: The breadboard stage does not sleep

`CONFIG_PM` stays off until input, display and storage are correct. A board
that resets on every press is harder to bring up than one that stays awake.
Accepted cost: the sleep path, which is where the boot-latency budget and the
30 µA target are decided, is the last thing to be exercised rather than the
first.

_Corrected 2026-08-05._ This entry originally claimed retained state had lived
in its RTC sections since the first commit, so that enabling deep sleep would
be a configuration change rather than a rewrite. That was not true: `Bag::State`
was an ordinary `.bss` object, and `app_logic.cpp` said so. The claim made the
remaining work look smaller than it was. See the 2026-08-05 entry on retained
memory.

## 2026-08-02: Host tests default to qemu_xtensa/dc233c

`native_sim` is faster and is the only host platform that emulates GPIO, but it
builds on Linux only, and development happens on macOS. `qemu_xtensa/dc233c`
runs a full Zephyr kernel on macOS and matches the target architecture, so it
is the default for `just fw-test`. CI overrides it with
`just simboard=native_sim`.

Board targets need their SoC qualifier under hardware model v2: plain
`qemu_xtensa` is a board name, not a target, and twister rejects it. The
dc233c core configuration comes from the `hal_xtensa` module, which the
manifest's module allowlist has to include.

Suites that drive the selector and Next need emulated GPIO and are kept in
their own directory, so a local run that cannot execute them is visible rather
than silently green.

Accepted cost: two host platforms to keep working, and the GPIO-driven suites
do not run on a developer machine without Docker.

## 2026-08-02: clang-format for firmware, scoped to repository files

Style is LLVM with Linux brace placement, four-space indent and a 100-column
limit, which matches the existing `Fsm` sources. `SortIncludes` is off because
Zephyr headers have ordering requirements. Hand-aligned transition tables are
wrapped in `// clang-format off`.

Three layers keep it away from Zephyr: `just fmt-fw` drives off `git ls-files`
so it can only reach tracked files; `deps/.clang-format` sets
`DisableFormat: true` and is the closest config for anything in the workspace;
and Zephyr ships its own config. `.clang-format-ignore` was not used because it
needs clang-format 18 and the system binary is 17.

Dependency justifications: `ruff` (dev-only, formats and lints `tools/`; the
stdlib-plus-pyyaml-plus-jsonschema runtime policy is unaffected) and
`prettier` with `prettier-plugin-astro` (dev-only, formats `website/` and
markdown). `.prettierignore` excludes `questions/`, whose formatting
`tools/validate.py --fix` owns.

## 2026-08-04: A bespoke binary bundle rather than a standard container

The question bundle stays the flat QDB2 binary in `sync_protocol.md` instead of
CBOR, MessagePack, protobuf or SQLite. Reviewed when the name turned out to be
undocumented and a standard format was floated as an alternative.

The format has exactly one writer (`tools/build_bundle.py`) and one reader
(`firmware/lib/qdb/`), both in this repo, and no third party ever parses it.
That removes interoperability — the usual reason to pick a standard — from the
argument, and leaves cost. A standard container needs a parser on the device:
zcbor or nanopb is a new module in `west.yml` and flash spent on generality the
device does not use. The current reader is about 120 lines, allocates nothing,
and hands out questions as pointers into the mapped bundle. SQLite for 240
read-only records on an MCU is not a serious option.

The signing pipeline also prefers a plain byte range: ed25519 over the raw
SHA-256 of the file, with no canonicalisation question to get wrong.

Accepted cost: no off-the-shelf tooling can open a bundle, so the format is
only as debuggable as `parse_bundle()` makes it, and the two decoders have to
be changed together. The firmware suites decode real bundles the writer emits,
which is what catches a one-sided change.

Naming: QDB is the Tischkarte Bundle and the trailing digit is the format
version. Nothing had recorded that, which is what prompted this entry. The name
is internal — no release has ever published a bundle — so renaming it remains a
mechanical change across about fifteen files if a better one turns up.

## 2026-08-04: Replace the rotary selector with a Category button

The default enclosure has two adjacent buttons, Category and Next. Category
cycles through `new_people`, `close`, `family`, `work`, `here`, and `wild`; the
e-paper names the active category. Next draws from the category shown. This
supersedes the 2026-07-26 six-position selector and English-bezel decisions for
the target product. It also supersedes the question-only e-paper rule from the
OLED deletion decision: the OLED stays deleted, while the active category now
shares the e-paper with the question.

Removing the knob deletes the shaft opening, labelled arc, uncommon SP6T part,
rotational load, and selector-specific ingress path. Category names move into
the existing language and font system. The question screen keeps the active
category visible so category state remains readable without power.

Accepted cost: category choice becomes sequential and software-retained rather
than mechanically absolute. Reaching a category may take five presses, and
each visible update costs an e-paper refresh. The two-button interaction must
pass a physical study before schematic capture.

The current USB breadboard and firmware are not changed by this decision. They
continue to use the six-way DIP switch as six one-hot category inputs and one
Next button until a second button is available. The input mapping, retained
category state, state machine, and tests change together in a later commit.

## 2026-08-05: Give Next physical priority over Category

The two controls are adjacent but intentionally unequal. Category uses a small,
nearly flush round cap with its label printed on the shell. Next uses a larger,
slightly raised rounded-pill cap with a shallow concave top and its label on the
cap. Both sit over the same tactile-switch part.

Next is the repeated conversational action; Category is changed occasionally.
Size, height, and shape communicate that frequency without adding an accent
color, icon, light, or different switch behavior.

Accepted cost: the two cap geometries need separate tooling and overload stops.
The physical model must confirm that Category remains easy to press
deliberately and that the larger Next cap does not dominate the face or cause
accidental presses.

## 2026-08-05: VBUS detect takes GPIO21, battery sense GPIO1

The two power inputs are assigned before the circuit that reads them exists.
Deep sleep needs every wake input inside GPIO0–21, and by the time the panel,
the six selector contacts, Next and the bring-up LED are placed, that range has
three pins left: GPIO1, GPIO14 and GPIO21.

Battery sense picks first because it has the tighter constraint. It has to be
sampled during a sync, when the radio is up, and the ESP32-S3's ADC2 shares
hardware with Wi-Fi — readings taken then can fail. That restricts it to ADC1,
GPIO1 to GPIO10, of which everything but GPIO1 is already spoken for or a
strapping pin. VBUS detect is an ordinary digital input, so it takes GPIO21 and
leaves the last ADC1 channel to the measurement that has nowhere else to go.

Neither pin gets a devicetree node yet, because nothing reads them and an
unused node rots. The assignment lives in the overlay header and the wiring
doc.

Accepted cost: one free RTC-capable pin remains, GPIO14, so anything else
needing a wake input competes with a second analogue measurement for it. On the
target PCB the bring-up LED goes away and GPIO2 returns to ADC1, which is the
slack if battery sense turns out to need a companion.

## 2026-08-05: Deep sleep is spiked before storage, on a branch

The 2026-08-02 entry above put `CONFIG_PM` behind input, display _and_ storage
being correct. Storage has not been started, so by that ordering the sleep path
waits for LittleFS, NVS and sync. It is being spiked now instead, ahead of all
three.

The reason is the cost that entry accepted: the sleep path is where the
boot-latency budget and the wake mechanism are decided, and it was scheduled
last. The first bench run turned two of those from theory into arithmetic. A
partial refresh takes 622 ms of a 1 s budget, leaving roughly 380 ms for ROM
boot, Zephyr init, selector read and draw. And the six selector contacts cannot
be armed as EXT1 wake sources the way the architecture describes: a deck is
selected by _holding_ one contact closed, so that pin is low for as long as the
device sits on the table, and a device armed to wake on it never sleeps. The
mask has to be computed at sleep time from the current selector position.

Both are the kind of finding that changes a design rather than a line, and
finding them after sync and the portal are written means rewriting sync and the
portal.

What the original entry protects is the everyday build: a board that reboots on
every press is miserable to bring up. That is preserved. `CONFIG_PM` stays off
in `prj.conf`, so `just fw-build` still produces a board that stays awake; the
spike proves the mechanism on a branch and merges the parts that stand on their
own. Retained memory is the first of those, and it is worth having before deep
sleep exists: without it every boot redraws a question the panel is already
showing, which now has a measured price of 2315 ms.

Accepted cost: two config paths to keep working until the spike lands for real,
and a decision entry that contradicts the ordering of an earlier one rather
than replacing it.

## 2026-08-05: One Category button replaces the six selector inputs

The firmware now reads two buttons. Category advances the deck one step and
wraps from Wild back to New People; Next asks for a question. The six one-hot
selector contacts, their 600 ms settle window and the invalid-combination
handling are gone.

`device_prototype.md` has specified Category and Next since the rotary selector
was dropped from the product; the six-input path was the bench rig catching up,
and it was waiting on a second physical button. There now is one.

What changes structurally is where the deck lives. A rotary selector holds its
own state — the knob position _is_ the deck, readable at zero power, which is
why the deck had no storage anywhere. A button has no position, so the active
deck moved into the retained block in RTC memory, and a cold boot starts on New
People.

It also simplifies deep sleep. Six contacts with one permanently closed could
not be armed as EXT1 wake sources without the closed one waking the device
immediately; two buttons are both open at rest, so both are normally in the
mask. The live reading stays, because a button held down at the moment of sleep
has the same problem in miniature.

Freed: GPIO 5, 6, 7, 15 and 16.

Accepted cost: the deck is no longer readable from the device when it is off.
Someone returning to a device showing a question cannot tell which deck it came
from without pressing Category, which changes it. The panel shows the name on
every advance, which is the mitigation the product design already chose.

## 2026-08-05: Zephyr is patched locally, through `west patch`

`ssd16xx` clears both controller RAM buffers, pulses the hardware reset and drives a full update from its init function, with no way to opt out. A deep-sleep wake on the ESP32-S3 is a reset, so all three ran on every press: the panel spent 2314 ms going white before the application's own refresh began, and the retained refresh counter described a panel state that no longer existed.

`CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT` skips all three. Measured on the bench: the refresh after a wake fell from 2919 ms to 624 ms, and the white flash is gone.

The alternatives were worse. Accepting the wipe meant a 2.3 s flash on every press and a retained counter that measured nothing. Restoring the controller RAM from a copy in RTC memory would have cost 3.8 KB and needed a second patch anyway, since upstream exposes `ssd16xx_read_ram()` but no write. Forcing a full refresh on every wake was correct and slow.

`deps/` is gitignored, so the change lives in `firmware/patches/` and is applied by `west patch`; see [patching zephyr](firmware_patches.md). It should go upstream — the case is general, not ours: any e-paper device whose wake is a reset and whose controller keeps power has it.

Accepted cost: a divergence from the pinned Zephyr that has to be re-checked at every version bump, and a build that is wrong in a way Kconfig only warns about if someone skips `just fw-patch`. `app/src/sleep.c` turns that warning into an `#error`.

## 2026-08-05: The wake press is replayed from the wake mask

EXT1 sees the press that ends a sleep before the kernel exists, and by the time the gpio-keys driver is listening the button is already down. `app/src/input.c` refuses to invent a press out of the bare release that follows — a rule worth keeping, since it is also what makes a button held at boot count once rather than twice.

So the press was reaching nobody. On hardware that looked like a device that ignored the first press of every conversation: eleven wakes in one capture, zero renders.

The wake mask is the record of that press, and the only one there is. `app/src/sleep.c` latches `esp_sleep_get_ext1_wakeup_status()` at `PRE_KERNEL_1`; `tk_app_init()` replays it — Category advances the deck before it is announced, Next is posted to the state machine, which gained a `BOOT --REDRAW--> DRAWING` transition to answer it.

That transition is checked before the retained-panel check, not after: waking by Next onto a panel that already holds a question is the ordinary case, and `RETAINED` would decide the panel was already correct and draw nothing — which is exactly the press the user just made. Announcing the deck first instead would be worse than useless, because `REFRESHING` drops presses made during a refresh, so the second press would be swallowed too.

## 2026-08-05: The panel's control lines are held through deep sleep

Deep sleep isolates every GPIO. The panel's RESET line is active low and the controller keeps its own power, so a line left floating for the length of a sleep can drift low and reset the controller — taking with it the RAM that the `ssd16xx` patch exists to preserve. CS floating is the same argument once removed: selected by accident, noise on the clock is a command.

RESET, CS and D/C are driven to their inactive level and held with `rtc_gpio_hold_en()` before `sys_poweroff()`, and released at `PRE_KERNEL_2` alongside the button pads. All three are RTC-capable (GPIO 8, 10, 18), which is what makes it possible at all.

Ghosting after a wake was fixed by this and the reset-pulse skip together, in one flash. Which of the two was load-bearing is not known; separating them means a run on the `fw-retain` image, which reboots without deep sleep.

## 2026-08-06: The panel controller is asked to sleep, behind a switch that is off

`sys_poweroff()` stops the SoC and nothing else. The SSD1680 is a separate chip on its own rail, and Zephyr's `ssd16xx` never issues command `0x10`: `SSD16XX_CMD_SLEEP_MODE` is defined in `ssd16xx_regs.h` and used nowhere in the driver. So through every deep sleep the controller has been sitting in whatever state the last refresh left it in.

`CONFIG_PM_DEVICE` does not cover this, despite `sleep.conf` having claimed it did. `lib/os/poweroff.c` locks interrupts and calls `z_sys_poweroff()` without touching device PM, and `ssd16xx` defines no PM action for it to call in any case. The comment has been corrected; the symbol stays for `PM_STATE` handling.

`CONFIG_TK_PANEL_DEEP_SLEEP` sends the controller into deep sleep mode 1 — RAM retained — from `sleep_now()`, before the control pins are parked, since holding CS would take the bus away mid-command. Mode 2 drops the RAM, which is the image on the glass.

Waking it needs the hardware reset the `ssd16xx` patch removed: a controller in deep sleep ignores SPI, and RES# is the only way back. The patch therefore grew a second symbol, `CONFIG_SSD16XX_PRESERVE_IMAGE_HW_RESET`, which restores the pulse while still skipping the clear and the update. `TK_PANEL_DEEP_SLEEP` selects it.

Off by default, because both halves of the trade are unmeasured. What it saves is unknown: a meter with 0.1 mA steps read zero across the panel's VCC in deep sleep, which bounds the draw under roughly 50 µA and rules out a controller that is fully awake, but does not distinguish 3 µA from 45 µA against a 30 µA whole-device budget. What it costs is also unknown: the patch's own comment holds that a hardware reset returns the SSD1680's RAM to defaults, and if that is right then every wake falls back to a 2315 ms full refresh and this is not worth having.

The two are worth settling together, on the power mule rather than the DevKitC — the devkit's USB bridge, regulator and LED swamp any sub-milliamp figure taken at the board level.

Splitting the symbol also settles the older question above: the reset skip and the clear skip arrived in one flash and were never told apart. `CONFIG_SSD16XX_PRESERVE_IMAGE_HW_RESET=y` with `TK_PANEL_DEEP_SLEEP=n` is the reset skip alone, which is the isolation that run needed.

Accepted cost: a third Kconfig combination that nothing on the bench has yet run, and a patch that now carries two symbols into every Zephyr version bump instead of one.

## 2026-08-06: The portal is entered by both buttons at boot, not by USB-plus-Next

[design.md](design.md) and [sync_protocol.md](sync_protocol.md) both specify "connect USB while holding Next" as the service gesture. That needs VBUS detect, which [hardware_wiring.md](hardware_wiring.md) reserves on GPIO21 and which is neither wired on the rig nor present in the devicetree — the pins were reserved so the deep-sleep work would not find the RTC-capable range full, and nothing reads them.

So the shipped gesture is both buttons held through a boot, confirmed for `CONFIG_TK_PORTAL_ENTRY_HOLD_MS` rather than sampled once. It needs no hardware that does not exist, it cannot happen by accident, and it survives as a fallback once VBUS lands — a device whose power path has failed can still be serviced.

The buttons are read from the pins rather than through the input layer, which cannot answer the question: `input.c` publishes a press on release and deliberately ignores a release with no press behind it, so a button already down when the device starts produces no event at all.

Accepted cost: two documents now describe a gesture the firmware does not implement, and they say so rather than being quietly corrected. When VBUS is wired, USB-plus-Next becomes the primary trigger and this stays as the fallback.

## 2026-08-06: The setup access point is open

A WPA2 SoftAP needs a passphrase, and the only places to put one are the e-paper and the case. Printing it on the case makes it a shared secret across every unit or a per-unit label to manage; putting it on the panel means it is readable by anyone who can already see the device, which is the same population that can reach the radio.

Open, plus a gesture that needs two hands on the device, plus a window that closes on its own after `CONFIG_TK_PORTAL_WINDOW_MS`, is the posture most consumer setup flows take. What is exposed during that window is the ability to set the Wi-Fi credentials and the language of a question deck. The status page names the saved network and never renders its password back.

The window is a hard cap rather than an idle timer, deliberately: an idle timer can be held open indefinitely by any phone that keeps probing, and an open access point that never closes is a worse thing to leave on a table than one that ends a setup session early.

## 2026-08-06: The portal's card is a state of the tabletop machine

The architecture said "service entry is not a state", on the grounds that the tabletop face stays a question display. It still does, and it is a state anyway, for a reason that is mechanical rather than aesthetic.

`app_logic` owns the sequence number every card is stamped with, and `tk_app_post_render()` discards any render result whose `seq` is not the one last published. That guard is what fixed the bug where one press appeared to do nothing and the next showed two questions. A card published straight from the `net` thread would allocate a sequence number behind `app_logic`'s back, so the guard would start dropping real renders instead of stale ones.

The portal therefore publishes on `chan_service`, `app` forwards it, and `AppFsm` gains a `SERVICE` state alongside `CATEGORY`. `app` stays the only thread that decides anything and the only publisher of `chan_question`, and the card gets the same refresh accounting as every other card.

One asymmetry falls out and is worth naming: a press arriving mid-refresh is dropped, and a service card arriving mid-refresh waits. A press is asking for a question nobody has read yet; a service card is somebody standing at the device waiting to be told which network to join.

## 2026-08-06: west.yml gains tf-psa-crypto and mldsa-native

The Wi-Fi driver selects `MBEDTLS`, and mbedtls 4 keeps PSA crypto in a separate repository that Zephyr fetches as its own west project rather than as a submodule. With the module allowlist as it was, CMake failed at `TF-PSA-Crypto target tfpsacrypto does not exist` before compiling anything. `mldsa-native` is referenced by the same CMake.

This is the widening the manifest comment anticipated — "widen if a build fails on a missing module rather than importing all ~60" — and it costs a longer `just fw-init`.

## 2026-08-06: Wi-Fi ships in the everyday image; the measurement images turn it off

It first landed as a build variant, `just fw-net`, on the reasoning that `CONFIG_TK_SLEEP` is off by default. The measured cost then made that look like the wrong axis to split on:

|             | without | with   | delta                           |
| ----------- | ------- | ------ | ------------------------------- |
| flash image | 238 KB  | 676 KB | +438 KB, against a 1344 KB slot |
| dram0_0_seg | 21.1%   | 60.6%  | +154 KB                         |
| iram0_0_seg | 13.8%   | 18.4%  | +18 KB                          |
| threads     | 6       | 14     | +8                              |

Both fit with room to spare, and a device whose Wi-Fi cannot be configured without a special build is not the device being built. So `just fw-build` carries the radio.

What the original reasoning was actually protecting is narrower than "the everyday image": it is the images that _measure_ something. `soak.conf` and `sleep.conf` therefore set `CONFIG_TK_NET=n`, `CONFIG_WIFI=n` and `CONFIG_NETWORKING=n`. A partial refresh has already been seen at 2769 ms during a portal session against the 622 ms this rig recorded without one, and until that is explained a ghosting run must not have a radio in it.

The configuration lives in `app/boards/esp32s3_devkitc_esp32s3_procpu.conf` rather than `prj.conf`. `prj.conf` is shared with qemu and native_sim, `CONFIG_WIFI_ESP32` needs a devicetree node only this SoC has, and putting it there breaks `just fw-sim`. The second Wi-Fi node AP+STA needs is in the matching board overlay for the same reason.

A portal image remains: the same image with `CONFIG_TK_DEBUG_PORTAL=y`, which skips the two-button gesture. The gesture needs two hands on the board at the moment it boots, which makes everything behind it awkward to work on.

## 2026-08-06: The qemu overlay had not followed the Category button

`just fw-sim` had been failing since the selector was replaced: `app/boards/qemu_xtensa_dc233c.overlay` still described six selector inputs and defined no `tk-category` alias, which `src/input.c` requires. The suites did not catch it because `tests/input` carries its own overlay, and nothing else builds `firmware/app` for qemu.

Fixed alongside the portal work rather than separately, because `src/net.c` reads the same two aliases to detect the service gesture. The overlay now describes the two buttons the target has.

## 2026-08-06: Deep sleep ships in the everyday image, and a wake does not join a network

Sleep and Wi-Fi were separate build variants, and a device needs both at once. `CONFIG_TK_SLEEP` therefore joins `CONFIG_TK_NET` in the devkit board conf, `sleep.conf` and `sleep.overlay` are gone, and `just fw-sleep` with them.

Not in `prj.conf`, for the reason the radio is not: that file is shared with qemu and native_sim, and `src/sleep.c` is written against the Espressif RTC and sleep APIs. The light-sleep state that has to be disabled moves into the board overlay alongside the second Wi-Fi node.

The two debug images cannot sleep at all now — `TK_SLEEP` gained `depends on !TK_DEBUG_SOAK && !TK_DEBUG_CHARSET`. Both keep a count in ordinary memory across presses, a soak run of its own presses and the charset image of which test page it is on, and a wake is a reboot that loses both. Making it structurally unavailable is better than a conf line somebody can forget: the failure is a confusing run rather than an error. `debug.conf` still turns it off by hand, because there is no symbol to depend on and a debugger loses its thread every two seconds otherwise.

**A wake does not join a network.** Deep sleep makes every press a fresh boot, so connecting on one would put a radio association in front of every question the device ever answers, for a connection nothing yet uses. `net.c` skips it when `tk_wake_button()` reports an EXT1 wake.

That leaves a cold boot as the only automatic trigger, and once the device sleeps a cold boot is rare — first power-up, the reset pin, or a flat cell. This is deliberate rather than a gap: the manual path already exists, since the service gesture raises the portal and the portal joins the saved network as the last step of its flow. The trigger the design actually wants is USB power plus a known network, and that needs VBUS on GPIO21, which is reserved and unwired. It belongs to the power branch.

## 2026-08-06: A full refresh at boot returns in 18 ms, and nobody knows why yet

Recorded because it was found while merging sleep and could easily be mistaken for something that merge caused. It is not.

`tk_panel_render()` reports the boot's full refresh completing in 18-19 ms, where a full update on this panel takes about 2300 ms. Partial refreshes in the same run are healthy, at 622-625 ms against the 622 ms this rig recorded. The same 18 ms appears in the soak image, which has no radio, no deep sleep and none of the ssd16xx patch symbols, and whose `panel.cpp` is byte-identical to the one on `main` — so neither the merge, the patch, nor the networking configuration is responsible.

It was 2301 ms earlier the same day on the same rig, which is what makes it worth writing down rather than assuming it has always been so. What has not been established is whether the glass is actually wrong, or only the number: the full-refresh path brackets the write with `display_blanking_on()` and `display_blanking_off()`, and `panel.cpp` already carries a comment describing a bug with exactly this signature — "the refresh takes about 20 ms, nothing changes on the glass" — from when only the first half was called.

Someone has to look at the panel after a cold boot and say whether the deck name is on it.

## 2026-08-06: Every shipped corpus goes in the image, so the portal's language choice means something

The setup portal offered a language from its first commit and threw the answer away. Worse than not wired: the image embedded exactly one corpus, the one `CONFIG_TK_CORPUS_LANGUAGE` named at build time, so there was no German text on the device to show even if the choice had been stored.

Both bundles now ship. `app/CMakeLists.txt` embeds every language in `TK_CORPUS_LANGUAGES`, `app/src/corpus.c` is the table, and the build fails on a missing bundle rather than shipping a portal that offers a language the device cannot display. Each is about 7 KB against a 1344 KB slot, which is a better trade than a setting that does nothing.

The choice lives in the same NVS as the Wi-Fi credentials and falls back to `CONFIG_TK_CORPUS_LANGUAGE`, which is what a device that has never seen the portal is. A stored language the image no longer carries is dropped rather than obeyed: firmware can ship with a different set than the one that stored it, and a device with no corpus to open would have nothing to draw at all.

Changing it publishes `chan_corpus` — the channel the architecture already specified for exactly this, and whose documented rule is that a new corpus applies on the next _requested_ draw. So the question on the panel stays until somebody presses Next. Reopening the store rebinds the bag, whose fingerprint no longer matches, so the shuffle bag resets: indices into the English corpus mean nothing once the German one is open.

## 2026-08-06: The setup form saves the language without the Wi-Fi password

The form posts every field whether or not it was touched, and the handler required a network name, so changing the language meant retyping a Wi-Fi password. That is a bad trade for a setting, and worse on an open access point where the password is the one thing worth not sending twice.

An empty network name now means "language only": nothing is stored against Wi-Fi, no join is attempted, and the saved network keeps whatever it had. The scan list gains a "Leave unchanged" option, selected by default, so submitting the form untouched is that case rather than an error.

## 2026-08-06: Sleep is inhibited across the entry gesture, not only across the portal

`CONFIG_TK_SLEEP_IDLE_MS` and `CONFIG_TK_PORTAL_ENTRY_HOLD_MS` are both two seconds, and the idle timer starts at the first render. So the timer expires while somebody is still holding both buttons to enter setup, and `sleep_now()` was reached with the gesture half-done.

It survived on the bench because sleeping needs at least one button open — `open_pin_mask()` returns zero when both read closed, and the guard refuses. But that covers exactly the instant when both are down and nothing either side of it. Press the two buttons a moment apart and only one is closed when the timer fires: the device sleeps armed on the other, and the press meant for the gesture is spent on the wake instead.

`tk_net_is_active()` therefore covers the confirmation window as well as the running portal. Widening the inhibit rather than lengthening the idle timer, because the relationship between those two numbers should not be load-bearing: either can be tuned for its own reasons.

The "both buttons read closed" message drops from error to warning at the same time. Holding both buttons is a thing people now do on purpose, and reaching that line means a button is held for some other reason or is stuck — worth saying, not a fault.

## 2026-08-06: The device fetches bundles over plain HTTP, and the signature is the security boundary

[sync_protocol.md](sync_protocol.md) names an HTTPS manifest. The device cannot do TLS honestly, and pretending otherwise would be worse than not doing it.

Certificate validation needs a trusted clock. This device has none: no RTC time source, no SNTP, and a deep-sleep wake is a fresh boot with no notion of when it is. Validating without a clock means either failing everything or disabling expiry checks — and a certificate you will accept after it expires or is revoked is most of the way to no certificate. Fetching the time first would mean trusting an unauthenticated channel to bootstrap the channel that is supposed to be authenticated.

What actually protects the corpus is already specified and already built into the pipeline: an Ed25519 signature over the bundle's SHA-256, verified against a public key compiled into the image. The 2026-07-24 entry above already reasons that "the firmware side can use any conforming verifier". A key in the image is a stronger statement than a TLS session to a hostname in the same image, and it does not expire.

Given up, plainly: an observer on the path learns that a Tischkarte fetched a bundle, and can block the fetch. The question database is public, so there is nothing to keep confidential, and TLS does not stop anyone blocking traffic either. Replaying an older but validly signed manifest is a real attack and is answered by refusing any manifest whose version is not newer than the installed one.

This also takes TLS, a CA bundle, and the CA-rotation failure mode out of an image already at 51% of its partition.

## 2026-08-06: Devices download the raw bundle; gzip stays for the website

`.qdb.gz` is gzip around the binary and the device stores the binary, so something had to inflate it. Zephyr has no compression subsystem in this version, which means vendoring an inflate and finding somewhere to stage 15 KB while it runs.

Measured, that buys nothing. English is 5807 bytes gzipped against 15166 raw, German 3053 against 6809 — about 9 KB per language, once per release, on a device that is plugged into power when it syncs.

So the manifest points devices at the raw `.qdb` and the signature covers those bytes. `.qdb.gz` continues to be published for the website and for people. Manifest `schema` goes to 3.

This is not a format change: QDB2 is untouched and the two decoders in `tools/build_bundle.py` and `firmware/lib/qdb/` still agree, so the rule that a format change must land in both is not engaged.

## 2026-08-06: The corpus goes in a LittleFS partition above the 4 MB line

The module carries 8 MB of flash (`esp32s3_wroom_n8.dtsi`) but the devicetree includes `partitions_0x0_amp_4M.dtsi`, whose last partition ends at 0x3FFFFF. Everything from 0x400000 up is unclaimed.

The filesystem goes there. The alternative — carving it out of the 192 KB `storage_partition`, or widening `slot0_partition` — would move partitions that already hold something: the Wi-Fi credentials and the chosen language live in NVS inside `storage_partition`, and shifting it would orphan them on every device that had been set up. Adding above the line moves nothing.

## 2026-08-06: The device carries its own Ed25519 verifier

mbedtls cannot do it. `PSA_ALG_PURE_EDDSA` is defined in `tf-psa-crypto/include/psa/crypto_values.h` and appears nowhere in that module's `core/` or `drivers/` — the algorithm identifier exists, the implementation does not. This is a long-standing gap in mbedtls rather than something a Kconfig option turns on.

That leaves changing the signature scheme or carrying a verifier. Changing it would reverse the 2026-07-24 decision, rewrite `tools/keygen.py`, the bundle pipeline and the spec, and land on a curve with more ways to be implemented wrongly. A verify-only Ed25519 is a few KB of field arithmetic, and the SHA-512 it needs is in mbedtls, which the Wi-Fi driver already links.

## 2026-08-06: Sync runs on a cold boot and on request, until VBUS exists

The specified trigger is USB power plus a known network. VBUS detect is reserved on GPIO21 and not wired, so the device cannot tell it has been plugged in.

Until it can: sync on a cold boot once the station has an address, and offer it from the portal's status page. A cold boot is rare once the device sleeps — first power-up, the reset pin, a flat cell — which is the right frequency for something nobody is waiting on, and the portal covers wanting it now. Neither waits on hardware, and the charging window replaces both when the power branch lands.

## 2026-08-06: A sync somebody asked for says so on the panel; one that just happens does not

The original rule was that sync never shows anything: "use the new bundle on the next requested draw without displaying a status message", and "sync does not refresh the e-paper or take attention from the current question". The reasoning was the product principle — sync runs during a charging window, possibly while people are mid-conversation, and interrupting them to announce housekeeping is engagement with the product rather than between people.

That reasoning holds for a sync nobody asked for, and only for that one. It misses a case: somebody who has just pressed "sync now" in the setup portal, or who has finished setting up Wi-Fi, is standing at the device waiting to learn whether it worked. Telling them nothing is not restraint, it is a device that appears to have ignored them — and the portal cannot tell them either, because joining a network drops the phone off the setup access point.

So the rule splits by who started it. An automatic sync stays silent and applies on the next requested draw, exactly as before. A user-initiated one puts a card on the panel saying what arrived, and it stays there until the next press, which is how every service card already behaves.

No new machinery: this is `TK_CARD_SERVICE` and the `SERVICE` state built for the portal, whose whole behaviour is already "show this until somebody presses". `chan_corpus` keeps its rule — it must not trigger a redraw — because the card travels on `chan_service` instead, which is a different channel with a different meaning.

## 2026-08-07: TLS after all, as a transport rather than as the security boundary

The 2026-08-06 entry above chose plain HTTP, on the grounds that a device with no clock cannot honestly validate a certificate. That reasoning about the clock still holds. What it got wrong was the assumption underneath it: that plain HTTP is something you can simply have.

It is not, any more. Cloudflare Pages force-redirects to HTTPS and GitHub Releases is HTTPS-only, so serving the device plain HTTP means deliberately engineering around the platform — a bespoke Worker route whose only job is to defeat a redirect, maintained forever, on the path a device depends on to update itself.

So the device speaks TLS, and it is explicit about what that does and does not buy. It does not authenticate the server: certificate validation needs a trusted clock and there is not one. What protects the corpus is unchanged — an Ed25519 signature over the bundle's SHA-256, verified against a key compiled into the image, which does not expire and does not depend on who answered the socket. TLS here is the transport hosts will accept, plus confidentiality from a passive observer, and it is worth roughly 40 KB of flash against an image at 51% of its partition.

Calling this security theatre would be fair only if it were the security boundary. It is not, and the code and the docs should keep saying so, because the failure mode of forgetting is somebody later assuming the connection is authenticated and dropping the signature check.

## 2026-08-07: The device fetches from the website, not from GitHub Releases

GitHub Releases is where bundles are published and will stay there — it is free, it is already working, and `bundle.yml` needs no changes. It is the wrong thing for a device to talk to.

Every release asset URL answers with a 302 to `objects.githubusercontent.com`, and Zephyr's `http_client` does not follow redirects: there is no handling of 301, 302 or `Location` anywhere in it. Supporting that means writing redirect following, re-resolving a second host, and opening a second TLS session to it — on the least controllable part of the update path, to reach a URL structure GitHub can change.

It also requires the repository to be public before a device can fetch anything at all: a private repository's release asset returns 404 to an unauthenticated request, which is what a device is. Publishing is the intent — this is described as an open-source system, the questions are CC0, and the licences are already in the tree — but a device's update path should not be the thing that forces the timing.

So the manifest points at the site, which gives stable paths with no cross-host hop, and `CONFIG_TK_SYNC_BASE_URL` makes the host a build-time setting rather than something compiled into the flow. Anything that can serve two files over TLS will do, including a plain object store, if the site is ever not the answer.

## 2026-08-07: TLS is written but disabled, and sync runs over plain HTTP for now

The 2026-08-07 entry above chose TLS as the transport. The code is there and behind `CONFIG_TK_SYNC_INSECURE=n`, and it does not build: enabling the PSA elliptic-curve support a public host's handshake needs makes tf-psa-crypto's own `psa_crypto_ecp.c` fail to compile on `mbedtls_ecc_group_from_psa`. That is a broken configuration combination inside the vendored mbedtls 4, not a missing symbol on this side, and chasing it further was not worth holding the rest of sync for.

So the shipped default is plain HTTP, with the reason written where the flag is set. What that costs is confidentiality — an observer learns a device fetched a question bundle — and not integrity: the Ed25519 signature is the security boundary either way, and a device with no clock could not authenticate a server even with TLS on. A tampered bundle is refused exactly as it would have been.

Worth retrying at the next Zephyr bump. Until then it is an open item in the architecture rather than a decision to unpick.

## 2026-08-07: Sleep is inhibited across a sync, not only across the portal

Found on the bench, and it made the whole feature silently do nothing: the device joined a network and was asleep before it had an address.

The idle timer starts at the first render and fires after `CONFIG_TK_SLEEP_IDLE_MS`, two seconds. An association plus DHCP plus a fetch takes rather longer. `tk_net_is_active()` covered the portal and the entry gesture, so nothing stopped `sleep_now()` running in the middle of a cold-boot sync — and since a wake is a fresh boot, the next attempt started over and lost again.

The inhibit now covers the window from asking to join until the sync finishes, with a deadline so a network that never arrives cannot keep the device awake for good. This is the third thing to need that guard, which is the argument for `tk_net_is_active()` being one question the sleep path asks rather than a list of conditions it checks.

## 2026-08-07: MCUboot, and the two keys an update is signed with

Firmware updates need a bootloader, and the flash was already laid out for one: Zephyr's `partitions_0x0_amp_4M.dtsi` has defined `boot_partition`, two 1344 KB image slots and a scratch partition since before any of this was written. The device was simply not using them — the ESP32-S3 default is "simple boot", which loads slot0 directly. So enabling MCUboot moved no partition and orphaned no device state: the NVS holding the Wi-Fi credentials and the LittleFS holding the corpus are both untouched, verified on hardware by reading them back after the switch.

What it costs is 57,696 B of the 64 KB `boot_partition`, 88%, and that is almost entirely the Ed25519 verification the bootloader exists to do — an unsigned MCUboot for this board is 37,648 B. The application grew from 716,388 B to 784,372 B, and none of that is code: with MCUboot the image's IROM and DROM segments are MMU-page aligned, which pads the binary. 57% of slot0 either way.

An update carries two signatures, made with two keys, and the reason is that they answer different questions. MCUboot's, made by `imgtool` with `FIRMWARE_SIGNING_KEY`, answers "may this run" — it gates execution and it is checked before a byte of slot1 is copied. The manifest's, made with the existing `BUNDLE_SIGNING_KEY` over the image's SHA-256, answers "is this the current release", and exists so a device can refuse a bad image before spending a download and a restart on it. That second question is exactly the one a question bundle's signature answers, so it reuses that key rather than inventing a third.

The two keys are separate because they rotate differently. The bundle key is compiled into the application, so replacing it is an ordinary release. The firmware key is compiled into the bootloader, and nothing but a wired reflash replaces a bootloader — a device that will not accept your images is a device you have to physically reach. Keeping them separate means a compromised bundle key is an update and a compromised firmware key is a recall, rather than both being a recall.

The development key in `firmware/keys/` is committed on purpose and its private half is public. Without it a local build produces an image the local bootloader rejects, which would make `just fw-build` useless out of the box. CI writes the release secret over that path before building, so a released bootloader trusts the release key and nothing else.

## 2026-08-07: Overwrite-only, so a bad release is a reflash rather than a revert

MCUboot's alternative is swap-with-revert: boot the new image as "test", and roll back automatically unless it confirms itself. That is the safer-sounding default and it is the wrong fit here.

Confirming means the device deciding "I booted successfully". This device deep-sleeps between presses and every wake is a fresh boot, so that is a judgement it would make dozens of times a day rather than once, and a confirm written at the wrong moment either defeats the revert or triggers it on a device that is working. The swap also costs a second copy pass in a window where power must hold — the measured single copy is already 4.5 seconds.

So the bootloader overwrites, and what is given up is recovery from a signed image of ours that crashes on boot. That is a release-process failure, answered by testing before tagging, and it is recoverable over the wire because every device is reachable with a cable. What overwrite-only does _not_ give up is protection from an image that is not ours: the signature is checked before the copy begins, which was verified on hardware by planting an image signed with the wrong key and watching the bootloader refuse it and boot slot0 unchanged.

## 2026-08-07: A device says it was updated by comparing versions in NVS, not by asking the bootloader

An update installs during a boot, which is the one moment nobody is looking: MCUboot does the copy before any application code runs, and the device comes up looking exactly as it did before. Without something saying so, the only evidence is a console nobody has attached.

Zephyr exposes `mcuboot_swap_type()`, and it is the wrong source twice over. It answers what will happen on the _next_ boot, not what happened on this one, and in overwrite-only mode there is nothing left afterwards to distinguish an image installed a moment ago from one that has run for months.

Comparing `APP_VERSION_STRING` against a version stored in NVS answers the question actually worth asking — is this different firmware than the one that ran here last — survives a flat cell, and clears itself, because writing the new value is what makes the next boot quiet. It is the mechanism `language.c` already uses for the corpus release, in the same settings subtree.

The card goes out on `chan_service`, so it behaves like every other service card and holds the panel until somebody presses. A device with no version recorded stores one silently: a person unboxing a device should not be told it has just been updated.

## 2026-08-07: Nothing was reading the settings back

Found while building the update notice, and it had been true since settings were introduced: no code anywhere called `settings_load()`. `settings_save_one()` initialises the subsystem by itself, so saving worked and looked complete, while every registered load handler sat unused.

The chosen language was written and never restored, so a device set to German came back in English on the next boot. The installed corpus version was likewise never read, which meant the anti-rollback check in `sync.cpp` compared every manifest against an empty string — the guard was there, and it was comparing against nothing.

`load_settings()` in `main.c` now runs at `APPLICATION` init level, which Zephyr runs before the static threads start, so `app` cannot observe a half-loaded configuration. It never fails fatally: a device that cannot read its settings still draws questions in the compiled-in language, which is a better answer than refusing to boot.

A second thing the same bug taught: the settings subsystem matches subtrees component by component, not by string prefix. A handler registered for `tk/fw` does not receive the key `tk/fw_ver`, because `fw` and `fw_ver` are different components — that key falls through to the `tk` handler in `language.c`, which returns `-ENOENT`. The key is `tk/fw/ver`. This was caught on hardware, by a device that reported itself factory-fresh on two consecutive boots.

## 2026-08-07: Wi-Fi credentials are at rest in the clear, and that is recorded rather than fixed

Demonstrated rather than assumed: with the device in its ordinary state, no gesture and no portal, one USB cable and eighteen seconds of `esptool read-flash 0x3b0000 0x30000` yields the network name and its password as printable strings. `wifi_credentials` stores them in NVS, NVS does not encrypt, and nothing else in the image does either.

The ESP32-S3 can fix this properly. It has AES-XTS flash encryption with the key in an eFuse that software cannot read, and MCUboot's Espressif port supports it — `boot/espressif/hal/src/flash_encrypt.c` is right there in the tree. Three things stand in the way, and together they make it the wrong change for now.

It needs a different bootloader. `ESP_FLASH_ENCRYPTION` in Zephyr is `depends on !ESP_SIMPLE_BOOT && !MCUBOOT`, and its help says the bootloader must be MCUboot's _Espressif_ port, built with IDF-style configuration. What this repository builds is the _Zephyr_ port, as a sysbuild image — the integration the OTA work is built on. Switching ports means giving that up.

It needs `write-block-size = 32` in the devicetree, against the 4 this board declares. That is not cosmetic: NVS, LittleFS and the OTA stream all align to it, so raising it re-lays-out the storage partition and orphans the credentials, the language and the corpus on every device already set up. That is the same hazard the 2026-08-06 partition decision was written to avoid, arriving from the other direction.

And it burns eFuses, irreversibly, per device. Release mode additionally disables UART read-back, which is what actually closes the hole above; development mode leaves re-flashing possible and is bypassable.

Two non-answers, so nobody spends time on them. Encrypting the password with a key compiled into the image is not encryption against this attack: the dump that yields the password also yields the image, and therefore the key. And the password cannot simply not be stored — every wake is a fresh boot, so the device has to rejoin a network unattended.

The middle option is real but is custom work: the S3's HMAC peripheral can hold an eFuse key software cannot read, which would let the application encrypt the password with a secret an attacker does not get from the flash. The vendored `hal_espressif` here exposes no HMAC component, so this is register-level work rather than a Kconfig switch.

So it stays as it is, deliberately, and this entry is the record. What that buys an attacker is a Wi-Fi password, from a device they are holding. The mitigation that costs nothing is the one already in use on the bench: give the device a guest network. Revisit at the PCB stage, where the eFuse step can be part of manufacturing rather than something done to a device already in use.

## 2026-08-07: The device gets its own host, over plain HTTP, and TLS stops being the plan

The 2026-08-07 entry "TLS after all, as a transport rather than as the security boundary" chose TLS on one premise: that plain HTTP is not something you can simply have any more, because Cloudflare Pages force-redirects and GitHub Releases is HTTPS-only. The premise was true about _those two hosts_ and was mistaken as a fact about the internet. Plenty of things serve plain HTTP without an argument — an S3 static-website endpoint does it by design, as does any small static host somebody runs.

So the conclusion inverts. Rather than making the device speak a protocol it cannot use honestly, the device gets a host that speaks the protocol it can. `/device/` moves off the website, onto something that serves two files without upgrading the request. The website stays where it is, on HTTPS, for people.

What decided it is that TLS here cannot ever be the security boundary. The device has no clock — no RTC source, no SNTP, and every wake is a fresh boot — so it cannot check a certificate's expiry or revocation. A TLS session it cannot validate authenticates nobody. The Ed25519 signatures do the work instead: one over each bundle and image, verified against keys compiled into the image and the bootloader, neither of which expires.

Given up, precisely: an observer on the path learns that a Tischkarte fetched a question bundle or a firmware image. Both are public artifacts of a public project. Not given up: an attacker on the path still cannot make the device install anything the keys did not sign, and cannot walk it backwards, because a manifest not newer than what is installed is refused.

The cost of the alternative was also real — about 40 KB of flash for TLS, on an image at 57% of its slot, for confidentiality on public data. And it does not build: enabling the PSA elliptic-curve support a public handshake needs breaks tf-psa-crypto's own `psa_crypto_ecp.c`. That code stays behind `CONFIG_TK_SYNC_INSECURE=n` and is worth retrying at the next Zephyr bump, for confidentiality alone rather than as a fix for anything.

The defaults become `http://tischkarte.invalid/device` — deliberately unresolvable, because a device pointed at a host that does not exist retries for a few seconds each cold boot and carries on, while one pointed at a host somebody else owns is a different matter. The real host replaces it when it exists.

## 2026-08-07: A power switch is the update trigger the hardware can actually have

The design's trigger is USB power plus a known network — a charging window, when the device has power to spare and nobody is waiting. It needs VBUS detect on GPIO21, which the pin map reserves and nothing is wired to, so the firmware cannot tell whether it is plugged in. Sync and the OTA check therefore run on a cold boot, which once the device sleeps means first power-up, the reset pin, or a flat cell.

A physical power switch turns that from an accident into an action. Off and on is a cold boot, and a cold boot is already the trigger, so "flip the switch to check for updates" needs no firmware change and no new GPIO. It also stops a device draining its cell on a shelf, which is wanted independently.

What it does not do, and these should be stated rather than discovered. It is manual: there is no unattended overnight update, and a device nobody touches stays on its version indefinitely. A hard power cut takes RTC memory with it, so the shuffle bag, the active deck and the refresh counter reset — the device comes back on New People with a full refresh, which is fine occasionally and tiresome daily. And it tells the firmware nothing about the battery, because sense on GPIO1 is unwired too, so nothing refuses an update on a weak cell.

That last one matters less than it sounds. MCUboot runs overwrite-only, and a brownout during its copy leaves the source image and the pending flag untouched in the spare slot, so the next power-up simply copies again. The window that sounds dangerous is the recoverable one.

Not built and not tested: there is no switch on the rig yet. Recorded now so the reasoning is not reconstructed later, and because it changes what VBUS is for — with a switch, VBUS stops being the only way to trigger an update and becomes the thing that makes updates unattended.

## 2026-08-07: The device endpoint is a bucket, and the website is not it

Once plain HTTP became the plan rather than the workaround, "which host" stopped being a detail. The requirement is narrow and unusual: answer port 80 without upgrading, publicly, at stable paths. Most modern static hosting is built to do the opposite — Cloudflare Pages and GitHub Pages both force HTTPS, which is correct for a website and fatal for a client that cannot follow a redirect.

An S3 static-website endpoint is HTTP-only by design. That is a limitation everywhere else and the feature here, so `/device/` moves onto one and the website stays on Pages. Two hosts, one job each, and `site.yml` publishes the same files to both — the Pages copy for people who want to look, the bucket copy for devices.

The trap worth writing down, because it will be found the hard way otherwise: if the DNS for that name is on Cloudflare, the record must be **DNS-only**, not proxied. A proxied record puts Cloudflare in front of the bucket and reintroduces exactly the HTTPS upgrade the arrangement exists to avoid, and a browser will show a perfectly working URL while every device fails.

The bootloader is deliberately not published there. It is not something a device fetches — it is what a wired first flash needs — and serving it beside the image invites installing one without the other, which is how a board ends up trusting a key its images are not signed with. It stays on the GitHub Release.

Cache lifetimes are split rather than defaulted: sixty seconds on the two manifests, immutable on everything they name. A manifest cached for an hour is an hour in which a release reaches nobody, while an artifact is named after its version and never changes.

The whole procedure, from buying the domain to the first device that updates itself, is in [hosting.md](hosting.md).

## 2026-08-09: The power bench gets a bq25185 and a 1500 mAh cell, and neither is the product part

[prototype bom](prototype_bom.md) planned Stage 2 around an Adafruit 4755 (BQ24074) and a 500 mAh LP503035, and said not to buy them yet. What was bought instead is an Adafruit 6092 — a bq25185 with a TPS62569 3.3 V buck on the same board — and an EEMB 524261, 3.7 V and 1500 mAh.

The charger board is the better bench part for a reason that has nothing to do with the charger: it brings its own regulated 3.3 V at 1 A, so the rig runs off a cell without a second regulator to build or a devkit LDO to burn a volt in. It also breaks out USB D+ and D-, which makes the single-connector arrangement rev A wants testable now rather than after a PCB.

The cell is not the product cell and is not meant to be. At roughly 61 x 42 x 5.2 mm it does not fit the 30 x 35 x 5 mm slot in [device prototype](device_prototype.md), let alone the 84 x 56 x 16 mm envelope; `hardware/pcb/BOM.md` keeps the 503035 and this branch does not touch the enclosure. Its job is to make the discharge long enough to measure.

Charge current is cut to 500 mA with the jumper on the back, which is 0.33C. Inside EEMB's standard rate, and a full charge lands near three and a half hours — clear of the chip's unmodifiable six-hour safety timeout, which would otherwise stop a slow charge part-way and look like a fault.

The rev A charger stays open. `hardware/pcb/BOM.md` still says "BQ2407x class, exact part open", and the 6092's onboard buck overlaps the XC6220 already chosen there. That is a decision the bench should inform rather than one this purchase makes.

Accepted cost: the runtime figures this bench produces are for a cell three times the size of the one the product is designed around, so they scale and cannot be quoted.

## 2026-08-09: Charged is a guess, and only the LED is allowed to believe it

The bq25185 exposes no charge-status pin. Adafruit's board has an orange LED wired to it and no pad, so the firmware cannot be told that charging has terminated — it can only look at the cell.

`CHARGING` and `CHARGED` are therefore one visit to external power split by a voltage threshold, `CONFIG_TK_POWER_FULL_MV`. A lithium cell under constant-voltage charge sits near 4.2 V for the last hour while the current tapers, so this reports full early, by an amount that depends on what the load is doing. There is no reading that would do better.

That is acceptable for an LED. Somebody glancing at a device to see whether it is done is not harmed by being told so ten minutes early, and the failure is self-correcting — they unplug it and it works. It would not be acceptable anywhere else, so nothing else reads it: the sync window, the refresh gate and the sleep inhibitor all ask about external power or about the floor, never about full.

The split earns its keep in the state machine rather than costing anything there. One visit to external power opens one sync window, and crossing the threshold in either direction does not open another — which matters, because during a taper the reading crosses it more than once.

Accepted cost: a green LED means "probably finished", and the docs have to say so rather than letting somebody infer a measurement that was never taken.

## 2026-08-09: VBUS is read at boot, not armed as a wake source

The plan was always that VBUS on GPIO21 would join the deep-sleep wake mask, and [firmware architecture](firmware_architecture.md) said so: "VBUS detect joins the mask with the opposite polarity, since plugging in drives it high." It cannot. EXT1 takes one trigger polarity for the entire pin mask, and the ESP32-S3 does not define `SOC_PM_SUPPORT_EXT1_WAKEUP_MODE_PER_PIN`, so a pin that is interesting when high cannot share a mask with two buttons that are interesting when low.

EXT0 can. It is a separate single-pin trigger with its own polarity, it is available on this SoC, and `esp_sleep_enable_ext0_wakeup(21, 1)` would do exactly what was wanted. The reason not to is in Espressif's `sleep_modes.c`: arming EXT0 forces `ESP_PD_DOMAIN_RTC_PERIPH` to stay powered through every sleep, where EXT1 alone leaves that domain off. That is a permanent standby cost against a 30 uA budget, paid on every sleep for the rest of the device's life, to save one button press.

So VBUS is polled — at boot, and every `CONFIG_TK_POWER_SAMPLE_MS` while awake. Plugging in does not wake a sleeping device; the next press does, and that boot finds VBUS high and opens the window. The gesture is "plug it in, press Next", which is the one [design](design.md) specified in the first place.

The EXT0 path is written up as `CONFIG_TK_POWER_WAKE_ON_USB`, default off, so the cost can be measured rather than argued about.

_2026-08-12: written up became built._ `sleep_now()` arms EXT0 on the same `tk-vbus-gpios` pin `power.c` samples, still behind that symbol and still off by default — a switch that changed nothing was a measurement nobody could take. An EXT0 wake reports no button, so no press is replayed and `net` takes its cold-boot branch, which is what waking on a plug-in is for. Neither half is verified: no board has slept yet.

Accepted cost: a device plugged in and left alone does not sync until somebody touches it.

## 2026-08-09: The device stays awake for as long as it is plugged in

Sleeping is decided by `sleep_now()`, which already refuses while a refresh is in flight and while the portal is on air. External power is now a third refusal, and it is a stronger one than the others: it holds for the whole charge, not for a bounded window.

The reason is the status LEDs. They need the SoC running to be lit, and red handing over to green across a charge is most of what they are for — a device that went dark five minutes into an overnight charge would be reporting that it had stopped charging. On mains the current this costs is not the cell's.

`CONFIG_TK_POWER_CHARGE_WINDOW_MS` survives, but it now bounds only the sync and update window rather than wakefulness. A device left on a charger overnight should not retry a download for eight hours; it should stay lit and stop asking.

This deleted something. An earlier draft gave the retained block a `charge_window_spent` flag and a layout version bump, because a device that slept mid-charge could not work out whether it had already synced — VBUS stays high for hours and there is no charge-status pin to distinguish a fresh plug-in from an old one. Staying awake removes the question entirely: from the press that opens the window to the unplug, the device never sleeps, so the answer is in RAM. The retained block is unchanged.

Accepted cost: a fault that leaves VBUS reading high would keep the device awake indefinitely and flatten the cell it thinks is charging.

## 2026-08-09: Two plain LEDs, because an addressable one never turns off

The device needed a way to say things the panel cannot — that a press was refused because the cell is flat, that a charge is running, that the portal is on air. Saying any of them on the e-paper is itself a refresh, which is the thing being refused.

The DevKitC has an addressable LED on GPIO48 and it is the wrong part. A WS2812 is not an LED with a driver; it is a controller — oscillator, shift register, data reshaping buffer — that happens to have three emitters attached. That controller is powered whenever the rail is up, so the package idles at around 0.6 mA with every emitter dark. Against a 30 uA whole-device budget that is twenty times the target, spent to display nothing, and fixing it means a high-side FET and a GPIO to drive it.

Two GPIOs, two resistors and two dice cost nothing at all when both pins are low. The bench uses two discrete LEDs and rev A uses one bi-colour package on the same two pins, so the firmware is identical and only the lens differs.

Red and green is the whole palette, since lighting both is amber, and there are more conditions than colours. The two pairs that would collide are separated by rhythm instead: a low-battery blink against a steady portal amber, and a transfer pulse against a steady charged green. Blue and magenta, which an earlier sketch used for the portal and for a failed ADC, are simply unreachable — the portal moved to amber and the ADC fault left the LED entirely, since it is a bench condition the console already reports and a fourth thing to tell apart would make the other three harder to read.

The bring-up LED on GPIO2 went with this. It toggled on every question that reached the panel, which mattered before the panel was wired and does not now. Its pin returns to the ADC1 pool, which [hardware wiring](hardware_wiring.md) always said was what it was being kept for.

Accepted cost: two separate packages on the bench do not blend into amber — they read as a red LED and a green LED both lit. Every transition and every priority rule is still exercised; only the final appearance waits for the rev A part.

## 2026-08-09: One USB-C on rev A, and a boot strap to go with it

The ESP32-S3 has a USB Serial/JTAG controller in silicon on GPIO19 and GPIO20. It presents two interfaces at once — a vendor JTAG interface OpenOCD claims and a CDC-ACM the host sees as a serial port — which `app/debug.overlay` already uses for the debug console, and esptool can drive the ROM download mode through the same CDC interface.

So rev A needs no CP2102, no auto-reset transistor pair, and no second connector. One USB-C: VBUS to the bq25185 input, D+ and D- to GPIO19 and GPIO20. `hardware/pcb/BOM.md` already lists a USB-C with 5.1 kOhm CC pulldowns and no bridge chip, so this confirms an assumption that was made without being written down.

It obliges one thing the current board gets for free. Download mode over USB Serial/JTAG is normally entered by command, but an image that reconfigures those two pins, or that crashes before USB enumerates, can only be recovered by strapping IO0 low at reset. The two product buttons are on GPIO4 and GPIO17. Rev A therefore wants a test point or an internal button on IO0, and that is the difference between a board somebody can rescue and a brick.

Worth stating next to it: a sleeping device is not on the bus at all. Deep sleep stops USB enumerating, so the port disappears between presses, which is already true today and surprises everyone once.

Not verified. Nothing in this repo has yet flashed over `/dev/cu.usbmodem*` — `just fw-flash` targets the CP2102 and `debug.overlay`'s own header still asserts that flashing wants the UART jack. It is one bench check, and until it is run this is a plan rather than a finding.

## 2026-08-12: A floor under the ladder, rather than an image that cannot draw a card

`CONFIG_TK_POWER` is on in the devkit board conf, so every image `just fw-build` and `just fw-flash` produce read the two dividers. Neither divider is soldered. A board with no divider fitted reads a floating pin and can refuse every refresh. That is what the bench would have hit on the next flash: a floating GPIO1 reads _something_, anything under 1600 mV at the tap walks the ladder to LOW or CRITICAL before a thread starts, and `refresh_allowed()` then refuses every press including the one that draws the first card. Blank panel, answered by three red blinks on LEDs that are not wired either.

The alternative was to hold the board-conf enable until the copper exists. Rejected: it would leave the power path in every image except the one anybody flashes, which is how a module rots.

So `lib/power` gets a floor instead. Below `CONFIG_TK_POWER_PLAUSIBLE_MV` — 2500 mV — a reading is not a very flat cell but no cell at all, and the machine returns to UNKNOWN, which already permits refreshes and is already the fail state. The threshold is sound rather than convenient: a protected cell disconnects between 2.5 and 3.0 V and the 3.3 V buck stops before that, so an SoC that is still executing cannot be reading 2.4 V from its own pack.

The VBUS half cannot be fixed the same way. It is a digital pin, high is high, and software cannot tell a floating input from a charger. An internal pull-down is not the answer either — it would load the planned 100k/150k divider to about 1.3 V, under V_IH, so the divider would stop working once it was fitted. What the firmware does instead is say so: a boot that finds VBUS high while the pack reads implausible logs a warning naming both dividers. The remedy is a jumper, and [hardware wiring](hardware_wiring.md) now asks for one.

Accepted cost: a real cell discharged below 2.5 V reads as UNKNOWN and the device stops refusing refreshes at exactly the point it should be most careful. That is a state a protected cell reaches by disconnecting, so the device is not running to have an opinion about it.

## 2026-08-14: The power bench passed, and what it hands rev A

The rig is built: an Adafruit 6092 with a bq25185, a 1500 mAh cell, both sense dividers and two status LEDs on a breadboard, feeding a DevKitC from the charger's 3.3 V buck. Everything the power path was written for now runs on it — the device reads its own cell within 1 % of a meter, sees VBUS on the first sample of a boot, opens one sync window per plug-in, stays awake for the whole charge, refuses a refresh below the floor and says so on the LEDs, and runs from the cell alone with no USB attached.

One number the bench cannot produce, and it is the one the design is about. **Sleep current is unmeasurable on a DevKitC.** Its power LED draws one to two milliamps from the 3V3 rail continuously and its onboard WS2812 idles at roughly another 0.6 mA, together one to two orders of magnitude above a 30 µA budget, under which the bq25185 and TPS62569 quiescent currents disappear entirely. Nothing measured here bounds rev A, and no runtime can be extrapolated from it.

That makes rev A the first board that can answer the question, and it only can if nothing on it draws continuously — which promotes "no always-on indicator" from a preference to a layout requirement, a power LED included.

Three smaller findings go with it:

The battery divider should be **1 MΩ over 470 kΩ**, and switched over both. 1M/1M halves the standing draw to 2.1 µA; what it costs is a 500 kΩ source, which shows up as roughly 25 mV of ADC-leakage offset at the tap against thresholds 200 mV apart. Switched, it costs nothing at all, which is what a product should spend across a cell it is supposed to be preserving.

The divider constants need no calibration. `TK_POWER_DIVIDER_NUM`/`_DEN` stay at 2/1: the rig logs 3890 mV against 3930 on a meter, 1.0 % low, and the error is an offset rather than a ratio, so scaling it would overcorrect at the low end where the reading decides something. It also errs towards refusing early, which is the safe direction.

A charger that reports **charge termination** would remove an estimate from the design. The bq25185 has no such pin, so CHARGED is inferred from voltage and runs early by an amount that depends on load — acceptable for an LED, and the reason nothing else is allowed to read it.

Accepted cost: the sleep budget stays a target rather than a measurement until rev A exists, so the runtime figure in the design is still arithmetic rather than an observation.

## 2026-08-14: The charged threshold has to clear the charger, not approach it

`TK_POWER_FULL_MV` was 4150 mV, fifty under the bq25185's 4.2 V regulation point. On the bench that threshold is unreachable: the divider reads about 40 mV low, so 4150 reported needs 4190 at the cell, and constant-voltage charge approaches 4.2 V asymptotically while the device draws its share of the current. The cell sat at 4140 mV for an hour and the LED stayed red.

An unreachable threshold costs the whole green half of a two-colour palette, to buy precision in a number the decision log already describes as an estimate that runs early by an unknown amount. 4050 instead — about 4090 at the cell, which is a full-enough LiPo for something glanced at across a table, and far enough below the regulation point that divider error and charger tolerance both fit in the gap.

Accepted cost: green arrives earlier than before, on a signal that was never a termination measurement.

## 2026-08-14: The contribution vocabulary lives in the schema, not in three copies

A contributor meets the deck, tag and depth vocabulary in three places: the website form, the GitHub issue form, and the validator. All three had their own copy, and the depth strings had already drifted — the website prefilled `3 — consequential disclosure` where the issue form declares `3 — vulnerability, conflict, fear, loss, or consequential disclosure`. GitHub drops a dropdown prefill it does not recognise, so every depth-3 submission arrived with its required depth field blank and the contributor had to fill it in again on a page they had been told was prefilled.

`questions/schema.json` `x-tischkarte` already held the decks and tags; it now also holds `depthLabels`, the exact option strings of the issue form. `website/src/config.ts` restates them so no page parses the schema at runtime, and two tests hold the copies to the original: a Python one over the issue template, a vitest one over the website constants.

Accepted cost: an option string cannot be reworded in one place any more — the schema and both forms move together, which is the point.

## 2026-08-14: A submission is checked while the issue is open, not at approval

Duplicates, denylist hits and text-rule violations were caught by `validate.py` inside the promotion job, which runs when a maintainer applies `approved`. A rejected submission therefore failed a workflow with no comment anywhere: the issue still looked approved, the contributor was told nothing, and the reason lived in the Actions tab.

`promote_issue.py check` now runs on issue open and on every edit. It parses the form, appends the entry to a temp copy of the database, and runs the real validator over it — the same rules that decide at merge time, rather than a second implementation of them. What it finds goes on the issue as a comment, with a `needs-changes` label that comes off when an edit passes. The promotion job additionally reports its own failures on the issue and removes `approved`, so a failed promotion is visible and retryable.

The website form closes the same loop earlier still: it now holds the active language's corpus and refuses a question the database already has, with a link to the one that exists.

Accepted cost: an editing contributor can generate several comments on one issue, and the check job runs on every edit to a question-submission issue.

## 2026-08-14: Playwright for the website end-to-end suite

First e2e runner in the tree (`website/e2e/`, `just test-e2e`). It drives the real `astro build` output through a real browser, which is what the vitest suites cannot do: the bugs it found on its first run lived between the built HTML and the island scripts, not inside either. Kept out of `just test` because it downloads a ~95 MB browser; CI runs it as its own job in the site workflow.

Chosen over the alternatives on one property: it can assert against the same generated payloads the page fetches, so a test states what the site should do rather than restating what it does.

Accepted cost: one more devDependency and a browser download in CI.

## 2026-08-14: `[hidden]` needs an author-level rule

Every island shows and hides things by setting the `hidden` property. The browser implements that as `display: none` in its own stylesheet, which any class rule in `global.css` outranks — so `.btn`, `.btn-quiet`, `.toolbar-row` and `.field .sub` were each silently defeating it. In practice: the incubator note on the contribute page was permanently visible, "show more" stayed on the browse page with nothing more to show, and a shared read-only deck offered its owner's share, export and import buttons.

One rule at the top of `global.css` — `[hidden] { display: none !important }` — is the standard fix, and `!important` is load-bearing rather than lazy here: the rule has to win against a class regardless of specificity.

Accepted cost: an element that genuinely needs to be laid out while carrying `hidden` would need a different mechanism. Nothing does.
