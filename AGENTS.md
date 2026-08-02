# AGENTS.md

Working notes for coding agents in this repo. Human-facing docs are [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/](docs/); this file records the things that are easy to get wrong.

## What this repo is

An open-source system for conversation questions, in three parts sharing one database:

- `questions/` — the database: YAML, one file per category per language, CC0. The core asset.
- `website/` — Astro 5 static site (play / browse / contribute / device / deck), MIT.
- `tools/` — Python validator and build scripts that turn the YAML into site payloads and device bundles, MIT.
- `firmware/` — ESP32-S3 device, Zephyr. Build system, board config, test harness, `lib/fsm` and a bring-up blinky exist; the application state machine does not. Start at `docs/firmware_primer.md` (hands-on), design in `docs/firmware_architecture.md`.
- `hardware/` — schematic and enclosure. Structure and contracts only.
- `deps/` — gitignored west workspace (zephyr + modules). Never edit or commit anything here.
- `docs/` — design rationale, decision log, language policy, sync protocol. Also an MkDocs site.

The governing product principle (docs/design.md): minimize time-to-question, maximize time-in-conversation. Features that increase engagement with the product rather than between people are rejected.

## Commands

`just` is the single entry point; run `just` to list everything. Recipes call `.venv/bin/python` directly, so only `just`, `python3` and `npm` need to be on PATH.

**Commits.** Conventional commits, enforced by `.githooks/commit-msg` (enabled by `just setup` or `just hooks`). Types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope lowercase (`site`, `questions`, `tools`, `ci`, `fw`, `hw`, `docs`).

**Docs.** Filenames in `docs/` are snake_case; root `README.md` / `CONTRIBUTING.md` / `CODE_OF_CONDUCT.md` keep their conventional names. `mkdocs.yml` runs in strict mode, so a broken internal link fails `just docs-build`. New docs pages need a `nav:` entry.

## Testing

Write tests alongside the code, in the same change, wherever they add value. The bar is whether a test would catch a real regression: logic with branches, parsing and validation, state transitions, format and protocol code, and any bug you fix all qualify. Skip them for glue that only wires existing pieces together, for generated files, and for anything whose only assertion would restate the implementation.

Each part of the tree has a harness already, so a new test almost never needs new infrastructure:

- `tools/` — Python `unittest`, run by `just test-tools`.
- `website/` — vitest, run by `just test-website`.
- `firmware/` — ztest suites under `firmware/tests/`, run by `just fw-test` (qemu) or `just fw-test-linux` (native_sim, the only platform that emulates GPIO).

`just test` runs everything that needs no hardware, and is what CI runs. Run it before proposing a change.

Prefer unit tests against the smallest unit that holds the logic — `lib/fsm` is table-driven with an injected clock precisely so it can be tested without a board. Reach for an integration test when the risk lives in the seam between parts rather than inside one: a bundle written by `tools/` and read by the firmware, or a validator run over the real `questions/` database. For anything hardware-dependent, state plainly what was verified on the device and what was not; `docs/firmware_architecture.md` marks unverified items *verify*, and that convention holds elsewhere.

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
