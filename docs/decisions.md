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
Retained state still lives in its RTC sections from the first commit, so
enabling deep sleep later is a configuration change rather than a rewrite.

Accepted cost: the sleep path, which is where the boot-latency budget and the
30 µA target are decided, is the last thing to be exercised rather than the
first.

## 2026-08-02: Host tests default to qemu_xtensa, not native_sim

`native_sim` is faster and is the only host platform that emulates GPIO, but it
builds on Linux only, and development happens on macOS. `qemu_xtensa` runs a
full Zephyr kernel on macOS and matches the target architecture, so it is the
default for `just fw-test`. CI overrides it with `just simboard=native_sim`.

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
