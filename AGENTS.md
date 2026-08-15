# AGENTS.md

Working notes for coding agents in this repo. Human-facing docs are [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/](docs/); this file records the things that are easy to get wrong.

## What this repo is

An open-source system for conversation questions. Its parts share one database:

- `questions/` — the database: YAML, one file per category per language, CC0. The core asset.
- `website/` — Astro 5 static site (play / browse / contribute / device / deck), MIT.
- `tools/` — Python validator and build scripts that turn the YAML into site payloads and device bundles, MIT.
- `firmware/` — Zephyr firmware for the ESP32-S3 device. The breadboard rig runs the tabletop loop, deep sleep, setup portal, bundle sync, signed OTA, and power-status path. Sleep current needs rev A hardware because the DevKitC indicators exceed the 30 µA budget. Hardware-free logic lives in `firmware/lib/`; Zephyr glue lives in `firmware/app/src/`. Start with `docs/firmware_primer.md`; the design is in `docs/firmware_architecture.md`.
- `hardware/` — schematic and enclosure. Structure and contracts only.
- `deps/` — gitignored west workspace (zephyr + modules). Never edit or commit anything here.
- `docs/` — design rationale, decision log, language policy, sync protocol. Also an MkDocs site.

The governing product principle (docs/design.md): minimize time-to-question, maximize time-in-conversation. Features that increase engagement with the product rather than between people are rejected.

## Repository boundary

This is the public product and engineering repository. It owns the public website, firmware, hardware design sources, reproducible BOMs, protocols, technical documentation, build instructions, the public roadmap, contribution guidance, licenses, and public-facing project assets.

It must not contain GTM plans, pricing strategy, margins, detailed internal COGS analysis, supplier quotes or contacts, cash planning, private brand research, unpublished product concepts, customer identities, raw research data, participant consent records, or private compliance and manufacturing operations. Content in those categories belongs in the sibling `../kveld-internal` repository.

Component price indications in `hardware/pcb/BOM.md` and `docs/prototype_bom.md` are reproduction aids for makers and stay here. A price appearing in a document is not by itself a reason to move it.

Never copy content from the private repository into this one without explicit publication approval.

If `../kveld-internal` is unavailable, stop and ask rather than putting private material here.

### Shared operating rules

These apply in both repositories.

- Before editing, resolve the current path and determine which repository owns the requested material.
- Run `git status` separately in each repository. The two working trees are independent.
- Never assume a branch, commit, or pull request spans both repositories.
- Use separate branches, commits, validation runs, and pull requests for each repository.
- To move material from public to private, copy it to the private destination and verify it there first, then remove the public copy.
- Do not create submodules, symlinks, imports, or documentation links that make a clean public clone depend on the private repository.
- Acknowledging that a private sibling repository exists is acceptable. Do not reveal its contents.

## Commands

`just` is the single entry point; run `just` to list everything. Recipes call `.venv/bin/python` directly, so only `just`, `python3` and `npm` need to be on PATH.

**Commits.** Conventional commits, enforced by `.githooks/commit-msg` (enabled by `just setup` or `just hooks`). Types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope lowercase (`site`, `questions`, `tools`, `ci`, `fw`, `hw`, `docs`).

**Docs.** Filenames in `docs/` are snake_case; root `README.md` / `CONTRIBUTING.md` / `CODE_OF_CONDUCT.md` keep their conventional names. `mkdocs.yml` runs in strict mode, so a broken internal link fails `just docs-build`. New docs pages need a `nav:` entry.

## Coding

- Follow clean coding practices.
- Update the docs when the architecture changes. Use a Mermaid diagram when it
  explains a flow or state transition more clearly than prose.

### Comments

Write a comment only when the code cannot state the rule clearly on its own.
Useful comments explain a constraint, side effect, unit, ownership rule,
hardware fact, protocol requirement, or non-obvious failure mode.

Keep comments short and local. Use plain English and describe the current
behaviour directly. Prefer `// GPIO32 starts gpio1, so subtract 32.` to a
paragraph about how the pin was once calculated incorrectly.

Delete comments that:

- repeat the code or nearby name;
- narrate each step of a test;
- preserve a bug report, review discussion, or sequence of past changes;
- justify a choice only by contrasting it with an abandoned design;
- address the reader or speculate about future work.

API comments should state the contract: inputs, outputs, ownership, timing, and
errors. Test comments should explain only setup or assertions that remain
unclear after reading the test name. Configuration comments should say what a
non-default value controls and, when useful, its measured basis.

