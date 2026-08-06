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

`added` is stripped from the main payloads (spec §5), so the browse page sorts "newest" by reverse position within each category file. Files are append-only, so file order *is* chronological order. `recent.{lang}.json` (which keeps `added`) stays the source for the contribute page's recent list.

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

## 2026-07-26: TKB2 stores one question with a deck mask

Device bundles store each question once with a six-bit eligibility mask,
editorial depth, and dark/spicy flags. The physical device no longer needs
repository IDs or OLED display numbers. Manifest schema and bundle magic both
advance to version 2.

Accepted cost: no backward compatibility with the unshipped TKB1 format. This
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

*Corrected 2026-08-05.* This entry originally claimed retained state had lived
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

The question bundle stays the flat TKB2 binary in `sync_protocol.md` instead of
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

Naming: TKB is the Tischkarte Bundle and the trailing digit is the format
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

The 2026-08-02 entry above put `CONFIG_PM` behind input, display *and* storage
being correct. Storage has not been started, so by that ordering the sleep path
waits for LittleFS, NVS and sync. It is being spiked now instead, ahead of all
three.

The reason is the cost that entry accepted: the sleep path is where the
boot-latency budget and the wake mechanism are decided, and it was scheduled
last. The first bench run turned two of those from theory into arithmetic. A
partial refresh takes 622 ms of a 1 s budget, leaving roughly 380 ms for ROM
boot, Zephyr init, selector read and draw. And the six selector contacts cannot
be armed as EXT1 wake sources the way the architecture describes: a deck is
selected by *holding* one contact closed, so that pin is low for as long as the
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
own state — the knob position *is* the deck, readable at zero power, which is
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

| | without | with | delta |
|---|---|---|---|
| flash image | 238 KB | 676 KB | +438 KB, against a 1344 KB slot |
| dram0_0_seg | 21.1% | 60.6% | +154 KB |
| iram0_0_seg | 13.8% | 18.4% | +18 KB |
| threads | 6 | 14 | +8 |

Both fit with room to spare, and a device whose Wi-Fi cannot be configured without a special build is not the device being built. So `just fw-build` carries the radio.

What the original reasoning was actually protecting is narrower than "the everyday image": it is the images that *measure* something. `soak.conf` and `sleep.conf` therefore set `CONFIG_TK_NET=n`, `CONFIG_WIFI=n` and `CONFIG_NETWORKING=n`. A partial refresh has already been seen at 2769 ms during a portal session against the 622 ms this rig recorded without one, and until that is explained a ghosting run must not have a radio in it.

The configuration lives in `app/boards/esp32s3_devkitc_esp32s3_procpu.conf` rather than `prj.conf`. `prj.conf` is shared with qemu and native_sim, `CONFIG_WIFI_ESP32` needs a devicetree node only this SoC has, and putting it there breaks `just fw-sim`. The second Wi-Fi node AP+STA needs is in the matching board overlay for the same reason.

`just fw-portal` remains: the same image with `CONFIG_TK_DEBUG_PORTAL=y`, which skips the two-button gesture. The gesture needs two hands on the board at the moment it boots, which makes everything behind it awkward to work on.

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

Changing it publishes `chan_corpus` — the channel the architecture already specified for exactly this, and whose documented rule is that a new corpus applies on the next *requested* draw. So the question on the panel stays until somebody presses Next. Reopening the store rebinds the bag, whose fingerprint no longer matches, so the shuffle bag resets: indices into the English corpus mean nothing once the German one is open.

## 2026-08-06: The setup form saves the language without the Wi-Fi password

The form posts every field whether or not it was touched, and the handler required a network name, so changing the language meant retyping a Wi-Fi password. That is a bad trade for a setting, and worse on an open access point where the password is the one thing worth not sending twice.

An empty network name now means "language only": nothing is stored against Wi-Fi, no join is attempted, and the saved network keeps whatever it had. The scan list gains a "Leave unchanged" option, selected by default, so submitting the form untouched is that case rather than an error.
