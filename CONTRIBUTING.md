# Contributing

## Adding a question (no developer setup needed)

This is the contribution that matters most. Two ways, both take about 30 seconds:

1. **Website:** the [contribute page](https://tischkarte.pages.dev/contribute) has a form that pre-fills everything for you.
2. **GitHub:** open a [new-question issue](../../issues/new?template=new-question.yml) directly.

Either way you'll need a free GitHub account. A maintainer who speaks your language reviews every submission; once approved, an automated workflow turns the issue into a pull request and your question ships to the website (within minutes of merge) and to every device (with the next database release).

### The public domain dedication (please read)

By submitting a question you **dedicate it to the public domain under [CC0-1.0](LICENSE-QUESTIONS)**. There is no attribution requirement and no take-backs: your question may be reused anywhere, by anyone, for any purpose — including commercial products and training data. The optional `author` field is a courtesy credit, not a legal claim. The submission form makes you confirm this explicitly; submissions without that confirmation are not merged.

### What makes a good question

- Answerable by a stranger — no shared context required.
- Open-ended — never yes/no.
- Specific enough to start a story ("When did you last…" beats "What do you think about…").
- 10–140 characters, ends with `?`, one question per entry.
- `family` questions must be safe and interesting for a 10-year-old.
- Read your language's style guide: [`questions/en/STYLE.md`](questions/en/STYLE.md), [`questions/de/STYLE.md`](questions/de/STYLE.md).

### Adding a new language

Languages are independent corpora, not translations — see [docs/languages.md](docs/languages.md). New languages start in `questions/incubator/{lang}/` and graduate once they have ≥ 150 questions (≥ 20 per category), a named fluent maintainer, a `STYLE.md`, and a `denylist.txt`. Never open a PR adding a top-level `questions/{lang}/` directory directly.

## Editing question files directly (developers)

One YAML file per category per language: `questions/{lang}/{category}.yaml`. Append your entry **without an `id` and without `added`** — CI assigns both:

```yaml
- text: "When did you last change your mind about something important?"
  tags: [reflective]
  author: "your name"   # optional
```

Then run the validator before pushing (`just setup` once, if you haven't):

```sh
just fix        # assigns ids/dates, normalizes formatting
just validate   # must pass clean
```

Rules the validator enforces: schema conformance, 10–140 chars ending in `?`, no duplicates within a language (across categories), per-language denylist, controlled tag vocabulary, `origin` references must exist. Never hand-write or edit an `id` — once assigned, ids are stable forever, even through typo fixes.

## Website development

```sh
just website        # dev server (regenerates website/src/data/*.json first)
just test-website   # vitest suite
```

Ground rules (from [docs/design.md](docs/design.md)): no UI frameworks, no Tailwind, no third-party scripts, no analytics, no accounts. Performance budget: ≤ 60 KB gzipped JS per page, Lighthouse mobile ≥ 95 on the play page. Every new dependency needs a one-line justification in [docs/decisions.md](docs/decisions.md).

## Firmware / hardware

Structure and interface docs live in [`firmware/`](firmware/) and [`hardware/`](hardware/); both are early-stage. Firmware targets ESP-IDF v5.3+ on ESP32-S3. Hardware is licensed CERN-OHL-S-2.0.

## Commit style

Conventional commits, small PR-sized changes: `feat(site): …`, `feat(questions): …`, `fix(tools): …`, `chore(ci): …`, `docs: …`.

The format is enforced locally by a zero-dependency `commit-msg` hook in [`.githooks/`](.githooks/) — `just setup` enables it (`git config core.hooksPath .githooks`). Allowed types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope optional and lowercase.

## Code of conduct

Everyone interacting in this project is expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