Put change history in the commit message. Record durable design decisions and
rejected alternatives in `docs/decisions.md`. Other docs describe the system as
it works now.

## Testing

Write tests alongside the code, in the same change, wherever they add value. The bar is whether a test would catch a real regression: logic with branches, parsing and validation, state transitions, format and protocol code, and any bug you fix all qualify. Skip them for glue that only wires existing pieces together, for generated files, and for anything whose only assertion would restate the implementation.

**End-to-end tests.** Use one when the thing that can break lives between the parts, not inside them: a user flow through the real site (play, browse, contribute), a bundle written by `tools/` and read back by the firmware, sync or OTA over the wire, or a bug that unit tests passed through. Drive the real artifact — the built site, the real YAML database, the actual bundle file — not mocks. Skip e2e for pure logic, single-function behavior, and anything a unit test already pins down; they are slow and they fail for reasons that are not the code.

Each part of the tree has a harness already, so a new test almost never needs new infrastructure:

- `tools/` — Python `unittest`, run by `just test-tools`.
- `website/` — vitest for units (`just test-website`); Playwright for end-to-end (`just test-e2e`), which builds the site and drives it in a browser. e2e stays out of `just test` because it downloads a browser; CI runs it as its own job.
- `firmware/` — ztest suites under `firmware/tests/`, run by `just fw-test` (qemu) or `just fw-test-linux` (native_sim, Linux only). Both platforms emulate GPIO: `CONFIG_GPIO_EMUL` follows a `zephyr,gpio-emul` devicetree node, not the host.

`just test` runs everything that needs no hardware, and is what CI runs. Run it before proposing a change.

Prefer unit tests against the smallest unit that holds the logic — `lib/fsm` is table-driven with an injected clock precisely so it can be tested without a board. Reach for an integration test when the risk lives in the seam between parts rather than inside one: a bundle written by `tools/` and read by the firmware, or a validator run over the real `questions/` database. For hardware-dependent work, state what was verified on the device and list any open measurements plainly.

## Writing Style Rules

Avoid patterns commonly recognized as "AI writing tells." When drafting or editing any text, follow these constraints:

- **No inflated significance.** Don't claim a topic "plays a vital role," "serves as a testament," "leaves a lasting impact," or "stands as a watershed moment." State facts plainly instead of dramatizing their importance.
- **No editorializing tags.** Don't append a clause explaining why a fact matters (e.g., "...highlighting its enduring influence," "...underscoring the importance of..."). Report the fact and stop.
- **No summary restatement.** Don't recap what was just said in a closing paragraph ("In summary," "Overall," "In conclusion"). End when the content ends.
- **No negative parallelism / false contrast.** Avoid "It's not just X, it's Y" or "It's not about X, it's about Y" constructions used as a rhetorical crutch.
- **No vague attribution.** Don't write "some critics argue," "many experts believe," or "studies show" without naming who or citing a source. Either cite specifically or drop the claim.
- **No generic superlatives or puffery.** Avoid "rich cultural heritage," "stunning," "vibrant," "unwavering commitment," "cutting-edge," and similar filler adjectives that carry no concrete information.
- **Limit boldface and headers.** Don't bold random phrases for emphasis or add headers/title case to short passages that don't need structural navigation.
- **Limit bullet-list overuse.** Don't convert straightforward prose into a bulleted list unless the content is genuinely enumerable (steps, discrete items). Default to prose for explanations.
- **No formulaic section templates.** Don't force content into rigid boilerplate sections (e.g., "Challenges," "Future Prospects," "Conclusion") unless the user's structure calls for them.
- **No leftover collaborative phrases.** Never leave in filler like "I hope this helps!", "let me know if you need anything else," or "Certainly! Here's..." in delivered content — write only the requested content itself.
- **No unnecessary transition padding.** Avoid leaning on a small rotation of transitions ("Moreover," "Furthermore," "Additionally," "That said") to connect every paragraph. Cut transitions that add no logical link.
- **No emoji in headers or lists** unless explicitly requested.
- **Be specific, not sweeping.** Prefer concrete details, numbers, and named examples over broad generalizations that could apply to almost any subject.
- **Match register to context.** Don't default to promotional or travel-brochure tone for neutral/informational writing; keep tone plain and matter-of-fact unless a different tone is requested.

These are stylistic defaults to reduce generic-sounding output, not hard bans. Follow user-specific formatting requests over these defaults when they conflict.
