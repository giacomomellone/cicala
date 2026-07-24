# Decisions

Append-only decision log (ADR-lite). One `##` per decision, newest last. Decisions marked DECIDED in the handoff specification (`Tischkarte — Project handoff specification.md` at the repo root) are not repeated here — this file records choices the spec left OPEN, plus every dependency added.

## 2026-07-24 — Repo bootstrapped from the handoff specification

Phase A (database + tools), Phase B (website), and structure-only stubs for firmware/hardware created in one pass. Git history starts here; the spec document itself is kept at the repo root as the founding reference.

## 2026-07-24 — Placeholder org and domain

GitHub org/repo is assumed as `tischkarte/tischkarte` and the site domain as `tischkarte.pages.dev` until the real ones exist. Both live in exactly one place each (`website/src/config.ts`; `$id` in `questions/schema.json`; `--base-url` default in `tools/build_bundle.py`; CODEOWNERS handles in `.github/CODEOWNERS` and maintainer names in `docs/languages.md`), so the rename is a one-commit operation as the spec requires.

## 2026-07-24 — Tests use stdlib unittest, not pytest

The tools dependency policy is "stdlib + pyyaml + jsonschema only"; adding pytest for tests would be the first violation of the policy in the same commit that introduces it. `python -m unittest discover tools/tests` needs nothing extra.

## 2026-07-24 — Ed25519 signing via system openssl, not a Python package

`build_bundle.py`/`keygen.py` shell out to `openssl` (≥ 1.1.1, present on GitHub runners and dev machines) instead of adding `cryptography` or `pynacl`. Keeps the tools dependency policy intact; the signature is standard ed25519 over the bundle's raw SHA-256 digest, so the firmware side can use any conforming verifier. Unsigned dev builds are allowed (`"sig": null`) and must be rejected by release firmware.

## 2026-07-24 — Denylist semantics: whole-word match on normalized text

Terms (including multi-word phrases) match case-insensitively as whole words against the NFC-lowercased, whitespace-collapsed text. Substring matching was rejected for Scunthorpe-problem false positives. A hit is a CI failure that asks for manual maintainer review — no cleverness beyond that, per spec.

## 2026-07-24 — `--fix` rewrites only files that validate cleanly

`validate.py --fix` never rewrites a file that produced errors, so a broken submission can't cause data-destroying reformats. Rewritten files carry a fixed 3-line header comment; hand-written comments in question files are not preserved (the files are tool-managed).

## 2026-07-24 — Browse "newest" sort uses reverse file order

`added` is stripped from the main payloads (spec §5), so the browse page sorts "newest" by reverse position within each category file — files are append-only, so file order *is* chronological order. `recent.{lang}.json` (which keeps `added`) stays the source for the contribute page's recent list.

## 2026-07-24 — Website shows the device's display id

The meta line's `#<id-short>` uses the same formula as the OLED (`decimal of first 2 id-hash bytes mod 10000`, e.g. `#6666`), so a question has one human-visible number across site and device. Collisions are cosmetic and accepted, same as on the device.

## 2026-07-24 — Permalinks switch the active language

Visiting `/q/<id>` sets the active language (`tk.lang`) to that question's language. A shared German link therefore lands in a fully German experience, and "next question" continues in German — the alternative (keeping the visitor's old language) would show a German question inside English chrome and then jump languages on the first keypress.

## 2026-07-24 — Generated site data is not committed

`website/src/data/*.json` is gitignored; `tools/build_site_data.py` regenerates it locally and in CI before every build. The question YAML stays the single source of truth, and PR diffs never contain generated blobs.

## 2026-07-24 — npm dependencies (website)

Per the guardrail, every package with a one-line justification:

- `astro` — the decided framework (spec §7.1).
- `@fontsource/literata`, `@fontsource/ibm-plex-mono` — the decided self-hosted fonts (spec §7.1); fontsource is the standard way to self-host with subset files.
- `typescript` (dev) — required for `.ts` islands and editor support; ships no runtime bytes.

## 2026-07-24 — firmware.yml gates on the project existing

The firmware CI workflow checks for `firmware/CMakeLists.txt` and exits green with a notice while only the structure stubs exist (this build), so CI isn't red for months. The `idf.py build` path activates automatically once the phase-C skeleton lands.

## 2026-07-24 — GitHub issue form uses a multi-select dropdown for tags

GitHub issue forms cannot URL-prefill checkbox groups, and the contribute page must prefill everything. Tags are therefore a `dropdown` with `multiple: true` (prefillable); the CC0 confirmation stays a required checkbox the contributor must tick on GitHub — which is exactly the explicit consent we want anyway.

## 2026-07-24 — Doc filenames are snake_case; just is the repo entry point

`docs/` files renamed from SCREAMING_SNAKE_CASE to snake_case (`design.md`, `decisions.md`, `languages.md`, `sync_protocol.md`) per maintainer preference; root-level `README.md`/`CONTRIBUTING.md`/`CODE_OF_CONDUCT.md` keep their conventional names because GitHub's UI treats them specially. A `justfile` is the single entry point: typing `just` lists every command (database, website, docs, tests, firmware); recipes call the `.venv` tools directly so nothing needs to be on PATH except `just`, `python3` and `npm`.

## 2026-07-24 — Docs are an MkDocs site (mkdocs-material)

`docs/` doubles as an MkDocs site (`mkdocs.yml`, strict mode, output gitignored at `site/`). Dependency justification: `mkdocs-material` — the de-facto standard MkDocs distribution; configured with `font: false` so the docs load no third-party resources, matching the website's ethos. Installed via the same `.venv` as the tools (`requirements.txt`).

## 2026-07-24 — Website tests: vitest + happy-dom

Dependency justifications (dev-only, zero runtime bytes): `vitest` — the Vite-native runner, shares Astro's transform pipeline so TS test files just work; `happy-dom` — lightweight DOM (localStorage, events) for testing `store.ts` and row rendering without a browser download. To make the core logic testable, the shuffle bag moved to `lib/bag.ts` and the question-text rules to `lib/rules.ts` as pure modules; `play.ts`/`contribute.ts` are thin DOM wrappers over them. CI runs `npm test` before every site build.
