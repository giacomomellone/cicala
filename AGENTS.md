# AGENTS.md

Working notes for coding agents in this repo. Human-facing docs are [README.md](README.md), [CONTRIBUTING.md](CONTRIBUTING.md) and [docs/](docs/); this file records the things that are easy to get wrong.

## What this repo is

An open-source system for conversation questions, in three parts sharing one database:

- `questions/` — the database: YAML, one file per category per language, CC0. The core asset.
- `website/` — Astro 5 static site (play / browse / contribute / device / deck), MIT.
- `tools/` — Python validator and build scripts that turn the YAML into site payloads and device bundles, MIT.
- `firmware/` — ESP32-S3 device, Zephyr. Build system, board config, test harness, `lib/fsm` and a bring-up blinky exist; the application state machine does not. Start at `FIRMWARE_GUIDE.md` (hands-on), design in `docs/firmware_architecture.md`.
- `hardware/` — schematic and enclosure. Structure and contracts only.
- `deps/` — gitignored west workspace (zephyr + modules). Never edit or commit anything here.
- `docs/` — design rationale, decision log, language policy, sync protocol. Also an MkDocs site.

The governing product principle (docs/design.md): minimize time-to-question, maximize time-in-conversation. Features that increase engagement with the product rather than between people are rejected.

## Commands

`just` is the single entry point; run `just` to list everything. Recipes call `.venv/bin/python` directly, so only `just`, `python3` and `npm` need to be on PATH.

```sh
just setup        # once: .venv from requirements.txt, npm install, git hooks
just validate     # question database: schema, dedup, denylist, origin refs
just fix          # assign ids/dates to new questions, normalize formatting
just data         # regenerate website/src/data/*.json from the database
just website      # dev server (runs `data` first)
just test         # validate + tools tests (unittest) + website tests (vitest)
just docs         # mkdocs serve

just fw-init      # once: west init/update into deps/, fetch espressif blobs
just fw-build     # build for the esp32s3 devkit
just fw-sim       # run under qemu on the host
just fw-test      # ztest suites via twister
just fmt          # clang-format + ruff + prettier
```

**Formatting.** `just fmt` covers firmware (clang-format), `tools/` (ruff) and
`website/` + docs (prettier). Never format `questions/` — `just fix`
(`tools/validate.py --fix`) owns that file formatting exactly, and prettier is
configured to ignore it. Hand-aligned FSM transition tables are wrapped in
`// clang-format off`.

**Commits.** Conventional commits, enforced by `.githooks/commit-msg` (enabled by `just setup` or `just hooks`). Types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope lowercase (`site`, `questions`, `tools`, `ci`, `fw`, `hw`, `docs`).

**Docs.** Filenames in `docs/` are snake_case; root `README.md` / `CONTRIBUTING.md` / `CODE_OF_CONDUCT.md` keep their conventional names. `mkdocs.yml` runs in strict mode, so a broken internal link fails `just docs-build`. New docs pages need a `nav:` entry.

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
