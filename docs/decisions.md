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

## 2026-08-06: CONFIG_TK_NET is off in the everyday image

The networking image is 676 KB against a 1344 KB `slot0_partition`; the everyday one is 238 KB. It fits, and it is still not what `just fw-build` should produce, for the reason `CONFIG_TK_SLEEP` is off: the panel and power work measure that image, and a subsystem that adds 438 KB of flash, 50 KB of Wi-Fi heap and eight threads should not appear underneath a refresh measurement.

`just fw-net` is the image with a radio in it. `just fw-portal` is the same thing with `CONFIG_TK_DEBUG_PORTAL=y`, which skips the two-button gesture — the gesture needs two hands on the board at the moment it boots, which makes everything behind it awkward to work on.
