# Contributing

## Adding a question (no developer setup needed)

This is the contribution that matters most. Two ways, both take about 30 seconds:

1. **Website:** [suggest a question](https://cicala.dev/suggest) without an account. Enter the question and language, optionally add a credit, and confirm the public-domain dedication. A maintainer assigns decks, depth, and tags.
2. **GitHub:** open a [new-question issue](../../issues/new?template=new-question.yml) directly.

The website submits through the repository's narrowly scoped GitHub App, so the resulting issue is opened by `cicala-bot` without exposing a contributor account. Direct GitHub submissions require a free GitHub account and ask the contributor to choose editorial metadata. Within a minute of opening either kind of issue, an automated check covers the applicable text, deck, tone, denylist, and duplicate rules. A maintainer who speaks the language reviews every submission; once approved, another workflow turns the issue into a pull request. The question ships to the website within minutes of merge and to devices with the next database release. [docs/contribution_pipeline.md](docs/contribution_pipeline.md) describes the path.

### The public domain dedication (please read)

By submitting a question you **dedicate it to the public domain under [CC0-1.0](LICENSE-QUESTIONS)**. There is no attribution requirement and no take-backs: your question may be reused anywhere, by anyone, for any purpose, including commercial products and training data. The optional `author` field is a courtesy credit and carries no legal claim. The submission form makes you confirm this explicitly; submissions without that confirmation are not merged.

### What makes a good question

- Answerable in every selected deck. `new_people` questions require no shared
  history; `close` questions may assume familiarity.
- Open-ended, never yes/no.
- Specific enough to start a story ("When did you last…" beats "What do you think about…").
- 10–140 characters, ends with `?`, one question per entry.
- `family` questions must be safe and interesting for a 10-year-old.
- Dark and spicy questions use `wild` only. Tone is separate from depth.
- Read your language's style guide: [`questions/en/STYLE.md`](questions/en/STYLE.md), [`questions/de/STYLE.md`](questions/de/STYLE.md).

### Adding a new language

Languages are independent corpora, not translations (see [docs/languages.md](docs/languages.md)). New languages start in `questions/incubator/{lang}/` and graduate once they have ≥ 150 questions (≥ 20 eligible for each deck), a named fluent maintainer, a `STYLE.md`, and a `denylist.txt`. Never open a PR adding a top-level `questions/{lang}/` directory directly.

## Editing question files directly (developers)

One corpus file per language: `questions/{lang}/questions.yaml`. Store a
question once and list every deck where it is eligible. Append the entry
**without an `id` and without `added`**; CI assigns both:

```yaml
- text: "When did you last change your mind about something important?"
  decks: [new_people, close]
  depth: 2
  tags: [reflective]
  author: "your name" # optional
```

Then run the validator before pushing (`just setup` once, if you haven't):

```sh
just fix        # assigns ids/dates, normalizes formatting
just validate   # must pass clean
```

Rules the validator enforces: schema conformance, 10–140 chars ending in `?`,
no duplicates within a language, one or more known decks, depth 1–3,
Wild-only dark/spicy tone, per-language denylist, controlled tags, and origin
references. Never hand-write or edit an `id`. Once assigned, IDs stay stable
through text, deck, and depth edits.

## Website development

```sh
just website        # dev server (regenerates website/src/data/*.json first)
just test-website   # vitest suite
just test-e2e       # playwright: builds the site and drives it in a browser
```

Ground rules (from [docs/design.md](docs/design.md)): no UI frameworks, no Tailwind, no analytics, no accounts. The play and browse surfaces load no third-party scripts; the suggestion page loads Cloudflare Turnstile for abuse prevention. Performance budget: ≤ 60 KB gzipped JS per page, Lighthouse mobile ≥ 95 on the play page. Every new dependency needs a one-line justification in [docs/decisions.md](docs/decisions.md).

## Firmware / hardware

Structure and interface docs live in [`firmware/`](firmware/) and [`hardware/`](hardware/); both are early-stage. Firmware targets ESP-IDF v5.3+ on ESP32-S3. Hardware is licensed CERN-OHL-S-2.0.

## Commit style

Conventional commits, small PR-sized changes: `feat(site): …`, `feat(questions): …`, `fix(tools): …`, `chore(ci): …`, `docs: …`.

The format is enforced locally by a zero-dependency `commit-msg` hook in [`.githooks/`](.githooks/). Enable it with `just hooks` (also part of `just setup`); it runs `git config core.hooksPath .githooks`. Allowed types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope optional and lowercase.

## Security

Do not open a public issue or pull request for anything security-sensitive — in the website, tools, firmware, hardware, or the release process. Report it privately instead: [SECURITY.md](SECURITY.md).

## Code of conduct

Everyone interacting in this project is expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
